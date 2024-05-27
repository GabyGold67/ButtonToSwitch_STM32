/**
  ******************************************************************************
  * @file	: 06_TmLtchMPBttn_b.cpp
  * @brief  : Example for the MpbAsSwitch_STM32 library TmlLtchMPBttn class
  *
  * 	The example instantiates a TmLtchMPBttn object using:
  * 		- The Nucleo board user pushbutton attached to GPIO_B00
  * 		- The Nucleo board user LED attached to GPIO_A05
  * 		- A LED attached to GPIO_C00 to visualize the isEnabled attribute flag status
  *
  * 	This example includes a timer that periodically toggle the isEnabled attribute flag value
  * 	showing the behavior of the instantiated object when enabled and when disabled.
  *
  * 	@author	: Gabriel D. Goldman
  *
  * 	@date	: 	01/01/2024 First release
  * 				22/04/2024 Last update
  *
  ******************************************************************************
  * @attention	This file is part of the Examples folder for the MPBttnAsSwitch_ESP32
  * library. All files needed are provided as part of the source code for the library.
  *
  ******************************************************************************
  */
//----------------------- BEGIN Specific to use STM32F4xxyy testing platform
#define MCU_SPEC
//======================> Replace the following two lines with the files corresponding with the used STM32 configuration files
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
//----------------------- End Specific to use STM32F4xxyy testing platform

/* Private includes ----------------------------------------------------------*/
//===========================>> Next lines used to avoid CMSIS wrappers
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "event_groups.h"
//===========================>> Previous lines used to avoid CMSIS wrappers

/* USER CODE BEGIN Includes */
#include "../../mpbAsSwitch_STM32/src/mpbAsSwitch_STM32.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
gpioPinId_t tstLedOnBoard{GPIOA, GPIO_PIN_5};	// Pin 0b 0010 0000
gpioPinId_t tstMpbOnBoard{GPIOC, GPIO_PIN_13};	// Pin 0b 0010 0000 0000 0000

gpioPinId_t ledOnPC00{GPIOC, GPIO_PIN_0};	//Pin 0b 0000 0001
TaskHandle_t tstDefTaskHandle {NULL};
BaseType_t xReturned;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void Error_Handler(void);

void tstDefTaskExec(void *pvParameters);
void swpEnableCb(TimerHandle_t  pvParam);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  /* Create the thread(s) */
  /* USER CODE BEGIN RTOS_THREADS */
  xReturned = xTaskCreate(
		  tstDefTaskExec, //taskFunction
		  "TstMainTask", //Task function legible name
		  256, // Stack depth in words
		  NULL,	//Parameters to pass as arguments to the taskFunction
		  configTIMER_TASK_PRIORITY,	//Set to the same priority level as the software timers
		  &tstDefTaskHandle
		  );
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  vTaskStartScheduler();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  while (1)
  {
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level for tstLedOnBoard*/
  HAL_GPIO_WritePin(tstLedOnBoard.portId, tstLedOnBoard.pinNum, GPIO_PIN_RESET);
  /*Configure GPIO pin Output Level for ledOnPC00))*/
  HAL_GPIO_WritePin(ledOnPC00.portId, ledOnPC00.pinNum, GPIO_PIN_RESET);

  /*Configure GPIO pin : tstMpbOnBoard_Pin */
  GPIO_InitStruct.Pin = tstMpbOnBoard.pinNum;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(tstMpbOnBoard.portId, &GPIO_InitStruct);

  /*Configure GPIO pin : tstLedOnBoard_Pin */
  GPIO_InitStruct.Pin = tstLedOnBoard.pinNum;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(tstLedOnBoard.portId, &GPIO_InitStruct);

  /*Configure GPIO pin : ledOnPC00_Pin */
  GPIO_InitStruct.Pin = ledOnPC00.pinNum;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ledOnPC00.portId, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
void tstDefTaskExec(void *pvParameters)
{
	TimerHandle_t enableSwpTmrHndl{NULL};
	BaseType_t tmrModRslt{pdFAIL};

	bool tmpBttnWasOn{false};
	bool tmpBttnIsOn{false};

	TmLtchMPBttn tstBttn(tstMpbOnBoard.portId, tstMpbOnBoard.pinNum, 3000, true, true, 0, 200);
	TmLtchMPBttn* tstBttnPtr {&tstBttn};

	tstBttn.setTmerRstbl(true);
	tstBttn.setIsOnDisabled(false);
	tstBttn.setTrnOffASAP(true);
	tstBttn.begin(20);

	enableSwpTmrHndl = xTimerCreate(
			"EnableSwapTimer",
			10000,
			pdTRUE,
			tstBttnPtr,
			swpEnableCb
			);
	if (enableSwpTmrHndl != NULL){
      tmrModRslt = xTimerStart(enableSwpTmrHndl, portMAX_DELAY);
   }
	if(tmrModRslt == pdFAIL){
	    Error_Handler();
	}

	for(;;)
	{
		tmpBttnWasOn = tmpBttnIsOn;
		tmpBttnIsOn = tstBttn.getIsOn();
		if(tmpBttnWasOn != tmpBttnIsOn){
			if(tmpBttnIsOn)
			  HAL_GPIO_WritePin(tstLedOnBoard.portId, tstLedOnBoard.pinNum, GPIO_PIN_SET);
			else
			  HAL_GPIO_WritePin(tstLedOnBoard.portId, tstLedOnBoard.pinNum, GPIO_PIN_RESET);
		}

		if(!(tstBttnPtr->getIsEnabled())){
			HAL_GPIO_WritePin(ledOnPC00.portId, ledOnPC00.pinNum, GPIO_PIN_SET);
		}
		else{
			HAL_GPIO_WritePin(ledOnPC00.portId, ledOnPC00.pinNum, GPIO_PIN_RESET);
		}
	}
}

void swpEnableCb(TimerHandle_t  pvParam){
	DbncdMPBttn* bttnArg = (DbncdMPBttn*) pvTimerGetTimerID(pvParam);

	bool curEnable = bttnArg->getIsEnabled();

	if(curEnable){
		bttnArg->disable();
	}
	else{
		bttnArg->enable();
	}

  return;
}
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM9 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM9) {
    HAL_IncTick();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
