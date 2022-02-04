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

/**
 * @brief Stop all led effect
 * @param None
 * @retval None
 */
void led_effect_stop(void);

/**
 * @brief Start timer for particular led effect
 * @param [0,3]
 * @retval None
 */
void led_effect_start(int n);

/**
 * @brief Turn off all led
 * @param None
 * @retval None
 */
void led_turn_off_all(void);

/**
 * @brief Turn on all led
 * @param None
 * @retval None
 */
void led_turn_on_all(void);

/**
 * @brief Turn on odd led only
 * @param None
 * @retval None
 */
void led_turn_on_odd(void);

/**
 * @brief Turn on even led only
 * @param None
 * @retval None
 */
void led_turn_on_even(void);

/**
 * @brief Turn all led on and off
 * @param None
 * @retval None
 */
void led_effect_1(void);

/**
 * @brief Turn led even and odd
 * @param None
 * @retval None
 */
void led_effect_2(void);

/**
 * @brief Turn on led in ascending order
 * @param None
 * @retval None
 */
void led_effect_3(void);

/**
 * @brief Turn on led in descending order
 * @param None
 * @retval None
 */
void led_effect_4(void);

#endif /* INC_LED_EFFECT_H_ */
