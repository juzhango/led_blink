#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <led_blink.h>

st_led_node_t *led1 = RT_NULL;
static init led_blink_demo_init(void)
{
    st_led_pin_t led_pin;
    led_pin.pin = PKG_LED_BLINK_DEMO_GPIO_PIN;
    led_pin.active_level = PKG_LED_BLINK_DEMO_ACTIVE_LEVEL;

    led1 = led_register(&led_pin, 50, 950, -1);
    return 0;
}
INIT_APP_EXPORT(led_blink_demo_init);
