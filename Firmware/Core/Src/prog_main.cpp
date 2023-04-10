#include "prog_main.h"

#include "main.h"
#include "uart_controllers.h"
#include "can_controllers.h"

#include <cstdio>
#include <cstring>

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern UART_HandleTypeDef huart1;
extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_HandleTypeDef hfdcan2;

extern uart_controller_t uart_controller_1;
extern can_controller_t can_controller_1;
extern can_controller_t can_controller_2;

#define FORWARDING
#ifdef FORWARDING
#define MODIFY_130
#endif

//#define KICKSTART_CAN1
//#define KICKSTART_CAN2

#define SHUTDOWN_DELAY_SECONDS (5 * 60)
#define ECHO_130_DELAY_SECONDS 2
#define ECHO_130_DELAY_HUNDREDTHS_OF_SECONDS (ECHO_130_DELAY_SECONDS * 100)

#ifdef DEBUG
char buff[256];
#endif

bool reset = true;
uint16_t seconds_since_ignition_bit_on = 0;
uint16_t hundredths_of_second_since_last_130 = 0;

#ifdef MODIFY_130
rx_can_message_t last_130_frame;
bool received_130_frame = false;
#endif

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM2)
	{
		// 1 Hz
#ifdef DEBUG
		if (reset)
			uart_controller_1.write_bytes("RESET\r\n");
		snprintf(buff, sizeof(buff), "Tick %u\r\n",
				seconds_since_ignition_bit_on);
		uart_controller_1.write_bytes(buff);
#endif

		if (seconds_since_ignition_bit_on < SHUTDOWN_DELAY_SECONDS)
			seconds_since_ignition_bit_on++;

#ifdef MODIFY_130
		if (!reset)
		{
			// If we haven't reached shutdown delay threshold
			// and we've stopped receiving packets from the car, echo
			if (hundredths_of_second_since_last_130
					>= ECHO_130_DELAY_HUNDREDTHS_OF_SECONDS
					&& seconds_since_ignition_bit_on < SHUTDOWN_DELAY_SECONDS)
			{
				if (received_130_frame)
				{
					// echo the last 130 packet we received
#ifdef DEBUG
					uart_controller_1.write_bytes("ECHO 130\r\n");
#endif

					rx_can_message_t dummy_130_frame;

					memcpy(&dummy_130_frame.RxHeader, &last_130_frame.RxHeader,
							sizeof(dummy_130_frame.RxHeader));
					memcpy(dummy_130_frame.RxData, last_130_frame.RxData,
							8 * sizeof(uint8_t));

					// Trick head unit
					dummy_130_frame.RxData[0] |= 0x01;
					can_controller_1.send_copy_of_rx_message(&dummy_130_frame);
				}
			}
		}
#endif
	}
	else if (htim->Instance == TIM3)
	{
		// 10 Hz
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	}
	else if (htim->Instance == TIM4)
	{
		// 100 Hz
		if (hundredths_of_second_since_last_130
				< ECHO_130_DELAY_HUNDREDTHS_OF_SECONDS)
			hundredths_of_second_since_last_130++;

		// If we've stopped receiving 130s and reached delay threshold, reset
		if (hundredths_of_second_since_last_130
				>= ECHO_130_DELAY_HUNDREDTHS_OF_SECONDS
				&& seconds_since_ignition_bit_on >= SHUTDOWN_DELAY_SECONDS)
		{
			reset = true;
		}

		if (reset)
		{
			seconds_since_ignition_bit_on = 0;
			hundredths_of_second_since_last_130 = 0;
		}
	}
}

void process_can_message(rx_can_message_t *rx_can_message,
		bool received_by_can1_ncan2)
{
// Message incoming
#ifdef DEBUG
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
		can_controller_2.send_copy_of_rx_message(rx_can_message);
	}
	else
	{
#ifdef MODIFY_130
		if (rx_can_message->RxHeader.Identifier == 0x130)
		{
			received_130_frame = true;
			hundredths_of_second_since_last_130 = 0;
			memcpy(&last_130_frame.RxHeader, &rx_can_message->RxHeader,
					sizeof(last_130_frame.RxHeader));
			memcpy(last_130_frame.RxData, rx_can_message->RxData,
					8 * sizeof(uint8_t));

			reset = false;

			// Byte 0 Bit 0 is 1 when the ignition is on
			// While the ignition is on, reset seconds
			if (rx_can_message->RxData[0] & 0x01)
				seconds_since_ignition_bit_on = 0;

			if (seconds_since_ignition_bit_on < SHUTDOWN_DELAY_SECONDS)
				rx_can_message->RxData[0] |= 0x01;
		}
#endif

		can_controller_1.send_copy_of_rx_message(rx_can_message);
	}
#endif
}

int prog_main(void)
{
	HAL_TIM_Base_Start_IT(&htim2);
	HAL_TIM_Base_Start_IT(&htim3);
	HAL_TIM_Base_Start_IT(&htim4);

	uart_controller_1.init(&huart1, USART1_IRQn);
	can_controller_1.init(&hfdcan1);
	can_controller_2.init(&hfdcan2);

	can_controller_1.silence(false);
	can_controller_2.silence(false);

	HAL_Delay(100);

#ifdef DEBUG
	for (uint8_t i = 0; i < 8; i++)
		uart_controller_1.write_bytes("\r\n");
	uart_controller_1.write_bytes("=======================\r\n");
	uart_controller_1.write_bytes(" BMW F20 AHU CAN Relay \r\n");
	uart_controller_1.write_bytes("=======================\r\n");

#endif

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
	uart_controller_1.write_bytes("Initial kick-start CAN1 sending...\r\n");
#endif
	uint8_t TxData1[] =
	{ 0xCA, 0x10, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB };

	TxHeader.Identifier = 0x1CA;

	can_controller_1.send_message(&TxHeader, TxData1);
#endif

#ifdef KICKSTART_CAN2
#ifdef DEBUG
	uart_controller_1.write_bytes("Initial kick-start CAN2 sending...\r\n");
#endif
	uint8_t TxData2[] =
	{ 0xCA, 0x20, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10 };

	TxHeader.Identifier = 0x2CA;

	can_controller_2.send_message(&TxHeader, TxData2);
#endif

	while (1)
	{
		rx_can_message_t rx_can_message;
		while (can_controller_1.read_message_0(&rx_can_message))
		{
			// Message incoming from head-unit
			process_can_message(&rx_can_message, true);
		}
		while (can_controller_2.read_message_0(&rx_can_message))
		{
			// Message incoming from car
			process_can_message(&rx_can_message, false);
		}
	}
}
