/*
 * prog_main.c
 *
 *  Created on: Apr 21, 2023
 *      Author: richa
 */

#include "bl_main.h"

#include "main.h"
#include "uart_controllers.h"
#include "can_controllers.h"


#define FLAGS_ADDRESS (FLASH_BASE + (38 * 0x400))
#define APP_MEM_ADDRESS (FLASH_BASE + (40 * 0x400))

extern TIM_HandleTypeDef htim2;
extern UART_HandleTypeDef huart1;
extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_HandleTypeDef hfdcan2;

extern uart_controller_t uart_controller_1;
extern can_controller_t can_controller_1;
extern can_controller_t can_controller_2;

volatile uint8_t branch = 0;

uint8_t binary[112 * 1024];

uint8_t tenths_of_seconds = 0;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM2)
	{
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

		// 10 Hz
		if (tenths_of_seconds < 50)
			tenths_of_seconds++;
		else
			branch = 1;
	}
}

void bootloader_jump_to_user_app()
{
	HAL_DeInit(); // to disable all the peripherals

	__DSB();
	__ISB();

	uint32_t mainStackPointer = *(volatile uint32_t*) (APP_MEM_ADDRESS);
	__set_MSP(mainStackPointer);
	uint32_t programResetHandlerAddress = *(volatile uint32_t*) (APP_MEM_ADDRESS
			+ 4);
	void (*programResetHandler)(
			void) = (void (*)(void)) programResetHandlerAddress;
	programResetHandler();
}

HAL_StatusTypeDef save_data(uint32_t Address, uint64_t data)
{
	HAL_FLASH_Unlock();
	FLASH_EraseInitTypeDef EraseInitStruct;
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Banks = FLASH_BANK_1;

	uint32_t page = (Address - FLASH_BASE) / (2 * 0x400);

	EraseInitStruct.Page = page;
	EraseInitStruct.NbPages = 1;

	uint32_t PageError;
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) //Erase the Page Before a Write Operation
		return HAL_ERROR;

	HAL_Delay(50);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, Address, (uint64_t) data);
	HAL_Delay(50);
	HAL_FLASH_Lock();

	return HAL_OK;
}

uint64_t read_data(uint32_t Address)
{
	__IO uint64_t read_data = *(__IO uint32_t*) (Address);
	return (uint64_t) read_data;
}

void bl_init()
{
	if (!(read_data(FLAGS_ADDRESS + 0) & 0x1))
		bootloader_jump_to_user_app();
}

int bl_main(void)
{
	save_data(FLAGS_ADDRESS, 0);

	can_controller_1.init(&hfdcan1);
	can_controller_2.init(&hfdcan2);

	can_controller_1.silence(false);
	can_controller_2.silence(false);

#ifdef DEBUG
	uart_controller_1.init(&huart1, USART1_IRQn);

	for (uint8_t i = 0; i < 8; i++)
		uart_controller_1.write_bytes("\r\n", 2);
	uart_controller_1.write_bytes("============\r\n",14);
	uart_controller_1.write_bytes(" Bootloader \r\n",14);
	uart_controller_1.write_bytes("============\r\n",14);
#endif

	HAL_TIM_Base_Start_IT(&htim2);
	while (!branch)
		;

	HAL_TIM_Base_Stop_IT(&htim2);
	HAL_TIM_Base_DeInit(&htim2);

	HAL_FDCAN_DeInit(&hfdcan1);
	HAL_FDCAN_DeInit(&hfdcan2);

	HAL_UART_Abort_IT(&huart1);
	HAL_UART_DeInit(&huart1);

	HAL_GPIO_DeInit(GPIOB, LED_Pin|FDCAN2_S_Pin);
	HAL_GPIO_DeInit(FDCAN1_S_GPIO_Port, FDCAN1_S_Pin);

	bootloader_jump_to_user_app();

	return 0;
}
