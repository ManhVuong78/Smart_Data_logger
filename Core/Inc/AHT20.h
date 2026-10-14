#ifndef AHT20_H_
#define AHT20_H_

#include "stm32f1xx_hal.h"

// Địa chỉ I2C của AHT20 (0x38 dịch trái 1 bit cho HAL)
#define AHT20_ADDR (0x38 << 1)

// Các hàm giao tiếp
void AHT20_Init(I2C_HandleTypeDef *hi2c);
uint8_t AHT20_Read(I2C_HandleTypeDef *hi2c, float *Temperature, float *Humidity);

#endif /* AHT20_H_ */
