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

#ifndef APP_DEMO_MULTI_SAMPLE_H
#define APP_DEMO_MULTI_SAMPLE_H

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <hi_time.h>
#include <hi_watchdog.h>
#include <hi_io.h>
#include "ssd1306_oled.h"
#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "iot_errno.h"

#define KEY_INTERRUPT_PROTECT_TIME (40)
#define KEY_DOWN_INTERRUPT          (1)
#define GPIO_DEMO_TASK_STAK_SIZE    (1024*2)
#define GPIO_DEMO_TASK_PRIORITY     (25)

#define MAX_FUNCTION_SINGLE_DEMO    (1)
#define MAX_FUNCTION_DEMO_NUM       (4)
#define MAX_COLORFUL_LIGHT_MODE     (8)
#define MAX_TRAFFIC_LIGHT_MODE      (4)
#define MAX_NFC_TAG_MODE            (6)
#define MAX_ENVIRONMENT_MODE        (5)

#define MAX_CONTROL_MODE_TYPE       (2)
#define MAX_COLORFUL_LIGHT_TYPE     (2)
#define MAX_PWM_CONTROL_TYPE        (4)
#define MAX_BIRGHTNESS_TYPE         (2)
#define RETURN_TYPE_MODE            (2)

#define MAX_TRAFFIC_CONTROL_TYPE    (3)
#define MAX_TRAFFIC_AUTO_TYPE       (2)
#define MAX_TRAFFIC_HUMAN_TYPE      (1)

#define OLED_FALG_ON                ((hi_u8)0x01)
#define OLED_FALG_OFF               ((hi_u8)0x00)

#define  KEY_GPIO_5                 (1)
#define  KEY_GPIO_7                 (2)

#define LIGHT_ONE_SECOND_INTERVAL       (60)
#define BEEP_ONE_SECOND_INTERVAL        (15)
#define BEEP_HALF_SECOND_INTERVAL       (8)
#define BEEP_QUARTER_SECOND_INTERVAL    (4)


#define RED_LIGHT_FLASH_TIME        (3)    // 红灯闪3秒
#define YELLOW_LIGHT_FLASH_TIME     (3)    // 黄灯闪3秒
#define GREEN_LIGHT_FLASH_TIME      (3)    // 绿灯闪3秒

#define TRAFFIC_AUTO_MODE_TIME_COUNT            (3)
#define TRAFFIC_HUMAN_MODE_NORMAL_TIME_COUNT    (3)
#define TRAFFIC_HUMAN_MODE_TIME_CONUT           (3)

#define COUNT_NUM (3)

#define FACTORY_HISPARK_BOARD_TEST(fmt, ...) \
do { \
    printf(fmt, ##__VA_ARGS__); \
    for (hi_s32 i = 0; i < COUNT_NUM; i++) { \
        HisparkBoardTest(HI_GPIO_VALUE0); \
        hi_udelay(DELAY_250_MS); \
        HisparkBoardTest(HI_GPIO_VALUE1); \
        hi_udelay(DELAY_250_MS); \
    } \
} while (0)

typedef enum {
    HI_GPIO_0 = 0,
    HI_GPIO_1,
    HI_GPIO_2,
    HI_GPIO_3,
    HI_GPIO_4,
    HI_GPIO_5,
    HI_GPIO_6,
    HI_GPIO_7,
    HI_GPIO_8,
    HI_GPIO_9,
    HI_GPIO_10,
    HI_GPIO_11,
    HI_GPIO_12,
    HI_GPIO_13,
    HI_GPIO_14
} HiGpioNum;

typedef enum {
    HI_PWM0 = 0,
    HI_PWM1,
    HI_PWM2,
    HI_PWM3,
    HI_PWM_OUT = 5,
    HI_PWM_MAX
} HiPwmChannal;

typedef enum {
    BEEP_BY_ONE_SECOND,
    BEEP_BY_HALF_SECOND,
    BEEP_BY_QUARTER_SECOND
} HiBeepStatus;

typedef enum {
    RED_COUNT = 0,
    YELLOW_COUNT,
    GREEN_COUNT,
    DEFAULT
} HiLedTimeCount;

typedef enum {
    MAIN_FUNCTION_SELECT_MODE = 0,
    SUB_MODE_SELECT_MODE
} HiMenuMode;

typedef enum {
    COLORFUL_LIGHT_FUNC = 0,
    TRAFFIC_LIGHT_FUNC,
    ENVIRONMENT_FUNC,
    NFC_TEST_FUNC
} HiSelectFunc;

typedef enum {
    COLORFUL_LIGHT_MENU = 0,
    TRAFFIC_LIGHT_MENU,
    ENVIRONMENT_MENU,
    NFC_TEST_MENU
} HiSelectMenuMode;

typedef enum {
    CONTROL_MODE = 0,
    COLORFUL_LIGHT_MODE,
    PWM_CONTROL_MODE,
    BIRGHTNESS_MODE,
    HUMAN_DETECT_MODE,
    LIGHT_DETECT_MODE,
    UNION_DETECT_MODE,
    RETURN_MODE
} HiColorfulLightMode;

typedef enum {
    TRAFFIC_CONTROL_MODE = 0,
    TRAFFIC_AUTO_MODE,
    TRAFFIC_HUMAN_MODE,
    TRAFFIC_RETURN_MODE
} HiTrafficLightMode;

typedef enum {
    TRAFFIC_NORMAL_TYPE = 0,
    TRAFFIC_HUMAN_TYPE
} HiTrafficLightType;

typedef enum {
    NFC_TAG_WECHAT_MODE = 0,
    NFC_TAG_TODAY_HEADLINE_MODE,
    NFC_TAG_TAOBAO_MODE,
    NFC_TAG_HW_LIFE_MODE,
    NFC_TAG_HISTREAMING_MODE,
    NFC_RETURN_MODE
} HiNfcMode;

typedef enum {
    ENV_ALL_MODE = 0,
    ENV_TEMPERRATURE_MODE,
    ENV_HUMIDITY_MODE,
    COMBUSTIBLE_GAS_MODE,
    ENV_RETURN_MODE
} HiEnvironmentMode;

typedef enum {
    RED_ON = 0,
    YELLOW_ON,
    GREEN_ON
} HiControlModeType;

typedef enum {
    BEEP_OFF = 0,
    BEEP_ON,
} HiBeepStatusType;

typedef enum {
    CYCLE_FOR_ONE_SECOND = 0,
    CYCLE_FOR_HALF_SECOND,
    CYCLE_FOR_QUARATER_SECOND
} HiColorfulLightType;

typedef enum {
    PWM_CONTROL_RED = 0,
    PWM_CONTROL_YELLOW,
    PWM_CONTROL_GREEN,
    PWM_CONTROL_PURPLE,
    PWM_CONTROL_ALL
} PwmControlModeType;

typedef enum {
    BRIGHTNESS_LOW = 0,
    BRIGHTNESS_MIDDLE,
    BRIGHTNESS_HIGH
} BrightnessType;

typedef enum {
    KEY_UP = 0,
    KEY_DOWN
} HiReturnSelectType;

typedef enum {
    ZERO = 0,
    ONE,
    TWO,
    THREE,
    FOUR,
    FIVE
} TimeCount;

typedef enum {
    MENU_SELECT = 0,
    MENU_MODE,
    CURRENT_MODE,
    CURRENT_TYPE,
    KEY_DOWN_FLAG,
    OC_BEEP_STATUS = 5,
    SOMEONE_WALKING,
}GloableStatuDef;

typedef struct {
    unsigned char g_menuSelect;
    unsigned char g_menuMode;
    unsigned char g_currentMode;
    unsigned char g_currentType;
    unsigned char g_keyDownFlag;
    unsigned int g_gpio5Tick;
    unsigned int g_gpio7Tick;
    unsigned int g_gpio8Tick;
    unsigned char g_gpio9Tick;
    unsigned int g_gpio7FirstKeyDwon;
    unsigned char g_gpio8CurrentType;
    unsigned char g_ocBeepStatus;
    unsigned char g_someoneWalkingFlag;
    unsigned char g_withLightFlag;
} GlobalStausType;

void GpioKey1IsrFuncMode(void);
void GpioKey2IsrFuncType(void);
void HisparkBoardTest(IotGpioValue value);
void GpioControl(unsigned int gpio, unsigned int id, IotGpioDir dir, IotGpioValue gpioVal, unsigned char val);
void PwmInit(unsigned int id, unsigned char val, unsigned int port);
void HiSwitchInit(unsigned int id, unsigned char val, unsigned int idx, IotGpioDir dir, IotIoPull pval);
void TestGpioInit(void);
void ControlModeSample(void);
unsigned char DelayAndCheckKeyInterrupt(unsigned int delayTime);
void CycleForOneSecond(void);
void CycleForHalfSecond(void);
void CycleForQuarterSecond(void);
void ColorfulLightSample(void);
void AllLightOut(void);
void RedLightDarkToBright(void);
void GreenLightDarkToBright(void);
void BlueLightDarkToBright(void);
void PurpleLightDarkToBright(void);
void AllLightDarkToBright(void);
void PwmControlSample(void);
void BrightnessControlSample(void);
void HumanDetectSample(void);
void LightDetectSample(void);
void UnionDetectSample(void);
void ReturnMainEnumSample(void);
void Gpio9LedLightFunc(void);
void Gpio8Interrupt(const char *param);
void AppMultiSampleDemo(void);
unsigned char SetKeyStatus(HiColorfulLightMode setSta);
unsigned char SetKeyType(HiColorfulLightMode setSta);
unsigned char GetKeyStatus(GloableStatuDef staDef);
#endif