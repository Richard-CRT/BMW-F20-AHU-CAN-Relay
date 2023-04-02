#include "prog_main.h"

#include "uart_controllers.h"
#include "can_controllers.h"

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

	char test_uart_message[] = "Test UART message\r\n"; // Data to send
	while (1)
	{
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		uart_controller_1.write_bytes(test_uart_message, sizeof(test_uart_message));
		uint8_t byte;
		while (uart_controller_1.read_byte(&byte))
			uart_controller_1.write_byte(byte);
		uart_controller_1.write_bytes("\r\n", 2);
		HAL_Delay(500);
	}
}
