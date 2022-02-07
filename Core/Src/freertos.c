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
typedef enum{
	HH_CONFIG,
	MM_CONFIG,
	SS_CONFIG,
}time_config;

typedef enum{
	DATE_CONFIG,
	MONTH_CONFIG,
	YEAR_CONFIG,
	DAY_CONFIG,
}date_config;
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
static uint8_t char2int(uint8_t *p, int len);
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
	const char* msg_rtc1 = 	"\n"
							"========================\n"
							"|         RTC          |\n"
							"========================\n";

	const char* msg_rtc2 = 	"\n"
							"Configure Time   ----> 0\n"
							"Configure Date   ----> 1\n"
							"Enable reporting ----> 2\n"
							"Exit             ----> 3\n"
							"Enter your choice here : ";
	const char *msg_rtc_hh = "Enter hour(1-12):";
	const char *msg_rtc_mm = "Enter minutes(0-59):";
	const char *msg_rtc_ss = "Enter seconds(0-59):";

	const char *msg_rtc_dd  = "Enter date(1-31):";
	const char *msg_rtc_mo  ="Enter month(1-12):";
	const char *msg_rtc_dow  = "Enter day(1-7 sun:1):";
	const char *msg_rtc_yr  = "Enter year(0-99):";

	const char *msg_conf = "Configuration successful\n";
	const char *msg_rtc_report = "Enable time&date reporting(y/n)?: ";

	uint32_t cmd_addr;
	command_t *cmd;

	static int rtc_state = 0;
	int menu_code;

	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;

	while(1)
	{
		//Wait until notify
		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);

		//print menu and show current time
		xQueueSend(queue_print, &msg_rtc1, portMAX_DELAY);
		rtc_show_time_date_serial();
		xQueueSend(queue_print, &msg_rtc2, portMAX_DELAY);

		while(curr_state != sMainMenu)
		{
			xTaskNotifyWait(0, 0, &cmd_addr, portMAX_DELAY);
			cmd = (command_t*) cmd_addr;

			switch(curr_state)
			{
				case sRtcMenu:{
					if(cmd->len == 1)
					{
						menu_code = cmd->payload[0] - 48;
						switch(menu_code)
						{
						case 0:
							curr_state = sRtcTimeConfig;
							xQueueSend(queue_print, &msg_rtc_hh, portMAX_DELAY);
							break;
						case 1:
							curr_state = sRtcDateConfig;
							xQueueSend(queue_print, &msg_rtc_dd, portMAX_DELAY);
							break;
						case 2:
							curr_state = sRtcReport;
							xQueueSend(queue_print, &msg_rtc_report, portMAX_DELAY);
							break;
						case 3:
							curr_state = sMainMenu;
							break;
						default:
							curr_state = sMainMenu;
							xQueueSend(queue_print, &msg_inv, portMAX_DELAY);
						}
					}
					else
					{
						curr_state = sMainMenu;
						xQueueSend(queue_print, &msg_inv, portMAX_DELAY);
					}
					break;
				}
				case sRtcTimeConfig:{
					/*get hh, mm, ss infor and configure RTC */
					/*take care of invalid entries */
					switch(rtc_state)
					{
						case HH_CONFIG:{
							uint8_t hour = char2int(cmd->payload, cmd->len);
							time.Hours = hour;
							rtc_state = MM_CONFIG;
							xQueueSend(queue_print, &msg_rtc_mm, portMAX_DELAY);
							break;
						}
						case MM_CONFIG:{
							uint8_t min = char2int(cmd->payload, cmd->len);
							time.Minutes = min;
							rtc_state = SS_CONFIG;
							xQueueSend(queue_print, &msg_rtc_ss, portMAX_DELAY);
							break;
						}
						case SS_CONFIG:{
							uint8_t sec = char2int(cmd->payload, cmd->len);
							time.Seconds = sec;
							if (!rtc_validate(&time, NULL))
							{
								rtc_configure_time(&time);
								xQueueSend(queue_print, &msg_conf, portMAX_DELAY);
								rtc_show_time_date_serial();
							}else
								xQueueSend(queue_print, &msg_inv, portMAX_DELAY);
							curr_state = sMainMenu;
							rtc_state = 0;
							break;
						}
					}
					break;
				}
				case sRtcDateConfig:{
					/*get date, month, day , year info and configure RTC */
					/*take care of invalid entries */
					switch (rtc_state) {
						case DATE_CONFIG:{
							uint8_t d = char2int(cmd->payload, cmd->len);
							date.Date = d;
							rtc_state = MONTH_CONFIG;
							xQueueSend(queue_print, &msg_rtc_mo, portMAX_DELAY);
							break;
						}
						case MONTH_CONFIG:{
							uint8_t month = char2int(cmd->payload, cmd->len);
							date.Month = month;
							rtc_state = DAY_CONFIG;
							xQueueSend(queue_print, &msg_rtc_dow, portMAX_DELAY);
							break;
						}
						case DAY_CONFIG:{
							uint8_t day = char2int(cmd->payload, cmd->len);
							date.WeekDay = day;
							rtc_state = YEAR_CONFIG;
							xQueueSend(queue_print, &msg_rtc_yr, portMAX_DELAY);
							break;
						}
						case YEAR_CONFIG:{
							uint8_t year = char2int(cmd->payload, cmd->len);
							date.Year = year;
							break;
							if (!rtc_validate(NULL, &date))
							{
								rtc_configure_date(&date);
								xQueueSend(queue_print, &msg_conf, portMAX_DELAY);
								rtc_show_time_date_serial();
							}else
								xQueueSend(queue_print, &msg_inv, portMAX_DELAY);
							curr_state = sMainMenu;
							rtc_state = 0;
							break;
						}
					}
					break;
				}
				case sRtcReport:{
					//Enable or disable RTC current time reporting over ITM
					if (cmd->len == 1)
					{
						if(cmd->payload[0] == 'y'){
							if(xTimerIsTimerActive(timer_rtc) == pdFALSE)
								xTimerStart(timer_rtc, portMAX_DELAY);
						}
						else if (cmd->payload[0] == 'n')
							xTimerStop(timer_rtc, portMAX_DELAY);
						else
							xQueueSend(queue_print, &msg_inv, portMAX_DELAY);
					}else
						xQueueSend(queue_print, &msg_inv, portMAX_DELAY);

					curr_state = sMainMenu;
					break;
				}
			}
		}
		/*Notify menu task */
		xTaskNotify(task_menu,0,eNoAction);
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


static uint8_t char2int(uint8_t *p, int len)
{
	int value;
	if (len > 1)
		value = ((p[0] - 48 * 10) + (p[1] - 48));
	else
		value = p[0] - 48;
	return value;
}
/* USER CODE END Application */

