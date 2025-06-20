#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "led_blink.h"
static st_led_node_t *led1 = RT_NULL;
static st_led_node_t *led2 = RT_NULL;

static int led_blink_demo_init(void)
{
    led1 = led_register(GET_PIN(E, 8), PIN_LOW);
    // led2 = led_register(GET_PIN(E, 9), PIN_LOW);

    if (led1)
        led_blink_start(led1, 50, 950, 0); // 快速闪烁
    // if (led2) led_blink_start(led2, 500, 500, 0); // 慢速闪烁

    return 0;
}
INIT_APP_EXPORT(led_blink_demo_init);