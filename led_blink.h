#ifndef _LED_BLINK_H_
#define _LED_BLINK_H_

typedef enum
{
    e_led_state_blink_stopped = 0,
    e_led_state_blink_on,
    e_led_state_blink_off,
} e_led_state_t;

typedef struct
{
    rt_base_t pin;
    rt_ssize_t active_level;
} st_led_pin_t;

typedef struct led_node
{
    st_led_pin_t led;
    unsigned int on_time;
    unsigned int off_time;
    unsigned int count;
    unsigned long next_change_time;
    unsigned int current_count;

    e_led_state_t state;
    struct led_node *next;
} st_led_node_t;

st_led_node_t *led_register(const st_led_pin_t *led, unsigned int on_time, unsigned int off_time, unsigned int count);
void led_unregister(st_led_node_t *node);

#endif