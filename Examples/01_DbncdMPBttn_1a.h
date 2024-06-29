/**
  ******************************************************************************
  * @file           : 01_DbncdMPBttn_1a.h
  * @brief          : Header for 01_DbncdMPBttn_1a.cpp file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#ifndef __STM32F4xx_HAL_H
	#include "stm32f4xx_hal.h"
#endif

/* Private defines -----------------------------------------------------------*/
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
