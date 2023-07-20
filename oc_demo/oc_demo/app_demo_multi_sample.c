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
#include <stdlib.h>
#include <memory.h>
#include <hi_time.h>
#include <hi_watchdog.h>
#include "iot_errno.h"
#include "iot_gpio.h"
#include "ssd1306_oled.h"
#include "app_demo_i2c_oled.h"
#include "iot_gpio_ex.h"
#include "app_demo_gl5537_1.h"
#include "app_demo_multi_sample.h"

#define DELAY_TIMES_100 100
#define DELAY_TIMES_5   5

#define PWM_FRQ_50      50
#define PWM_FRQ_10      10
#define PWM_FRQ_100     100
#define PWM_FRQ_20      20

#define TICK_COUNT_MAX  255
#define PAIR_2          2

GlobalStausType globalStaType = {0};

/* Set key status */
unsigned char SetKeyStatus(HiColorfulLightMode setSta)
{
    globalStaType.g_currentMode = setSta;
    return globalStaType.g_currentMode;
}
/* Set key status */
unsigned char SetKeyType(HiColorfulLightMode setSta)
{
    globalStaType.g_currentType = setSta;
    return globalStaType.g_currentType;
}

/* GetKeyStatus */
unsigned char GetKeyStatus(GloableStatuDef staDef)
{
    unsigned char status = 0;
    switch (staDef) {
        case MENU_SELECT:
            status = globalStaType.g_menuSelect;
            break;
        case MENU_MODE:
            status = globalStaType.g_menuMode;
            break;
        case CURRENT_MODE:
            status = globalStaType.g_currentMode;
            break;
        case CURRENT_TYPE:
            status = globalStaType.g_currentType;
            break;
        case KEY_DOWN_FLAG:
            status = globalStaType.g_keyDownFlag;
            break;
        case OC_BEEP_STATUS:
            status = globalStaType.g_ocBeepStatus;
            break;
        case SOMEONE_WALKING:
            status = globalStaType.g_someoneWalkingFlag;
            break;
        default:
            break;
    }
    return status;
}

/* factory test HiSpark board */
void HisparkBoardTest(IotGpioValue value)
{
    IoSetFunc(HI_GPIO_9, 0); // GPIO9
    IoTGpioSetDir(HI_GPIO_9, IOT_GPIO_DIR_OUT); // GPIO9
    IoTGpioSetOutputVal(HI_GPIO_9, value); // GPIO9
}
/* gpio init */
void GpioControl(unsigned int gpio, unsigned int id, IotGpioDir dir, IotGpioValue  gpioVal,
                 unsigned char val)
{
    IoSetFunc(gpio, val);
    IoTGpioSetDir(id, dir);
    IoTGpioSetOutputVal(id, gpioVal);
}
/* pwm init */
void PwmInit(unsigned int id, unsigned char val, unsigned int port)
{
    IoSetFunc(id, val); /* PWM0 OUT */
    IoTPwmInit(port);
}
/* gpio key init */
void HiSwitchInit(unsigned int id, unsigned char val,
                  unsigned int idx, IotGpioDir dir, IotIoPull pval)
{
    IoSetFunc(id, val);
    IoTGpioSetDir(idx, dir);
    IoSetPull(id, pval);
}

/*
 * VCC：5V
 * blue:  gpio12 yellow
 * green: gpio11 green
 * red: gpio10 red
 * switch ：
 * gpio7 switch1
 * gpio5 switch2
 */

void TestGpioInit(void)
{
    // switch2,GPIO5,GPIO8,0
    HiSwitchInit(HI_GPIO_5, 0, HI_GPIO_5, IOT_GPIO_DIR_IN, IOT_IO_PULL_UP);
    // switch2,GPIO5,GPIO8,0
    HiSwitchInit(HI_GPIO_8, 0, HI_GPIO_8, IOT_GPIO_DIR_IN, IOT_IO_PULL_UP);

    PwmInit(HI_GPIO_10, HI_PWM_OUT, HI_PWM1); // GPIO10 PWM port1,5
    PwmInit(HI_GPIO_11, HI_PWM_OUT, HI_PWM2); // GPIO11 PWM port2,5
    PwmInit(HI_GPIO_12, HI_PWM_OUT, HI_PWM3); // GPIO12 PWM port3,5
}

/* 所有pwm的占空比为1，所有灯熄灭 */
void AllLightOut(void)
{
    IoTPwmStart(HI_PWM1, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 1 */
    IoTPwmStart(HI_PWM2, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 2 */
    IoTPwmStart(HI_PWM3, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 3 */
}

/*
 * 三色灯控制模式：每按一下类型S1按键，在绿灯亮、黄灯亮、红灯亮，三个状态之间切换
 * mode:1.control mode
 * type:1.red on,2.yellow on,3.green on,RED_ON = 1,YELLOW_ON,GREEN_ON,
 */
void ControlModeSample(void)
{
    unsigned char currentType = 0;
    unsigned char currentMode = 0;

    currentMode = GetKeyStatus(CURRENT_MODE);
    currentType = GetKeyStatus(CURRENT_TYPE);
    while (1) {
        switch (GetKeyStatus(CURRENT_TYPE)) {
            case RED_ON:
                IoTPwmStart(HI_PWM1, PWM_DUTY, PWM_SMALL_DUTY); /* 1 */
                IoTPwmStart(HI_PWM2, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 2 */
                IoTPwmStart(HI_PWM3, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 3 */
                break;
            case YELLOW_ON:
                IoTPwmStart(HI_PWM1, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 1 */
                IoTPwmStart(HI_PWM2, PWM_DUTY, PWM_SMALL_DUTY); /* 2 */
                IoTPwmStart(HI_PWM3, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 3 */
                break;
            case GREEN_ON:
                IoTPwmStart(HI_PWM1, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 1 */
                IoTPwmStart(HI_PWM2, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 2 */
                IoTPwmStart(HI_PWM3, PWM_DUTY, PWM_SMALL_DUTY); /* 3 */
                break;
            default:
                break;
        }

        if ((currentMode != GetKeyStatus(CURRENT_MODE)) || (currentType != GetKeyStatus(CURRENT_TYPE))) {
            AllLightOut();
            break;
        }
        hi_sleep(SLEEP_1_MS);
    }
}

/* 中断检测函数，每10ms检测一次 */
unsigned char DelayAndCheckKeyInterrupt(unsigned int delayTime)
{
    unsigned int cycleCount = delayTime / DELAY_10_MS;
    unsigned char currentType = 0;
    unsigned char currentMode = 0;
    
    currentMode = GetKeyStatus(CURRENT_MODE);
    currentType = GetKeyStatus(CURRENT_TYPE);

    for (unsigned int i = 0; i < cycleCount; i++) {
        if ((currentMode != GetKeyStatus(CURRENT_MODE)) || (currentType != GetKeyStatus(CURRENT_TYPE))) {
            return KEY_DOWN_INTERRUPT;
        }
        hi_udelay(DELAY_5_MS); // 10ms
        hi_sleep(SLEEP_1_MS);
    }
    return HI_NULL;
}

/* 红、黄、绿每1秒轮流亮 */
void CycleForOneSecond(void)
{
    while (1) {
        IoTPwmStart(HI_PWM1, PWM_DUTY, PWM_SMALL_DUTY); /* 1 */
        if (DelayAndCheckKeyInterrupt(DELAY_1_S) == KEY_DOWN_INTERRUPT) {
            IoTPwmStart(HI_PWM1, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 1 */
            break;
        }

        IoTPwmStart(HI_PWM1, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 1 */
        IoTPwmStart(HI_PWM2, PWM_DUTY, PWM_SMALL_DUTY); /* 2 */

        if (DelayAndCheckKeyInterrupt(DELAY_1_S) == KEY_DOWN_INTERRUPT) {
            IoTPwmStart(HI_PWM2, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 2 */
            break;
        }

        IoTPwmStart(HI_PWM2, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 2 */
        IoTPwmStart(HI_PWM3, PWM_DUTY, PWM_SMALL_DUTY); /* 3 */

        if (DelayAndCheckKeyInterrupt(DELAY_1_S) == KEY_DOWN_INTERRUPT) {
            IoTPwmStart(HI_PWM3, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 3 */
            break;
        }
        IoTPwmStart(HI_PWM3, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 3 */
        hi_sleep(SLEEP_1_MS);
    }
}

/* 红、黄、绿每0.5秒轮流亮 */
void CycleForHalfSecond(void)
{
    while (1) {
        IoTPwmStart(HI_PWM1, PWM_DUTY, PWM_SMALL_DUTY); /* 1 */
        if (DelayAndCheckKeyInterrupt(DELAY_500_MS) == KEY_DOWN_INTERRUPT) {
            IoTPwmStart(HI_PWM1, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 1 */
            break;
        }
        IoTPwmStart(HI_PWM1, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 1 */
        IoTPwmStart(HI_PWM2, PWM_DUTY, PWM_SMALL_DUTY); /* 2 */

        if (DelayAndCheckKeyInterrupt(DELAY_500_MS) == KEY_DOWN_INTERRUPT) {
            IoTPwmStart(HI_PWM2, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 2 */
            break;
        }

        IoTPwmStart(HI_PWM2, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 2 */
        IoTPwmStart(HI_PWM3, PWM_DUTY, PWM_SMALL_DUTY); /* 3 */

        if (DelayAndCheckKeyInterrupt(DELAY_500_MS) == KEY_DOWN_INTERRUPT) {
            IoTPwmStart(HI_PWM3, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 3 */
            break;
        }
        IoTPwmStart(HI_PWM3, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 3 */
        hi_sleep(SLEEP_1_MS);
    }
}

/* 红、黄、绿每0.25秒轮流亮 */
void CycleForQuarterSecond(void)
{
    while (1) {
        IoTPwmStart(HI_PWM1, PWM_DUTY, PWM_SMALL_DUTY); /* 1 */

        if (DelayAndCheckKeyInterrupt(DELAY_250_MS) == KEY_DOWN_INTERRUPT) {
            IoTPwmStart(HI_PWM1, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 1 */
            break;
        }

        IoTPwmStart(HI_PWM1, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 1 */
        IoTPwmStart(HI_PWM2, PWM_DUTY, PWM_SMALL_DUTY); /* 2 */

        if (DelayAndCheckKeyInterrupt(DELAY_250_MS) == KEY_DOWN_INTERRUPT) {
            IoTPwmStart(HI_PWM2, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 2 */
            break;
        }

        IoTPwmStart(HI_PWM2, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 2 */
        IoTPwmStart(HI_PWM3, PWM_DUTY, PWM_SMALL_DUTY); /* 3 */

        if (DelayAndCheckKeyInterrupt(DELAY_250_MS) == KEY_DOWN_INTERRUPT) {
            IoTPwmStart(HI_PWM3, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 3 */
            break;
        }
        IoTPwmStart(HI_PWM3, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 3 */
        hi_sleep(SLEEP_1_MS);
    }
}

/*
 * mode:2.Colorful light
 * type:1.period by 1s,2.period by 0.5s,3.period by 0.25s
 * 模式2：炫彩模式，每按一下S2，不同的炫彩类型，
 * 类型1：红、黄、绿三色灯每隔1秒轮流亮
 * 类型2：红、黄、绿每0.5秒轮流亮
 * 类型3：红、黄、绿每0.25秒轮流亮。
 */

void ColorfulLightSample(void)
{
    unsigned char currentType = 0;
    unsigned char currentMode = 0;
    
    currentMode = GetKeyStatus(CURRENT_MODE);
    currentType = GetKeyStatus(CURRENT_TYPE);

    while (1) {
        switch (GetKeyStatus(CURRENT_TYPE)) {
            case CYCLE_FOR_ONE_SECOND:
                CycleForOneSecond();
                break;
            case CYCLE_FOR_HALF_SECOND:
                CycleForHalfSecond();
                break;
            case CYCLE_FOR_QUARATER_SECOND:
                CycleForQuarterSecond();
                break;
            default:
                break;
        }

        if ((currentMode != GetKeyStatus(CURRENT_MODE)) || (currentType != GetKeyStatus(CURRENT_TYPE))) {
            break;
        }
        hi_sleep(SLEEP_1_MS);
    }
}

/* 红色由暗到最亮 */
void RedLightDarkToBright(void)
{
    unsigned char currentType = 0;
    unsigned char currentMode = 0;
    
    currentMode = GetKeyStatus(CURRENT_MODE);
    currentType = GetKeyStatus(CURRENT_TYPE);
    AllLightOut();
    while (1) {
        for (unsigned int i = 1; i < DELAY_TIMES_100; i++) { /* 计算延时100次 */
            IoTPwmStart(HI_PWM1, i, PWM_SMALL_DUTY); /* 1 */
            if ((currentMode != GetKeyStatus(CURRENT_MODE)) || (currentType != GetKeyStatus(CURRENT_TYPE))) {
                return NULL;
            }
            hi_sleep((SLEEP_6_S - i + 1) / SLEEP_30_MS); /* 1 */
        }
    }
}

/* 绿灯由暗到最亮 */
void GreenLightDarkToBright(void)
{
    unsigned char currentType = 0;
    unsigned char currentMode = 0;
    
    currentMode = GetKeyStatus(CURRENT_MODE);
    currentType = GetKeyStatus(CURRENT_TYPE);
    AllLightOut();
    while (1) {
        for (hi_s32 i = 1; i < DELAY_TIMES_100; i++) { /* 计算延时100次 */
            IoTPwmStart(HI_PWM2, i, PWM_SMALL_DUTY); /* PWM0 */
            if ((currentMode != GetKeyStatus(CURRENT_MODE)) || (currentType != GetKeyStatus(CURRENT_TYPE))) {
                return NULL;
            }
            hi_sleep((SLEEP_6_S - i + 1) / SLEEP_30_MS); /* 1 */
        }
    }
}

/* 蓝灯由暗到最亮 */
void BlueLightDarkToBright(void)
{
    unsigned char currentType = 0;
    unsigned char currentMode = 0;
    
    currentMode = GetKeyStatus(CURRENT_MODE);
    currentType = GetKeyStatus(CURRENT_TYPE);
    AllLightOut();
    while (1) {
        for (int i = 1; i < DELAY_TIMES_100; i++) { /* 计算延时100次 */
            if ((currentMode != GetKeyStatus(CURRENT_MODE)) || (currentType != GetKeyStatus(CURRENT_TYPE))) {
                return NULL;
            }
            IoTPwmStart(HI_PWM3, i, PWM_SMALL_DUTY); /* PWM3 */

            hi_sleep((SLEEP_6_S - i + 1) / SLEEP_30_MS); /* 1 */
        }
    }
}

/* 紫色灯由暗到最亮 */
void PurpleLightDarkToBright(void)
{
    unsigned char currentType = 0;
    unsigned char currentMode = 0;
    
    currentMode = GetKeyStatus(CURRENT_MODE);
    currentType = GetKeyStatus(CURRENT_TYPE);
    AllLightOut();
    while (1) {
        for (int i = 0; i < DELAY_TIMES_5; i++) { /* 计算延时5次 */
            IoTPwmStart(HI_PWM1, PWM_FRQ_50 + (i * PWM_FRQ_10), PWM_SMALL_DUTY); /* 1 */
            IoTPwmStart(HI_PWM2, i, PWM_SMALL_DUTY); /* 2 */
            IoTPwmStart(HI_PWM3, PWM_FRQ_50 + (i * PWM_FRQ_10), PWM_SMALL_DUTY); /* 3 */
            if ((currentMode != GetKeyStatus(CURRENT_MODE)) || (currentType != GetKeyStatus(CURRENT_TYPE))) {
                return NULL;
            }
            hi_sleep(SLEEP_50_MS);
        }
    }
}

/* 白灯由暗到全亮。 */
void AllLightDarkToBright(void)
{
    unsigned char currentType = 0;
    unsigned char currentMode = 0;
    
    currentMode = GetKeyStatus(CURRENT_MODE);
    currentType = GetKeyStatus(CURRENT_TYPE);
    AllLightOut();
    while (1) {
        for (int i = 1; i < DELAY_TIMES_100; i++) { /* 计算延时100次 */
            IoTPwmStart(HI_PWM1, i, PWM_SMALL_DUTY); /* 1 */
            IoTPwmStart(HI_PWM2, i, PWM_SMALL_DUTY); /* 2 */
            IoTPwmStart(HI_PWM3, i, PWM_SMALL_DUTY); /* 3 */
            if ((currentMode != GetKeyStatus(CURRENT_MODE)) || (currentType != GetKeyStatus(CURRENT_TYPE))) {
                return NULL;
            }
            hi_sleep((SLEEP_6_S - i + 1) / SLEEP_30_MS); /* 1 */
        }
    }
}

/*
 * mode:3.pwm control
 * type:1.red,2.yellow,3.green,4.purple,5.all
 * 模式3：PWM无极调光模式，每按一下S2，不同的调光类型
 * 类型1：红色由暗到最亮
 * 类型2：黄灯由暗到最亮
 * 类型3：绿灯由暗到最亮
 * 类型4：紫色由暗到最亮
 * 类型5：由暗到全亮。
 */

void PwmControlSample(void)
{
    unsigned char currentType = 0;
    unsigned char currentMode = 0;
    
    currentMode = GetKeyStatus(CURRENT_MODE);
    currentType = GetKeyStatus(CURRENT_TYPE);
    printf("PwmControlSample\n");
    AllLightOut();
    while (1) {
        switch (GetKeyStatus(CURRENT_TYPE)) {
            case PWM_CONTROL_RED:
                RedLightDarkToBright();
                break;
            case PWM_CONTROL_YELLOW:
                RedLightDarkToBright();
                break;
            case PWM_CONTROL_GREEN:
                RedLightDarkToBright();
                break;
            case PWM_CONTROL_PURPLE:
                RedLightDarkToBright();
                break;
            case PWM_CONTROL_ALL:
                AllLightDarkToBright();
                break;
            default:
                break;
        }

        if ((currentMode != GetKeyStatus(CURRENT_MODE)) || (currentType != GetKeyStatus(CURRENT_TYPE))) {
            break;
        }
        hi_sleep(SLEEP_20_MS);
    }
}

/* 用按键控制白灯的亮度 */
void BrightnessControlSample(void)
{
    unsigned char currentType = 0;
    unsigned char currentMode = 0;
    
    currentMode = GetKeyStatus(CURRENT_MODE);
    currentType = GetKeyStatus(CURRENT_TYPE);
    AllLightOut();
    while (1) {
        switch (GetKeyStatus(CURRENT_TYPE)) {
            case BRIGHTNESS_LOW:
                IoTPwmStart(HI_PWM1, PWM_FRQ_20, PWM_SMALL_DUTY); /* 1, 20 */
                IoTPwmStart(HI_PWM2, PWM_FRQ_20, PWM_SMALL_DUTY); /* 2, 20 */
                IoTPwmStart(HI_PWM3, PWM_FRQ_20, PWM_SMALL_DUTY); /* 3, 20 */
                break;
            case BRIGHTNESS_MIDDLE:
                IoTPwmStart(HI_PWM1, PWM_FRQ_50, PWM_SMALL_DUTY); /* 1, 20 */
                IoTPwmStart(HI_PWM2, PWM_FRQ_50, PWM_SMALL_DUTY); /* 2, 20 */
                IoTPwmStart(HI_PWM3, PWM_FRQ_50, PWM_SMALL_DUTY); /* 3, 20 */
                break;
            case BRIGHTNESS_HIGH:
                IoTPwmStart(HI_PWM1, PWM_FRQ_100, PWM_SMALL_DUTY); /* 1, 20 */
                IoTPwmStart(HI_PWM2, PWM_FRQ_100, PWM_SMALL_DUTY); /* 2, 20 */
                IoTPwmStart(HI_PWM3, PWM_FRQ_100, PWM_SMALL_DUTY); /* 3, 20 */
                break;
            default:
                break;
        }

        if ((currentMode != GetKeyStatus(CURRENT_MODE)) || (currentType != GetKeyStatus(CURRENT_TYPE))) {
            break;
        }
        hi_sleep(SLEEP_10_MS);
    }
}

/*
 * mode:5.Human detect
 * 模式5：人体红外检测模式（设置一个好看的灯亮度）,有人靠近灯亮，无人在检测区域灯灭
 */
void HumanDetectSample(void)
{
    unsigned char currentMode = 0;
    unsigned char someoneWalking = 0;
    
    currentMode = GetKeyStatus(CURRENT_MODE);
    someoneWalking = GetKeyStatus(SOMEONE_WALKING);
    printf("HumanDetectSample start\n");
    IoSetFunc(HI_GPIO_7, 0); // gpio7
    IoTGpioSetDir(HI_GPIO_7, IOT_GPIO_DIR_IN); // gpio7
    IotGpioValue gpio_val = IOT_GPIO_VALUE1;

    while (1) {
        /* use adc channal_0 */
        /* use gpio7 */
        IoTGpioGetInputVal(HI_GPIO_7, &gpio_val); // gpio7

        if (gpio_val == IOT_GPIO_VALUE1) {
            someoneWalking = OLED_FALG_ON;
        } else {
            someoneWalking = OLED_FALG_OFF;
        }
        if (someoneWalking == OLED_FALG_ON) {
            IoTPwmStart(HI_PWM1, PWM_FRQ_50, PWM_SMALL_DUTY); /* 1,占空比百分之50 */
            IoTPwmStart(HI_PWM2, PWM_FRQ_50, PWM_SMALL_DUTY); /* 2,占空比百分之50 */
            IoTPwmStart(HI_PWM3, PWM_FRQ_50, PWM_SMALL_DUTY); /* 3,占空比百分之50 */
        } else if (someoneWalking == OLED_FALG_OFF) {
            IoTPwmStart(HI_PWM1, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 1,占空比百分之50 */
            IoTPwmStart(HI_PWM2, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 2,占空比百分之50 */
            IoTPwmStart(HI_PWM3, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 3,占空比百分之50 */
        }
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            IoTPwmStart(HI_PWM1, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 1,占空比百分之50 */
            IoTPwmStart(HI_PWM2, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 2,占空比百分之50 */
            IoTPwmStart(HI_PWM3, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 3,占空比百分之50 */
            break;
        }
        hi_sleep(SLEEP_1_MS);
    }
}

/*
 * mode:6.Light detect
 * 模式6：光敏检测模式,无光灯亮，有光灯灭
 */
void LightDetectSample(void)
{
    unsigned char currentMode = 0;
    unsigned char withLigh = 0;
    
    currentMode = GetKeyStatus(CURRENT_MODE);
    while (1) {
        withLigh = GetLightStatus();
        if (withLigh == OLED_FALG_ON) {
            IoTPwmStart(HI_PWM1, PWM_FRQ_50, PWM_SMALL_DUTY); /* 1,占空比百分之50 */
            IoTPwmStart(HI_PWM2, PWM_FRQ_50, PWM_SMALL_DUTY); /* 2,占空比百分之50 */
            IoTPwmStart(HI_PWM3, PWM_FRQ_50, PWM_SMALL_DUTY); /* 3,占空比百分之50 */
        } else if (withLigh == OLED_FALG_OFF) {
            IoTPwmStart(HI_PWM1, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 1,占空比百分之50 */
            IoTPwmStart(HI_PWM2, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 2,占空比百分之50 */
            IoTPwmStart(HI_PWM3, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 3,占空比百分之50 */
        }
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            IoTPwmStart(HI_PWM1, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 1,占空比百分之50 */
            IoTPwmStart(HI_PWM2, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 2,占空比百分之50 */
            IoTPwmStart(HI_PWM3, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 3,占空比百分之50 */
            break;
        }
        hi_sleep(SLEEP_1_MS);
    }
}

/*
 * mode:7.Union detect
 * 模式7：人体红外+光敏联合检测模式。
 * 无人有光照 => 灯不亮
 * 无人无光照 => 灯不亮
 * 有人有光照 => 灯不亮
 * 有人无光照 => 灯亮
 * g_withLightFlag = OLED_FALG_ON：无光照(黑夜)，OLED_FALG_OFF有光照(白天)
 * g_someoneWalkingFlag = OLED_FALG_ON(有人)，OLED_FALG_OFF(无人)
 */
void UnionDetectSample(void)
{
    unsigned char currentMode = 0;
    unsigned char withLigh = 0;
    unsigned char someoneWalking = 0;
    
    currentMode = GetKeyStatus(CURRENT_MODE);
    someoneWalking = GetKeyStatus(SOMEONE_WALKING);
    IoSetFunc(HI_GPIO_7, 0); // gpio7
    IoTGpioSetDir(HI_GPIO_7, IOT_GPIO_DIR_IN); // gpio7
    IotGpioValue gpio_val = IOT_GPIO_VALUE1;

    while (1) {
        IoTGpioGetInputVal(HI_GPIO_7, &gpio_val); // gpio7
        if (gpio_val == IOT_GPIO_VALUE1) {
            someoneWalking =  OLED_FALG_ON;
        } else {
            someoneWalking =  OLED_FALG_OFF;
        }
        withLigh = GetLightStatus();
        if (((someoneWalking == OLED_FALG_ON) && (withLigh == OLED_FALG_OFF)) ||
            (someoneWalking == OLED_FALG_OFF)) {
            IoTPwmStart(HI_PWM1, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 1,占空比百分之50 */
            IoTPwmStart(HI_PWM2, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 2,占空比百分之50 */
            IoTPwmStart(HI_PWM3, PWM_LOW_DUTY, PWM_SMALL_DUTY); /* 3,占空比百分之50 */
        } else if ((withLigh == OLED_FALG_ON) && (someoneWalking == OLED_FALG_ON)) {
            IoTPwmStart(HI_PWM1, PWM_FRQ_50, PWM_SMALL_DUTY); /* 1,占空比百分之50 */
            IoTPwmStart(HI_PWM2, PWM_FRQ_50, PWM_SMALL_DUTY); /* 2,占空比百分之50 */
            IoTPwmStart(HI_PWM3, PWM_FRQ_50, PWM_SMALL_DUTY); /* 3,占空比百分之50 */
        }
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            IoTPwmStart(HI_PWM1, PWM_LOW_DUTY, PWM_SMALL_DUTY);
            break;
        }
        hi_sleep(SLEEP_1_MS);
    }
}

/*
 * mode:8.Creturn
 * 模式8：返回主界面
 */
void ReturnMainEnumSample(void)
{
    unsigned char currentType = 0;
    unsigned char currentMode = 0;
    
    currentMode = GetKeyStatus(CURRENT_MODE);
    currentType = GetKeyStatus(CURRENT_TYPE);

    while (1) {
        switch (GetKeyStatus(CURRENT_TYPE)) {
            case KEY_UP:
                break;
            case KEY_DOWN:
                return;
            default:
                break;
        }

        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            break;
        }
        task_msleep(SLEEP_1_MS);
    }
}
void Gpio9LedLightFunc(void)
{
    globalStaType.g_gpio9Tick++;
    if (globalStaType.g_gpio9Tick % PAIR_2 == 0) { /* tick 时间 对2 求余， 余数为0则打开LED灯，否则关闭LED灯，即不断闪烁 */
        printf("led off\r\n");
        HisparkBoardTest(IOT_GPIO_VALUE1);
    } else {
        printf("led on\r\n");
        HisparkBoardTest(IOT_GPIO_VALUE1);
    }
}

void OledShowMenuSelect(void)
{
    unsigned char currentMode = 0;

    switch (GetKeyStatus(MENU_SELECT)) {
        case COLORFUL_LIGHT_MENU:
            if (GetKeyStatus(CURRENT_MODE) >= MAX_COLORFUL_LIGHT_MODE) {
                currentMode = SetKeyStatus(CONTROL_MODE);
            }
            break;
        case TRAFFIC_LIGHT_MENU:
            if (GetKeyStatus(CURRENT_MODE) >= MAX_TRAFFIC_LIGHT_MODE) {
                currentMode = SetKeyStatus(CONTROL_MODE);
            }
            break;
        case ENVIRONMENT_MENU:
            if (GetKeyStatus(CURRENT_MODE) >= MAX_ENVIRONMENT_MODE) {
                currentMode = SetKeyStatus(CONTROL_MODE);
            }
            break;
        case NFC_TEST_MENU:
            if (GetKeyStatus(CURRENT_MODE) >= MAX_NFC_TAG_MODE) {
                currentMode = SetKeyStatus(CONTROL_MODE);
            }
            break;
        default:
            break;
    }
}

/*
 * 按键7每按下一次，就会产生中断并调用该函数
 * 用来调整类型
 */
void GpioKey1IsrFuncMode(void)
{
    unsigned char currentType = 0;
    unsigned int currentGpio5Tick = hi_get_tick();
    unsigned int tickInterval = currentGpio5Tick - globalStaType.g_gpio5Tick;
    if (tickInterval < KEY_INTERRUPT_PROTECT_TIME) {
        return HI_NULL;
    }
    globalStaType.g_gpio5Tick = currentGpio5Tick;
    globalStaType.g_keyDownFlag = KEY_GPIO_5;
    printf("gpio key 1 down\r\n");
    if (GetKeyStatus(MENU_MODE) == MAIN_FUNCTION_SELECT_MODE) {
        printf("gpio key 1 menu select\r\n");
        globalStaType.g_menuSelect++;
        if (globalStaType.g_menuSelect >= MAX_FUNCTION_DEMO_NUM) {
            globalStaType.g_menuSelect = COLORFUL_LIGHT_MENU;
        }
        return HI_ERR_SUCCESS;
    }

    if (GetKeyStatus(MENU_MODE) == SUB_MODE_SELECT_MODE) {
        printf("gpio key 1 current mode\r\n");
        globalStaType.g_currentMode++;
        if (globalStaType.g_currentMode > TRAFFIC_RETURN_MODE) {
            globalStaType.g_currentMode = 0;
        }
        currentType = SetKeyType(CONTROL_MODE);
        OledShowTrafficLightMenuSelect();
    }
}

void OledShowColorfulLightMenuSelect(void)
{
    unsigned char currentType = 0;
    switch (GetKeyStatus(CURRENT_MODE)) {
        case CONTROL_MODE:
            if (GetKeyStatus(CURRENT_TYPE) > MAX_CONTROL_MODE_TYPE) {
                currentType = SetKeyType(RED_ON);
            }
            break;
        case COLORFUL_LIGHT_MODE:
            if (GetKeyStatus(CURRENT_TYPE) > MAX_COLORFUL_LIGHT_TYPE) {
                currentType = SetKeyType(CYCLE_FOR_ONE_SECOND);
            }
            break;
        case PWM_CONTROL_MODE:
            if (GetKeyStatus(CURRENT_TYPE) > MAX_PWM_CONTROL_TYPE) {
                currentType = SetKeyType(PWM_CONTROL_RED);
            }
            break;
        case BIRGHTNESS_MODE:
            if (GetKeyStatus(CURRENT_TYPE) > MAX_BIRGHTNESS_TYPE) {
                currentType = SetKeyType(BRIGHTNESS_LOW);
            }
            break;
        case HUMAN_DETECT_MODE:
        case LIGHT_DETECT_MODE:
        case UNION_DETECT_MODE:
        case RETURN_MODE:
            if (GetKeyStatus(CURRENT_TYPE) > RETURN_TYPE_MODE) {
                currentType = SetKeyType(KEY_UP);
            }
            break;
        default:
            break;
    }
}
void OledShowTrafficLightMenuSelect(void)
{
    unsigned char currentType = 0;
    switch (GetKeyStatus(CURRENT_MODE)) {
        case TRAFFIC_CONTROL_MODE:
            if (GetKeyStatus(CURRENT_TYPE) >= MAX_TRAFFIC_CONTROL_TYPE) {
                currentType = SetKeyType(TRAFFIC_NORMAL_TYPE);
            }
            break;
        case TRAFFIC_AUTO_MODE:
            if (GetKeyStatus(CURRENT_TYPE) > MAX_TRAFFIC_AUTO_TYPE) {
                currentType = SetKeyType(TRAFFIC_NORMAL_TYPE);
            }
            break;
        case TRAFFIC_HUMAN_MODE:
            if (GetKeyStatus(CURRENT_TYPE) > MAX_TRAFFIC_HUMAN_TYPE) {
                currentType = SetKeyType(TRAFFIC_NORMAL_TYPE);
            }
            break;
        case TRAFFIC_RETURN_MODE:
            if (GetKeyStatus(CURRENT_TYPE) > RETURN_TYPE_MODE) {
                currentType = SetKeyType(KEY_UP);
            }
            break;
        default:
            break;
    }
}
/*
 * 按键7每按下一次，就会产生中断并调用该函数
 * 用来调整类型
 */
void GpioKey2IsrFuncType(void)
{
    /* 按键防抖 */
    unsigned int currentGpio7Tick = hi_get_tick();
    unsigned int tickInterval = currentGpio7Tick - globalStaType.g_gpio7Tick;

    if (tickInterval < KEY_INTERRUPT_PROTECT_TIME) {
        return HI_NULL;
    }
    globalStaType.g_gpio7Tick = currentGpio7Tick;
    globalStaType.g_keyDownFlag = KEY_GPIO_7;
    printf("gpio key 2 down %d\r\n", globalStaType.g_currentType);
    globalStaType.g_currentType++;
    OledShowTrafficLightMenuSelect();
}

void Gpio8Interrupt(const char *param)
{
    hi_unref_param(param);
    unsigned int currentGpio8Tick = hi_get_tick();
    unsigned int tickInterval = currentGpio8Tick - globalStaType.g_gpio8Tick;
    printf("gpio8 interrupt \r\n");

    if (tickInterval < KEY_INTERRUPT_PROTECT_TIME) {
        printf("gpio8 interrupt return\r\n");
        return HI_NULL;
    }
    globalStaType.g_gpio8CurrentType++;
    if (globalStaType.g_gpio8CurrentType % PAIR_2 == 0) {  /* tick 时间 对2 求余， 余数为0则打开蜂鸣器响，否则关闭L蜂鸣器 */
        globalStaType.g_ocBeepStatus = BEEP_OFF;
    } else {
        globalStaType.g_ocBeepStatus = BEEP_ON;
    }
    if (globalStaType.g_gpio8CurrentType >= TICK_COUNT_MAX) { /* 避免越界，清零 */
        globalStaType.g_gpio8CurrentType = 0;
    }
}

/*
 * S2(gpio5)是模式按键，S1(gpio7)是类型按键。
 * 1、三色灯控制模式：每按一下类型S1按键，在绿灯亮、黄灯亮、红灯亮，三个状态之间切换
 * 2、炫彩模式，每按一下S2，不同的炫彩类型
 * 3、PWM无极调光模式，每按一下S2，不同的调光类型
 * 4、人体红外检测模式（设置一个好看的灯亮度）
 * 5、光敏检测模式
 * 6、人体红外+光敏联合检测模式。
 * 7、返回模式
 * 创建两个按键中断，按键响应后会调用对应的回调函数
 */
void AppMultiSampleDemo(void)
{
    unsigned int ret = 0;
    hi_gpio_init();
    globalStaType.g_gpio5Tick = hi_get_tick();

    globalStaType.g_menuSelect = TRAFFIC_LIGHT_MENU;
    globalStaType.g_menuMode = SUB_MODE_SELECT_MODE;

    ret = IoTGpioRegisterIsrFunc(HI_GPIO_5, IOT_INT_TYPE_EDGE,
        IOT_GPIO_EDGE_FALL_LEVEL_LOW, GetGpio5Voltage, NULL); /* 5 */
    if (ret == IOT_SUCCESS) {
        printf(" register gpio 5\r\n");
    }
    globalStaType.g_gpio8Tick = hi_get_tick();
    ret = IoTGpioRegisterIsrFunc(HI_GPIO_8, IOT_INT_TYPE_EDGE,
        IOT_GPIO_EDGE_FALL_LEVEL_LOW, Gpio8Interrupt, NULL); /* 8 */
    if (ret == IOT_SUCCESS) {
        printf(" register gpio8\r\n");
    }
}