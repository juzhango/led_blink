
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <stdio.h>
#include "led_blink.h"

static rt_mutex_t led_list_lock_;
static st_led_node_t *led_list_head_ = NULL;

static unsigned long get_current_ms(void)
{
    return (unsigned long)rt_tick_get() * 1000 / RT_TICK_PER_SECOND;
}

static void led_on(const st_led_pin_t *led)
{
    rt_pin_write(led->pin, led->active_level);
}

static void led_off(const st_led_pin_t *led)
{
    rt_pin_write(led->pin, !led->active_level);
}

st_led_node_t *led_register(const st_led_pin_t *led, unsigned int on_time, unsigned int off_time, unsigned int count)
{
    st_led_node_t *new_led = rt_malloc(sizeof(st_led_node_t));
    if (!new_led)
    {
        rt_kprintf("create led node fail");
        goto exit;
    }
    rt_memset(new_led, 0, sizeof(st_led_node_t));

    new_led->led.pin = led->pin;
    new_led->led.active_level = led->active_level;
    new_led->on_time = on_time;
    new_led->off_time = off_time;
    new_led->count = count;
    new_led->next_change_time = 0;
    new_led->current_count = 0;
    new_led->state = e_led_state_blink_off;
    new_led->next = NULL;

    rt_pin_mode(new_led->led.pin, PIN_MODE_OUTPUT);
    led_off(led);

    if (led_list_head_)
    {
        led_list_head_->next = new_led;
    }
    else
    {
        led_list_head_ = new_led;
    }
    return new_led;

exit:
    return NULL;
}

// 移除指定的LED节点
void jz_ed_unregister(st_led_node_t *node)
{
    if (!node)
        return;

    st_led_node_t *current = led_list_head_;
    st_led_node_t *prev = RT_NULL;

    while (current && current != node)
    {
        prev = current;
        current = current->next;
    }

    if (!current)
    {
        return; // Node not found
    }

    rt_mutex_take(led_list_lock_, RT_WAITING_FOREVER);
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

void led_blink_daemon_(void *parameter)
{
    rt_kprintf("led_blink_daemon_ start\n");
    while (1)
    {
        unsigned long now = get_current_ms();
        st_led_node_t *current = led_list_head_;

        unsigned long min_delay = RT_WAITING_FOREVER; // 最小休眠时间
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

                // 更新剩余时间
                remaining = current->next_change_time - now;
            }

            if (remaining > 0 && (unsigned long)remaining < min_delay)
            {
                min_delay = remaining;
            }

            current = current->next;
        }

        // 如果没有有效的 delay，默认休眠一段时间防止 CPU 占用过高
        if (min_delay == RT_WAITING_FOREVER)
        {
            rt_thread_mdelay(100);
        }
        else
        {
            rt_kprintf("min_delay %d\n", min_delay);
            rt_thread_mdelay((min_delay < 10) ? 10 : min_delay); // 至少休眠 10ms
        }
    }
}

static int led_blink_init(void)
{
    rt_thread_t thread;
    led_list_lock_ = rt_mutex_create("led_lock", 0);

    thread = rt_thread_create("led_blink_daemon_",
                              led_blink_daemon_,
                              RT_NULL,
                              2048,
                              RT_THREAD_PRIORITY_MAX - 1,
                              20);
    if (!thread)
        return -1;

    rt_thread_startup(thread);
    rt_kprintf("ok\n");
    return 0;
}
INIT_COMPONENT_EXPORT(led_blink_init);