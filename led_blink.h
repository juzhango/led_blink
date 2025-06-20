#ifndef __LED_BLINK_H__
#define __LED_BLINK_H__

#include <rtthread.h>

typedef enum {
    e_led_state_blink_stopped,
    e_led_state_blink_off,
    e_led_state_blink_on
} e_led_state_t;

typedef struct st_led_pin_t {
    rt_base_t pin;
    rt_ssize_t active_level;  /**< 0: Low level active, 1: High level active */
} st_led_pin_t;

typedef struct st_led_node_t {
    st_led_pin_t led;
    unsigned int on_time;
    unsigned int off_time;
    unsigned int count;
    unsigned int current_count;
    unsigned long next_change_time;
    e_led_state_t state;
    struct st_led_node_t *next;
} st_led_node_t;

/* Public APIs */
st_led_node_t *led_register(rt_base_t pin, rt_ssize_t active_level);
void led_unregister(st_led_node_t *node);

void led_blink_start(st_led_node_t *node, unsigned int on_time, unsigned int off_time, unsigned int count);
void led_blink_stop(st_led_node_t *node);

void led_on(const st_led_pin_t *led);
void led_off(const st_led_pin_t *led);

#endif /* __LED_BLINK_H__ */