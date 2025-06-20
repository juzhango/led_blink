/**
 * @file led_blink.c
 * @brief LED 闪烁控制模块（RT-Thread 平台）
 *
 * 提供多 LED 注册管理、后台线程控制、基于时间的闪烁逻辑。
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "led_blink.h"

/** 全局互斥锁，保护 LED 链表访问 */
static rt_mutex_t led_list_lock_ = RT_NULL;
/** 全局 LED 节点链表头指针 */
static st_led_node_t *led_list_head_ = RT_NULL;

/**
 * @brief 获取当前系统时间（单位：毫秒）
 *
 * @return 当前时间（ms）
 */
static unsigned long get_current_ms(void)
{
    return (unsigned long)rt_tick_get() * 1000 / RT_TICK_PER_SECOND;
}

/**
 * @brief 点亮指定 LED
 *
 * @param led LED 引脚信息结构体指针
 */
void led_on(const st_led_pin_t *led)
{
    rt_pin_write(led->pin, led->active_level);
}

/**
 * @brief 关闭指定 LED
 *
 * @param led LED 引脚信息结构体指针
 */
void led_off(const st_led_pin_t *led)
{
    rt_pin_write(led->pin, !led->active_level);
}

/**
 * @brief 注册一个新的 LED 控制节点
 *
 * @param pin           GPIO 引脚编号
 * @param active_level  激活电平（0=低电平有效，1=高电平有效）
 *
 * @return 成功返回新创建的 LED 节点指针，失败返回 NULL
 */
st_led_node_t *led_register(rt_base_t pin, rt_ssize_t active_level)
{
    // 检查是否已注册相同引脚
    st_led_node_t *current = led_list_head_;
    while (current)
    {
        if (current->led.pin == pin)
        {
            rt_kprintf("Pin %d already registered\n", pin);
            return RT_NULL;
        }
        current = current->next;
    }

    // 创建新节点
    st_led_node_t *new_led = rt_malloc(sizeof(st_led_node_t));
    if (!new_led)
    {
        rt_kprintf("create led node fail\n");
        return RT_NULL;
    }

    rt_memset(new_led, 0, sizeof(st_led_node_t));
    new_led->led.pin = pin;
    new_led->led.active_level = active_level;
    new_led->state = e_led_state_blink_stopped;
    new_led->next = RT_NULL;

    // 初始化引脚为输出模式并默认关闭
    rt_pin_mode(pin, PIN_MODE_OUTPUT);
    led_off(&new_led->led);

    rt_mutex_take(led_list_lock_, RT_WAITING_FOREVER);
    if (led_list_head_)
    {
        led_list_head_->next = new_led;
    }
    else
    {
        led_list_head_ = new_led;
    }
    rt_mutex_release(led_list_lock_);

    return new_led;
}

/**
 * @brief 移除指定的 LED 节点
 *
 * @param node 要移除的 LED 节点指针
 */
void led_unregister(st_led_node_t *node)
{
    if (!node)
        return;

    st_led_node_t *current = led_list_head_;
    st_led_node_t *prev = RT_NULL;

    rt_mutex_take(led_list_lock_, RT_WAITING_FOREVER);

    while (current && current != node)
    {
        prev = current;
        current = current->next;
    }

    if (!current)
    {
        rt_mutex_release(led_list_lock_);
        return; // Node not found
    }

    if (prev)
    {
        prev->next = current->next;
    }
    else
    {
        led_list_head_ = current->next;
    }

    rt_mutex_release(led_list_lock_);
    rt_free(current);
}

/**
 * @brief 启动 LED 闪烁
 *
 * @param node      LED 节点指针
 * @param on_time   LED 亮的时间（ms）
 * @param off_time  LED 灭的时间（ms）
 * @param count     闪烁次数（0 表示无限循环）
 */
void led_blink_start(st_led_node_t *node, unsigned int on_time, unsigned int off_time, unsigned int count)
{
    if (!node)
        return;

    rt_mutex_take(led_list_lock_, RT_WAITING_FOREVER);
    node->on_time = on_time;
    node->off_time = off_time;
    node->count = count;
    node->current_count = 0;
    node->next_change_time = get_current_ms();
    node->state = e_led_state_blink_off;
    rt_mutex_release(led_list_lock_);
}

/**
 * @brief 停止 LED 闪烁，并关闭 LED
 *
 * @param node LED 节点指针
 */
void led_blink_stop(st_led_node_t *node)
{
    if (!node)
        return;

    rt_mutex_take(led_list_lock_, RT_WAITING_FOREVER);
    node->state = e_led_state_blink_stopped;
    led_off(&node->led);
    rt_mutex_release(led_list_lock_);
}

/**
 * @brief LED 控制后台守护线程
 *
 * @param parameter 线程启动参数（未使用）
 */
void led_blink_daemon_(void *parameter)
{
    rt_kprintf("led_blink_daemon_ started\n");

    while (1)
    {
        unsigned long now = get_current_ms();
        st_led_node_t *current = led_list_head_;

        unsigned long min_delay = RT_WAITING_FOREVER;

        rt_mutex_take(led_list_lock_, RT_WAITING_FOREVER);

        while (current)
        {
            if (current->state == e_led_state_blink_stopped)
            {
                current = current->next;
                continue;
            }

            long remaining = current->next_change_time - now;

            // 处理常灭状态
            if (current->on_time == 0)
            {
                led_off(&current->led);
                current->state = e_led_state_blink_stopped;
                current = current->next;
                continue;
            }

            // 处理常亮状态
            if (current->off_time == 0)
            {
                led_on(&current->led);
                current->state = e_led_state_blink_stopped;
                current = current->next;
                continue;
            }

            // 正常闪烁逻辑
            if (remaining <= 0)
            {
                if (current->state != e_led_state_blink_on)
                {
                    led_on(&current->led);
                    current->state = e_led_state_blink_on;
                    current->next_change_time = now + current->on_time;
                }
                else
                {
                    led_off(&current->led);
                    current->state = e_led_state_blink_off;
                    current->next_change_time = now + current->off_time;

                    if (current->count != 0)
                    {
                        current->current_count++;
                        if (current->current_count >= current->count)
                        {
                            current->state = e_led_state_blink_stopped;
                        }
                    }
                }

                remaining = current->next_change_time - now;
            }

            if (remaining > 0 && (unsigned long)remaining < min_delay)
            {
                min_delay = remaining;
            }

            current = current->next;
        }

        rt_mutex_release(led_list_lock_);

        if (min_delay == RT_WAITING_FOREVER)
        {
            rt_thread_mdelay(100);
        }
        else
        {
            rt_thread_mdelay((min_delay < 10) ? 10 : min_delay);
        }
    }
}

/**
 * @brief 初始化 LED 控制模块
 *
 * @return 成功返回 0，失败返回 -1
 */
static int led_blink_init(void)
{
    rt_thread_t thread;

    led_list_lock_ = rt_mutex_create("led_lock", RT_IPC_FLAG_FIFO);

    thread = rt_thread_create("led_blink_daemon",
                              led_blink_daemon_,
                              RT_NULL,
                              512,
                              RT_THREAD_PRIORITY_MAX / 2,
                              20);
    if (!thread)
        return -1;

    rt_thread_startup(thread);
    rt_kprintf("LED blink module initialized successfully.\n");
    return 0;
}
INIT_COMPONENT_EXPORT(led_blink_init); // 使用标准初始化导出