/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include "sample_media_ai.h"
#include "posix_help.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define SKPAIR_FDS 2

/*
 * 创建Socketpair
 * Socketpair create
 */
int SkPairCreate(SkPair* chn)
{
    HI_ASSERT(chn);
    int fds[SKPAIR_FDS];

    if (socketpair(PF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, fds) < 0) {
        HI_ASSERT(0);
    }
    chn->in = fds[0];
    chn->out = fds[1];
    return 0;
}

/*
 * 读完整的消息
 * Read complete message
 */
int FdReadMsg(int fd, HI_VOID* msgBuf, int msgSize)
{
    HI_ASSERT(msgBuf && msgSize > 0);
    static const int logPerCount = 1000; // Circulate 1000 times and log information once
    int loopNum = 0;
    int total = 0;
    int ret;
    while (total < msgSize) {
        ret = read(fd, (char*)msgBuf + total, msgSize - total);
        if (ret > 0) {
            total += ret;
        } else if (ret == 0) { // fd closed by peer
            SAMPLE_PRT("read FAIL, for fd closed by peer\n");
            return -1;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) { // no data now
            if (total == 0) { // When no data has been read before, return
                return 0;
            }
            if (++loopNum % logPerCount == 0) {
                SAMPLE_PRT("FdReadMsg wait %ds, ERR, CHECK it\n", loopNum / logPerCount);
            }
            usleep(HI_USLEEP_MS); // wait 1ms, and try again
        } else {
            return -1; // read error
        }
    }
    return msgSize;
}

/*
 * 写完整的消息
 * Write complete message
 */
int FdWriteMsg(int fd, const HI_VOID* msgData, int msgLen)
{
    HI_ASSERT(msgData && msgLen > 0);
    static const int logPerCount = 1000; // Circulate 1000 times and log information once
    int loopNum = 0;
    int total = 0;
    int ret;

    while (total < msgLen) {
        ret = write(fd, (const char*)msgData + total, msgLen - total);
        if (ret > 0) {
            total += ret;
        } else if (ret == 0 || (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) { // no data now
            if (++loopNum % logPerCount == 0) {
                SAMPLE_PRT("FdWriteMsg wait %ds, ERR, CHECK it\n", loopNum / logPerCount);
            }
            usleep(HI_USLEEP_MS); // wait 1ms, and try again
        } else { // write error
            HI_ASSERT(ret < 0);
            SAMPLE_PRT("FdWriteMsg FAIL, err='%s'\n", strerror(errno));
            return -1;
        }
    }
    return msgLen;
}

/*
 * 销毁socketpair
 * Socketpair destroy
 */
void SkPairDestroy(SkPair* chn)
{
    HI_ASSERT(chn);

    if (chn->in >= 0) {
        if (close(chn->in) < 0) {
            HI_ASSERT(0);
        }
        chn->in = -1;
    }
    if (chn->out >= 0) {
        if (close(chn->out) < 0) {
            HI_ASSERT(0);
        }
        chn->out = -1;
    }
}

/*
 * 初始化recursive pmutex
 * Init recursive pmutex
 */
void RecurMutexInit(pthread_mutex_t* mutex)
{
    HI_ASSERT(mutex);
    pthread_mutexattr_t attr;
    int res;

    res = pthread_mutexattr_init(&attr);
    HI_ASSERT(!res);

    res = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    HI_ASSERT(!res);
    res = pthread_mutex_init(mutex, &attr);
    HI_ASSERT(!res);

    pthread_mutexattr_destroy(&attr);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
