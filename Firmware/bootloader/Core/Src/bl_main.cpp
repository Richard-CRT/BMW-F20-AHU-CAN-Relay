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
#include "programmer_bootloader_frames.h"

#define FLAGS_ADDRESS (FLASH_BASE + (38 * 0x400))
#define APP_MEM_ADDRESS (FLASH_BASE + (40 * 0x400))
#define APP_MEM_SIZE (FLASH_SIZE - (APP_MEM_ADDRESS - FLASH_BASE));

extern TIM_HandleTypeDef htim2;
extern UART_HandleTypeDef huart1;
extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_HandleTypeDef hfdcan2;

extern uart_controller_t uart_controller_1;
extern can_controller_t<512, 4> can_controller_1;
extern can_controller_t<512, 4> can_controller_2;

enum class State_t
{
	BLWaitingForAcknowledgement,
	BLWaitingForFrameCount,
	BLWaitingForCRC,
	BLWaitingForChunkResync,
	BLWaitingForChunkData
};
volatile State_t state = State_t::BLWaitingForAcknowledgement;

uint8_t TxData_BLWaitingForAcknowledgement[8];
uint8_t TxData_BLWaitingForFrameCount[8];
uint8_t TxData_BLWaitingForCRC[8];
uint8_t TxData_BLWaitingForChunkResync[8];
FDCAN_TxHeaderTypeDef TxHeader;

uint8_t binary[128 * 1024];

uint8_t tenths_of_seconds = 0;
volatile bool branch = false;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM2)
	{
		// 10 Hz
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

		if (tenths_of_seconds >= 50)
			branch = true;
		else
			tenths_of_seconds++;

#ifdef DEBUG
		switch (state)
		{
		case State_t::BLWaitingForAcknowledgement:
			uart_controller_1.write_bytes("BLWaitingForAcknowledgement\r\n", 27);
			break;
		case State_t::BLWaitingForFrameCount:
			uart_controller_1.write_bytes("BLWaitingForFrameCount\r\n", 22);
			break;
		case State_t::BLWaitingForCRC:
			uart_controller_1.write_bytes("BLWaitingForCRC\r\n", 15);
			break;
		case State_t::BLWaitingForChunkResync:
			uart_controller_1.write_bytes("BLWaitingForChunkResync\r\n", 23);
			break;
		case State_t::BLWaitingForChunkData:
			uart_controller_1.write_bytes("BLWaitingForChunkData\r\n", 21);
			break;
		}
#endif

		switch (state)
		{
		case State_t::BLWaitingForAcknowledgement:
			can_controller_1.send_message(&TxHeader,
					TxData_BLWaitingForAcknowledgement);
			can_controller_2.send_message(&TxHeader,
					TxData_BLWaitingForAcknowledgement);
			break;
		case State_t::BLWaitingForFrameCount:
			can_controller_1.send_message(&TxHeader,
					TxData_BLWaitingForFrameCount);
			break;
		case State_t::BLWaitingForCRC:
			can_controller_1.send_message(&TxHeader,
					TxData_BLWaitingForCRC);
			break;
		case State_t::BLWaitingForChunkResync:
			can_controller_1.send_message(&TxHeader,
					TxData_BLWaitingForChunkResync);
			break;
		case State_t::BLWaitingForChunkData:
			break;
		}
	}
}

void initialise_can_data()
{
	TxHeader.Identifier = 0x41A; // 1050
	TxHeader.IdType = FDCAN_STANDARD_ID;
	TxHeader.TxFrameType = FDCAN_DATA_FRAME;
	TxHeader.DataLength = FDCAN_DLC_BYTES_8;
	TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
	TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
	TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	TxHeader.MessageMarker = 0;

	TxData_BLWaitingForAcknowledgement[0] = BL_WAITING_FOR_ACKNOWLEDGEMENT_DATA_0;
	TxData_BLWaitingForAcknowledgement[1] = BL_WAITING_FOR_ACKNOWLEDGEMENT_DATA_1;
	TxData_BLWaitingForAcknowledgement[2] = BL_WAITING_FOR_ACKNOWLEDGEMENT_DATA_2;
	TxData_BLWaitingForAcknowledgement[3] = BL_WAITING_FOR_ACKNOWLEDGEMENT_DATA_3;
	TxData_BLWaitingForAcknowledgement[4] = BL_WAITING_FOR_ACKNOWLEDGEMENT_DATA_4;
	TxData_BLWaitingForAcknowledgement[5] = BL_WAITING_FOR_ACKNOWLEDGEMENT_DATA_5;
	TxData_BLWaitingForAcknowledgement[6] = BL_WAITING_FOR_ACKNOWLEDGEMENT_DATA_6;
	TxData_BLWaitingForAcknowledgement[7] = BL_WAITING_FOR_ACKNOWLEDGEMENT_DATA_7;

	TxData_BLWaitingForFrameCount[0] = BL_WAITING_FOR_FRAMECOUNT_DATA_0;
	TxData_BLWaitingForFrameCount[1] = BL_WAITING_FOR_FRAMECOUNT_DATA_1;
	TxData_BLWaitingForFrameCount[2] = BL_WAITING_FOR_FRAMECOUNT_DATA_2;
	TxData_BLWaitingForFrameCount[3] = BL_WAITING_FOR_FRAMECOUNT_DATA_3;
	TxData_BLWaitingForFrameCount[4] = BL_WAITING_FOR_FRAMECOUNT_DATA_4;
	TxData_BLWaitingForFrameCount[5] = BL_WAITING_FOR_FRAMECOUNT_DATA_5;
	TxData_BLWaitingForFrameCount[6] = BL_WAITING_FOR_FRAMECOUNT_DATA_6;
	TxData_BLWaitingForFrameCount[7] = BL_WAITING_FOR_FRAMECOUNT_DATA_7;

	TxData_BLWaitingForCRC[0] = BL_WAITING_FOR_CRC_DATA_0;
	TxData_BLWaitingForCRC[1] = BL_WAITING_FOR_CRC_DATA_1;
	TxData_BLWaitingForCRC[2] = BL_WAITING_FOR_CRC_DATA_2;
	TxData_BLWaitingForCRC[3] = BL_WAITING_FOR_CRC_DATA_3;
	TxData_BLWaitingForCRC[4] = BL_WAITING_FOR_CRC_DATA_4;
	TxData_BLWaitingForCRC[5] = BL_WAITING_FOR_CRC_DATA_5;
	TxData_BLWaitingForCRC[6] = BL_WAITING_FOR_CRC_DATA_6;
	TxData_BLWaitingForCRC[7] = BL_WAITING_FOR_CRC_DATA_7;

	TxData_BLWaitingForChunkResync[0] = BL_WAITING_FOR_CHUNK_RESYNC_DATA_0;
	TxData_BLWaitingForChunkResync[1] = BL_WAITING_FOR_CHUNK_RESYNC_DATA_1;
	TxData_BLWaitingForChunkResync[2] = BL_WAITING_FOR_CHUNK_RESYNC_DATA_2;
	TxData_BLWaitingForChunkResync[3] = BL_WAITING_FOR_CHUNK_RESYNC_DATA_3;
	TxData_BLWaitingForChunkResync[4] = BL_WAITING_FOR_CHUNK_RESYNC_DATA_4;
	TxData_BLWaitingForChunkResync[5] = BL_WAITING_FOR_CHUNK_RESYNC_DATA_5;
	TxData_BLWaitingForChunkResync[6] = BL_WAITING_FOR_CHUNK_RESYNC_DATA_6;
	TxData_BLWaitingForChunkResync[7] = BL_WAITING_FOR_CHUNK_RESYNC_DATA_7;
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

	FDCAN_FilterTypeDef sFilterConfig;
	sFilterConfig.IdType = FDCAN_STANDARD_ID;
	sFilterConfig.FilterIndex = 0;
	sFilterConfig.FilterType = FDCAN_FILTER_MASK;
	sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	sFilterConfig.FilterID1 = 0x7A4; // 1956
	sFilterConfig.FilterID2 = 0x7FF; // 11 bits (2047)

	HAL_StatusTypeDef hfdcan1_status = HAL_FDCAN_ConfigFilter(&hfdcan1,
			&sFilterConfig);
	HAL_StatusTypeDef hfdcan2_status = HAL_FDCAN_ConfigFilter(&hfdcan2,
			&sFilterConfig);

	HAL_StatusTypeDef hfdcan1_global_status = HAL_FDCAN_ConfigGlobalFilter(
			&hfdcan1,
			FDCAN_REJECT,
			FDCAN_REJECT,
			FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

	HAL_StatusTypeDef hfdcan2_global_status = HAL_FDCAN_ConfigGlobalFilter(
			&hfdcan2,
			FDCAN_REJECT,
			FDCAN_REJECT,
			FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

	if (hfdcan1_status != HAL_OK || hfdcan2_status != HAL_OK
			|| hfdcan1_global_status != HAL_OK
			|| hfdcan2_global_status != HAL_OK)
		Error_Handler();

	can_controller_1.init(&hfdcan1);
	can_controller_2.init(&hfdcan2);

	can_controller_1.silence(false);
	can_controller_2.silence(false);

#ifdef DEBUG
	uart_controller_1.init(&huart1, USART1_IRQn);

	for (uint8_t i = 0; i < 8; i++)
		uart_controller_1.write_bytes("\r\n", 2);
	uart_controller_1.write_bytes("============\r\n", 14);
	uart_controller_1.write_bytes(" Bootloader \r\n", 14);
	uart_controller_1.write_bytes("============\r\n", 14);
#endif

	HAL_TIM_Base_Start_IT(&htim2);

	while (!branch)
	{

	}

	HAL_TIM_Base_Stop_IT(&htim2);
	HAL_TIM_Base_DeInit(&htim2);

	HAL_FDCAN_DeInit(&hfdcan1);
	HAL_FDCAN_DeInit(&hfdcan2);

	HAL_UART_Abort_IT(&huart1);
	HAL_UART_DeInit(&huart1);

	HAL_GPIO_DeInit(GPIOB, LED_Pin | FDCAN2_S_Pin);
	HAL_GPIO_DeInit(FDCAN1_S_GPIO_Port, FDCAN1_S_Pin);

	bootloader_jump_to_user_app();

	return 0;
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
