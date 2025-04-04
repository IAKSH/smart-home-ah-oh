#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "hi_io.h"
#include "iot_i2c.h"

#include "ssd1306.h"

/**
 * 汉字字模在线： https://www.23bei.com/tool-223.html
 * 数据排列：从左到右从上到下
 * 取模方式：横向8位左高位
**/
void TestDrawChinese1(void)
{
    const uint32_t W = 16, H = 16;
    uint8_t fonts[][32] = {
        {
            /* -- ID:0,字符:"你",ASCII编码:C4E3,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节 */
            0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x23, 0xFC, 0x22, 0x04, 0x64, 0x08, 0xA8, 0x40, 0x20, 0x40,
            0x21, 0x50, 0x21, 0x48, 0x22, 0x4C, 0x24, 0x44, 0x20, 0x40, 0x20, 0x40, 0x21, 0x40, 0x20, 0x80,
        }, {
            /* -- ID:1,字符:"好",ASCII编码:BAC3,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节 */
            0x10, 0x00, 0x11, 0xFC, 0x10, 0x04, 0x10, 0x08, 0xFC, 0x10, 0x24, 0x20, 0x24, 0x24, 0x27, 0xFE,
            0x24, 0x20, 0x44, 0x20, 0x28, 0x20, 0x10, 0x20, 0x28, 0x20, 0x44, 0x20, 0x84, 0xA0, 0x00, 0x40,
        }, {
            /* -- ID:2,字符:"鸿",ASCII编码:BAE8,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节 */
            0x40, 0x20, 0x30, 0x48, 0x10, 0xFC, 0x02, 0x88, 0x9F, 0xA8, 0x64, 0x88, 0x24, 0xA8, 0x04, 0x90,
            0x14, 0x84, 0x14, 0xFE, 0xE7, 0x04, 0x3C, 0x24, 0x29, 0xF4, 0x20, 0x04, 0x20, 0x14, 0x20, 0x08,
        }, {
            /* -- ID:3,字符:"蒙",ASCII编码:C3C9,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节 */
            0x04, 0x48, 0x7F, 0xFC, 0x04, 0x40, 0x7F, 0xFE, 0x40, 0x02, 0x8F, 0xE4, 0x00, 0x00, 0x7F, 0xFC,
            0x06, 0x10, 0x3B, 0x30, 0x05, 0xC0, 0x1A, 0xA0, 0x64, 0x90, 0x18, 0x8E, 0x62, 0x84, 0x01, 0x00,
        }
    };

    ssd1306_Fill(Black);
    for (size_t i = 0; i < sizeof(fonts) / sizeof(fonts[0]); i++) {
        ssd1306_DrawRegion(i * W, 0, W, H, fonts[i], sizeof(fonts[0]), W);
    }
    ssd1306_UpdateScreen();
    sleep(1);
}

extern osMutexId_t dht11_mutex;
extern unsigned int dht11_data[4];

static void oled_task(void* arg) {
    (void) arg;

    printf("[oled] startup!\n");

    IoTGpioInit(HI_IO_NAME_GPIO_13);
    IoTGpioInit(HI_IO_NAME_GPIO_14);

    hi_io_set_func(HI_IO_NAME_GPIO_13, HI_IO_FUNC_GPIO_13_I2C0_SDA);
    hi_io_set_func(HI_IO_NAME_GPIO_14, HI_IO_FUNC_GPIO_14_I2C0_SCL);

    printf("[oled] setting i2c0 baudrate to 400kbps\n");
    IoTI2cInit(0, 400000); // 400kbps

    usleep(20 * 1000);

    printf("[oled] ssd1306 initing\n");
    ssd1306_Init();
    
    TestDrawChinese1();
    
    osDelay(300);

    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_DrawString("Hello Hi3861      ", Font_7x10, Black);
    ssd1306_UpdateScreen();

    char s[24];
    uint32_t start = HAL_GetTick();
    uint32_t oled_update_start,oled_update_end;
    float uptime_ms;
    int _dht11_data[4];

    while (1) {
        uptime_ms = (float)(HAL_GetTick() - start);
        
        sprintf(s,"up %d h %d m %.1f s ",(int)uptime_ms / 3600000,(int)uptime_ms / 60000 % 60,(float)((int)uptime_ms % 60000) / 1000 );
        ssd1306_SetCursor(0, 12);
        ssd1306_DrawString(s, Font_7x10, White);

        sprintf(s,"latency: %d ms    ",oled_update_end - oled_update_start);
        ssd1306_SetCursor(0, 24);
        ssd1306_DrawString(s, Font_7x10, White);

        // display dht11 data
        if (osMutexAcquire(dht11_mutex, 0) == osOK) {
            memcpy(_dht11_data,dht11_data,sizeof(int) * 4);
            osMutexRelease(dht11_mutex);

            sprintf(s,"temp: %d.%d", _dht11_data[2], _dht11_data[3]);
            ssd1306_SetCursor(0, 36);
            ssd1306_DrawString(s, Font_7x10, White);

            sprintf(s,"humi: %d.%d",_dht11_data[0], _dht11_data[1]);
            ssd1306_SetCursor(0, 48);
            ssd1306_DrawString(s, Font_7x10, White);
        }

        oled_update_start = HAL_GetTick();
        ssd1306_UpdateScreen();
        oled_update_end = HAL_GetTick();

        osDelay(1);
    }
}

static void oled_entry(void) {
    osThreadAttr_t attr = {0};
    attr.name = "oled_test";
    attr.stack_size = 2560;
    attr.priority = osPriorityBelowNormal;
    if (osThreadNew((osThreadFunc_t)oled_task, NULL, &attr) == NULL) {
        printf("[oled] Failed to create oled_task!\n");
    }
}

SYS_RUN(oled_entry);
