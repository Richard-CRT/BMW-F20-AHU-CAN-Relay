#include "prog_main.h"

#include "main.h"
#include "uart_controllers.h"
#include "can_controllers.h"

#include <cstdio>

extern UART_HandleTypeDef huart1;
extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_HandleTypeDef hfdcan2;

extern uart_controller_t uart_controller_1;
extern can_controller_t can_controller_1;
extern can_controller_t can_controller_2;

#define KICKSTART_CAN1
#define KICKSTART_CAN2

#ifdef DEBUG
uint32_t count = 0;
char buff[100];
#endif

void process_can_message(rx_can_message_t *rx_can_message,
		bool received_by_can1_ncan2)
{
#ifdef DEBUG
	// Message incoming

	if (rx_can_message->RxHeader.DataLength > FDCAN_DLC_BYTES_8)
		Error_Handler();
	uint8_t bytes_length = rx_can_message->RxHeader.DataLength >> 16;

	if (received_by_can1_ncan2)
		snprintf(buff, sizeof(buff), "CAN1 received: ID 0x%lX Len %uB Data ",
				rx_can_message->RxHeader.Identifier, bytes_length);
	else
		snprintf(buff, sizeof(buff), "CAN2 received: ID 0x%lX Len %uB Data ",
				rx_can_message->RxHeader.Identifier, bytes_length);
	uart_controller_1.write_bytes(buff);
	uint32_t data_l = 0;
	uint32_t data_h = 0;
	for (uint8_t i = 0; i < bytes_length; i++)
	{
		if (i < 4)
			data_l = (data_l << 8) | rx_can_message->RxData[i];
		else
			data_h = (data_h << 8) | rx_can_message->RxData[i];
	}
	snprintf(buff, sizeof(buff), "0x%lX_%lX \r\n", data_l, data_h);
	uart_controller_1.write_bytes(buff);

	if (received_by_can1_ncan2)
		snprintf(buff, sizeof(buff), "%lu CAN2 sending... ", count);
	else
		snprintf(buff, sizeof(buff), "%lu CAN1 sending... ", count);
	uart_controller_1.write_bytes(buff);
	count++;
#endif

	if (received_by_can1_ncan2)
		can_controller_2.send_copy_of_rx_message(rx_can_message);
	else
		can_controller_1.send_copy_of_rx_message(rx_can_message);
}

int prog_main(void)
{
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
#ifdef DEBUG
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
#endif

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

#ifdef DEBUG
		HAL_Delay(100);
#else
		HAL_Delay(1);
#endif
	}
}
