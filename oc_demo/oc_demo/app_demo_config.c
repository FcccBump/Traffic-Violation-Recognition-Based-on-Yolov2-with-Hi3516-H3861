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
#include "iot_pwm.h"
#include "iot_i2c.h"
#include "iot_gpio.h"
#include "hi_timer.h"
#include "hi_gpio.h"
#include "app_demo_multi_sample.h"
#include "ssd1306_oled.h"
#include "app_demo_i2c_oled.h"
#include "app_demo_config.h"

#define IOT_GPIO_INDEX_9 9
#define IOT_GPIO_INDEX_10 10
#define IOT_GPIO_INDEX_11 11
#define IOT_GPIO_INDEX_12 12

#define IO_FUNC_GPIO_9_PWM0_OUT 5
#define IO_FUNC_GPIO_OUT 0

unsigned char g_ledCountFlag = 0;

#define HUMAN_MODU_RED_COUNT_SECOND     (30)
#define COUNT_FIVE_SECOND               (5)
#define COUNT_THREE_SECOND_FLASH        (3)

#define COUNT_MAX_SECOND        (4294967296)

#define DELAY_CYCLE_COUNT   (20000)
#define CN_QUEUE_WAITTIMEOUT   1000
#define LED_FLASHING_TIME  (7)
#define LAMP_FLASHING_TIME_RESET (1)
#define LAMP_FLASHING_TIME_RESET_0 (0)
#define DATA_OUT_OF_BOUND (-1)
#define AUTO_MODULE_LIGHT_ON_TIME   (6)
#define DATA_OUT_OF_BOUND (-1)
#define AUTO_MODULE_LIGHT_ON_TIME_10_SECOND   (10)
#define AUTO_MODULE_LIGHT_ON_TIME_30_SECOND   (31)

static unsigned int g_redLedAutoModuTimeCount = 0;
static unsigned int g_yellowLedAutoModuTimeCount = 0;
static unsigned int g_greenLedAutoModuTimeCount = 0;

static unsigned int redLedHumanModuTimeCount = 0;
static unsigned int yellowLedHumanModuTimeCount = 0;
static unsigned int greenLedHumanModuTimeCount = 0;

unsigned int g_redLedHumanModuTimeCounts = 0;
unsigned int g_yellowLedHumanModuTimeCounts = 0;
unsigned int g_greenLedHumanModuTimeCounts = 0;

static unsigned int timer1Count = LED_FLASHING_TIME;
static unsigned int timer7Count = LED_FLASHING_TIME;
static unsigned int humanTimerCount = LED_FLASHING_TIME;
static int timer2Count = AUTO_MODULE_LIGHT_ON_TIME;
static int timer6Count = AUTO_MODULE_LIGHT_ON_TIME_30_SECOND;
static int humanRedTimerCount = AUTO_MODULE_LIGHT_ON_TIME;
static unsigned int timer3Count = LED_FLASHING_TIME;
static unsigned int timer8Count = LED_FLASHING_TIME;
static unsigned int yellowHumanCount = LED_FLASHING_TIME;
static unsigned int timer4Count = AUTO_MODULE_LIGHT_ON_TIME;
static unsigned int timer9Count = AUTO_MODULE_LIGHT_ON_TIME;
static unsigned int humanGreenTimerCount = AUTO_MODULE_LIGHT_ON_TIME;
static unsigned int timer5Count = LED_FLASHING_TIME;
static unsigned int timer10Count = LED_FLASHING_TIME;
static unsigned int humanGreenFlashCount = LED_FLASHING_TIME;

TrafficLedStatusType trafficStaType = {0};

LightTimerCfg TrafficLightTimer = {
    .Timer1Status = 0,
    .Timer1Count = 0xff,
    .Timer2Status = 0,
    .Timer2Count = 0xff,
    .Timer3Status = 0,
    .Timer3Count = 0xff,
    .Timer3Status = 0,
    .Timer4Count = 0xff,
    .Timer5Status = 0,
    .Timer5Count = 0xff,
    .Timer6Status = 0,
    .Timer6Count = 0xff,
    .Timer7Status = 0,
    .Timer7Count = 0xff,
    .Timer8Status = 0,
    .Timer8Count = 0xff,
    .Timer9Status = 0,
    .Timer9Count = 0xff,
    .Timer10Status = 0,
    .Timer10Count = 0xff,
    .TimerYellowLightStatus = 0,
    .TimerRedLightStatus = 0,
    .TimerGreenLightStatus = 0,
};

/* GetKeyStatus */
unsigned char GetLedStatus(TrafficLedStatusDef staDef)
{
    unsigned char status = 0;
    switch (staDef) {
        case RED_LED_AUTOMODE_TIMECOUNT:
            status = trafficStaType.g_redLedAutoModuTimeCount;
            break;
        case YELLOW_LED_AUTOMODE_TIMECOUNT:
            status = trafficStaType.g_yellowLedAutoModuTimeCount;
            break;
        case GREEN_LED_AUTOMODE_TIMECOUNT:
            status = trafficStaType.g_greenLedAutoModuTimeCount;
            break;
        case RED_LED_HUMANMODE_TIMECOUNT:
            status = trafficStaType.g_redLedHumanModuTimeCount;
            break;
        case YELLOW_LED_HUMANMODE_TIMECOUNT:
            status = trafficStaType.g_yellowLedHumanModuTimeCount;
            break;
        case GREEN_LED_HUMANMODE_TIMECOUNT:
            status = trafficStaType.g_greenLedHumanModuTimeCount;
            break;
        default:
            break;
    }
    return status;
}

/* Set timer count  status */
unsigned char SetTimeCount(unsigned char setTimeCount)
{
    return setTimeCount;
}
/*
 * @bref delay time and fresh screen
 * @param unsigned int delay_time :delay unit us
 * unsigned char beep_status: 0/1, 0:beep off, 1:beep on
 */
#define MODULUS_OR_REMAINDER (10)

static void RedOledLedLigthTimeShowNum9(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:9 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:9 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void RedOledLedLigthTimeShowNum8(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:8 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:8 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void RedOledLedLigthTimeShowNum7(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:7 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:7 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void RedOledLedLigthTimeShowNum6(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:6 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:6 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void RedOledLedLigthTimeShowNum5(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:5 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:5 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void RedOledLedLigthTimeShowNum4(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:4 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:4 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void RedOledLedLigthTimeShowNum3(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:3 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:3 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void RedOledLedLigthTimeShowNum2(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:2 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:2 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void RedOledLedLigthTimeShowNum1(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:1 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:1 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void RedOledLedLigthTimeShowNum0(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}
// 绿色
static void GreenOledLedLigthTimeShowNum9(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:9 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:9 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void GreenOledLedLigthTimeShowNum8(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:8 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:8 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void GreenOledLedLigthTimeShowNum7(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:7 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:7 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void GreenOledLedLigthTimeShowNum6(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:6 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:6 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void GreenOledLedLigthTimeShowNum5(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:5 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:5 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void GreenOledLedLigthTimeShowNum4(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:4 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:4 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void GreenOledLedLigthTimeShowNum3(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:3 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:3 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void GreenOledLedLigthTimeShowNum2(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:2 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:2 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void GreenOledLedLigthTimeShowNum1(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:1 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:1 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void GreenOledLedLigthTimeShowNum0(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}
int OledLedLightHumanReckonShow(int timeCount)
{
    int ret = 0;
    unsigned char showTimeCount[4] = {0};
    unsigned char beepStatus[2] = {0};
    char *space = "  ";
    static int status = 0;
    static int sta = 0;
    static unsigned char currentMode = 0;
    static unsigned char currentType = 0;

    if (!status) {
        status = HI_TRUE;
        currentMode = GetKeyStatus(CURRENT_MODE);
        currentType = GetKeyStatus(CURRENT_TYPE);
    }
    if ((currentMode != GetKeyStatus(CURRENT_MODE)) || (currentType != GetKeyStatus(CURRENT_TYPE))) {
        GpioControl(IOT_GPIO_INDEX_10, IOT_GPIO_INDEX_10, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
        return KEY_DOWN_INTERRUPT;
    }
    ret = snprintf_s(showTimeCount, sizeof(showTimeCount), sizeof(showTimeCount), "%d", timeCount);
    if (ret < 0) {
        printf("change data format failed\r\n");
    }
    if (timeCount == AUTO_MODULE_LIGHT_ON_TIME_30_SECOND - 1) { /* 1: 0ffset */
        sta = HI_FALSE;
    }
    if (timeCount < AUTO_MODULE_LIGHT_ON_TIME_10_SECOND && sta == HI_FALSE) {
        sta = HI_TRUE;
        OledShowStr(OLED_X_POSITION_16, OLED_Y_POSITION_7,
                    space, OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
    OledShowStr(OLED_X_POSITION_16, OLED_Y_POSITION_7,
                showTimeCount, OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        ret = snprintf_s(beepStatus, sizeof(beepStatus), sizeof(beepStatus), "%d", BEEP_ON);
        if (ret < 0) {
            printf("change data format failed\r\n");
        }
        OledShowStr(OLED_X_POSITION_120, OLED_Y_POSITION_7,
                    beepStatus, OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        ret = snprintf_s(beepStatus, sizeof(beepStatus), sizeof(beepStatus), "%d", BEEP_OFF);
        if (ret < 0) {
            printf("change data format failed\r\n");
        }
        OledShowStr(OLED_X_POSITION_120, OLED_Y_POSITION_7,
                    beepStatus, OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
    return 0;
}

// 红灯
void OledLedLigthReckonByTimeShow(int timeCount)
{
    switch (timeCount) {
        case FIVE:
            RedOledLedLigthTimeShowNum5();
            break;
        case FOUR:
            RedOledLedLigthTimeShowNum4();
            break;
        case THREE:
            RedOledLedLigthTimeShowNum3();
            break;
        case TWO:
            RedOledLedLigthTimeShowNum2();
            break;
        case ONE:
            RedOledLedLigthTimeShowNum1();
            break;
        case ZERO:
            RedOledLedLigthTimeShowNum0();
            break;
        default:
            break;
    }
}

// 绿灯
void GreenOledLedLigthReckonByTimeShow(unsigned int timeCount)
{
    switch (timeCount) {
        case FIVE:
            GreenOledLedLigthTimeShowNum5();
            break;
        case FOUR:
            GreenOledLedLigthTimeShowNum4();
            break;
        case THREE:
            GreenOledLedLigthTimeShowNum3();
            break;
        case TWO:
            GreenOledLedLigthTimeShowNum2();
            break;
        case ONE:
            GreenOledLedLigthTimeShowNum1();
            break;
        case ZERO:
            GreenOledLedLigthTimeShowNum0();
            break;
        default:
            break;
    }
}

void SetBeepOperation(unsigned char beep)
{
    unsigned char bp = beep;

    if (bp == BEEP_ON) {
        IoTPwmStart(HI_PWM0, PWM_DUTY, PWM_SMALL_DUTY); /* 0, xx, xx */
        bp = BEEP_OFF;
    } else {
        IoTPwmStart(HI_PWM0, PWM_LOW_DUTY, PWM_FULL_DUTY); /* 0, xx, xx */
        bp = BEEP_ON;
    }
}

void SetBeepStatus(int cnt, unsigned char beepStatus, unsigned char beep)
{
    switch (beepStatus) {
        case BEEP_BY_ONE_SECOND:
            if ((cnt % BEEP_ONE_SECOND_INTERVAL) == DEFAULT_TYPE) {
                SetBeepOperation(beep);
            }
            break;
        case BEEP_BY_HALF_SECOND:
            if ((cnt % BEEP_HALF_SECOND_INTERVAL) == DEFAULT_TYPE) {
                SetBeepOperation(beep);
            }
            break;
        case BEEP_BY_QUARTER_SECOND:
            if ((cnt % BEEP_QUARTER_SECOND_INTERVAL) == DEFAULT_TYPE) {
                SetBeepOperation(beep);
            }
            break;
        default:
            break;
    }
}

unsigned char g_trafficAutoModeCurrent = TRAFFIC_AUTO_MODE;
void RedTimeCount6(void)
{
    GpioControl(IOT_GPIO_INDEX_10, IOT_GPIO_INDEX_10, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:3 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:3 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}
void RedTimeCount4(void)
{
    GpioControl(IOT_GPIO_INDEX_10, IOT_GPIO_INDEX_10, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:2 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:2 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}
void RedTimeCount2(void)
{
    GpioControl(IOT_GPIO_INDEX_10, IOT_GPIO_INDEX_10, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:1 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:1 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}
void RedTimeCount0(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}

static void RedLedLigthReckonByTimeShow(unsigned int timeCount)
{
    switch (timeCount) {
        case TIME_COUNT_6:
            RedTimeCount6();
            break;
        case TIME_COUNT_5:
            GpioControl(IOT_GPIO_INDEX_10, IOT_GPIO_INDEX_10, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
            break;
        case TIME_COUNT_4:
            RedTimeCount4();
            break;
        case TIME_COUNT_3:
            GpioControl(IOT_GPIO_INDEX_10, IOT_GPIO_INDEX_10, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
            break;
        case TIME_COUNT_2:
            RedTimeCount2();
            break;
        case TIME_COUNT_1:
            GpioControl(IOT_GPIO_INDEX_10, IOT_GPIO_INDEX_10, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
            break;
        case TIME_COUNT_0:
            RedTimeCount0();
            break;
        default:
            break;
    }
}

static int OledAutoRedLedLigthReckonByTimeShow(unsigned int timeCount)
{
    static unsigned char currentMode = 0;
    static unsigned char currentType = 0;
    static unsigned char sta = 0;
    int ret = 0;

    OledAutoHumanGreenBeepControl();
    /* auto mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_AUTO_MODE) {
        currentMode = GetKeyStatus(CURRENT_MODE);
        currentType = GetKeyStatus(CURRENT_TYPE);
        RedLedLigthReckonByTimeShow(timeCount);
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE) {
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    return 0;
}

static int OledHumanNormalRedLedLigthReckonByTimeShow(unsigned int timeCount)
{
    static unsigned char currentMode = 0;
    static unsigned char currentType = 0;
    static unsigned char sta = 0;
    int ret = 0;

    OledAutoHumanGreenBeepControl();
    /* human control mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE &&
        GetKeyStatus(CURRENT_TYPE) == TRAFFIC_NORMAL_TYPE) {
        currentMode = GetKeyStatus(CURRENT_MODE);
        currentType = GetKeyStatus(CURRENT_TYPE);
        RedLedLigthReckonByTimeShow(timeCount);
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_RETURN_MODE) {
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    
    if (currentType != GetKeyStatus(CURRENT_TYPE)) {
        currentType = GetKeyStatus(CURRENT_TYPE);
        return KEY_DOWN_INTERRUPT;
    }
    return 0;
}

static int OledHumanRedLedLigthReckonByTimeShow(unsigned int timeCount)
{
    static unsigned char currentMode = 0;
    static unsigned char currentType = 0;
    static unsigned char sta = 0;
    int ret = 0;

    OledAutoHumanGreenBeepControl();
    /* human control mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE &&
        GetKeyStatus(CURRENT_TYPE) == TRAFFIC_HUMAN_TYPE) {
        currentMode = GetKeyStatus(CURRENT_MODE);
        currentType = GetKeyStatus(CURRENT_TYPE);
        RedLedLigthReckonByTimeShow(timeCount);
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_RETURN_MODE) {
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    
    if (currentType != GetKeyStatus(CURRENT_TYPE)) {
        currentType = GetKeyStatus(CURRENT_TYPE);
        return KEY_DOWN_INTERRUPT;
    }
    return 0;
}

static int OledRedLedLigthOnFlashRecordShow(unsigned int timeCount)
{
    int ret = 0;

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_AUTO_MODE) {
        ret = OledAutoRedLedLigthReckonByTimeShow(timeCount);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE) {
        ret = OledHumanRedLedLigthReckonByTimeShow(timeCount);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
    }

    return 0;
}

static int OledAutoRedLedLigthFiveSecondCountShow(unsigned int timeCount)
{
    static unsigned char currentMode = 0;
    static unsigned char currentType = 0;
    static unsigned char sta = 0;
    int ret = 0;

    OledAutoHumanGreenBeepControl();
    /* auto mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_AUTO_MODE) {
        currentMode = GetKeyStatus(CURRENT_MODE);
        currentType = GetKeyStatus(CURRENT_TYPE);
        RedLedLightOnFiveSecond(timeCount);
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE) {
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    return 0;
}

static int OledHumanRedLedLigthFiveSecondCountShow(unsigned int timeCount)
{
    static unsigned char currentMode = 0;
    static unsigned char currentType = 0;
    static unsigned char sta = 0;
    int ret = 0;

    OledAutoHumanGreenBeepControl();
    /* human control mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE &&
        GetKeyStatus(CURRENT_TYPE) == TRAFFIC_HUMAN_TYPE) {
        currentMode = GetKeyStatus(CURRENT_MODE);
        currentType = GetKeyStatus(CURRENT_TYPE);
        RedLedLightOnFiveSecond(timeCount);
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_RETURN_MODE) {
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    if (currentType != GetKeyStatus(CURRENT_TYPE)) {
        currentType = GetKeyStatus(CURRENT_TYPE);
        return KEY_DOWN_INTERRUPT;
    }
    return 0;
}

static int OledRedLedLigthOnFiveSecondCountShow(unsigned int timeCount)
{
    int ret = 0;

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_AUTO_MODE) {
        ret = OledAutoRedLedLigthFiveSecondCountShow(timeCount);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE) {
        ret = OledHumanRedLedLigthFiveSecondCountShow(timeCount);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
    }

    return 0;
}

void YellowTimeCount6(void)
{
    GpioControl(IOT_GPIO_INDEX_12, IOT_GPIO_INDEX_12, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:3 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:3 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}
void YellowTimeCount4(void)
{
    GpioControl(IOT_GPIO_INDEX_12, IOT_GPIO_INDEX_12, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:2 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:2 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}
void YellowTimeCount2(void)
{
    GpioControl(IOT_GPIO_INDEX_12, IOT_GPIO_INDEX_12, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:1 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:1 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}
void YellowTimeCount0(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}
static int OledYellowLedLigthReckonByTimeShow(unsigned char timeCount)
{
    switch (timeCount) {
        case TIME_COUNT_6:
            YellowTimeCount6();
            break;
        case TIME_COUNT_5:
            GpioControl(IOT_GPIO_INDEX_12, IOT_GPIO_INDEX_12, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
            break;
        case TIME_COUNT_4:
            YellowTimeCount4();
            break;
        case TIME_COUNT_3:
            GpioControl(IOT_GPIO_INDEX_12, IOT_GPIO_INDEX_12, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
            break;
        case TIME_COUNT_2:
            YellowTimeCount2();
            break;
        case TIME_COUNT_1:
            GpioControl(IOT_GPIO_INDEX_12, IOT_GPIO_INDEX_12, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
            break;
        case TIME_COUNT_0:
            YellowTimeCount0();
            break;
        default:
            break;
    }
}
void GreenTimeCount6(void)
{
    GpioControl(IOT_GPIO_INDEX_11, IOT_GPIO_INDEX_11, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:3 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:3 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}
void GreenTimeCount4(void)
{
    GpioControl(IOT_GPIO_INDEX_11, IOT_GPIO_INDEX_11, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:2 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:2 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}
void GreenTimeCount2(void)
{
    GpioControl(IOT_GPIO_INDEX_11, IOT_GPIO_INDEX_11, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:1 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:0 Y:0 G:1 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}
void GreenTimeCount0(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:1 Y:0 G:0 B:1 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    } else {
        OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7,
                    "R:1 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    }
}
int OledGreenLedLigthReckonByTimeShow(unsigned int timeCount)
{
    switch (timeCount) {
        case TIME_COUNT_6:
            GreenTimeCount6();
            break;
        case TIME_COUNT_5:
            GpioControl(IOT_GPIO_INDEX_11, IOT_GPIO_INDEX_11, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
            break;
        case TIME_COUNT_4:
            GreenTimeCount4();
            break;
        case TIME_COUNT_3:
            GpioControl(IOT_GPIO_INDEX_11, IOT_GPIO_INDEX_11, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
            break;
        case TIME_COUNT_2:
            GreenTimeCount2();
            break;
        case TIME_COUNT_1:
            GpioControl(IOT_GPIO_INDEX_11, IOT_GPIO_INDEX_11, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
            break;
        case TIME_COUNT_0:
            GreenTimeCount0();
            break;
        default:
            break;
    }
}

#define COUNT_RESET ((hi_u8)0x0)

void RedLedLightOnFiveSecond(unsigned int timeCount)
{
    // 红灯�?5�?
    GpioControl(IOT_GPIO_INDEX_10, IOT_GPIO_INDEX_10, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        SetBeepStatus(DELAY_1_S, BEEP_BY_HALF_SECOND, BEEP_ON);
    } else {
        SetBeepStatus(DELAY_1_S, BEEP_BY_HALF_SECOND, BEEP_OFF);
    }

    if (timeCount < AUTO_MODULE_LIGHT_ON_TIME) {
        OledLedLigthReckonByTimeShow(timeCount);
    } else {
        RedOledLedLigthTimeShowNum0();
    }
    
    if (timeCount == COUNT_RESET) {
        GpioControl(IOT_GPIO_INDEX_10, IOT_GPIO_INDEX_10, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
        TrafficLightTimer.Timer2Status = HI_TRUE;
    }
}

static int OledAutoGreenLedLigthFiveSecondCountShow(unsigned int timeCount)
{
    static unsigned char currentMode = 0;
    static unsigned char currentType = 0;
    static unsigned char sta = 0;
    int ret = 0;

    OledAutoHumanGreenBeepControl();
    /* auto mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_AUTO_MODE) {
        currentMode = GetKeyStatus(CURRENT_MODE);
        currentType = GetKeyStatus(CURRENT_TYPE);
        GreenLedLightOnFiveSecond(timeCount);
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE) {
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    return 0;
}

static int OledHumanGreenLedLigthFiveSecondCountShow(unsigned int timeCount)
{
    static unsigned char currentMode = 0;
    static unsigned char currentType = 0;
    static unsigned char sta = 0;
    int ret = 0;

    OledAutoHumanGreenBeepControl();
    /* human control mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE) {
        currentMode = GetKeyStatus(CURRENT_MODE);
        currentType = GetKeyStatus(CURRENT_TYPE);
        GreenLedLightOnFiveSecond(timeCount);
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_RETURN_MODE) {
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    if (currentType != GetKeyStatus(CURRENT_TYPE)) {
        currentType = GetKeyStatus(CURRENT_TYPE);
        return KEY_DOWN_INTERRUPT;
    }
    return 0;
}

static int OledGreenLedLigthOnFiveSecondCountShow(unsigned int timeCount)
{
    int ret = 0;

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_AUTO_MODE) {
        ret = OledAutoGreenLedLigthFiveSecondCountShow(timeCount);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE) {
        ret = OledHumanGreenLedLigthFiveSecondCountShow(timeCount);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
    }

    return 0;
}

void GreenLedLightOnFiveSecond(unsigned int timeCount)
{
    GpioControl(IOT_GPIO_INDEX_11, IOT_GPIO_INDEX_11, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        SetBeepStatus(DELAY_1_S, BEEP_BY_HALF_SECOND, BEEP_ON);
    } else {
        SetBeepStatus(DELAY_1_S, BEEP_BY_HALF_SECOND, BEEP_OFF);
    }
    if (timeCount < AUTO_MODULE_LIGHT_ON_TIME) {
        GreenOledLedLigthReckonByTimeShow(timeCount);
    }
    if (timeCount == COUNT_RESET) {
        GpioControl(IOT_GPIO_INDEX_11, IOT_GPIO_INDEX_11, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
        if (TrafficLightTimer.HumanTimerGreenLightStatus) {
            TrafficLightTimer.Timer9Status = HI_TRUE;
        } else {
            TrafficLightTimer.Timer3Status = HI_TRUE;
        }
    }
}

int OledRedLedLigthOnRecordShow(int timer2, unsigned int timer1)
{
    int ret;

    if ((TrafficLightTimer.TimerYellowLightStatus == HI_FALSE) &&
        (TrafficLightTimer.TimerGreenLightStatus == HI_FALSE) &&
        (TrafficLightTimer.Timer1Status == HI_FALSE) &&
        (TrafficLightTimer.Timer2Status == HI_FALSE)) {
        ret = OledRedLedLigthOnFiveSecondCountShow(timer2);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    /* 闪烁3s */
    if (TrafficLightTimer.Timer2Status == HI_TRUE) {
        TrafficLightTimer.Timer1Status = HI_TRUE;
        ret = OledRedLedLigthOnFlashRecordShow(timer1);
        if (ret == KEY_DOWN_INTERRUPT) {
            TrafficLightTimer.Timer2Status = HI_FALSE;
            TrafficLightTimer.Timer1Status = HI_FALSE;
            return KEY_DOWN_INTERRUPT;
        }
        if (timer1Count == LAMP_FLASHING_TIME_RESET_0) {
            TrafficLightTimer.TimerYellowLightStatus = HI_TRUE;
            TrafficLightTimer.Timer2Status = HI_FALSE;
            TrafficLightTimer.Timer1Status = HI_FALSE;
        }
    }
    return 0;
}

void AutoHumanYellowStatusSet(unsigned int timeCount)
{
    if ((TrafficLightTimer.TimerYellowLightStatus == HI_TRUE) &&
        (TrafficLightTimer.Timer1Status == HI_FALSE)) {
        if (timeCount == LAMP_FLASHING_TIME_RESET_0) {
            TrafficLightTimer.TimerYellowLightStatus = HI_FALSE;
            TrafficLightTimer.TimerGreenLightStatus = HI_TRUE;
        }
    }
    if (TrafficLightTimer.HumanTimerYellowLightStatus == HI_TRUE &&
        TrafficLightTimer.Timer6Status == HI_FALSE &&
        TrafficLightTimer.Timer7Status == HI_FALSE) {
        if (timeCount == LAMP_FLASHING_TIME_RESET_0) {
            TrafficLightTimer.HumanTimerYellowLightStatus = HI_FALSE;
            TrafficLightTimer.HumanTimerGreenLightStatus = HI_TRUE;
        }
    }
}

void OledAutoHumanGreenBeepControl(void)
{
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        SetBeepStatus(DELAY_1_S, BEEP_BY_HALF_SECOND, BEEP_ON);
    } else {
        SetBeepStatus(DELAY_1_S, BEEP_BY_HALF_SECOND, BEEP_OFF);
    }
}

static int OledAutoYellowLedLigthOnRecordShow(unsigned int timeCount)
{
    static unsigned char currentMode = 0;
    static unsigned char currentType = 0;
    static unsigned char sta = 0;
    int ret = 0;

    OledAutoHumanGreenBeepControl();
    /* auto mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_AUTO_MODE) {
        currentMode = GetKeyStatus(CURRENT_MODE);
        currentType = GetKeyStatus(CURRENT_TYPE);
        OledYellowLedLigthReckonByTimeShow(timeCount);
    }
    /* status config */
    AutoHumanYellowStatusSet(timeCount);
    /* Exit current mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE) {
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    return 0;
}

static int OledHumanRedLedLigth30SecondCountShow(unsigned int timeCount)
{
    static unsigned char currentMode = 0;
    static unsigned char currentType = 0;
    static unsigned char sta = 0;
    int ret = 0;

    OledAutoHumanGreenBeepControl();
    /* human control mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE &&
        GetKeyStatus(CURRENT_TYPE) == TRAFFIC_NORMAL_TYPE) {
        currentMode = GetKeyStatus(CURRENT_MODE);
        currentType = GetKeyStatus(CURRENT_TYPE);
        NormalTypeRedLedLightOnRecord30Second(timeCount);
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_RETURN_MODE) {
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    if (currentType != GetKeyStatus(CURRENT_TYPE)) {
        currentType = GetKeyStatus(CURRENT_TYPE);
        return KEY_DOWN_INTERRUPT;
    }
    return 0;
}

static int OledHumanNormalYellowLedLigthOnRecordShow(unsigned int timeCount)
{
    static unsigned char currentMode = 0;
    static unsigned char currentType = 0;
    static unsigned char sta = 0;
    int ret = 0;

    OledAutoHumanGreenBeepControl();
    /* human mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE &&
        GetKeyStatus(CURRENT_TYPE) == TRAFFIC_NORMAL_TYPE) {
        currentMode = GetKeyStatus(CURRENT_MODE);
        currentType = GetKeyStatus(CURRENT_TYPE);
        OledYellowLedLigthReckonByTimeShow(timeCount);
    }
    /* status config */
    AutoHumanYellowStatusSet(timeCount);
    /* Exit current mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_RETURN_MODE) {
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    if ((currentType != GetKeyStatus(CURRENT_TYPE))) {
        currentType = GetKeyStatus(CURRENT_TYPE);
        return KEY_DOWN_INTERRUPT;
    }
    return 0;
}

static int OledHumanYellowLedLigthOnRecordShow(unsigned int timeCount)
{
    static unsigned char currentMode = 0;
    static unsigned char currentType = 0;
    static unsigned char sta = 0;
    int ret = 0;

    OledAutoHumanGreenBeepControl();
    /* human mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE &&
        GetKeyStatus(CURRENT_TYPE) == TRAFFIC_HUMAN_TYPE) {
        currentMode = GetKeyStatus(CURRENT_MODE);
        currentType = GetKeyStatus(CURRENT_TYPE);
        OledYellowLedLigthReckonByTimeShow(timeCount);
    }
    /* status config */
    AutoHumanYellowStatusSet(timeCount);
    /* Exit current mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_RETURN_MODE) {
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    if ((currentType != GetKeyStatus(CURRENT_TYPE))) {
        currentType = GetKeyStatus(CURRENT_TYPE);
        return KEY_DOWN_INTERRUPT;
    }
    return 0;
}

static int OledYellowLedLigthOnRecordShow(unsigned int timeCount)
{
    int ret = 0;

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_AUTO_MODE) {
        ret = OledAutoYellowLedLigthOnRecordShow(timeCount);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE) {
        ret = OledHumanYellowLedLigthOnRecordShow(timeCount);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
    }

    return 0;
}

static int OledAutoGreenLedLigthReckonByTimeShow(unsigned char timeCount)
{
    static unsigned char currentMode = 0;
    static unsigned char currentType = 0;
    static unsigned char sta = 0;
    int ret = 0;

    OledAutoHumanGreenBeepControl();
    /* auto mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_AUTO_MODE) {
        currentMode = GetKeyStatus(CURRENT_MODE);
        currentType = GetKeyStatus(CURRENT_TYPE);
        OledGreenLedLigthReckonByTimeShow(timeCount);
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE) {
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    return 0;
}

static int OledHumanNormalGreenLedLigthReckonByTimeShow(unsigned char timeCount)
{
    static unsigned char currentMode = 0;
    static unsigned char currentType = 0;
    static unsigned char sta = 0;
    int ret = 0;

    OledAutoHumanGreenBeepControl();
    /* auto mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE &&
        GetKeyStatus(CURRENT_TYPE) == TRAFFIC_NORMAL_TYPE) {
        currentMode = GetKeyStatus(CURRENT_MODE);
        currentType = GetKeyStatus(CURRENT_TYPE);
        OledGreenLedLigthReckonByTimeShow(timeCount);
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_RETURN_MODE) {
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            return KEY_DOWN_INTERRUPT;
        }
    }

    if (currentType != GetKeyStatus(CURRENT_TYPE)) {
        currentType = GetKeyStatus(CURRENT_TYPE);
        return KEY_DOWN_INTERRUPT;
    }
    return 0;
}

static int OledHumanGreenLedLigthReckonByTimeShow(unsigned char timeCount)
{
    static unsigned char currentMode = 0;
    static unsigned char currentType = 0;
    static unsigned char sta = 0;
    int ret = 0;

    OledAutoHumanGreenBeepControl();
    /* auto mode */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE &&
        GetKeyStatus(CURRENT_TYPE) == TRAFFIC_HUMAN_TYPE) {
        currentMode = GetKeyStatus(CURRENT_MODE);
        currentType = GetKeyStatus(CURRENT_TYPE);
        OledGreenLedLigthReckonByTimeShow(timeCount);
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_RETURN_MODE) {
        if (currentMode != GetKeyStatus(CURRENT_MODE)) {
            return KEY_DOWN_INTERRUPT;
        }
    }

    if (currentType != GetKeyStatus(CURRENT_TYPE)) {
        currentType = GetKeyStatus(CURRENT_TYPE);
        return KEY_DOWN_INTERRUPT;
    }
    return 0;
}

static int OledGreenLedLigthOnFlashRecordShow(unsigned int timeCount)
{
    int ret = 0;

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_AUTO_MODE) {
        ret = OledAutoGreenLedLigthReckonByTimeShow(timeCount);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
    }

    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE) {
        ret = OledHumanGreenLedLigthReckonByTimeShow(timeCount);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
    }

    return 0;
}

int OledGreenLedLigthOnRecordShow(unsigned char timer4, unsigned char timer5)
{
    unsigned char ret = 0;

    if ((TrafficLightTimer.TimerGreenLightStatus == HI_TRUE) &&
        (TrafficLightTimer.Timer3Status == HI_FALSE) &&
        (TrafficLightTimer.Timer2Status == HI_FALSE) &&
        (TrafficLightTimer.Timer1Status == HI_FALSE)) {
        ret = OledGreenLedLigthOnFiveSecondCountShow(timer4);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    /* 绿灯闪烁3s */
    if (TrafficLightTimer.Timer3Status == HI_TRUE) {
        ret = OledGreenLedLigthOnFlashRecordShow(timer5);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
        if (timer5Count == LAMP_FLASHING_TIME_RESET_0) {
            TrafficLightTimer.TimerGreenLightStatus = HI_FALSE;
            TrafficLightTimer.Timer3Status = HI_FALSE;
        }
    }
    return 0;
}

int OledHumanModeGreenLedLigthOnRecordShow(unsigned char timer9, unsigned char timer10)
{
    int ret = 0;

    if ((TrafficLightTimer.HumanTimerGreenLightStatus == HI_TRUE) &&
        (TrafficLightTimer.Timer6Status == HI_FALSE) &&
        (TrafficLightTimer.Timer7Status == HI_FALSE) &&
        (TrafficLightTimer.Timer10Status == HI_FALSE)) {
        ret = OledHumanGreenLedLigthFiveSecondCountShow(timer9);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    if (TrafficLightTimer.Timer9Status == HI_TRUE) {
        TrafficLightTimer.Timer10Status = HI_TRUE;
        ret = OledHumanNormalGreenLedLigthReckonByTimeShow(timer10);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
        if (timer10Count == LAMP_FLASHING_TIME_RESET_0) {
            SetKeyType(TRAFFIC_NORMAL_TYPE);
            TrafficLightTimer.Timer6Status = HI_FALSE;
            TrafficLightTimer.Timer7Status = HI_FALSE;
            TrafficLightTimer.Timer8Status = HI_FALSE;
            TrafficLightTimer.Timer9Status = HI_FALSE;
            TrafficLightTimer.Timer10Status = HI_FALSE;
            TrafficLightTimer.HumanTimerYellowLightStatus = HI_FALSE;
            TrafficLightTimer.HumanTimerGreenLightStatus = HI_FALSE;
            TrafficLightTimer.HumanTimerGreenLightStatus = HI_FALSE;
            return KEY_DOWN_INTERRUPT;
        }
    }
    return 0;
}

#define CN_QUEUE_WAITTIMEOUT   1000
void AutoModeCountReset(void)
{
    timer1Count = SetTimeCount(LED_FLASHING_TIME);
    timer2Count = SetTimeCount(AUTO_MODULE_LIGHT_ON_TIME);
    timer3Count = SetTimeCount(LED_FLASHING_TIME);
    timer4Count = SetTimeCount(AUTO_MODULE_LIGHT_ON_TIME);
    timer5Count = SetTimeCount(LED_FLASHING_TIME);
    timer6Count = SetTimeCount(AUTO_MODULE_LIGHT_ON_TIME_30_SECOND);
    timer7Count = SetTimeCount(LED_FLASHING_TIME);
    timer8Count = SetTimeCount(LED_FLASHING_TIME);
    timer9Count = SetTimeCount(AUTO_MODULE_LIGHT_ON_TIME);
    timer10Count = SetTimeCount(LED_FLASHING_TIME);

    humanTimerCount = SetTimeCount(LED_FLASHING_TIME);
    humanRedTimerCount = SetTimeCount(AUTO_MODULE_LIGHT_ON_TIME);
    yellowHumanCount = SetTimeCount(LED_FLASHING_TIME);
    humanGreenTimerCount = SetTimeCount(AUTO_MODULE_LIGHT_ON_TIME);
    humanGreenFlashCount = SetTimeCount(LED_FLASHING_TIME);

    TrafficLightTimer.Timer1Count = SetTimeCount(LED_FLASHING_TIME);
    TrafficLightTimer.Timer2Count = SetTimeCount(AUTO_MODULE_LIGHT_ON_TIME);
    TrafficLightTimer.Timer3Count = SetTimeCount(LED_FLASHING_TIME);
    TrafficLightTimer.Timer4Count = SetTimeCount(AUTO_MODULE_LIGHT_ON_TIME);
    TrafficLightTimer.Timer5Count = SetTimeCount(LED_FLASHING_TIME);
    TrafficLightTimer.Timer6Count = SetTimeCount(AUTO_MODULE_LIGHT_ON_TIME_30_SECOND);
    TrafficLightTimer.Timer7Count = SetTimeCount(LED_FLASHING_TIME);
    TrafficLightTimer.Timer8Count = SetTimeCount(LED_FLASHING_TIME);
    TrafficLightTimer.Timer9Count = SetTimeCount(AUTO_MODULE_LIGHT_ON_TIME);
    TrafficLightTimer.Timer10Count = SetTimeCount(LED_FLASHING_TIME);

    TrafficLightTimer.Timer1Status = 0;
    TrafficLightTimer.Timer2Status = 0;
    TrafficLightTimer.Timer3Status = 0;
    TrafficLightTimer.Timer4Status = 0;
    TrafficLightTimer.Timer5Status = 0;
    TrafficLightTimer.Timer6Status = 0;
    TrafficLightTimer.Timer7Status = 0;
    TrafficLightTimer.Timer8Status = 0;
    TrafficLightTimer.Timer9Status = 0;
    TrafficLightTimer.Timer10Status = 0;
    TrafficLightTimer.TimerRedLightStatus = 0;
    TrafficLightTimer.TimerYellowLightStatus = 0;
    TrafficLightTimer.TimerGreenLightStatus = 0;
    TrafficLightTimer.HumanTimerRedLightStatus = 0;
    TrafficLightTimer.HumanTimerYellowLightStatus = 0;
    TrafficLightTimer.HumanTimerGreenLightStatus = 0;
}

/* traffic auto mode */
void TrafficAutoModeSample(void)
{
    int ret = 0;

    IoTPwmInit(0); /* PWM0 */
    IoSetFunc(IOT_GPIO_INDEX_9, IO_FUNC_GPIO_9_PWM0_OUT);
    IoTGpioSetDir(IOT_GPIO_INDEX_9, IOT_GPIO_DIR_OUT);
    AutoModeCountReset();
    while (1) {
        ret = OledRedLedLigthOnRecordShow(TrafficLightTimer.Timer2Count,
            TrafficLightTimer.Timer1Count);
        if (ret == KEY_DOWN_INTERRUPT) {
            printf("red led close\r\n");
            AllLedOff();
            (void)SetKeyType(TRAFFIC_NORMAL_TYPE);
            break;
        }
        ret = OledYellowLedLigthOnRecordShow(TrafficLightTimer.Timer3Count);
        if (ret == KEY_DOWN_INTERRUPT) {
            printf("yellow led close\r\n");
            AllLedOff();
            (void)SetKeyType(TRAFFIC_NORMAL_TYPE);
            break;
        }
        ret = OledGreenLedLigthOnRecordShow(TrafficLightTimer.Timer4Count,
                                            TrafficLightTimer.Timer5Count);
        if (ret == KEY_DOWN_INTERRUPT) {
            printf("green led close\r\n");
            AllLedOff();
            (void)SetKeyType(TRAFFIC_NORMAL_TYPE);
            break;
        }
    }
}

void NormalTypeRedLedLightOnRecord30Second(unsigned char normalTimeCount)
{
    GpioControl(IOT_GPIO_INDEX_10, IOT_GPIO_INDEX_10, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE1, IO_FUNC_GPIO_OUT);
    if (GetKeyStatus(OC_BEEP_STATUS) == BEEP_ON) {
        SetBeepStatus(DELAY_1_S, BEEP_BY_HALF_SECOND, BEEP_ON);
    } else {
        SetBeepStatus(DELAY_1_S, BEEP_BY_HALF_SECOND, BEEP_OFF);
    }

    if (normalTimeCount < AUTO_MODULE_LIGHT_ON_TIME_30_SECOND) {
        OledLedLightHumanReckonShow(normalTimeCount);
    }  else {
        RedOledLedLigthTimeShowNum0();
    }
    
    if (normalTimeCount == COUNT_RESET) {
        GpioControl(IOT_GPIO_INDEX_10, IOT_GPIO_INDEX_10, IOT_GPIO_DIR_OUT, IOT_GPIO_VALUE0, IO_FUNC_GPIO_OUT);
        TrafficLightTimer.Timer6Status = HI_TRUE;
    }
}

int NormalTypeRedLedLightOnRecord(unsigned char normalTimeCount1, unsigned char normalTimeCount2)
{
    int ret = 0;
    // 红灯�?30�?
    if (TrafficLightTimer.Timer6Status == HI_FALSE &&
        TrafficLightTimer.Timer7Status == HI_FALSE &&
        TrafficLightTimer.HumanTimerYellowLightStatus == HI_FALSE &&
        TrafficLightTimer.HumanTimerGreenLightStatus == HI_FALSE &&
        TrafficLightTimer.Timer8Status == HI_FALSE &&
        TrafficLightTimer.Timer9Status == HI_FALSE) {
        ret = OledHumanRedLedLigth30SecondCountShow(normalTimeCount1);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
    }
    
    // 红灯�?3�?
    if (TrafficLightTimer.Timer6Status == HI_TRUE &&
        TrafficLightTimer.Timer2Status == HI_FALSE &&
        TrafficLightTimer.Timer1Status == HI_FALSE) {
        TrafficLightTimer.Timer7Status = HI_TRUE;
        ret = OledHumanNormalRedLedLigthReckonByTimeShow(normalTimeCount2);
        if (ret == KEY_DOWN_INTERRUPT) {
            return KEY_DOWN_INTERRUPT;
        }
        if (normalTimeCount2 == LAMP_FLASHING_TIME_RESET_0) {
            TrafficLightTimer.HumanTimerYellowLightStatus = HI_TRUE;
            TrafficLightTimer.Timer6Status = HI_FALSE;
            TrafficLightTimer.Timer7Status = HI_FALSE;
        }
    }
    return 0;
}

/* traffic normal type */
void TrafficNormalType(void)
{
    int ret = 0;

    IoTPwmInit(0); /* PWM0 */
    IoSetFunc(IOT_GPIO_INDEX_9, IO_FUNC_GPIO_9_PWM0_OUT);
    IoTGpioSetDir(IOT_GPIO_INDEX_9, IOT_GPIO_DIR_OUT);
    AutoModeCountReset();
    while (1) {
        ret = NormalTypeRedLedLightOnRecord(TrafficLightTimer.Timer6Count,
                                            TrafficLightTimer.Timer7Count);
        if (ret == KEY_DOWN_INTERRUPT) {
            printf("normal type red led\r\n");
            AllLedOff();
            break;
        }
        ret = OledHumanNormalYellowLedLigthOnRecordShow(TrafficLightTimer.Timer8Count);
        if (ret == KEY_DOWN_INTERRUPT) {
            printf("normal type yellow led\r\n");
            AllLedOff();

            break;
        }
        ret = OledHumanModeGreenLedLigthOnRecordShow(TrafficLightTimer.Timer9Count,
            TrafficLightTimer.Timer10Count);
        if (ret == KEY_DOWN_INTERRUPT) {
            printf("normal type green led\r\n");
            AllLedOff();
            break;
        }
    }
}

/*
    @bref traffic human type
    @param void
*/
void TrafficHumanType(void)
{
    TrafficAutoModeSample();
}

/* traffic human mode */
void TrafficHumanModeSample(void)
{
    IoTPwmInit(0); /* PWM0 */
    IoSetFunc(IOT_GPIO_INDEX_9, IO_FUNC_GPIO_9_PWM0_OUT);
    IoTGpioSetDir(IOT_GPIO_INDEX_9, IOT_GPIO_DIR_OUT);

    switch (GetKeyStatus(CURRENT_TYPE)) {
        case TRAFFIC_NORMAL_TYPE:
            TrafficNormalType();
            break;
        case TRAFFIC_HUMAN_TYPE:
            TrafficHumanType();
            break;
        default:
            break;
    }
}

void OledTrafficControlModeShow(void)
{
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_0, \
                "WiFi-AP OFF  U:0", OLED_DISPLAY_STRING_TYPE_1); /* 0, 0, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_1, \
                "                ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 1, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_2, \
                " Traffic Light  ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 2, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_3, \
                "                ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 3, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_4, \
                "Mode:           ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 4, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_5, \
                "1.Control mode: ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 5, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_6, \
                "                ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 6, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7, \
                "                ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    switch (GetKeyStatus(CURRENT_TYPE)) {
        case RED_ON:
            OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7, "1.Red On       ",
                        OLED_DISPLAY_STRING_TYPE_1);  /* 0, 7, xx, 1 */
            break;
        case YELLOW_ON:
            OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7, "2.Yellow On     ",
                        OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
            break;
        case GREEN_ON:
            OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7, "3.Green On      ",
                        OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
            break;
        default:
            break;
    }
    TrafficControlModeSample();
}

void OledTrafficAutoModeShow(void)
{
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_4, \
                "Mode:           ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 4, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_5, \
                "2.Auto mode:    ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 5, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_6, \
                "                ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 6, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7, \
                "R:0 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    AllLedOff();
    TrafficAutoModeSample();
}

void OledTrafficHumanModeShow(void)
{
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_4, \
                "Mode:           ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 4, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_5, \
                "3.Human mode:   ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 5, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_6, \
                "                ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 6, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7, \
                "R:0 Y:0 G:0 B:0 ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    AllLedOff();
    TrafficHumanModeSample();
}

void OledTrafficReturnModeShow(void)
{
    static int status = 0;
    static unsigned char currentMode = 0;

    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_4, \
                "Return Menu     ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 4, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_5, \
                "                ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 5, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_6, \
                "                ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 6, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7, \
                "Continue        ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
    AllLedOff();
    IoTPwmStart(HI_PWM0, PWM_LOW_DUTY, PWM_FULL_DUTY); // 关闭beep
    if (!status) {
        status = HI_TRUE;
        currentMode = GetKeyStatus(CURRENT_MODE);
    }
    if (currentMode != GetKeyStatus(CURRENT_MODE)) {
        return;
    }
}

void TrafficDisplayInit(void)
{
    // clean screen
    OledFillScreen(OLED_CLEAN_SCREEN);
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_0, \
                "WiFi-AP  ON  U:1", OLED_DISPLAY_STRING_TYPE_1); /* 0, 0, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_1, \
                "                ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 1, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_2, \
                " Traffic Light  ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 2, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_3, \
                "                ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 3, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_4, \
                "Mode:           ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 4, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_5, \
                "1.Control mode: ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 5, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_6, \
                "                ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 6, xx, 1 */
    OledShowStr(OLED_X_POSITION_0, OLED_Y_POSITION_7, \
                "                ", OLED_DISPLAY_STRING_TYPE_1); /* 0, 7, xx, 1 */
}

void TrafficDisplay(void)
{
    unsigned char currentType = 0;
    while (HI_ERR_SUCCESS != OledInit()) {
    }
    /* 按键中断初始�? */
    TestGpioInit();
    // clean screen
    TrafficDisplayInit();
    currentType = SetKeyType(KEY_UP);
    while (1) {
        switch (GetKeyStatus(CURRENT_MODE)) {
            case TRAFFIC_CONTROL_MODE:
                OledTrafficControlModeShow();
                break;
            case TRAFFIC_AUTO_MODE:
                OledTrafficAutoModeShow();
                break;
            case TRAFFIC_HUMAN_MODE:
                OledTrafficHumanModeShow();
                break;
            case TRAFFIC_RETURN_MODE:
                OledTrafficReturnModeShow();
                break;
            default:
                break;
        }
    }
}
/* auto mode red led flash time count */
void Timer1Callback(unsigned int arg)
{
    if (timer1Count == LAMP_FLASHING_TIME_RESET_0) {
        timer1Count = LED_FLASHING_TIME;
        TrafficLightTimer.Timer1Count = LED_FLASHING_TIME;
        return;
    }
    if (timer7Count == LAMP_FLASHING_TIME_RESET_0) {
        timer7Count = LED_FLASHING_TIME;
        TrafficLightTimer.Timer7Count = LED_FLASHING_TIME;
        return;
    }
    if (humanTimerCount == LAMP_FLASHING_TIME_RESET_0) {
        humanTimerCount = LED_FLASHING_TIME;
        TrafficLightTimer.Timer1Count = LED_FLASHING_TIME;
        return;
    }
    if ((TrafficLightTimer.Timer2Status == HI_TRUE) &&
        (TrafficLightTimer.Timer1Status == HI_TRUE)) {
        if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_AUTO_MODE) {
            timer1Count--;
            TrafficLightTimer.Timer1Count = timer1Count;
            trafficStaType.g_redLedAutoModuTimeCount = timer1Count;
            printf("Timer1Count = %d\r\n", TrafficLightTimer.Timer1Count);
        }
    }
    if (TrafficLightTimer.Timer6Status == HI_TRUE &&
        TrafficLightTimer.Timer7Status == HI_TRUE) {
        if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE &&
            GetKeyStatus(CURRENT_TYPE) == TRAFFIC_NORMAL_TYPE) {
            timer7Count--;
            TrafficLightTimer.Timer7Count = timer7Count;
            if (TrafficLightTimer.Timer7Count == DATA_OUT_OF_BOUND) {
                TrafficLightTimer.Timer7Count = LAMP_FLASHING_TIME_RESET_0;
            }
            printf("Timer7Count = %d\r\n", TrafficLightTimer.Timer7Count);
        }
    }
    /* 人工干预模式 */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE &&
        GetKeyStatus(CURRENT_TYPE) == TRAFFIC_HUMAN_TYPE &&
        TrafficLightTimer.Timer2Status == HI_TRUE &&
        TrafficLightTimer.Timer1Status == HI_TRUE) {
        humanTimerCount--;
        TrafficLightTimer.Timer1Count = humanTimerCount;
        printf("humanTimerCount = %d\r\n", TrafficLightTimer.Timer1Count);
    }
}

void HumanTimer2Cb(void)
{
    if (TrafficLightTimer.Timer6Status == HI_FALSE &&
        TrafficLightTimer.Timer7Status == HI_FALSE &&
        TrafficLightTimer.HumanTimerYellowLightStatus == HI_FALSE &&
        TrafficLightTimer.HumanTimerGreenLightStatus == HI_FALSE &&
        TrafficLightTimer.Timer8Status == HI_FALSE &&
        TrafficLightTimer.Timer9Status == HI_FALSE) {
        if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE &&
            GetKeyStatus(CURRENT_TYPE) == TRAFFIC_NORMAL_TYPE) {
            timer6Count--;
            TrafficLightTimer.Timer6Count = timer6Count;
            trafficStaType.g_redLedHumanModuTimeCount = timer6Count;
            if (TrafficLightTimer.Timer6Count == DATA_OUT_OF_BOUND) {
                TrafficLightTimer.Timer6Count = LAMP_FLASHING_TIME_RESET_0;
            }
        }
        if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE &&
            GetKeyStatus(CURRENT_TYPE) == TRAFFIC_HUMAN_TYPE &&
            TrafficLightTimer.Timer2Status == HI_FALSE &&
            TrafficLightTimer.Timer1Status == HI_FALSE &&
            (TrafficLightTimer.TimerYellowLightStatus == HI_FALSE) &&
            (TrafficLightTimer.TimerGreenLightStatus == HI_FALSE)) {
            humanRedTimerCount--;
            TrafficLightTimer.Timer2Count = humanRedTimerCount;
            trafficStaType.g_redLedHumanModuTimeCount = humanRedTimerCount;
            if (TrafficLightTimer.Timer2Count == DATA_OUT_OF_BOUND) {
                TrafficLightTimer.Timer2Count = LAMP_FLASHING_TIME_RESET_0;
            }
        }
    }
}
/* auto mode red led time count */
void Timer2Callback(int arg)
{
    if (timer2Count == LAMP_FLASHING_TIME_RESET_0) {
        TrafficLightTimer.Timer2Count = AUTO_MODULE_LIGHT_ON_TIME;
        timer2Count = AUTO_MODULE_LIGHT_ON_TIME;
        return;
    }
    if (timer6Count == LAMP_FLASHING_TIME_RESET_0) {
        TrafficLightTimer.Timer6Count = AUTO_MODULE_LIGHT_ON_TIME_30_SECOND;
        timer6Count = AUTO_MODULE_LIGHT_ON_TIME_30_SECOND;
        return;
    }
    if ((TrafficLightTimer.Timer2Status == HI_FALSE) &&
        (TrafficLightTimer.Timer1Status == HI_FALSE) &&
        (TrafficLightTimer.TimerYellowLightStatus == HI_FALSE) &&
        (TrafficLightTimer.TimerGreenLightStatus == HI_FALSE)) {
        if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_AUTO_MODE) {
            timer2Count--;
            TrafficLightTimer.Timer2Count = timer2Count;
            trafficStaType.g_redLedAutoModuTimeCount = timer2Count;
            if (TrafficLightTimer.Timer2Count == DATA_OUT_OF_BOUND) {
                TrafficLightTimer.Timer2Count = LAMP_FLASHING_TIME_RESET_0;
            }
        }
    }
    HumanTimer2Cb();
}
/* auto mode yellow time count */
void Timer3Callback(unsigned int arg)
{
    if (timer3Count == LAMP_FLASHING_TIME_RESET_0) {
        timer3Count = LED_FLASHING_TIME;
        TrafficLightTimer.Timer3Count = LED_FLASHING_TIME;
        return;
    }
    if (timer8Count == LAMP_FLASHING_TIME_RESET_0) {
        timer8Count = LED_FLASHING_TIME;
        TrafficLightTimer.Timer8Count = LED_FLASHING_TIME;
        return;
    }
    if (yellowHumanCount == LAMP_FLASHING_TIME_RESET_0) {
        yellowHumanCount = LED_FLASHING_TIME;
        TrafficLightTimer.Timer3Count = LED_FLASHING_TIME;
        return;
    }
    if ((TrafficLightTimer.TimerYellowLightStatus == HI_TRUE) &&
        (TrafficLightTimer.TimerGreenLightStatus == HI_FALSE) &&
        (TrafficLightTimer.Timer1Status == HI_FALSE) &&
        (TrafficLightTimer.Timer2Status == HI_FALSE)) {
        if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_AUTO_MODE) {
            timer3Count--;
            TrafficLightTimer.Timer3Count = timer3Count;
            trafficStaType.g_yellowLedAutoModuTimeCount = timer3Count;
            printf("Timer3Count = %d\r\n", TrafficLightTimer.Timer3Count);
        }
    }
    if (TrafficLightTimer.HumanTimerYellowLightStatus == HI_TRUE &&
        TrafficLightTimer.HumanTimerGreenLightStatus == HI_FALSE &&
        TrafficLightTimer.Timer6Status == HI_FALSE &&
        TrafficLightTimer.Timer7Status == HI_FALSE) {
        if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE) {
            timer8Count--;
            TrafficLightTimer.Timer8Count = timer8Count;
            trafficStaType.g_yellowLedHumanModuTimeCount = timer8Count;
            printf("Timer8Count = %d\r\n", TrafficLightTimer.Timer8Count);
        }
    }
    /* 人工干预模式 */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE &&
        GetKeyStatus(CURRENT_TYPE) == TRAFFIC_HUMAN_TYPE &&
        TrafficLightTimer.TimerYellowLightStatus == HI_TRUE &&
        TrafficLightTimer.TimerGreenLightStatus == HI_FALSE &&
        TrafficLightTimer.Timer1Status == HI_FALSE &&
        TrafficLightTimer.Timer2Status == HI_FALSE) {
        yellowHumanCount--;
        TrafficLightTimer.Timer3Count = yellowHumanCount;
        printf("yellowHumanCount = %d\r\n", TrafficLightTimer.Timer3Count);
    }
}
/* auto mode green time count */
void Timer4Callback(unsigned int arg)
{
    if (timer4Count == LAMP_FLASHING_TIME_RESET_0) {
        timer4Count = AUTO_MODULE_LIGHT_ON_TIME;
        TrafficLightTimer.Timer4Count = AUTO_MODULE_LIGHT_ON_TIME;
        return;
    }
    if (timer9Count == LAMP_FLASHING_TIME_RESET_0) {
        timer9Count = AUTO_MODULE_LIGHT_ON_TIME;
        TrafficLightTimer.Timer9Count = AUTO_MODULE_LIGHT_ON_TIME;
        return;
    }
    if (humanGreenTimerCount == LAMP_FLASHING_TIME_RESET_0) {
        humanGreenTimerCount = AUTO_MODULE_LIGHT_ON_TIME;
        TrafficLightTimer.Timer9Count = AUTO_MODULE_LIGHT_ON_TIME;
        return;
    }
    if ((TrafficLightTimer.TimerGreenLightStatus == HI_TRUE) &&
        (TrafficLightTimer.TimerYellowLightStatus == HI_FALSE) &&
        (TrafficLightTimer.Timer3Status == HI_FALSE) &&
        (TrafficLightTimer.Timer1Status == HI_FALSE) &&
        (TrafficLightTimer.Timer2Status == HI_FALSE)) {
        if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_AUTO_MODE) {
            timer4Count--;
            TrafficLightTimer.Timer4Count = timer4Count;
            trafficStaType.g_greenLedAutoModuTimeCount = timer4Count;
            printf("Timer4Count = %d\r\n", TrafficLightTimer.Timer4Count);
        }
    }
    if (TrafficLightTimer.HumanTimerGreenLightStatus == HI_TRUE &&
        TrafficLightTimer.HumanTimerYellowLightStatus == HI_FALSE &&
        TrafficLightTimer.Timer10Status == HI_FALSE) {
        if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE) {
            timer9Count--;
            TrafficLightTimer.Timer9Count = timer9Count;
            trafficStaType.g_greenLedHumanModuTimeCount = timer9Count;
            printf("Timer9Count = %d\r\n", TrafficLightTimer.Timer9Count);
        }
    }
    /* 人工干预模式 */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE &&
        GetKeyStatus(CURRENT_TYPE) == TRAFFIC_HUMAN_TYPE &&
        (TrafficLightTimer.TimerGreenLightStatus == HI_TRUE) &&
        (TrafficLightTimer.TimerYellowLightStatus == HI_FALSE) &&
        (TrafficLightTimer.Timer3Status == HI_FALSE) &&
        (TrafficLightTimer.Timer1Status == HI_FALSE) &&
        (TrafficLightTimer.Timer2Status == HI_FALSE)) {
        humanGreenTimerCount--;
        TrafficLightTimer.Timer4Count = humanGreenTimerCount;
        printf("humanGreenTimerCount = %d\r\n", TrafficLightTimer.Timer4Count);
    }
}
/* auto mode green flash time count */
void Timer5Callback(unsigned int arg)
{
    if (timer5Count == LAMP_FLASHING_TIME_RESET_0) {
        timer5Count = LED_FLASHING_TIME;
        TrafficLightTimer.Timer5Count = LED_FLASHING_TIME;
        return;
    }
    if (timer10Count == LAMP_FLASHING_TIME_RESET_0) {
        timer10Count = LED_FLASHING_TIME;
        TrafficLightTimer.Timer10Count = LED_FLASHING_TIME;
        return;
    }
    if (humanGreenFlashCount == LAMP_FLASHING_TIME_RESET_0) {
        humanGreenFlashCount = LED_FLASHING_TIME;
        TrafficLightTimer.Timer5Count = LED_FLASHING_TIME;
        return;
    }
    if ((TrafficLightTimer.TimerGreenLightStatus == HI_TRUE) &&
        (TrafficLightTimer.TimerYellowLightStatus == HI_FALSE) &&
        (TrafficLightTimer.Timer3Status == HI_TRUE) &&
        (TrafficLightTimer.Timer1Status == HI_FALSE) &&
        (TrafficLightTimer.Timer2Status == HI_FALSE)) {
        if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_AUTO_MODE) {
            timer5Count--;
            TrafficLightTimer.Timer5Count = timer5Count;
            trafficStaType.g_greenLedAutoModuTimeCount = timer5Count;
            printf("Timer5Count = %d\r\n", TrafficLightTimer.Timer5Count);
        }
    }
    /* human mode */
    if (TrafficLightTimer.Timer10Status == HI_TRUE) {
        if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE) {
            timer10Count--;
            TrafficLightTimer.Timer10Count = timer10Count;
            printf("Timer10Count = %d\r\n", TrafficLightTimer.Timer10Count);
        }
    }
    /* 人工干预模式 */
    if (GetKeyStatus(CURRENT_MODE) == TRAFFIC_HUMAN_MODE &&
        GetKeyStatus(CURRENT_TYPE) == TRAFFIC_HUMAN_TYPE &&
        (TrafficLightTimer.TimerGreenLightStatus == HI_TRUE) &&
        (TrafficLightTimer.TimerYellowLightStatus == HI_FALSE) &&
        (TrafficLightTimer.Timer3Status == HI_TRUE) &&
        (TrafficLightTimer.Timer1Status == HI_FALSE) &&
        (TrafficLightTimer.Timer2Status == HI_FALSE)) {
        humanGreenFlashCount--;
        TrafficLightTimer.Timer5Count = humanGreenFlashCount;
        printf("humanGreenFlashCount = %d\r\n", TrafficLightTimer.Timer5Count);
    }
}

#define CN_QUEUE_MSGNUM 16
#define CN_QUEUE_MSGSIZE (sizeof(hi_pvoid))

void SoftwareTimersTaskEntry(void)
{
    unsigned int timerId1;
    unsigned int timerId2;
    unsigned int timerId3;
    unsigned int timerId4;
    unsigned int timerId5;
    unsigned int ret;

    /* 创建周期性软件定时器，时间为1000ms,启动�?1000ms数时执行回调函数1 */
    ret = hi_timer_create(&timerId1);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to create timer 1\r\n");
    }
    /* 创建周期性软件定时器，每1000ms数执行回调函�?2 */
    ret = hi_timer_create(&timerId2);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to create timer 2\r\n");
    }
    /* 创建周期性软件定时器，每1000ms数执行回调函�?3 */
    ret = hi_timer_create(&timerId3);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to create timer 3\r\n");
    }
    /* 创建周期性软件定时器，每1000ms数执行回调函�?4 */
    ret = hi_timer_create(&timerId4);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to create timer 4\r\n");
    }
    /* 创建周期性软件定时器，每1000ms数执行回调函�?5 */
    ret = hi_timer_create(&timerId5);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to create timer 5\r\n");
    }

    ret = hi_timer_start(timerId1, HI_TIMER_TYPE_PERIOD, 1000, Timer1Callback, 0); /* 1000: timer 1000ms */
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to start timer 1\r\n");
    }

    ret = hi_timer_start(timerId2, HI_TIMER_TYPE_PERIOD, 1000, Timer2Callback, 0); /* 1000: timer 1000ms */
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to start timer 2\r\n");
    }

    ret = hi_timer_start(timerId3, HI_TIMER_TYPE_PERIOD, 1000, Timer3Callback, 0); /* 1000: timer 1000ms */
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to start timer 2\r\n");
    }

    ret = hi_timer_start(timerId4, HI_TIMER_TYPE_PERIOD, 1000, Timer4Callback, 0); /* 1000: timer 1000ms */
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to start timer 2\r\n");
    }

    ret = hi_timer_start(timerId5, HI_TIMER_TYPE_PERIOD, 1000, Timer5Callback, 0); /* 1000: timer 1000ms */
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to start timer 2\r\n");
    }
}

#define TIMER_TASK_STACK  (1024 * 4)

static void TimerTask(void)
{
    osThreadAttr_t attr;
    IoTWatchDogDisable();

    attr.name = "TrafficLightDemo";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = TIMER_TASK_STACK;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)SoftwareTimersTaskEntry, NULL, &attr) == NULL) {
        printf("[TrafficLight] Falied to create SoftwareTimersTaskEntry!\n");
    }
}

SYS_RUN(TimerTask);
