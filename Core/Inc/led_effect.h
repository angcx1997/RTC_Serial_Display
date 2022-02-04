/*
 * led_effect.h
 *
 *  Created on: Feb 4, 2022
 *      Author: angcx
 */

#ifndef INC_LED_EFFECT_H_
#define INC_LED_EFFECT_H_

#include "main.h"

extern TimerHandle_t handle_led_timer[4];

void led_effect_stop(void);
void led_effect_start(int n);
void led_turn_off_all(void);
void led_turn_on_all(void);
void led_turn_on_odd(void);
void led_turn_on_even(void);

void led_effect_1(void);
void led_effect_2(void);
void led_effect_3(void);
void led_effect_4(void);


#endif /* INC_LED_EFFECT_H_ */
