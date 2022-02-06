/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SERIAL_UART USART3
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern TaskHandle_t task_led;
extern TaskHandle_t task_print;
extern TaskHandle_t task_command;
extern TaskHandle_t task_rtc;
extern TaskHandle_t task_menu;
//queue handler
extern QueueHandle_t queue_print;
extern QueueHandle_t queue_data;

extern UART_HandleTypeDef huart3;
//software timer handles
extern TimerHandle_t timer_led[4];
extern TimerHandle_t timer_rtc;

extern state_t curr_state;

const char *msg_inv = "////Invalid option////\n";
/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void process_command(command_t* cmd);
static int extract_command(command_t* cmd);
/* USER CODE END FunctionPrototypes */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void Task_Menu(void *argument)
{
	uint32_t cmd_addr;
	command_t *cmd;

	int option;
	const char* msg_menu = 	"\n"
							"========================\n"
							"|         Menu         |\n"
							"========================\n"
							"LED effect    -------> 0\n"
							"Date and time -------> 1\n"
							"Exit          -------> 2\n"
							"Enter your choice here : ";

	while(1)
	{
		if(xQueueSend(queue_print, &msg_menu, portMAX_DELAY) != pdPASS)
		{
			printf("Fail to send msg menu queue");
		}

		//Wait for user input menu command
		xTaskNotifyWait(0, 0, &cmd_addr, portMAX_DELAY);
		cmd = (command_t*)cmd_addr;
		if (cmd->len == 1)
		{
			option = cmd->payload[0] - 48;
			switch(option)
			{
			case 0:
				curr_state = sLedEffect;
				xTaskNotify(task_led, 0, eNoAction);
				break;
			case 1:
				curr_state = sRtcMenu;
				xTaskNotify(task_rtc, 0, eNoAction);
				break;
			case 2:
				//Implement quit
				break;
			default:
				xQueueSend(queue_print, &msg_inv, portMAX_DELAY);
				continue;
			}
		}
		else{
			//Invalid entry
			xQueueSend(queue_print, &msg_inv, portMAX_DELAY);
			continue;
		}

		//Wait to run again when some other task notifies
		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
	}
}

void Task_Led(void *argument)
{
	uint32_t cmd_addr;
	command_t *cmd;
	const char* msg_led =	"\n"
							"========================\n"
							"|      LED Effect     |\n"
							"========================\n"
							"(none,e1,e2,e3,e4)\n"
							"Enter your choice here : ";

	while(1)
	{
		//Wait for notification(notify wait)
		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
		//Print LED menu
		xQueueSend(queue_print,&msg_led,portMAX_DELAY);

		//wait for led command
		xTaskNotifyWait(0, 0, &cmd_addr, portMAX_DELAY);
		cmd = (command_t*) cmd_addr;

		if(cmd->len <= 4)
		{
			if (!strcmp((char*)cmd->payload, "none"))
				led_effect_stop();
			else if (!strcmp((char*)cmd->payload, "e1"))
				led_effect_start(0);
			else if (!strcmp((char*)cmd->payload, "e2"))
				led_effect_start(1);
			else if (!strcmp((char*)cmd->payload, "e3"))
				led_effect_start(2);
			else if (!strcmp((char*)cmd->payload, "e4"))
				led_effect_start(3);
			else
				xQueueSend(queue_print, &msg_inv, portMAX_DELAY);
		}
		else
			xQueueSend(queue_print, &msg_inv, portMAX_DELAY);

		//Update state variable
		curr_state = sMainMenu;

		//Notify menu task
		xTaskNotify(task_menu, 0, eNoAction);
	}

}
void Task_Print(void *argument)
{
	uint32_t* msg;
	while(1)
	{
		xQueueReceive(queue_print, &msg, portMAX_DELAY);
		if (HAL_UART_Transmit(&huart3, (uint8_t*) msg, strlen((char*)msg), HAL_MAX_DELAY) != HAL_OK)
		{
			Error_Handler();
		}
	}
}


void Task_RTC(void* argument)
{
	while(1)
	{

	}
}
void Task_Command(void *argument)
{
	BaseType_t flag;
	command_t cmd;

	while(1)
	{
		flag = xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);

		if (flag == pdTRUE)
			//process user input command and store in data queue
			process_command(&cmd);
	}
}

void process_command(command_t* cmd)
{
	extract_command(cmd);
	switch (curr_state) {
		case sMainMenu:
			xTaskNotify(task_menu, (uint32_t) cmd, eSetValueWithOverwrite);
			break;
		case sLedEffect:
			xTaskNotify(task_led, (uint32_t) cmd, eSetValueWithOverwrite);
			break;
		case sRtcMenu:
		case sRtcReport:
		case sRtcDateConfig:
		case sRtcTimeConfig:
			xTaskNotify(task_rtc, (uint32_t) cmd, eSetValueWithOverwrite);
			break;
	}
}

int extract_command(command_t* cmd)
{
	uint8_t item;
	BaseType_t status;

	//Check if any message in the queue
	status = uxQueueMessagesWaiting(queue_data);
	if(status == 0) return -1;

	uint8_t i = 0;

	do
	{
		status = xQueueReceive(queue_data, &item, 0);
		if (status == pdTRUE) cmd->payload[i++] = item;
	}while(item != '\n');

	cmd->payload[i-1] = '\0'; //replace \n with null character
	cmd->len = i-1; //save length of command excluding null

	return 0;
}
/* USER CODE END Application */

