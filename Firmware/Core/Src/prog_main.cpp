#include "prog_main.h"

#include "main.h"
#include "uart_controllers.h"
#include "can_controllers.h"

#include <cstdio>

extern TIM_HandleTypeDef htim1;
extern UART_HandleTypeDef huart1;
extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_HandleTypeDef hfdcan2;

extern uart_controller_t uart_controller_1;
extern can_controller_t can_controller_1;
extern can_controller_t can_controller_2;

#define FORWARDING
//#define KICKSTART_CAN1
//#define KICKSTART_CAN2
#define SHUTDOWN_DELAY_SECONDS 300

#ifdef DEBUG
char buff[100];
#endif

uint16_t seconds = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
#ifdef DEBUG
	snprintf(buff, sizeof(buff), "Tick %u\r\n", seconds);
	uart_controller_1.write_bytes(buff);
#endif
	if (seconds < SHUTDOWN_DELAY_SECONDS)
		seconds++;
}

void process_can_message(rx_can_message_t *rx_can_message,
		bool received_by_can1_ncan2)
{
	// Message incoming
#ifdef DEBUG

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
#endif

#ifdef FORWARDING
	if (received_by_can1_ncan2)
	{
		can_controller_2.send_copy_of_rx_message(rx_can_message);
	}
	else
	{
		if (rx_can_message->RxHeader.Identifier == 0x130)
		{
			// Byte 0 Bit 0 is 1 when the ignition is on

			// While the ignition is on, reset seconds
			if (rx_can_message->RxData[0] & 0x01)
			{
				seconds = 0;
			}
			else if (seconds < SHUTDOWN_DELAY_SECONDS)
			{
				// Haven't reached delay threshold yet, trick head unit
				rx_can_message->RxData[0] |= 0x01;
			}
		}

		can_controller_1.send_copy_of_rx_message(rx_can_message);
	}
#endif
}

int prog_main(void)
{
	HAL_TIM_Base_Start_IT(&htim1);

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
	snprintf(buff, sizeof(buff), "Initial kick-start CAN1 sending... ");
	uart_controller_1.write_bytes(buff);
#endif
	uint8_t TxData1[] =
	{ 0xCA, 0x10, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB };

	TxHeader.Identifier = 0x1CA;

	can_controller_1.send_message(&TxHeader, TxData1);
#endif

#ifdef KICKSTART_CAN2
#ifdef DEBUG
	snprintf(buff, sizeof(buff), "Initial kick-start CAN2 sending... ");
	uart_controller_1.write_bytes(buff);
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
