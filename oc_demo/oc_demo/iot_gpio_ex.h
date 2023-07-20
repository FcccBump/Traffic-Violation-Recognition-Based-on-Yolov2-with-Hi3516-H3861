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

/**
 * @file iot_gpio_ex.h
 *
 * @brief Declares the extended GPIO interface functions.
 *
 * These functions are used for settings GPIO pulls and driver strength. \n
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef IOT_GPIO_EX_H
#define IOT_GPIO_EX_H

/**
 * @brief Enumerates GPIO pull-up or pull-down settings.
 */
typedef enum {
    /** No pull */
    IOT_IO_PULL_NONE,
    /** Pull-up */
    IOT_IO_PULL_UP,
    /** Pull-down */
    IOT_IO_PULL_DOWN,
    /** Maximum value */
    IOT_IO_PULL_MAX,
} IotIoPull;

/**
 * @ingroup iot_io
 *
 * GPIO pin ID. CNcomment:IO硬件管脚编号。CNend
 */
typedef enum {
    IOT_IO_NAME_GPIO_0, /* <GPIO0 */
    IOT_IO_NAME_GPIO_1, /* <GPIO1 */
    IOT_IO_NAME_GPIO_2, /* <GPIO2 */
    IOT_IO_NAME_GPIO_3, /* <GPIO3 */
    IOT_IO_NAME_GPIO_4, /* <GPIO4 */
    IOT_IO_NAME_GPIO_5, /* <GPIO5 */
    IOT_IO_NAME_GPIO_6, /* <GPIO6 */
    IOT_IO_NAME_GPIO_7, /* <GPIO7 */
    IOT_IO_NAME_GPIO_8, /* <GPIO8 */
    IOT_IO_NAME_GPIO_9, /* <GPIO9 */
    IOT_IO_NAME_GPIO_10, /* <GPIO10 */
    IOT_IO_NAME_GPIO_11, /* <GPIO11 */
    IOT_IO_NAME_GPIO_12, /* <GPIO12 */
    IOT_IO_NAME_GPIO_13, /* <GPIO13 */
    IOT_IO_NAME_GPIO_14, /* <GPIO14 */
    IOT_IO_NAME_SFC_CSN, /* <SFC_CSN */
    IOT_IO_NAME_SFC_IO1, /* <SFC_IO1 */
    IOT_IO_NAME_SFC_IO2, /* <SFC_IO2 */
    IOT_IO_NAME_SFC_IO0, /* <SFC_IO0 */
    IOT_IO_NAME_SFC_CLK, /* <SFC_CLK */
    IOT_IO_NAME_SFC_IO3, /* <SFC_IO3 */
    IOT_IO_NAME_MAX,
} IotIoName;

/**
 * @ingroup iot_io
 *
 * GPIO_0 pin function.CNcomment:GPIO_0管脚功能。CNend
 */
typedef enum {
    IOT_IO_FUNC_GPIO_0_GPIO,
    IOT_IO_FUNC_GPIO_0_UART1_TXD = 2,
    IOT_IO_FUNC_GPIO_0_SPI1_CK,
    IOT_IO_FUNC_GPIO_0_JTAG_TDO,
    IOT_IO_FUNC_GPIO_0_PWM3_OUT,
    IOT_IO_FUNC_GPIO_0_I2C1_SDA,
} IotIoFuncGpio0;

/**
* @ingroup iot_io
*
* GPIO_1 pin function.CNcomment:GPIO_1管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_GPIO_1_GPIO,
    IOT_IO_FUNC_GPIO_1_UART1_RXD = 2,
    IOT_IO_FUNC_GPIO_1_SPI1_RXD,
    IOT_IO_FUNC_GPIO_1_JTAG_TCK,
    IOT_IO_FUNC_GPIO_1_PWM4_OUT,
    IOT_IO_FUNC_GPIO_1_I2C1_SCL,
    IOT_IO_FUNC_GPIO_1_BT_FREQ,
} IotIoFuncGpio1;

/**
* @ingroup iot_io
*
* GPIO_2 pin function.CNcomment:GPIO_2管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_GPIO_2_GPIO,
    IOT_IO_FUNC_GPIO_2_UART1_RTS_N = 2,
    IOT_IO_FUNC_GPIO_2_SPI1_TXD,
    IOT_IO_FUNC_GPIO_2_JTAG_TRSTN,
    IOT_IO_FUNC_GPIO_2_PWM2_OUT,
    IOT_IO_FUNC_GPIO_2_SSI_CLK = 7,
} IotIoFuncGpio2;

/**
* @ingroup iot_io
*
* GPIO_3 pin function.CNcomment:GPIO_3管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_GPIO_3_GPIO,
    IOT_IO_FUNC_GPIO_3_UART0_TXD,
    IOT_IO_FUNC_GPIO_3_UART1_CTS_N,
    IOT_IO_FUNC_GPIO_3_SPI1_CSN,
    IOT_IO_FUNC_GPIO_3_JTAG_TDI,
    IOT_IO_FUNC_GPIO_3_PWM5_OUT,
    IOT_IO_FUNC_GPIO_3_I2C1_SDA,
    IOT_IO_FUNC_GPIO_3_SSI_DATA,
} IotIoFuncGpio3;

/**
* @ingroup iot_io
*
* GPIO_4 pin function.CNcomment:GPIO_4管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_GPIO_4_GPIO,
    IOT_IO_FUNC_GPIO_4_UART0_RXD = 2,
    IOT_IO_FUNC_GPIO_4_JTAG_TMS = 4,
    IOT_IO_FUNC_GPIO_4_PWM1_OUT,
    IOT_IO_FUNC_GPIO_4_I2C1_SCL,
} IotIoFuncGpio4;

/**
* @ingroup iot_io
*
* GPIO_5 pin function.CNcomment:GPIO_5管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_GPIO_5_GPIO,
    IOT_IO_FUNC_GPIO_5_UART1_RXD = 2,
    IOT_IO_FUNC_GPIO_5_SPI0_CSN,
    IOT_IO_FUNC_GPIO_5_PWM2_OUT = 5,
    IOT_IO_FUNC_GPIO_5_I2S0_MCLK,
    IOT_IO_FUNC_GPIO_5_BT_STATUS,
} IotIoFuncGpio5;

/**
* @ingroup iot_io
*
* GPIO_6 pin function.CNcomment:GPIO_6管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_GPIO_6_GPIO,
    IOT_IO_FUNC_GPIO_6_UART1_TXD = 2,
    IOT_IO_FUNC_GPIO_6_SPI0_CK,
    IOT_IO_FUNC_GPIO_6_PWM3_OUT = 5,
    IOT_IO_FUNC_GPIO_6_I2S0_TX,
    IOT_IO_FUNC_GPIO_6_COEX_SWITCH,
} IotIoFuncGpio6;

/**
* @ingroup iot_io
*
* GPIO_7 pin function.CNcomment:GPIO_7管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_GPIO_7_GPIO,
    IOT_IO_FUNC_GPIO_7_UART1_CTS_N = 2,
    IOT_IO_FUNC_GPIO_7_SPI0_RXD,
    IOT_IO_FUNC_GPIO_7_PWM0_OUT = 5,
    IOT_IO_FUNC_GPIO_7_I2S0_BCLK,
    IOT_IO_FUNC_GPIO_7_BT_ACTIVE,
} IotIoFuncGpio7;

/**
* @ingroup iot_io
*
* GPIO_8 pin function.CNcomment:GPIO_8管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_GPIO_8_GPIO,
    IOT_IO_FUNC_GPIO_8_UART1_RTS_N = 2,
    IOT_IO_FUNC_GPIO_8_SPI0_TXD,
    IOT_IO_FUNC_GPIO_8_PWM1_OUT = 5,
    IOT_IO_FUNC_GPIO_8_I2S0_WS,
    IOT_IO_FUNC_GPIO_8_WLAN_ACTIVE,
} IotIoFuncGpio8;

/**
* @ingroup iot_io
*
* GPIO_9 pin function.CNcomment:GPIO_9管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_GPIO_9_GPIO,
    IOT_IO_FUNC_GPIO_9_I2C0_SCL,
    IOT_IO_FUNC_GPIO_9_UART2_RTS_N,
    IOT_IO_FUNC_GPIO_9_SDIO_D2,
    IOT_IO_FUNC_GPIO_9_SPI0_TXD,
    IOT_IO_FUNC_GPIO_9_PWM0_OUT,
    IOT_IO_FUNC_GPIO_9_I2S0_MCLK = 7,
} IotIoFuncGpio9;

/**
* @ingroup iot_io
*
* GPIO_10 pin function.CNcomment:GPIO_10管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_GPIO_10_GPIO,
    IOT_IO_FUNC_GPIO_10_I2C0_SDA,
    IOT_IO_FUNC_GPIO_10_UART2_CTS_N,
    IOT_IO_FUNC_GPIO_10_SDIO_D3,
    IOT_IO_FUNC_GPIO_10_SPI0_CK,
    IOT_IO_FUNC_GPIO_10_PWM1_OUT,
    IOT_IO_FUNC_GPIO_10_I2S0_TX = 7,
} IotIoFuncGpio10;

/**
* @ingroup iot_io
*
* GPIO_11 pin function.CNcomment:GPIO_11管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_GPIO_11_GPIO,
    IOT_IO_FUNC_GPIO_11_UART2_TXD = 2,
    IOT_IO_FUNC_GPIO_11_SDIO_CMD,
    IOT_IO_FUNC_GPIO_11_SPI0_RXD,
    IOT_IO_FUNC_GPIO_11_PWM2_OUT,
    IOT_IO_FUNC_GPIO_11_RF_TX_EN_EXT,
    IOT_IO_FUNC_GPIO_11_I2S0_RX,
} IotIoFuncGpio11;

/**
* @ingroup iot_io
*
* GPIO_12 pin function.CNcomment:GPIO_12管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_GPIO_12_GPIO,
    IOT_IO_FUNC_GPIO_12_UART2_RXD = 2,
    IOT_IO_FUNC_GPIO_12_SDIO_CLK,
    IOT_IO_FUNC_GPIO_12_SPI0_CSN,
    IOT_IO_FUNC_GPIO_12_PWM3_OUT,
    IOT_IO_FUNC_GPIO_12_RF_RX_EN_EXT,
    IOT_IO_FUNC_GPIO_12_I2S0_BCLK,
} IotIoFuncGpio12;

/**
* @ingroup iot_io
*
* GPIO_13 pin function.CNcomment:GPIO_13管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_GPIO_13_SSI_DATA,
    IOT_IO_FUNC_GPIO_13_UART0_TXD,
    IOT_IO_FUNC_GPIO_13_UART2_RTS_N,
    IOT_IO_FUNC_GPIO_13_SDIO_D0,
    IOT_IO_FUNC_GPIO_13_GPIO,
    IOT_IO_FUNC_GPIO_13_PWM4_OUT,
    IOT_IO_FUNC_GPIO_13_I2C0_SDA,
    IOT_IO_FUNC_GPIO_13_I2S0_WS,
} IotIoFuncGpio13;

/**
* @ingroup iot_io
*
* GPIO_14 pin function.CNcomment:GPIO_14管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_GPIO_14_SSI_CLK,
    IOT_IO_FUNC_GPIO_14_UART0_RXD,
    IOT_IO_FUNC_GPIO_14_UART2_CTS_N,
    IOT_IO_FUNC_GPIO_14_SDIO_D1,
    IOT_IO_FUNC_GPIO_14_GPIO,
    IOT_IO_FUNC_GPIO_14_PWM5_OUT,
    IOT_IO_FUNC_GPIO_14_I2C0_SCL,
} IotIoFuncGpio14;

/**
* @ingroup iot_io
*
* SFC_CSN pin function.CNcomment:SFC_CSN管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_SFC_CSN_SFC_CSN,
    IOT_IO_FUNC_SFC_CSN_SDIO_D2,
    IOT_IO_FUNC_SFC_CSN_GPIO9,
    IOT_IO_FUNC_SFC_CSN_SPI0_TXD = 4,
} IotIoFuncSfcCsn;

/**
* @ingroup iot_io
*
* SFC_DO pin function.CNcomment:SFC_DO管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_SFC_IO_1_SFC_DO,
    IOT_IO_FUNC_SFC_IO_1_SDIO_D3,
    IOT_IO_FUNC_SFC_IO_1_GPIO10,
    IOT_IO_FUNC_SFC_IO_1_SPI0_CK = 4,
} IotIoFuncSfcIo1;

/**
* @ingroup iot_io
*
* SFC_WPN pin function.CNcomment:SFC_WPN管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_SFC_IO_2_SFC_WPN,
    IOT_IO_FUNC_SFC_IO_2_SDIO_CMD,
    IOT_IO_FUNC_SFC_IO_2_GPIO11,
    IOT_IO_FUNC_SFC_IO_2_RF_TX_EN_EXT,
    IOT_IO_FUNC_SFC_IO_2_SPI0_RXD,
} IotIoFuncSfcIo2;

/**
* @ingroup iot_io
*
* SFC_DI pin function.CNcomment:SFC_DI管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_SFC_IO_0_SFC_DI,
    IOT_IO_FUNC_SFC_IO_0_SDIO_CLK,
    IOT_IO_FUNC_SFC_IO_0_GPIO12,
    IOT_IO_FUNC_SFC_IO_0_RF_RX_EN_EXT,
    IOT_IO_FUNC_SFC_IO_0_SPI0_CSN,
} IotIoFuncSfcIo0;

/**
* @ingroup iot_io
*
* SFC_CLK pin function.CNcomment:SFC_CLK管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_SFC_CLK_SFC_CLK,
    IOT_IO_FUNC_SFC_CLK_SDIO_D0,
    IOT_IO_FUNC_SFC_CLK_GPIO13,
    IOT_IO_FUNC_SFC_CLK_SSI_DATA = 4,
} IotIoFuncSfcClk;

/**
* @ingroup iot_io
*
* SFC_HOLDN pin function.CNcomment:SFC_HOLDN管脚功能。CNend
*/
typedef enum {
    IOT_IO_FUNC_SFC_IO_3_SFC_HOLDN,
    IOT_IO_FUNC_SFC_IO_3_SDIO_D1,
    IOT_IO_FUNC_SFC_IO_3_GPIO14,
    IOT_IO_FUNC_SFC_IO_3_SSI_CLK = 4,
} IotIoFuncSfcIo3;

/**
* @ingroup iot_io
*
* I/O drive capability.CNcomment:IO驱动能力。CNend
* Note: The HI_IO_NAME_GPIO_0~HI_IO_NAME_GPIO_11 and HI_IO_NAME_GPIO_13~HI_IO_NAME_GPIO_14 driver capabilities are
* optional.The value range is HI_IO_DRIVER_STRENGTH_0~HI_IO_DRIVER_STRENGTH_3, and the other I/O ranges are
* HI_IO_DRIVER_STRENGTH_0~HI_IO_DRIVER_STRENGTH_7.CNcomment:注意:HI_IO_NAME_GPIO_0~HI_IO_NAME_GPIO_11、
* HI_IO_NAME_GPIO_13~HI_IO_NAME_GPIO_14驱动能力可选范围是HI_IO_DRIVER_STRENGTH_0~HI_IO_DRIVER_STRENGTH_3，
* 其余IO范围是HI_IO_DRIVER_STRENGTH_0~HI_IO_DRIVER_STRENGTH_7。CNend
*/
typedef enum {
    IOT_IO_DRIVER_STRENGTH_0 = 0,   /* <Drive strength level 0 (highest).CNcomment:驱动能力0级，驱动能力最高CNend */
    IOT_IO_DRIVER_STRENGTH_1,   /* <Drive strength level 1.CNcomment:驱动能力1级CNend */
    IOT_IO_DRIVER_STRENGTH_2,   /* <Drive strength level 2.CNcomment:驱动能力2级CNend */
    IOT_IO_DRIVER_STRENGTH_3,   /* <Drive strength level 3.CNcomment:驱动能力3级CNend */
    IOT_IO_DRIVER_STRENGTH_4,   /* <Drive strength level 4.CNcomment:驱动能力4级CNend */
    IOT_IO_DRIVER_STRENGTH_5,   /* <Drive strength level 5.CNcomment:驱动能力5级CNend */
    IOT_IO_DRIVER_STRENGTH_6,   /* <Drive strength level 6.CNcomment:驱动能力6级CNend */
    IOT_IO_DRIVER_STRENGTH_7,   /* <Drive strength level 7 (lowest).CNcomment:驱动能力7级，驱动能力最低CNend */
    IOT_IO_DRIVER_STRENGTH_MAX,
} IotIoDriverStrength;


unsigned int IoSetPull(unsigned int id, IotIoPull val);


unsigned int IoSetFunc(unsigned int id, unsigned char val);

unsigned int TaskMsleep(unsigned int ms);

#endif