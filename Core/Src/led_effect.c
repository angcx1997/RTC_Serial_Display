/*
 * led_effect.c
 *
 *  Created on: Feb 4, 2022
 *      Author: angcx
 */

#include "led_effect.h"
#define LD2_Pin GPIO_PIN_7
#define LD2_GPIO_Port GPIOB
#define LD1_Pin GPIO_PIN_0
#define LD1_GPIO_Port GPIOB
#define LD3_Pin GPIO_PIN_14
#define LD3_GPIO_Port GPIOB

static void led_control(int value);

void led_effect_stop(void)
{
	for (int i = 0; i < 3; i++)
		xTimerStop(handle_led_timer[i], portMAX_DELAY);
}

void led_effect_start(int n)
{
	assert(n >= 0 && n < 4);
	led_effect_stop();
	if (xTimerStart(handle_led_timer[n], portMAX_DELAY) != pdPASS)
	{
		printf("Timer for LED %d failed to start\n", n);
		Error_Handler();
	}
}

void led_turn_off_all(void)
{
	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
}

void led_turn_on_all(void)
{
	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
}

void led_turn_on_odd(void)
{
	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
}

void led_turn_on_even(void)
{

	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
}

void led_control(int value) {
	for (int i = 0; i < 3; i++)
		HAL_GPIO_WritePin(LD3_GPIO_Port, (LD1_Pin + i * 7), ((value >> i) & 0x1));
}

void led_effect_1(void)
{
	static int flag = 1;
	(flag ^= 1) ? led_turn_off_all() : led_turn_on_all();
}

void led_effect_2(void)
{
	static int flag = 1;
	(flag ^= 1) ? led_turn_on_even() : led_turn_on_odd();
}

void led_effect_3(void)
{
	static int i = 0;
	led_control(0x1 << (i++ % 3));
}

void led_effect_4(void)
{

	static int i = 0;
	led_control(0x4 >> (i++ % 3));
}
