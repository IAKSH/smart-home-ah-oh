/*
 * Copyright (C) 2021 HiHope Open Source Organization .
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 *
 * limitations under the License.
 */

/**
 * Private configuration file for the SSD1306 library.
 * This example is configured for STM32F0, I2C and including all fonts.
 */

 #ifndef __SSD1306_CONF_H__
 #define __SSD1306_CONF_H__
 
 // Choose a microcontroller family
 // #define STM32F0
 // #define STM32F1
 // #define STM32F4
 // #define STM32L0
 // #define STM32L4
 // #define STM32F3
 // #define STM32H7
 // #define STM32F7
 
 // Choose a bus
 #define SSD1306_USE_I2C
 // #define SSD1306_USE_SPI
 
 // I2C Configuration
 // #define SSD1306_I2C_PORT        hi2c1
 #define SSD1306_I2C_ADDR        (0x78)
 
 // Mirror the screen if needed
 // #define SSD1306_MIRROR_VERT
 // #define SSD1306_MIRROR_HORIZ
 
 // Set inverse color if needed
 // # define SSD1306_INVERSE_COLOR
 
 // Include only needed fonts
 #define SSD1306_INCLUDE_FONT_6x8
 #define SSD1306_INCLUDE_FONT_7x10
 #define SSD1306_INCLUDE_FONT_11x18
 #define SSD1306_INCLUDE_FONT_16x26
 
 #endif /* __SSD1306_CONF_H__ */