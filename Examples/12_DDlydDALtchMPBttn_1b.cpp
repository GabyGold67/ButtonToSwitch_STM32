/**
  ******************************************************************************
  * @file	: 12_DDlydDALtchMPBttn_1b.cpp
  * @brief  : Example for the ButtonToSwitch for STM32 library DDlydDALtchMPBttn class
  *
  * 	The example instantiates a DDlydDALtchMPBttn object using:
  * 	- The Nucleo board user pushbutton attached to GPIO_B00
  * 	- The Nucleo board user LED attached to GPIO_A05 to visualize the isOn attribute flag status
  * 	- A digital output to GPIO_PC01 to show the isOnScndry attribute flag status.
  *	- A digital output to GPIO_PC00 to show the isEnabled attribute flag state
  *
  * ### This example creates one Task and a timer:
  *
  * This simple example creates a single Task, instantiates the DDlydDALtchMPBttn object
  * in it and checks it's attribute flags locally through the getters methods.
  * When a change in the outputs attribute flags values is detected, it manages the
  * loads and resources that the switch turns On and Off, in this example case are
  * the output of some GPIO pins.
  *
  * A software timer is created so that it periodically toggles the isEnabled attribute flag
  * value, showing the behavior of the instantiated object when enabled and when disabled.
  *
  * 	@author	: Gabriel D. Goldman
  *
  * 	@date	: 	01/01/2024 First release
  * 				11/06/2024 Last update
  *
  ******************************************************************************
  * @attention	This file is part of the Examples folder for the ButtonToSwitch for STM32
  * library. All files needed are provided as part of the source code for the library.
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
//===========================>> Previous lines used to avoid CMSIS wrappers
/* USER CODE BEGIN Includes */
#include "../../ButtonToSwitch_STM32/src/ButtonToSwitch_STM32.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
gpioPinId_t tstLedOnBoard{GPIOA, GPIO_PIN_5};	// Pin 0b 0000 0000 0010 0000
gpioPinId_t tstMpbOnBoard{GPIOC, GPIO_PIN_13};	// Pin 0b 0010 0000 0000 0000
gpioPinId_t ledIsEnabled{GPIOC, GPIO_PIN_0};		// Pin 0b 0000 0000 0000 0001
gpioPinId_t ledIsOnScndry{GPIOC, GPIO_PIN_1};	// Pin 0b 0000 0000 0000 0010

TaskHandle_t mainCtrlTskHndl {NULL};
BaseType_t xReturned;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void Error_Handler(void);

/* USER CODE BEGIN FP */
// Tasks
void mainCtrlTsk(void *pvParameters);
// SW Timers
void swpEnableCb(TimerHandle_t  pvParam);
/* USER CODE END FP */

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

  /* Create the thread(s) */
  /* USER CODE BEGIN RTOS_THREADS */
  xReturned = xTaskCreate(
		  mainCtrlTsk, //taskFunction
		  "MainControlTask", //Task function legible name
		  256, // Stack depth in words
		  NULL,	//Parameters to pass as arguments to the taskFunction
		  configTIMER_TASK_PRIORITY,	//Set to the same priority level as the software timers
		  &mainCtrlTskHndl);
  if(xReturned != pdPASS)
	  Error_Handler();
/* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  vTaskStartScheduler();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  while (1)
  {
  }
}
/* USER CODE BEGIN */
void mainCtrlTsk(void *pvParameters)
{
	TimerHandle_t enableSwpTmrHndl{NULL};
	BaseType_t tmrModRslt{pdFAIL};

	DDlydDALtchMPBttn tstBttn(tstMpbOnBoard.portId, tstMpbOnBoard.pinNum, true, true, 50, 50);
	DblActnLtchMPBttn* tstBttnPtr {&tstBttn};
	tstBttn.setScndModActvDly(2000);
	tstBttn.setIsOnDisabled(false);

	enableSwpTmrHndl = xTimerCreate(
			"EnableSwapTimer",
			15000,
			pdTRUE,
			tstBttnPtr,
			swpEnableCb
			);

	tstBttn.begin(5);

	if (enableSwpTmrHndl != NULL)
      tmrModRslt = xTimerStart(enableSwpTmrHndl, portMAX_DELAY);
	if(tmrModRslt == pdFAIL)
	    Error_Handler();

	for(;;)
	{
		if(tstBttn.getOutputsChange()){
			if(tstBttn.getIsOn())
			  HAL_GPIO_WritePin(tstLedOnBoard.portId, tstLedOnBoard.pinNum, GPIO_PIN_SET);
			else
			  HAL_GPIO_WritePin(tstLedOnBoard.portId, tstLedOnBoard.pinNum, GPIO_PIN_RESET);

			if(tstBttn.getIsOnScndry())
			  HAL_GPIO_WritePin(ledIsOnScndry.portId, ledIsOnScndry.pinNum, GPIO_PIN_SET);
			else
			  HAL_GPIO_WritePin(ledIsOnScndry.portId, ledIsOnScndry.pinNum, GPIO_PIN_RESET);

			if(!(tstBttn.getIsEnabled()))
			  HAL_GPIO_WritePin(ledIsEnabled.portId, ledIsEnabled.pinNum, GPIO_PIN_SET);
			else
			  HAL_GPIO_WritePin(ledIsEnabled.portId, ledIsEnabled.pinNum, GPIO_PIN_RESET);
		}
	}
}

void swpEnableCb(TimerHandle_t  pvParam){
	TgglLtchMPBttn* bttnArg = (TgglLtchMPBttn*) pvTimerGetTimerID(pvParam);

	bool curEnable = bttnArg->getIsEnabled();

	if(curEnable)
		bttnArg->disable();
	else
		bttnArg->enable();

  return;
}
/* USER CODE END */


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

  /*Configure GPIO pin : tstMpbOnBoard_Pin */
  GPIO_InitStruct.Pin = tstMpbOnBoard.pinNum;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(tstMpbOnBoard.portId, &GPIO_InitStruct);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(tstLedOnBoard.portId, tstLedOnBoard.pinNum, GPIO_PIN_RESET);

  /*Configure GPIO pin : tstLedOnBoard_Pin */
  GPIO_InitStruct.Pin = tstLedOnBoard.pinNum;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(tstLedOnBoard.portId, &GPIO_InitStruct);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ledIsOnScndry.portId, ledIsOnScndry.pinNum, GPIO_PIN_RESET);
  /*Configure GPIO pin : ledIsOnScndry */
  GPIO_InitStruct.Pin = ledIsOnScndry.pinNum;
  HAL_GPIO_Init(ledIsOnScndry.portId, &GPIO_InitStruct);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ledIsEnabled.portId, ledIsEnabled.pinNum, GPIO_PIN_RESET);
  /*Configure GPIO pin : ledIsEnabled */
  GPIO_InitStruct.Pin = ledIsEnabled.pinNum;
  HAL_GPIO_Init(ledIsEnabled.portId, &GPIO_InitStruct);
}

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
