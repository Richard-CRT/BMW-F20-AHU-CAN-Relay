#include <app_main.h>
#include "main.h"
#include "uart_controllers.h"
#include "can_controllers.h"
#include "programmer_bootloader_frames.h"

#include <cstdio>
#include <cstring>

#define FLAGS_ADDRESS (FLASH_BASE + (38 * 0x400))

#define LED_INDICATOR
#define FORWARDING
#ifdef FORWARDING
#define MODIFY_130
#endif

//#define KICKSTART_CAN1
//#define KICKSTART_CAN2

#define SHUTDOWN_DELAY_SECONDS (5 * 60)
#define SHUTDOWN_DELAY_HUNDREDTHS_OF_SECONDS (SHUTDOWN_DELAY_SECONDS * 100)
#define ECHO_130_DELAY_SECONDS 2
#define ECHO_130_DELAY_HUNDREDTHS_OF_SECONDS (ECHO_130_DELAY_SECONDS * 100)

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern UART_HandleTypeDef huart1;
extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_HandleTypeDef hfdcan2;

extern uart_controller_t uart_controller_1;
extern can_controller_t<512, 4> can_controller_1;
extern can_controller_t<512, 4> can_controller_2;

#ifdef DEBUG
char buff[256];
#endif

enum class State_t
{
	Idle, PreIdle, F1, Echo
};
volatile State_t state = State_t::Idle;

uint16_t hundredths_of_seconds_since_ignition_bit_on = 0;
uint16_t hundredths_of_seconds_since_last_130 = 0;
uint8_t hundredths_of_seconds_since_last_led_flip = 0;

#ifdef MODIFY_130
rx_can_message_t last_130_frame;
bool received_130_frame = false;
#endif

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM3)
	{
		// 1 Hz
#ifdef DEBUG
		switch (state)
		{
		case State_t::Idle:
			uart_controller_1.write_bytes("Idle\r\n", 6);
			break;
		case State_t::F1:
			uart_controller_1.write_bytes("F1\r\n", 4);
			break;
		case State_t::PreIdle:
			uart_controller_1.write_bytes("PreIdle\r\n", 9);
			break;
		case State_t::Echo:
			uart_controller_1.write_bytes("Echo\r\n", 6);
			break;
		}
		snprintf(buff, sizeof(buff), "SSI %u\r\n",
				hundredths_of_seconds_since_ignition_bit_on / 100); // integer division
		uart_controller_1.write_bytes(buff);
#endif

#ifdef MODIFY_130

		switch (state)
		{
		case State_t::Echo:
			if (received_130_frame)
			{
				rx_can_message_t dummy_130_frame;

				memcpy(&dummy_130_frame.RxHeader, &last_130_frame.RxHeader,
						sizeof(dummy_130_frame.RxHeader));
				memcpy(dummy_130_frame.RxData, last_130_frame.RxData,
						8 * sizeof(uint8_t));

				// Trick head unit
				dummy_130_frame.RxData[0] |= 0x01;
				can_controller_1.send_copy_of_rx_message(&dummy_130_frame);
			}
			break;
		default:
			break;
		}

#endif
	}
	else if (htim->Instance == TIM4)
	{
		// 100 Hz

#ifdef LED_INDICATOR
		bool flip = false;
		switch (state)
		{
		case State_t::Idle:
			if (hundredths_of_seconds_since_last_led_flip >= 200 - 1) // 0.5 Hz
				flip = true;
			break;
		case State_t::PreIdle:
			if (hundredths_of_seconds_since_last_led_flip >= 100 - 1) // 1 Hz
				flip = true;
			break;
		case State_t::F1:
			if (hundredths_of_seconds_since_last_led_flip >= 5 - 1) // 20 Hz
				flip = true;
			break;
		case State_t::Echo:
			if (hundredths_of_seconds_since_last_led_flip >= 20 - 1) // 5 Hz
				flip = true;
			break;
		}

		if (flip)
		{
			hundredths_of_seconds_since_last_led_flip = 0;
			HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		}
		else if (hundredths_of_seconds_since_last_led_flip < 255)
		{
			hundredths_of_seconds_since_last_led_flip++;
		}
#endif

		switch (state)
		{
		case State_t::F1:
		case State_t::PreIdle:
		case State_t::Echo:
			if (hundredths_of_seconds_since_ignition_bit_on
					< SHUTDOWN_DELAY_HUNDREDTHS_OF_SECONDS)
				hundredths_of_seconds_since_ignition_bit_on++;
			break;
		default:
			break;
		}

		if (hundredths_of_seconds_since_ignition_bit_on
				>= SHUTDOWN_DELAY_HUNDREDTHS_OF_SECONDS)
		{
			// TIME

			switch (state)
			{
			case State_t::F1:
				state = State_t::PreIdle;
				break;
			case State_t::Echo:
				state = State_t::Idle;
				break;
			default:
				break;
			}
		}

		switch (state)
		{
		case State_t::F1:
		case State_t::PreIdle:
			if (hundredths_of_seconds_since_last_130
					< ECHO_130_DELAY_HUNDREDTHS_OF_SECONDS)
				hundredths_of_seconds_since_last_130++;
			break;
		default:
			break;
		}

		// If we've stopped receiving 130s and reached delay threshold
		if (hundredths_of_seconds_since_last_130
				>= ECHO_130_DELAY_HUNDREDTHS_OF_SECONDS)
		{
			// LOSS

			switch (state)
			{
			case State_t::F1:
				state = State_t::Echo;
				break;
			case State_t::PreIdle:
				state = State_t::Idle;
				break;
			default:
				break;
			}
		}
	}
}

bool process_can_message(rx_can_message_t *rx_can_message,
		bool received_by_can1_ncan2)
{
// Message incoming
#if defined(DEBUG) && False
	if (rx_can_message->RxHeader.Identifier == 0x130)
	{
		if (rx_can_message->RxHeader.DataLength > FDCAN_DLC_BYTES_8)
			Error_Handler();
		uint8_t bytes_length = rx_can_message->RxHeader.DataLength >> 16;

		if (received_by_can1_ncan2)
			snprintf(buff, sizeof(buff), "C1R %lX %uB",
					rx_can_message->RxHeader.Identifier, bytes_length);
		else
			snprintf(buff, sizeof(buff), "C2R %lX %uB",
					rx_can_message->RxHeader.Identifier, bytes_length);
		uart_controller_1.write_bytes(buff);

		/*
		 uint32_t data_l = 0;
		 uint32_t data_h = 0;
		 for (uint8_t i = 0; i < bytes_length; i++)
		 {
		 if (i < 4)
		 data_l = (data_l << 8) | rx_can_message->RxData[i];
		 else
		 data_h = (data_h << 8) | rx_can_message->RxData[i];
		 }
		 snprintf(buff, sizeof(buff), " 0x%08lX_%08lX", data_l, data_h);
		 uart_controller_1.write_bytes(buff);
		 */

		uart_controller_1.write_bytes("\r\n");
	}
#endif

#ifdef FORWARDING
	if (received_by_can1_ncan2)
	{
		return can_controller_2.send_copy_of_rx_message(rx_can_message);
	}
	else
	{
#ifdef MODIFY_130
		if (rx_can_message->RxHeader.Identifier == 0x130)
		{
			received_130_frame = true;
			memcpy(&last_130_frame.RxHeader, &rx_can_message->RxHeader,
					sizeof(last_130_frame.RxHeader));
			memcpy(last_130_frame.RxData, rx_can_message->RxData,
					8 * sizeof(uint8_t));

			hundredths_of_seconds_since_last_130 = 0;

			// Byte 0 Bit 0 is 1 when the ignition is on
			bool ignition_on = rx_can_message->RxData[0] & 0x01;

			switch (state)
			{
			case State_t::Idle:
			case State_t::Echo:
				hundredths_of_seconds_since_ignition_bit_on = 0;
				state = State_t::F1;
				break;
			case State_t::F1:
			case State_t::PreIdle:
				if (ignition_on)
				{
					hundredths_of_seconds_since_ignition_bit_on = 0;
					state = State_t::F1;
				}
				break;
			}

			switch (state)
			{
			case State_t::F1:
				rx_can_message->RxData[0] |= 0x01;
				break;
			default:
				break;
			}
		}
#endif
		return can_controller_1.send_copy_of_rx_message(rx_can_message);
	}
#endif
}

int app_main(void)
{
	FDCAN_FilterTypeDef sFilterConfig;
	sFilterConfig.IdType = FDCAN_STANDARD_ID;
	sFilterConfig.FilterIndex = 0;
	sFilterConfig.FilterType = FDCAN_FILTER_MASK;
	sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
	sFilterConfig.FilterID1 = 0x7A4; // 1956
	sFilterConfig.FilterID2 = 0x7FF; // 11 bits (2047)

	HAL_StatusTypeDef hfdcan1_status = HAL_FDCAN_ConfigFilter(&hfdcan1,
			&sFilterConfig);
	HAL_StatusTypeDef hfdcan2_status = HAL_FDCAN_ConfigFilter(&hfdcan2,
			&sFilterConfig);

	HAL_StatusTypeDef hfdcan1_global_status = HAL_FDCAN_ConfigGlobalFilter(
			&hfdcan1,
			FDCAN_ACCEPT_IN_RX_FIFO0,
			FDCAN_ACCEPT_IN_RX_FIFO0,
			FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

	HAL_StatusTypeDef hfdcan2_global_status = HAL_FDCAN_ConfigGlobalFilter(
			&hfdcan2,
			FDCAN_ACCEPT_IN_RX_FIFO0,
			FDCAN_ACCEPT_IN_RX_FIFO0,
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
	uart_controller_1.write_bytes("=======================\r\n", 25);
	uart_controller_1.write_bytes(" BMW F20 AHU CAN Relay \r\n", 25);
	uart_controller_1.write_bytes("=======================\r\n", 25);

	uint8_t data;
#endif

	HAL_TIM_Base_Start_IT(&htim3);
	HAL_TIM_Base_Start_IT(&htim4);

#if defined(KICKSTART_CAN1) or defined(KICKSTART_CAN2)
	FDCAN_TxHeaderTypeDef TxHeader;

	TxHeader.IdType = FDCAN_STANDARD_ID;
	TxHeader.TxFrameType = FDCAN_DATA_FRAME;
	TxHeader.DataLength = FDCAN_DLC_BYTES_8;
	TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
	TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
	TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	TxHeader.MessageMarker = 0;
#endif

#ifdef KICKSTART_CAN1
#ifdef DEBUG
	uart_controller_1.write_bytes("Initial kick-start CAN1 sending...\r\n",36);
#endif
	uint8_t TxData1[] =
	{ 0xCA, 0x10, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB };

	TxHeader.Identifier = 0x1CA;

	can_controller_1.send_message(&TxHeader, TxData1);
#endif

#ifdef KICKSTART_CAN2
#ifdef DEBUG
	uart_controller_1.write_bytes("Initial kick-start CAN2 sending...\r\n",36);
#endif
	uint8_t TxData2[] =
	{ 0xCA, 0x20, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10 };

	TxHeader.Identifier = 0x2CA;

	can_controller_2.send_message(&TxHeader, TxData2);
#endif

	rx_can_message_t rx_can_message;
	while (1)
	{
#ifdef DEBUG
		if (uart_controller_1.read_byte(&data))
		{
			uart_controller_1.write_byte(data);
			if (data == 'r')
				flag_bootloader();
		}
#endif

		if (can_controller_1.read_message_0(&rx_can_message))
		{
			// Message incoming from head-unit
			bool processed = process_can_message(&rx_can_message, true);
			if (processed)
				can_controller_1.pop_message_0();
		}
		if (can_controller_2.read_message_0(&rx_can_message))
		{
			// Message incoming from car
			bool processed = process_can_message(&rx_can_message, false);
			if (processed)
				can_controller_2.pop_message_0();
		}

		bool received_programming_frame = false;
		if (can_controller_1.read_message_1(&rx_can_message))
		{
			// Message incoming from head-unit with programming ID
			can_controller_1.pop_message_1();
			received_programming_frame = true;
		}
		if (can_controller_2.read_message_1(&rx_can_message))
		{
			// Message incoming from car with programming ID
			can_controller_2.pop_message_1();
			received_programming_frame = true;
		}
		if (received_programming_frame)
		{
			if (rx_can_message.RxData[0] == PROG_GO_TO_BL_DATA_0
					&& rx_can_message.RxData[1] == PROG_GO_TO_BL_DATA_1
					&& rx_can_message.RxData[2] == PROG_GO_TO_BL_DATA_2
					&& rx_can_message.RxData[3] == PROG_GO_TO_BL_DATA_3
					&& rx_can_message.RxData[4] == PROG_GO_TO_BL_DATA_4
					&& rx_can_message.RxData[5] == PROG_GO_TO_BL_DATA_5
					&& rx_can_message.RxData[6] == PROG_GO_TO_BL_DATA_6
					&& rx_can_message.RxData[7] == PROG_GO_TO_BL_DATA_7)
				flag_bootloader();
		}
	}
}

HAL_StatusTypeDef flag_bootloader()
{
	HAL_FLASH_Unlock();
	FLASH_EraseInitTypeDef EraseInitStruct;
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Banks = FLASH_BANK_1;

	uint32_t page = (FLAGS_ADDRESS - FLASH_BASE) / (2 * 0x400);

	EraseInitStruct.Page = page;
	EraseInitStruct.NbPages = 1;

	uint32_t PageError;
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) //Erase the Page Before a Write Operation
		return HAL_ERROR;

	HAL_Delay(50);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, FLAGS_ADDRESS, 1);
	HAL_Delay(50);
	HAL_FLASH_Lock();

	NVIC_SystemReset();

	return HAL_OK;
}

