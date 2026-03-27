/*
 * uart_app.h
 *
 *  Created on: Mar 22, 2026
 *      Author: parkdoyoung
 */



#ifndef INC_UART_APP_H_
#define INC_UART_APP_H_

#include "main.h"

void UartApp_Init(void);
void UartApp_Task(void);
void UartApp_RxCpltCallback(UART_HandleTypeDef *huart);

#endif /* INC_UART_APP_H_ */
