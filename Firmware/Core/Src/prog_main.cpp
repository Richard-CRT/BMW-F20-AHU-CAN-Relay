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

int prog_main(void)
{
	uart_controller_1.init(&huart1, USART1_IRQn);
	can_controller_1.init(&hfdcan1);
	can_controller_2.init(&hfdcan2);

	HAL_Delay(100);

	while (1)
	{
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

		rx_can_message_t rx_can_message;
		while (can_controller_1.read_message_0(&rx_can_message))
		{
			char buff[100];
			snprintf(buff, sizeof(buff), "CAN1 received: %lx %lu\r\n", rx_can_message.RxHeader.Identifier,					(rx_can_message.RxHeader.DataLength >> 16));
			uart_controller_1.write_bytes(buff);
		}
		while (can_controller_2.read_message_0(&rx_can_message))
		{
			char buff[100];
			snprintf(buff, sizeof(buff), "CAN2 received: %lx %lu\r\n", rx_can_message.RxHeader.Identifier,					(rx_can_message.RxHeader.DataLength >> 16));
			uart_controller_1.write_bytes(buff);
		}

		can_controller_1.send_message(0);
		can_controller_2.send_message(0);

		HAL_Delay(500);
	}
}
