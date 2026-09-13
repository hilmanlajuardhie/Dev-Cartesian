/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MY_MCU_ID      1
#define MAX_ROWS       10
#define MAX_COLS       10
#define STEPS_PER_UNIT 100
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

PCD_HandleTypeDef hpcd_USB_FS;

/* USER CODE BEGIN PV */
int current_row = 0;
int current_col = 0;

#define RX_BUFFER_SIZE 15
uint8_t rx_byte;
char rx_str_buffer[RX_BUFFER_SIZE];
uint8_t rx_index = 0;
volatile uint8_t command_ready = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USB_PCD_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void delay_us(uint32_t us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

void send_status(const char* format, ...) {
    char buffer[64];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (len > 0) {
        char final_msg[80];
        int final_len = snprintf(final_msg, sizeof(final_msg), "%s\r\n", buffer);
        HAL_UART_Transmit(&huart1, (uint8_t*)final_msg, final_len, HAL_MAX_DELAY);
    }
}

void execute_motor_steps(const char* axis_name, int total_steps, int forward) {
    if (strcmp(axis_name, "Vertical") == 0) {
        GPIO_PinState dir_state = forward ? GPIO_PIN_SET : GPIO_PIN_RESET;
        HAL_GPIO_WritePin(GPIOB, VERT_DIR_Pin, dir_state);
        HAL_GPIO_WritePin(GPIOB, VERT_EN_Pin, GPIO_PIN_RESET); // Enable driver

        for (int i = 0; i < total_steps; i++) {
            HAL_GPIO_WritePin(GPIOB, VERT_STEP_Pin, GPIO_PIN_SET);
            delay_us(5);
            HAL_GPIO_WritePin(GPIOB, VERT_STEP_Pin, GPIO_PIN_RESET);
            delay_us(5);
        }
    } else {
        GPIO_PinState dir_state = forward ? GPIO_PIN_SET : GPIO_PIN_RESET;
        HAL_GPIO_WritePin(GPIOB, HORZ_DIR_Pin, dir_state);
        HAL_GPIO_WritePin(GPIOB, HORZ_EN_Pin, GPIO_PIN_RESET); // Enable driver

        for (int i = 0; i < total_steps; i++) {
            HAL_GPIO_WritePin(GPIOB, HORZ_STEP_Pin, GPIO_PIN_SET);
            delay_us(5);
            HAL_GPIO_WritePin(GPIOB, HORZ_STEP_Pin, GPIO_PIN_RESET);
            delay_us(5);
        }
    }
}

void move_to_coordinate(int target_row, int target_col) {
    if (target_row < 0 || target_row >= MAX_ROWS || target_col < 0 || target_col >= MAX_COLS) {
        return;
    }

    int row_diff = target_row - current_row;
    int col_diff = target_col - current_col;

    if (row_diff != 0) {
        int forward = (row_diff > 0) ? 1 : 0;
        int total_steps = abs(row_diff) * STEPS_PER_UNIT;
        execute_motor_steps("Vertical", total_steps, forward);
        current_row = target_row;
    }

    if (col_diff != 0) {
        int forward = (col_diff > 0) ? 1 : 0;
        int total_steps = abs(col_diff) * STEPS_PER_UNIT;
        execute_motor_steps("Horizontal", total_steps, forward);
        current_col = target_col;
    }
}

void run_conveyor_until_sensor(void) {
    HAL_GPIO_WritePin(GPIOB, CONV_DIR_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, CONV_EN_Pin, GPIO_PIN_RESET);

    while (HAL_GPIO_ReadPin(LASER_SENS_GPIO_Port, LASER_SENS_Pin) == GPIO_PIN_SET) {
        HAL_GPIO_WritePin(GPIOB, CONV_STEP_Pin, GPIO_PIN_SET);
        delay_us(1000);
        HAL_GPIO_WritePin(GPIOB, CONV_STEP_Pin, GPIO_PIN_RESET);
        delay_us(1000);
    }

    send_status("Laser: Object Detected");
    
    HAL_GPIO_WritePin(GPIOB, CONV_EN_Pin, GPIO_PIN_SET);
    send_status("Conveyor: Stoped");
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USB_PCD_Init();
  /* USER CODE BEGIN 2 */
  // Start microsecond timer
  HAL_TIM_Base_Start(&htim2);

  // Start listening for incoming commands on UART1 via interrupt
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (command_ready){
command_ready = 0;
      
      send_status("1. Message Received: \"%s\"", rx_str_buffer);
      
      if (strlen(rx_str_buffer) >= 3) {
        int target_mcu_id = rx_str_buffer[0] - '0';
        char row_char = rx_str_buffer[1];
        int target_c = atoi(&rx_str_buffer[2]);
        
        if (target_mcu_id != MY_MCU_ID) {
          send_status("2. Address Wrong: \"%s\"", rx_str_buffer);
          send_status("3. Sending to MCU: %d", target_mcu_id);
          
          char forward_msg[RX_BUFFER_SIZE + 2];
          sprintf(forward_msg, "%s\r\n", rx_str_buffer);
          HAL_UART_Transmit(&huart2, (uint8_t*)forward_msg, strlen(forward_msg), HAL_MAX_DELAY);
        } else {
          send_status("4. Address Confirmed: \"%s\"", rx_str_buffer);
          send_status("5. Current Position: %d, %c, %d", MY_MCU_ID, current_row + 'A', current_col);
          
          int target_r = -1;
          if (row_char >= 'A' && row_char <= 'J') {
              target_r = row_char - 'A';
          } else if (row_char >= 'a' && row_char <= 'j') {
              target_r = row_char - 'a';
          }
          
          if (target_r >= 0 && target_c >= 0 && target_c < MAX_COLS) {
            send_status("6. Moving To Desired Point: \"%c%d\"", row_char, target_c);
            
            // 1. Move table to target coordinates
            move_to_coordinate(target_r, target_c);
            send_status("7. Current Position: %d, %c, %d", MY_MCU_ID, current_row + 'A', current_col);
            
            // 2. Picking phase
            send_status("8. Picking: \"5 Pick\"");
            char pick_msg[] = "5_pick\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)pick_msg, strlen(pick_msg), HAL_MAX_DELAY);
            
            HAL_Delay(10000);
            send_status("9. Picking: Done");
            
            // 3. Moving to drop point (A0)
            send_status("10. Moving to Drop point: \"A0\"");
            move_to_coordinate(0, 0);
            send_status("11. Current Position: %d, %c, %d", MY_MCU_ID, current_row + 'A', current_col);
            
            send_status("12. Droping: \"5 Drop\"");
            // (Optional drop transmission if needed, status logged)
            
            HAL_Delay(2000); // Small drop pause if needed
            send_status("13. Droping: Done");
            
            // 4. Moving to Home
            send_status("14. Moving to Home \"A0\"");
            move_to_coordinate(0, 0);
            send_status("15. Current Position: %d, %c, %d", MY_MCU_ID, current_row + 'A', current_col);
            
            // 5. Conveyor delivery
            send_status("16. Send Conveyor task: \"1 Deliver\"");
            run_conveyor_until_sensor();
            
            send_status("20. Task: Finished");
          }
        }
      }
    }
                /* USER CODE END WHILE */
                /* USER CODE BEGIN 3 */
    }
              /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
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
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USB Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_PCD_Init(void)
{

  /* USER CODE BEGIN USB_Init 0 */

  /* USER CODE END USB_Init 0 */

  /* USER CODE BEGIN USB_Init 1 */

  /* USER CODE END USB_Init 1 */
  hpcd_USB_FS.Instance = USB;
  hpcd_USB_FS.Init.dev_endpoints = 8;
  hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_Init 2 */

  /* USER CODE END USB_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, CONV_STEP_Pin|CONV_DIR_Pin|CONV_EN_Pin|VERT_STEP_Pin
                          |VERT_DIR_Pin|VERT_EN_Pin|HORZ_STEP_Pin|HORZ_DIR_Pin
                          |HORZ_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : CONV_STEP_Pin CONV_DIR_Pin CONV_EN_Pin VERT_STEP_Pin
                           VERT_DIR_Pin VERT_EN_Pin HORZ_STEP_Pin HORZ_DIR_Pin
                           HORZ_EN_Pin */
  GPIO_InitStruct.Pin = CONV_STEP_Pin|CONV_DIR_Pin|CONV_EN_Pin|VERT_STEP_Pin
                          |VERT_DIR_Pin|VERT_EN_Pin|HORZ_STEP_Pin|HORZ_DIR_Pin
                          |HORZ_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : LASER_SENS_Pin */
  GPIO_InitStruct.Pin = LASER_SENS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(LASER_SENS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        if (rx_byte == '\n' || rx_byte == '\r') {
            if (rx_index > 0) {
                rx_str_buffer[rx_index] = '\0';
                command_ready = 1;
                rx_index = 0;
            }
        } else {
            if (rx_index < (RX_BUFFER_SIZE - 1)) {
                rx_str_buffer[rx_index++] = rx_byte;
            }
        }
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}
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
#ifdef USE_FULL_ASSERT
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
