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

/*
GPIO ，按键，串口收发数据等

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>
#include "hisignalling.h"

#define MSG(args...) printf(args)
pthread_t hisignallingMsgHandleID;
#define HISIGNALLING_HEAD_1 (0xaa)
#define HISIGNALLING_HEAD_2 (0x55)

/*
 * @berf 用于将指定编号的引脚导出，作为GPIO使用
 * @param pin:指定导出的引脚
 *
 * @berf It is used to export the pin with the specified number and use it as GPIO
 * @param pin: Specify the exported pin
 */
static int GpioExport(int pin)
{
    char buffer[64] = {0};
    int len = -1;
    int fd = -1;

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        MSG("Failed to open export for writing!\n");
        close(fd);
        return -1;
    }

    len = snprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, "%d", pin);
    if (len < 0) {
        MSG("printf msg failed\r\n");
    }
    if (write(fd, buffer, len) < 0) {
        MSG("Failed to export gpio!");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/*
 * @berf 用于将导出的GPIO删除掉
 * @param pin:指定删除的引脚
 *
 * @berf Used to delete the exported GPIO
 * @param pin: Specifies the pin to delete
 */
static int GpioUnexport(int pin)
{
    char buffer[64] = {0};
    int len = -1;
    int fd = -1;

    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0) {
        MSG("Failed to open unexport for writing!\n");
        close(fd);
        return -1;
    }

    len = snprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, "%d", pin);
    if (len < 0) {
        MSG("printf msg failed\r\n");
    }
    if (write(fd, buffer, len) < 0) {
        MSG("Failed to unexport gpio!");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/*
 * direction设置输出还是输入模式，0-->IN为输入，1-->OUT为输出
 * direction sets the output or input mode, 0-->IN is input, 1-->OUT is output
 */
static int GpioDirection(int pin, int dir)
{
    static const char dirStr[] = "in\0out";
    char path[64] = {0};
    int fd = -1;

    int len = snprintf_s(path, sizeof(path), sizeof(path) - 1, "/sys/class/gpio/gpio%d/direction", pin);
    if (len < 0) {
        MSG("printf msg failed\r\n");
    }
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        MSG("Failed to open gpio direction for writing!\n");
        close(fd);
        return -1;
    }

    if (write(fd, &dirStr[dir == 0 ? 0 : 3], dir == 0 ? 2 : 3) < 0) { /* 3, 2, 3 gpio register */
        MSG("Failed to set direction!\n");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/*
 * @brief 设置输出值
 * @param pin: 引脚
 * @param value: 输出值
 *
 * @brief set output value
 * @param pin: pin
 * @param value: output value
 */
static int GpioWrite(int pin, int value)
{
    static const char valuesStr[] = "01";
    char path[64] = {0};
    int fd = -1;

    int len = snprintf_s(path, sizeof(path), sizeof(path) - 1, "/sys/class/gpio/gpio%d/value", pin);
    if (len < 0) {
        MSG("printf Msg failed\r\n");
    }
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        MSG("Failed to open gpio value for writing!\n");
        close(fd);
        return -1;
    }

    if (write(fd, &valuesStr[value == 0 ? 0 : 1], 1) < 0) { /* 1, 1 gpio register */
        MSG("Failed to write value!\n");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/*
 * @brief 读取输入值
 * @param pin: 引脚
 *
 * @brief reads the input value
 * @param pin: pin
 */
static int GpioRead(int pin)
{
    char path[64] = {0};
    char value_str[3] = {0};
    int fd = -1;

    int len = snprintf_s(path, sizeof(path), sizeof(path) - 1, "/sys/class/gpio/gpio%d/value", pin);
    if (len < 0) {
        MSG("printf msg failed\r\n");
    }
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        MSG("Failed to open gpio value for reading!\n");
        close(fd);
        return -1;
    }

    if (read(fd, value_str, 3) < 0) { /* 3: gpio register */
        MSG("Failed to read value!\n");
        close(fd);
        return -1;
    }

    close(fd);
    return (atoi(value_str));
}

/*
 * none表示引脚为输入，不是中断引脚
 * rising表示引脚为中断输入，上升沿触发
 * falling表示引脚为中断输入，下降沿触发
 * both表示引脚为中断输入，边沿触发
 *
 * none indicates that the pin is an input, not an interrupt pin
 * rising indicates that the pin is an interrupt input and is triggered by a rising edge
 * falling means that the pin is an interrupt input, triggered by a falling edge
 * both indicates that the pin is an interrupt input, edge-triggered
 * 0-->none, 1-->rising, 2-->falling, 3-->both
 */
static int GpioEdge(int pin, int edge)
{
    const char dirStr[] = "none\0rising\0falling\0both";
    char ptr = 0;
    char path[64] = {0};
    int fd = -1;

    switch (edge) {
        case EAGE_0:
            ptr = 0;
            break;
        case EAGE_1:
            ptr = 5; /* 5: gpio register */
            break;
        case EAGE_2:
            ptr = 12; /* 12: gpio register */
            break;
        case EAGE_3:
            ptr = 20; /* 20: gpio register */
            break;
        default:
            ptr = 0;
    }
    int len = snprintf_s(path, sizeof(path), sizeof(path) - 1, "/sys/class/gpio/gpio%d/edge", pin);
    if (len < 0) {
        MSG("printf Msg failed\r\n");
    }

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        MSG("Failed to open gpio edge for writing!\n");
        close(fd);
        return -1;
    }

    if (write(fd, &dirStr[ptr], strlen(&dirStr[ptr])) < 0) {
        MSG("Failed to set edge!\n");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/*
 * 初始化按键1
 * Initialize button 1
 */
void InitGpio1(void)
{
    MSG("\n =============== InitGpio1 start ========== \n");
    GpioUnexport(1); /* 1: gpio pin */
    GpioExport(1); /* 1: gpio pin */
    GpioDirection(1, 0); /* 1, 0: gpio pin */
    GpioEdge(1, 2); /* 1, 2: gpio pin */
    MSG("\n =============== InitGpio1 end ========== \n");
}

/*
 * 初始化按键2
 * Initialize button 2
 */
void InitGpio2(void)
{
    MSG("\n =============== InitGpio2 start ========== \n");
    GpioUnexport(2); /* 2: gpio pin */
    GpioExport(2); /* 2: gpio pin */
    GpioDirection(2, 0); /* 2, 0: gpio pin */
    GpioEdge(2, 2); /* 2, 2: gpio pin */
    MSG("\n =============== InitGpio2 end ========== \n");
}

/*
 * 串口设置
 * Serial port settings
 */
int Uart1Config(int fd)
{
    struct termios newtio = {0}, oldtio = {0};
    /*
     * 获取原有串口配置
     * Get the original serial port configuration
     */
    if (tcgetattr(fd, &oldtio) != 0) {
        perror("SetupSerial 1");
        return -1;
    }
    (void)memset_s(&newtio, sizeof(newtio), 0, sizeof(newtio));
    /*
     * CREAD开启串行数据接收，CLOCAL打开本地连接模式
     * CREAD opens serial data reception, CLOCAL opens local connection mode
     */
    newtio.c_cflag  |=  CLOCAL | CREAD;

    /*
     * 设置数据位
     * set data bit
     */
    newtio.c_cflag &= ~CSIZE;
    newtio.c_cflag |= CS8;
    /*
     * 设置奇偶校验位
     * set parity bit
     */
    newtio.c_cflag &= ~PARENB; // 无奇偶校验
    /*
     * 设置波特率 115200
     * set baud rate 115200
     */
    cfsetispeed(&newtio, B115200);
    cfsetospeed(&newtio, B115200);

    /*
     * 设置停止位
     * set stop bit
     */
    newtio.c_cflag &=  ~CSTOPB; /* 默认为一位停止位 */
    /*
     * 设置最少字符和等待时间，对于接收字符和等待时间没有特别的要求时
     *
     * Set the minimum characters and waiting time,
     * when there are no special requirements for receiving characters and waiting time
     */
    newtio.c_cc[VTIME]  = 0; /* 非规范模式读取时的超时时间 */
    newtio.c_cc[VMIN] = 0; /* 非规范模式读取时的最小字符数 */
    /*
     * tcflush清空终端未完成的输入/输出请求及数据；TCIFLUSH表示清空正收到的数据，且不读取出来
     *
     * tcflush clears the unfinished input/output requests and data of the terminal;
     * TCIFLUSH means clearing the data being received and not reading it out
     */
    tcflush(fd, TCIFLUSH);
    if ((tcsetattr(fd, TCSANOW, &newtio)) != 0) {
        perror("com set error");
        return -1;
    }
    return 0;
}

/*
 * @berf uart 发送数据
 * @param int fd: 串口文件描述符
 * @param void *buf:发送数据buffer
 * @param int len:数据缓冲长度
 *
 * @berf uart send
 * @param int fd: uart file descriptor
 * @param void *buf:send data buf
 * @param int len:data buf len
 */
int UartSend(int fd, char *buf, int len)
{
    int ret = 0;
    int count = 0;
    char *sendBuf = buf;
    int sendLen = len;

    tcflush(fd, TCIFLUSH);

    while (sendLen > 0) {
        ret = write(fd, (char*)sendBuf + count, sendLen);
        if (ret < 1) {
            printf("write data below 1 byte % d\r\n", ret);
            break;
        }
        count += ret;
        sendLen = sendLen - ret;
    }

    return count;
}

/*
 * @berf uart 读取数据
 * @param int uart_fd: 串口文件描述符
 * @param void *buf: 读取数据buffer
 * @param int len: 数据缓冲区长度
 * @param int timeoutMs: 读取数据时间
 *
 * @berf uart read
 * @param int uart_fd: uart file descriptor
 * @param void *buf: read data buf
 * @param int len: data buf len
 * @param int timeoutMs: read data time
 */
int UartRead(int uartFd, char *buf, int len, int timeoutMs)
{
    int ret = 0;
    size_t  rsum = 0;
    ret = 0;
    fd_set rset;
    struct timeval time;
    int timeout = timeoutMs;
    char *readBuf = buf;
    int readLen = len;

    while (rsum < readLen) {
        time.tv_sec = timeout / 1000; /* 1000:转换成秒 */
        time.tv_usec = (timeout - time.tv_sec * 1000) * 1000; /* 1000, 1000:转换为微秒 */
        FD_ZERO(&rset);
        FD_SET(uartFd, &rset);
        ret = select(uartFd + 1, &rset, NULL, NULL, &time);
        if (ret <= 0) {
            if (ret == 0) {
                printf("time over!\r\n");
                return -1;
            }
            if (ret == -1) {
                printf("select error!\r\n");
                continue; // 信号中断
            }
            return -1;
        } else {
            ret = read(uartFd, (char *)readBuf + rsum, readLen - rsum);
            if (ret < 0) {
                printf("read data failed\r\n");
                return ret;
            } else {
                rsum += ret;
            }
        }
    }
    return rsum;
}

/*
 * crc32校验
 * crc32 Verification implementation
 */
static const unsigned int crc32table[] = {
};

/*
 * @berf CRC校验
 * @param const unsigned char *buf: 待校验数据buff
 * @param unsigned int size: 待校验数据长度
 *
 * @berf CRC check
 * @param const unsigned char *buf: Data to be verified buff
 * @param unsigned int size: Data to be verified length
 */
static unsigned int crc32(const unsigned char *buf, unsigned int size)
{
    unsigned int  i, crc = 0xFFFFFFFF;

    for (i = 0; i < size; i++) {
        crc = crc32table[(crc ^ buf[i]) & 0xff] ^ (crc >> 8); /* 8: 右移8bit */
    }
    return crc ^ 0xFFFFFFFF;
}

#define RIGHT_MOVE_8_BIT (8)
#define RIGHT_MOVE_16_BIT (16)
#define RIGHT_MOVE_24_BIT (24)

/*
 * 对数据进行打包
 * Pack the data
 */
static unsigned int HisignallingDataPackage(HisignallingProtocalType *buf,
    unsigned int len, unsigned char *hisignallingDataBuf)
{
    unsigned int crcCheckSend = 0x1010;
    unsigned int packageLen = 0;
    unsigned int DataPackLen = len;

    (void)memcpy_s(hisignallingDataBuf, HISGNALLING_MSG_FRAME_HEADER_LEN,
        buf->frameHeader, HISGNALLING_MSG_FRAME_HEADER_LEN);
    (void)memcpy_s(&hisignallingDataBuf[HISGNALLING_MSG_FRAME_HEADER_LEN],
        DataPackLen, buf->hisignallingMsgBuf, DataPackLen);
    (void)memcpy_s(&hisignallingDataBuf[HISGNALLING_MSG_FRAME_HEADER_LEN + DataPackLen],
        HISIGNALLING_MSG_HEADER_LEN, &(buf->endOfFrame), HISIGNALLING_MSG_HEADER_LEN);
    crcCheckSend = crc32(hisignallingDataBuf, (DataPackLen + HISIGNALLING_MSG_HEADER_TAIL_LEN));
    hisignallingDataBuf[DataPackLen + HISIGNALLING_MSG_HEADER_TAIL_LEN] =
        (unsigned char)((crcCheckSend & 0xff000000) >> RIGHT_MOVE_24_BIT);
    hisignallingDataBuf[DataPackLen + HISIGNALLING_MSG_HEADER_TAIL_LEN + 1] = /* 1: addr offset */
        (unsigned char)((crcCheckSend & 0x00ff0000) >> RIGHT_MOVE_16_BIT);
    hisignallingDataBuf[DataPackLen + HISIGNALLING_MSG_HEADER_TAIL_LEN + 2] = /* 2: addr offset */
        (unsigned char)((crcCheckSend & 0x0000ff00) >> RIGHT_MOVE_8_BIT);
    hisignallingDataBuf[DataPackLen + HISIGNALLING_MSG_HEADER_TAIL_LEN + 3] = /* 3: addr offset */
        (unsigned char)crcCheckSend;
    packageLen = DataPackLen + HISIGNALLING_MSG_HEADER_TAIL_LEN + 4; /* 4: crc check lenght */
    return packageLen;
}

/*
 * Hi3561DV300接收数据
 * Hi3561DV300 message recevice
 */
static HisignallingErrorType HisignallingMsgReceive(int fd, unsigned char *buf, unsigned int len)
{
    unsigned int crcCheckReceived = 0;
    int i = 0, readLen = 0;
    unsigned int RecvLen = len;

    /*
     * Hi3516dv300 uart串口读数据
     * Hi3516dv300 uart read data
     */
    readLen = UartRead(fd, buf, RecvLen, 1000); /* 1000 :time out */
    if (readLen <= 0) {
        printf("uart_read data failed\r\n");
        return HISIGNALLING_RET_VAL_ERROR;
    }
    printf("read_len=%d\r\n", readLen);
    /*
     * 输出收到的数据
     * output received data
     */
    for (i = 0; i < RecvLen; i++) {
        printf("0x%x ", buf[i]);
    }
    printf("\r\n");

    return HISIGNALLING_RET_VAL_CORRECT;
}

/*
 * @berf hisignalling 协议发送消息
 * @param void *buf: 发送数据缓冲区
 * @param unsigned int data_len: 发送数据长度
 *
 * @berf hisignalling protocol send msg
 * @param void *buf: send data buff
 * @param unsigned int data_len: send data length
 */
static unsigned int HisignallingMsgSend(int fd, char *buf, unsigned int dataLen)
{
    unsigned int ret = 0;
    HisignallingProtocalType hisignallingMsg = {0};
    unsigned char hisignallingSendBuf[HISIGNALLING_MSG_BUFF_LEN] = {0};
    unsigned int hisignallingPackageLen = 0;
    unsigned int writeDataLen = 0;

    hisignallingMsg.frameHeader[0]= 0xAA; /* Protocol head data 1 */
    hisignallingMsg.frameHeader[1]= 0x55; /* Protocol head data 2 */
    (void)memcpy_s(hisignallingMsg.hisignallingMsgBuf, dataLen, buf, dataLen);
    hisignallingMsg.endOfFrame = 0xff; /* Protocol tail data */

    hisignallingPackageLen = HisignallingDataPackage(&hisignallingMsg, dataLen, hisignallingSendBuf);
    if (!hisignallingPackageLen) {
        printf("hisignalling_data_package failed\r\n");
        return -1;
    }
    if (*hisignallingSendBuf == 0) {
        printf("hisignalling send buf is null!\r\n");
        return -1;
    }

    ret = UartSend(fd, hisignallingSendBuf, hisignallingPackageLen);
    if (ret < 0) {
        printf("write data failed\r\n");
        return -1;
    }

    return 0;
}

/*
 * hisignalling消息句柄
 * hisignalling message handle
 */
void *HisignallingMsgHandle(char *param)
{
    unsigned int err = 0;

    while (1) {
        printf("hisignalling_msg_handle\r\n");
        usleep(HISGNALLING_FREE_TASK_TIME);
    }
    err = pthread_join(hisignallingMsgHandleID, NULL);
    if (err != 0) {
        printf("Failed to delete hisignalling msg task\r\n");
    }
}

#define HISIGNALLING_TASK_STACK_SIZE (2048)

/*
 * hisignalling消息任务
 * hisignalling message task
 */
unsigned int HisignallingMsgTask(void)
{
    unsigned int  ret = 0;
    int err = 0, stacksize = HISIGNALLING_TASK_STACK_SIZE;

    pthread_attr_t hisignallingAttr = {0};
    err = pthread_attr_init(&hisignallingAttr);
    err = pthread_attr_setstacksize(&hisignallingAttr, stacksize);
    ret = pthread_create(&hisignallingMsgHandleID, &hisignallingAttr, HisignallingMsgHandle, NULL);
    if (ret != 0) {
        printf("Failed to create hisignalling msg task\r\n");
        return -1;
    }
    return 0;
}

static void UartProcess(int uartFd, int Gpio1Fd, int Gpio2Fd, struct pollfd fdS1, struct pollfd fdS2)
{
    int ret = 0;
    char buff[10] = {0};
    unsigned char writeBuffer[4]  = {0, 2, 0, 3};
    unsigned char writeBuffer2[4] = {0, 2, 0, 4};
    unsigned char writeBuffer3[4] = {0, 2, 0, 5};
    unsigned char readBuff[16] = {0};

    Uart1Config(uartFd);
    while (1) {
        /*
         * 按键操作
         * key operation
         */
        ret = read(Gpio1Fd, buff, 10); /* 10:read data lenght */
        if (ret == -1) {
            MSG("gpio1 read error\n");
        }
        ret = poll(&fdS1, 1, 0); /* 1: 监视一个文件描述符 */
        if (ret == -1) {
            MSG("gpio1 poll error\n");
        }
        if (fdS1.revents & POLLPRI) {
            ret = lseek(Gpio1Fd, 0, SEEK_SET);
            if (ret == -1) {
                MSG("gpio1 lseek error\n");
            }
            MSG("sw2 Pressed \n");
            /*
             * 按键触发发送
             * key trigger sending
             */
            HisignallingMsgSend(uartFd, writeBuffer2, sizeof(writeBuffer2) / sizeof(writeBuffer2[0]));
        }
        ret = read(Gpio2Fd, buff, 10); /* 10:read data lenght */
        if (ret == -1) {
            MSG("gpio2 read error\n");
        }
        ret = poll(&fdS2, 1, 0); /* 1: 监视一个文件描述符 */
        if (ret == -1) {
            MSG("gpio2 poll error\n");
        }
        if (fdS2.revents & POLLPRI) {
            ret = lseek(Gpio2Fd, 0, SEEK_SET);
            if (ret == -1) {
                MSG("gpio1 lseek error\n");
            }
            MSG("sw1 Pressed \n");
            /*
             * 按键触发发送
             * key trigger sending
             */
#ifdef  EXPANSION_BOARD
            HisignallingMsgSend(uartFd, writeBuffer3, sizeof(writeBuffer3) / sizeof(writeBuffer3[0]));
#elif defined (ROBOT_BOARD)
            HisignallingMsgSend(uartFd, writeBuffer, sizeof(writeBuffer) / sizeof(writeBuffer[0]));
#endif
        }
        /*
         * 串口读操作
         * Serial read operation
         */
        HisignallingMsgReceive(uartFd, readBuff, HISIGNALLING_MSG_MOTOR_ENGINE_LEN);
        usleep(HISGNALLING_FREE_TASK_TIME);
    }
}

unsigned int UartOpenInit(void)
{
    int fd;
    char *uart1 = "/dev/ttyAMA1";

    if ((fd = open(uart1, O_RDWR | O_NOCTTY | O_NDELAY)) < 0) {
        printf("open %s is failed", uart1);
        return -1;
    } else {
        Uart1Config(fd);
    }
    return fd;
}

void UartSendRead(int fd, refuseClassification refuseType)
{
    /*
     * 测试buffer
     * Test buffer
     */
    unsigned char writeBuffer2[4] = {0, 2, 0, 1};
    unsigned char writeBuffer3[4] = {0, 2, 0, 2};
    unsigned char writeBuffer4[4] = {0, 2, 0, 3};
    unsigned char writeBuffer5[4] = {0, 2, 0, 4};
    unsigned char writeBuffer6[4] = {0, 2, 0, 5};
    unsigned char writeBuffer7[4] = {0, 2, 0, 6};
    unsigned char writeBuffer8[4] = {0, 2, 0, 7};
    unsigned char writeBuffer9[4] = {0, 2, 0, 8};
    unsigned char readBuff[16] = {0};

#ifdef  EXPANSION_BOARD
    switch (refuseType) {
        case FistGesture:
            HisignallingMsgSend(fd, writeBuffer2, sizeof(writeBuffer2)/sizeof(writeBuffer2[0]));
            printf("send gesture status:FistGesture\r\n");
            break;
        case ForefingerGesture:
            HisignallingMsgSend(fd, writeBuffer3, sizeof(writeBuffer3)/sizeof(writeBuffer3[0]));
            printf("send gesture status:ForefingerGesture\r\n");
            break;
        case OkGesture:
            HisignallingMsgSend(fd, writeBuffer4, sizeof(writeBuffer4)/sizeof(writeBuffer4[0]));
            printf("send gesture status:OkGesture\r\n");
            break;
        case PalmGesture:
            HisignallingMsgSend(fd, writeBuffer5, sizeof(writeBuffer5)/sizeof(writeBuffer5[0]));
            printf("send gesture status:PalmGesture\r\n");
            break;
        case YesGesture:
            HisignallingMsgSend(fd, writeBuffer6, sizeof(writeBuffer6)/sizeof(writeBuffer6[0]));
            printf("send gesture status:YesGesture\r\n");
            break;
        case ForefingerAndThumbGesture:
            HisignallingMsgSend(fd, writeBuffer7, sizeof(writeBuffer7)/sizeof(writeBuffer7[0]));
            printf("send gesture status:ForefingerAndThumbGesture\r\n");
            break;
        case LittleFingerAndThumbGesture:
            HisignallingMsgSend(fd, writeBuffer8, sizeof(writeBuffer8)/sizeof(writeBuffer8[0]));
            printf("send gesture status:LittleFingerAndThumbGesture\r\n");
            break;
        case InvalidGesture:
            HisignallingMsgSend(fd, writeBuffer9, sizeof(writeBuffer9)/sizeof(writeBuffer9[0]));
            printf("send gesture status:InvalidGesture\r\n");
            break;
    }
#endif
    /*
     * 串口读操作
     * Serial read operation
     */
    if (readBuff[0] == HISIGNALLING_HEAD_1 && readBuff[1] == HISIGNALLING_HEAD_2) {
        HisignallingMsgReceive(fd, readBuff, HISIGNALLING_MSG_MOTOR_ENGINE_LEN);
    }
}

int AiUartTransmit(void)
{
    /*
     * 按键初始化定义
     * Key initialization definition
     */
    int gpio1Fd = -1;
    int gpio2Fd = -1;
    int ret1     = -1;
    int ret2     = -1;
    struct pollfd fds1;
    struct pollfd fds2;
    /*
     * 串口初始化定义
     * Serial port initialization definition
     */
    int fd = 0;
    char *uart1 = "/dev/ttyAMA1";

    printf("hisignal task start\r\n");
    /*
     * 按键操作
     * key operation
     */
    InitGpio1();
    InitGpio2();
    gpio1Fd = open("/sys/class/gpio/gpio1/value", O_RDONLY);
    if (gpio1Fd < 0) {
        MSG("Failed to open gpio1 !\n");
        return -1;
    }
    fds1.fd     = gpio1Fd;
    fds1.events = POLLPRI;

    gpio2Fd = open("/sys/class/gpio/gpio2/value", O_RDONLY);
    if (gpio2Fd < 0) {
        MSG("Failed to open gpio1 !\n");
        return -1;
    }
    fds2.fd     = gpio2Fd;
    fds2.events = POLLPRI;
    /*
     * 串口读写
     * Serial read and write
     */
    if ((fd = open(uart1, O_RDWR | O_NOCTTY | O_NDELAY)) < 0) {
        printf("open %s is failed", uart1);
        return -1;
    } else {
        UartProcess(fd, gpio1Fd, gpio2Fd, fds1, fds2);
    }
    return 0;
}
