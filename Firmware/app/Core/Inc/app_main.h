#ifndef INC_APP_MAIN_H_
#define INC_APP_MAIN_H_

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#include "stm32g0xx_hal.h"
#include "rx_can_message_t.h"
#include <stdbool.h>

bool process_can_message(rx_can_message_t *rx_can_message,
		bool received_by_can1_ncan2);

/**
 * \fn prog_main
 * \brief The main program for all projects
 * \return int
 */
EXTERNC int app_main(void);

HAL_StatusTypeDef flag_bootloader();

#endif /* INC_APP_MAIN_H_ */
