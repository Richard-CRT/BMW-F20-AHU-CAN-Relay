/*
 * prog_main.h
 *
 *  Created on: Apr 21, 2023
 *      Author: richa
 */

#ifndef INC_BL_MAIN_H_
#define INC_BL_MAIN_H_

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

void bootloader_jump_to_user_app();

EXTERNC void bl_init(void);
EXTERNC int bl_main(void);

#endif /* INC_BL_MAIN_H_ */
