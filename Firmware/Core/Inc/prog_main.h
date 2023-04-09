#ifndef INC_PROG_MAIN_H_
#define INC_PROG_MAIN_H_

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#include "stm32g0xx_hal.h"

/**
 * \fn prog_main
 * \brief The main program for all projects
 * \return int
 */
EXTERNC int prog_main(void);

#endif /* INC_PROG_MAIN_H_ */
