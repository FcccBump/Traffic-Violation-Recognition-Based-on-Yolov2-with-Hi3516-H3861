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

#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_i2c.h"
#include "iot_gpio.h"
#include "iot_pwm.h"
#include "hi_gpio.h"
#include "app_demo_multi_sample.h"
#include "ssd1306_oled.h"
#include "app_demo_traffic_sample.h"

#define IOT_GPIO_INDEX_9 9
#define IOT_GPIO_INDEX_10 10
#define IOT_GPIO_INDEX_11 11
#define IOT_GPIO_INDEX_12 12
#define PWM_CLK_160M 160000000
#define IO_FUNC_GPIO_9_PWM0_OUT 5
#define IO_FUNC_GPIO_OUT 0
#define HI_FRQ_31   (31)
/*
 * @bref traffic control mode
 * @param void
 */
void TrafficControlModeSample(void)
{
    IoTPwmInit(0); /* PWM0 */
    IoSetFunc(IOT_GPIO_INDEX_9, IO_FUNC_GPIO_9_PWM0_OUT); // set Gpio9 PWM
    IoTGpioSetDir(IOT_GPIO_INDEX_9, IOT_GPIO_DIR_OUT);
    unsigned char beep = 0;

    unsigned char currentType = 0;
    unsigned char currentMode = 0;
    
    currentMode = GetKeyStatus(CURRENT_MODE);
    currentType = GetKeyStatus(CURRENT_TYPE);
    switch (GetKeyStatus(CURRENT_TYPE)) {
        case RED_ON:
            GpioControl(IOT_GPIO_INDEX_10, IOT_GPIO_INDEX_10,
                        IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
            GpioControl(IOT_GPIO_INDEX_11, IOT_GPIO_INDEX_11,
                        IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
            break;
        case YELLOW_ON:
            GpioControl(IOT_GPIO_INDEX_10, IOT_GPIO_INDEX_10,
                        IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
            GpioControl(IOT_GPIO_INDEX_12, IOT_GPIO_INDEX_12,
                        IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
            break;
        case GREEN_ON:
            GpioControl(IOT_GPIO_INDEX_12, IOT_GPIO_INDEX_12,
                        IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
            GpioControl(IOT_GPIO_INDEX_11, IOT_GPIO_INDEX_11,
                        IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
            break;
        default:
            break;
    }

    while (1) {
        if ((currentMode !=  GetKeyStatus(CURRENT_MODE)) || (currentType != GetKeyStatus(CURRENT_TYPE))) {
            break;
        }
        if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
            if (beep == BEEP_OFF) {
                beep = BEEP_ON;
                IoTPwmStart(HI_PWM0, HI_FRQ_31, PWM_CLK_160M / PWM_FULL_DUTY); /* PWM0 */
                hi_udelay(DELAY_1_S);
            } else {
                beep = BEEP_OFF;
                IoTPwmStart(HI_PWM0, PWM_LOW_DUTY, PWM_CLK_160M / PWM_FULL_DUTY); /* PWM0 */
                hi_udelay(DELAY_1_S);
            }
        } else {
            IoTPwmStart(HI_PWM0, PWM_LOW_DUTY, PWM_FULL_DUTY); /* PWM0 */
        }

        TaskMsleep(SLEEP_1_MS); // 1ms
    }
}

/* traffic light function handle and  display */
void TrafficLightFunc(void)
{
     /* 初始化时屏幕 i2c baudrate setting */
    IoTI2cInit(IOT_I2C_IDX_0, HI_I2C_IDX_BAUDRATE); /* baudrate: 400kbps */
    IoTGpioInit(HI_GPIO_13); /* GPIO13 */
    IoSetFunc(HI_GPIO_13, HI_I2C_SDA_SCL); /* GPIO13 SDA */
    IoTGpioInit(HI_GPIO_14);  /* GPIO14 */
    IoSetFunc(HI_GPIO_14, HI_I2C_SDA_SCL); /* GPIO14 SCL */

    TrafficDisplay();
}
