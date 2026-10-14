#include "AHT20.h"

void AHT20_Init(I2C_HandleTypeDef *hi2c) {
    uint8_t initCmd[3] = {0xBE, 0x08, 0x00};
    HAL_Delay(40); // Đợi cảm biến ổn định sau khi cấp nguồn
    HAL_I2C_Master_Transmit(hi2c, AHT20_ADDR, initCmd, 3, HAL_MAX_DELAY);
    HAL_Delay(10);
}

uint8_t AHT20_Read(I2C_HandleTypeDef *hi2c, float *Temperature, float *Humidity) {
    uint8_t measureCmd[3] = {0xAC, 0x33, 0x00};
    uint8_t data[6];

    // Gửi lệnh kích hoạt đo lường
    HAL_I2C_Master_Transmit(hi2c, AHT20_ADDR, measureCmd, 3, HAL_MAX_DELAY);

    // Đợi 80ms để cảm biến hoàn thành việc đo
    HAL_Delay(80);

    // Đọc 6 byte dữ liệu phản hồi
    if (HAL_I2C_Master_Receive(hi2c, AHT20_ADDR, data, 6, HAL_MAX_DELAY) == HAL_OK) {
        // Kiểm tra bit Busy (bit 7 của byte đầu tiên). Nếu bằng 0 là dữ liệu đã sẵn sàng.
        if ((data[0] & 0x80) == 0) {
            uint32_t humi_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
            uint32_t temp_raw = (((uint32_t)(data[3] & 0x0F)) << 16) | ((uint32_t)data[4] << 8) | data[5];

            // Chuyển đổi sang giá trị thực tế
            *Humidity = ((float)humi_raw * 100.0f) / 1048576.0f;
            *Temperature = ((float)temp_raw * 200.0f) / 1048576.0f - 50.0f;

            return 1; // Đọc thành công
        }
    }
    return 0; // Đọc thất bại
}
