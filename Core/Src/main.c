/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Smart Data Logger - AHT20 with Buzzer & LED (No Button)
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define AHT20_ADDR     (0x38 << 1) // Địa chỉ I2C AHT20 (0x70)

// Ngưỡng cảnh báo
#define TEMP_THRESHOLD 28.0f       // Ngưỡng nhiệt độ (> 28.0°C)
#define HUMI_THRESHOLD 54.0f       // Ngưỡng độ ẩm (> 54.0%)

// Định nghĩa chân GPIO cảnh báo
#define LED_PORT       GPIOA
#define LED_PIN        GPIO_PIN_0  // PA0 nối LED

#define BUZZER_PORT    GPIOA
#define BUZZER_PIN     GPIO_PIN_1  // PA1 nối Còi chip 5V

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  char uart_buf[150];
  uint8_t aht20_cmd[3];
  uint8_t aht20_data[6];
  float temperature = 0.0f;
  float humidity = 0.0f;

  // Tiêu đề trang trí
  char *header = "\r\n=================== AHT20 DATA LOGGER & ALARM ===================\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t*)header, strlen(header), 1000);

  // Khởi tạo cảm biến AHT20
  HAL_Delay(40);
  aht20_cmd[0] = 0xBE;
  aht20_cmd[1] = 0x08;
  aht20_cmd[2] = 0x00;
  HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDR, aht20_cmd, 3, 1000);
  HAL_Delay(10);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // 1. Gửi lệnh yêu cầu AHT20 đo
    aht20_cmd[0] = 0xAC;
    aht20_cmd[1] = 0x33;
    aht20_cmd[2] = 0x00;

    if (HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDR, aht20_cmd, 3, 1000) == HAL_OK)
    {
      HAL_Delay(80); // Chờ 80ms để cảm biến hoàn tất đo

      // 2. Đọc dữ liệu từ AHT20
      if (HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDR, aht20_data, 6, 1000) == HAL_OK)
      {
        if ((aht20_data[0] & 0x80) == 0)
        {
          uint32_t humi_raw = ((uint32_t)aht20_data[1] << 12) | ((uint32_t)aht20_data[2] << 4) | (aht20_data[3] >> 4);
          uint32_t temp_raw = (((uint32_t)(aht20_data[3] & 0x0F)) << 16) | ((uint32_t)aht20_data[4] << 8) | aht20_data[5];

          humidity = ((float)humi_raw * 100.0f) / 1048576.0f;
          temperature = ((float)temp_raw * 200.0f) / 1048576.0f - 50.0f;

          // 3. Tính mốc thời gian (HH:MM:SS)
          uint32_t total_sec = HAL_GetTick() / 1000;
          uint32_t hours   = total_sec / 3600;
          uint32_t minutes = (total_sec % 3600) / 60;
          uint32_t seconds = total_sec % 60;

          // 4. In thông số đo đạc qua UART
          sprintf(uart_buf, "[%02lu:%02lu:%02lu]  |  Nhiet do: %.2f C  |  Do am: %.2f %%\r\n",
                  hours, minutes, seconds, temperature, humidity);
          HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 1000);

          // 5. Kiểm tra điều kiện Cảnh báo (Nhiệt độ > 28°C HOẶC Độ ẩm > 54%)
          if (temperature > TEMP_THRESHOLD || humidity > HUMI_THRESHOLD)
          {
            // In thông báo cảnh báo qua UART
            char alert_buf[150];
            sprintf(alert_buf, "   ==> [CANH BAO] Temp > %.1f C hoac Humi > %.1f %% -> BUZZER & LED FLASH!\r\n",
                    TEMP_THRESHOLD, HUMI_THRESHOLD);
            HAL_UART_Transmit(&huart1, (uint8_t*)alert_buf, strlen(alert_buf), 1000);

            // Còi và LED cùng nhấp nháy 3 lần (Bật 150ms, Tắt 150ms -> Tổng 900ms)
            for (int i = 0; i < 3; i++)
            {
              HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);         // Bật LED
              HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);   // Bật Còi
              HAL_Delay(150);

              HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);       // Tắt LED
              HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET); // Tắt Còi
              HAL_Delay(150);
            }

            // Nghỉ nốt 4100ms cho đủ chu kỳ 5 giây
            HAL_Delay(4100);
          }
          else
          {
            // Tắt còi & LED khi thông số bình thường
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);

            // Chờ đủ 5 giây cho chu kỳ đo bình thường
            HAL_Delay(5000);
          }
        }
      }
      else
      {
        HAL_I2C_DeInit(&hi2c1);
        HAL_Delay(10);
        HAL_I2C_Init(&hi2c1);
        HAL_Delay(4980);
      }
    }
    else
    {
      HAL_I2C_DeInit(&hi2c1);
      HAL_Delay(10);
      HAL_I2C_Init(&hi2c1);
      HAL_Delay(4980);
    }
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
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Cấu hình mức điện áp ban đầu cho PA0 (LED) và PA1 (Buzzer) là LOW */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_RESET);

  /* Cấu hình chân PA0 và PA1 làm Output Push-Pull */
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
