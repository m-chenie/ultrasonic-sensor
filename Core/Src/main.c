/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "string.h"
#include <stdio.h>


void SystemClock_Config(void);

/**
  * @brief  The application entry point.
  * @retval int
  */

void usTimer(uint32_t uSec);

uint8_t icFlag = 0;
uint8_t captureIdx=0;
uint32_t edge1Time=0, edge2Time=0;

const float speedOfSound = 0.0343/2;
float distance;

uint8_t uartBuf[100];

int main(void)
{
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
//	  strcpy((char*)uartBuf, "Hello\r\n");
//	  HAL_UART_Transmit(&huart2, uartBuf, strlen((char*)uartBuf), 1000);
//	  HAL_Delay(1000);

//     set trigger to low for a few seconds
	  HAL_GPIO_WritePin(trigger_GPIO_Port, trigger_Pin, 0);
	  usTimer(3);

	  // output 10 high pulse
	  HAL_GPIO_WritePin(trigger_GPIO_Port, trigger_Pin, 1);
	  usTimer(10);
	  HAL_GPIO_WritePin(trigger_GPIO_Port, trigger_Pin, 0);

	  // start input capture timer
	  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);

	  // wait for IC flag
	  uint32_t startTick = HAL_GetTick();

	  uint16_t timerState = HAL_TIM_IC_GetState(&htim3);

	  do{
		  if(icFlag) break;
	  }while((HAL_GetTick() - startTick) < 500);
	  icFlag = 0;
	  HAL_TIM_IC_Stop_IT(&htim3, TIM_CHANNEL_1);


	  //Calculate distance in cm
	if(edge2Time > edge1Time)
	{
		distance = ((edge2Time - edge1Time) + 0.0f)*speedOfSound;
	}
	else
	{
		distance = 0.0f;
	}

	//Print to UART terminal for debugging

	sprintf((char*)uartBuf, "Distance (mm)  = %.1f\r\n", distance);
	HAL_UART_Transmit(&huart2, uartBuf, strlen((char*)uartBuf), 100);

	HAL_Delay(1000);


  }
  /* USER CODE END 3 */
}

void usTimer(uint32_t uSec)
{

	//When an object is accessed through a pointer, the arrow operator -> is used to access its members
	// TIM4 is a pointer to the timer object

	TIM4->ARR = uSec-1; // set auto reload to uSec
	// writes 1 in the EGR reinitializes the timer counter and generates an update of the registers.
	// This ensures that the timer starts counting from 0 with the new ARR value.
	TIM4->EGR = 1; //This line sets the Event Generation Register (EGR) to generate an update event

	//acknowledge that an update event has occurred and to reset the Update Interrupt Flag for the next event
	TIM4->SR &= ~1; // clears the least significant bit (LSB) of the status register
	// ~ bitwise not operation (inverts all the bits of its operand)
	// &= bitwise AND assignment operator. It performs a bitwise AND operation between the left and right operands and assigns the result back to the left operand.

	// This line enables the timer by setting the Counter Enable (CEN) bit (bit 0) in the Control Register 1 (CR1).
	TIM4->CR1 |= 1;

	// waits in a loop until the UIF bit in the SR is set, indicating that the timer counter has reached the ARR value and the desired delay has elapsed
	while((TIM4->SR&0x0001) != 1);

	// clears the UIF bit again to reset the status of the timer
	TIM4->SR &= ~(0x0001);

}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1){

		HAL_GPIO_TogglePin(greenLed_GPIO_Port, greenLed_Pin);

			if(captureIdx == 0) // first edge
			{
				edge1Time = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1); //__HAL_TIM_GetCounter(&htim3);//

				captureIdx = 1;
			}
			else if(captureIdx == 1) //2nd edge
			{
				edge2Time = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
				captureIdx = 0;
				icFlag = 1;
			}
	}
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
