/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
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
 * limitations under the License.
 */

#ifndef SSD1306_OLED_H
#define SSD1306_OLED_H

#define SEND_CMD_LEN    (8)

#define OLED_X_POSITION_0    (0)
#define OLED_X_POSITION_15   (15)
#define OLED_X_POSITION_16   (16)
#define OLED_X_POSITION_18   (18)
#define OLED_X_POSITION_40   (40)
#define OLED_X_POSITION_48   (48)
#define OLED_X_POSITION_56   (56)
#define OLED_X_POSITION_60   (60)
#define OLED_X_POSITION_81   (81)
#define OLED_X_POSITION_120  (120)

#define OLED_Y_POSITION_0     ((unsigned char)0x0)
#define OLED_Y_POSITION_1     ((unsigned char)0x1)
#define OLED_Y_POSITION_2     ((unsigned char)0x2)
#define OLED_Y_POSITION_3     ((unsigned char)0x3)
#define OLED_Y_POSITION_4     ((unsigned char)0x4)
#define OLED_Y_POSITION_5     ((unsigned char)0x5)
#define OLED_Y_POSITION_6     ((unsigned char)0x6)
#define OLED_Y_POSITION_7     ((unsigned char)0x7)

#define OLED_DISPLAY_STRING_TYPE_1   (1)
#define OLED_DISPLAY_STRING_TYPE_16  (16)

#define OLED_ADDRESS                0x78 // 默认地址为0x78
#define OLED_ADDRESS_WRITE_CMD      0x00 // 0000000 写命令
#define OLED_ADDRESS_WRITE_DATA     0x40 // 0100 0000(0x40)
#define HI_I2C_IDX_BAUDRATE         400000 // 400k
/* delay */
#define SLEEP_1_MS                  1
#define SLEEP_10_MS                 10
#define SLEEP_20_MS                 20
#define SLEEP_30_MS                 30
#define SLEEP_50_MS                 50
#define SLEEP_100_MS                100
#define SLEEP_1S                    1000
#define SLEEP_6_S                   6000
#define DELAY_5_MS                  5000
#define DELAY_10_MS                 10000
#define DELAY_100_MS                100000
#define DELAY_250_MS                250000
#define DELAY_500_MS                500000
#define DELAY_1_S                   1000000
#define DELAY_5_S                   5000000
#define DELAY_6_S                   6000000
#define DELAY_10_S                  10000000
#define DELAY_30_S                  30000000
#define DEFAULT_TYPE                ((hi_u8)0)
#define INIT_TIME_COUNT             ((hi_u8)1)
#define TIME_PEROID_COUNT           10
/* pwm duty */
#define PWM_LOW_DUTY                1
#define PWM_SLOW_DUTY               1000
#define PWM_SMALL_DUTY              4000
#define PWM_LITTLE_DUTY             10000
#define PWM_DUTY                    50
#define PWM_MIDDLE_DUTY             40000
#define PWM_FULL_DUTY               65530
/* oled init */
#define OLED_CLEAN_SCREEN           ((hi_u8)0x00)
/* ssd1306 register cmd */
#define DISPLAY_OFF                 0xAE
#define SET_LOW_COLUMN_ADDRESS      0x00
#define SET_HIGH_COLUMN_ADDRESS     0x10
#define SET_START_LINE_ADDRESS      0x40
#define SET_PAGE_ADDRESS            0xB0
#define CONTRACT_CONTROL            0x81
#define FULL_SCREEN                 0xFF
#define SET_SEGMENT_REMAP           0xA1
#define NORMAL                      0xA6
#define SET_MULTIPLEX               0xA8
#define DUTY                        0x3F
#define SCAN_DIRECTION              0xC8
#define DISPLAY_OFFSET              0xD3
#define DISPLAY_TYPE                0x00
#define OSC_DIVISION                0xD5
#define DIVISION                    0x80
#define COLOR_MODE_OFF              0xD8
#define COLOR                       0x05
#define PRE_CHARGE_PERIOD           0xD9
#define PERIOD                      0xF1
#define PIN_CONFIGUARTION           0xDA
#define CONFIGUARTION               0x12
#define SET_VCOMH                   0xDB
#define VCOMH                       0x30
#define SET_CHARGE_PUMP_ENABLE      0x8D
#define PUMP_ENABLE                 0x14
#define TURN_ON_OLED_PANEL          0xAF

#define IOT_IO_NAME_GPIO9            (9)
#define IOT_GPIO_IDX_9               (9)

typedef struct {
    /** Pointer to the buffer storing data to send */
    unsigned char *sendBuf;
    /** Length of data to send */
    unsigned int  sendLen;
    /** Pointer to the buffer for storing data to receive */
    unsigned char *receiveBuf;
    /** Length of data received */
    unsigned int  receiveLen;
} IotI2cData;

typedef enum {
    IOT_I2C_IDX_0,
    TOT_I2C_IDX_1,
} IotI2cIdx;

unsigned int OledInit(void);
void OledSetPosition(unsigned char x, unsigned char y);
void OledFillScreen(unsigned char fiiData);
void OledShowChar(unsigned char x, unsigned char y, unsigned char chr, unsigned char charSize);
void OledShowStr(unsigned char x, unsigned char y, unsigned char *chr, unsigned char charSize);
#endif