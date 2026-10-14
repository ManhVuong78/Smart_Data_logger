/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Smart Data Logger - Read AHT20 and Send via UART
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define AHT20_ADDR (0x38 << 1) // Địa chỉ I2C của AHT20 dịch trái 1 bit (0x70)

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
  char uart_buf[100];
  uint8_t aht20_cmd[3];
  uint8_t aht20_data[6];
  float temperature = 0.0f;
  float humidity = 0.0f;

  // Gửi thông báo khởi động lên PC
  char *start_msg = "=== STM32F103 & AHT20 Data Logger Start ===\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t*)start_msg, strlen(start_msg), 1000);

  // 1. Khởi tạo cảm biến AHT20
  HAL_Delay(40); // Đợi AHT20 ổn định điện áp sau khi cấp nguồn
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
    // 2. Gửi lệnh yêu cầu AHT20 bắt đầu đo
    aht20_cmd[0] = 0xAC;
    aht20_cmd[1] = 0x33;
    aht20_cmd[2] = 0x00;

    if (HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDR, aht20_cmd, 3, 1000) == HAL_OK)
    {
      HAL_Delay(80); // Chờ 80ms để cảm biến hoàn thành quá trình đo

      // 3. Đọc 6 byte dữ liệu từ AHT20
      if (HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDR, aht20_data, 6, 1000) == HAL_OK)
      {
        // Kiểm tra bit Busy (Bit 7 của Byte 0 phải bằng 0)
        if ((aht20_data[0] & 0x80) == 0)
        {
          // Ghép byte tính độ ẩm raw (20-bit)
          uint32_t humi_raw = ((uint32_t)aht20_data[1] << 12) | ((uint32_t)aht20_data[2] << 4) | (aht20_data[3] >> 4);

          // Ghép byte tính nhiệt độ raw (20-bit)
          uint32_t temp_raw = (((uint32_t)(aht20_data[3] & 0x0F)) << 16) | ((uint32_t)aht20_data[4] << 8) | aht20_data[5];

          // Công thức chuyển đổi từ Datasheet AHT20
          humidity = ((float)humi_raw * 100.0f) / 1048576.0f;
          temperature = ((float)temp_raw * 200.0f) / 1048576.0f - 50.0f;

          // 4. Định dạng chuỗi và gửi qua UART lên PC
          sprintf(uart_buf, "Nhiet do: %.2f C | Do am: %.2f %%\r\n", temperature, humidity);
          HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 1000);
        }
        else
        {
          char *err_busy = "AHT20 dang bận...\r\n";
          HAL_UART_Transmit(&huart1, (uint8_t*)err_busy, strlen(err_busy), 1000);
        }
      }
      else
      {
        char *err_rx = "Loi: Khong nhan duoc du lieu I2C tu AHT20!\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)err_rx, strlen(err_rx), 1000);
      }
    }
    else
    {
      char *err_tx = "Loi: Khong gui duoc lenh I2C! Dang khoi tao lai I2C...\r\n";
      HAL_UART_Transmit(&huart1, (uint8_t*)err_tx, strlen(err_tx), 1000);

      // Thêm 3 dòng này để reset bộ I2C1 khi bị treo bus
      HAL_I2C_DeInit(&hi2c1);
      HAL_Delay(10);
      HAL_I2C_Init(&hi2c1);
      HAL_Delay(10);
    }

    HAL_Delay(1000); // Đọc và gửi dữ liệu mỗi 1 giây
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
  hi2c1.Init.ClockSpeed = 100000; // Tốc độ I2C Standard Mode (100kHz)
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
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
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
