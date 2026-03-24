/*
 * uart_cmd.h
 *
 *  Created on: Mar 23, 2026
 *      Author: parkdoyoung
 */

#ifndef INC_UART_CMD_H_
#define INC_UART_CMD_H_


#include "stm32f4xx_hal.h"
#include <stdint.h>

void UartCmd_Init(void);
void UartCmd_Task(void);
void UartCmd_RxCpltCallback(UART_HandleTypeDef *huart);



#endif /* INC_UART_CMD_H_ */
