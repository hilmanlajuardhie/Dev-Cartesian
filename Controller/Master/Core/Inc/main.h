/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CONV_STEP_Pin GPIO_PIN_0
#define CONV_STEP_GPIO_Port GPIOB
#define CONV_DIR_Pin GPIO_PIN_1
#define CONV_DIR_GPIO_Port GPIOB
#define CONV_EN_Pin GPIO_PIN_2
#define CONV_EN_GPIO_Port GPIOB
#define LASER_SENS_Pin GPIO_PIN_8
#define LASER_SENS_GPIO_Port GPIOA
#define VERT_STEP_Pin GPIO_PIN_3
#define VERT_STEP_GPIO_Port GPIOB
#define VERT_DIR_Pin GPIO_PIN_4
#define VERT_DIR_GPIO_Port GPIOB
#define VERT_EN_Pin GPIO_PIN_5
#define VERT_EN_GPIO_Port GPIOB
#define HORZ_STEP_Pin GPIO_PIN_6
#define HORZ_STEP_GPIO_Port GPIOB
#define HORZ_DIR_Pin GPIO_PIN_7
#define HORZ_DIR_GPIO_Port GPIOB
#define HORZ_EN_Pin GPIO_PIN_8
#define HORZ_EN_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
