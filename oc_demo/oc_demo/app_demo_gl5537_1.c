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

#include <hi_stdlib.h>
#include <hi_watchdog.h>
#include <hi_early_debug.h>
#include <hi_time.h>
#include <hi_io.h>
#include <hi_gpio.h>
#include <hi_adc.h>
#include <hi_task.h>
#include <hi_pwm.h>
#include "ssd1306_oled.h"
#include "iot_adc.h"

#define     ADC_TEST_LENGTH             (20)
#define     VLT_MIN                     (100)
#define     OLED_FALG_ON                ((unsigned char)0x01)
#define     OLED_FALG_OFF               ((unsigned char)0x00)

#define VOLTAGE_1_8_V  ((float)1.8)
#define VOLTAGE_4_TIMES (4)

#define VOLTAGE_0_6_V   ((float)0.6)
#define VOLTAGE_1_V   ((float)1.0)
#define VOLTAGE_1_5_V   ((float)1.5)
#define VOLTAGE_3_V   ((float)3.0)

#define ADC_CHANNAL_RANGE ((float)4096.0)

unsigned short g_adcBuf[ADC_TEST_LENGTH] = { 0 };
unsigned short g_gpio5AdcBuf[ADC_TEST_LENGTH] = { 0 };

/* get data from adc test */
unsigned char GetLightStatus(void)
{
    unsigned short data = 0;
    unsigned short vlt;

    float voltage;
    float vltMax = 0;
    float vltMin = VLT_MIN;
    memset_s(g_adcBuf, sizeof(g_adcBuf), 0x0, sizeof(g_adcBuf));
    for (int i = 0; i < ADC_TEST_LENGTH; i++) {
        // ADC_Channal_6  自动识别模式  CNcomment:4次平均算法模式 CNend
        unsigned int ret = AdcRead(IOT_ADC_CHANNEL_4, &data, HI_ADC_EQU_MODEL_4, HI_ADC_CUR_BAIS_DEFAULT, 0xF0);
        if (ret != HI_ERR_SUCCESS) {
            printf("ADC Read Fail\n");
            return  HI_NULL;
        }
        g_adcBuf[i] = data;
    }
    for (int j = 0; j < ADC_TEST_LENGTH; j++) {
        vlt = g_adcBuf[j];
        voltage = (float)vlt * VOLTAGE_1_8_V * \
                        VOLTAGE_4_TIMES / ADC_CHANNAL_RANGE; /* vlt * 1.8 * 4 / 4096.0为将码字转换为电压 */
        vltMax = (voltage > vltMax) ? voltage : vltMax;
        vltMin = (voltage < vltMin) ? voltage : vltMin;
    }

    if (vltMax > VOLTAGE_3_V) { /* 判断电压是否大于3.0V */
        return OLED_FALG_ON;
    } else {
        return OLED_FALG_OFF;
    }
}
/* get gpio5 Voltage */
void GetGpio5Voltage(const char *param)
{
    unsigned short data = 0;
    unsigned short vlt;
    float voltage;
    float vltMax = 0;
    float vltMin = VLT_MIN;

    hi_unref_param(param);
    memset_s(g_gpio5AdcBuf, sizeof(g_gpio5AdcBuf), 0x0, sizeof(g_gpio5AdcBuf));
    for (int i = 0; i < ADC_TEST_LENGTH; i++) {
        // ADC_Channal_2  自动识别模式  CNcomment:4次平均算法模式 CNend */
        unsigned int ret = AdcRead(IOT_ADC_CHANNEL_2, &data, IOT_ADC_EQU_MODEL_4, IOT_ADC_CUR_BAIS_DEFAULT, 0xF0);
        if (ret != HI_ERR_SUCCESS) {
            printf("ADC Read Fail\n");
            return  HI_NULL;
        }
        g_gpio5AdcBuf[i] = data;
    }

    for (int i = 0; i < ADC_TEST_LENGTH; i++) {
        vlt = g_gpio5AdcBuf[i];
        /* vlt * 1.8* 4 / 4096.0为将码字转换为电压 */
        voltage = (float)vlt * VOLTAGE_1_8_V * VOLTAGE_4_TIMES / ADC_CHANNAL_RANGE;
        vltMax = (voltage > vltMax) ? voltage : vltMax;
        vltMin = (voltage < vltMin) ? voltage : vltMin;
    }
    if (vltMax > VOLTAGE_0_6_V && vltMax < VOLTAGE_1_V) { /* 电压最大值大于0.6小于1.0 */
        GpioKey1IsrFuncMode();
    } else if (vltMin > VOLTAGE_1_V && vltMin < VOLTAGE_1_5_V) { /* 电压最大值大于1.0小于1.5 */
        GpioKey2IsrFuncType();
    } else if (vltMax < VOLTAGE_0_6_V) { /* 电压最大值小于0.6 */
        printf("gpio9_led_light:vltMax=%0.2f, vltMin=%0.2f\r\n", vltMax, vltMin);
        Gpio9LedLightFunc();
    }
    printf("key_5:vltMax=%0.2f, vltMin=%0.2f\r\n", vltMax, vltMin);
}