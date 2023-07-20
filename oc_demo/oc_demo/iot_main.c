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

/* we use the mqtt to connect to the IoT platform */
/*
 * STEPS:
 * 1, CONNECT TO THE IOT SERVER
 * 2, SUBSCRIBE  THE DEFAULT TOPIC
 * 3, WAIT FOR ANY MESSAGE COMES OR ANY MESSAGE TO SEND
 */

#include <string.h>
#include <securec.h>
#include <hi_task.h>
#include "cmsis_os2.h"
#include "iot_watchdog.h"
#include "iot_errno.h"
#include "iot_config.h"
#include "iot_log.h"
#include "MQTTClient.h"
#include "iot_main.h"

// this is the configuration head
#define CN_IOT_SERVER    "tcp://121.36.42.100:1883"

#define CONFIG_COMMAND_TIMEOUT    10000L
#define CN_KEEPALIVE_TIME    50
#define CN_CLEANSESSION    1
#define CN_HMAC_PWD_LEN   65 // SHA256 IS 32 BYTES AND END APPEND'\0'
#define CN_EVENT_TIME    "1970000100"
#define CN_CLIENTID_FMT    "%s_0_0_%s" // This is the cient ID format, deviceID_0_0_TIME
#define CN_QUEUE_WAITTIMEOUT    1000
#define CN_QUEUE_MSGNUM    16
#define CN_QUEUE_MSGSIZE    (sizeof(hi_pvoid))

#define CN_TASK_PRIOR    28
#define CN_TASK_STACKSIZE    0X2000
#define CN_TASK_NAME    "IoTMain"

typedef enum {
    EN_IOT_MSG_PUBLISH = 0,
    EN_IOT_MSG_RECV,
}EnIotMsgT;

typedef struct {
    EnIotMsgT type;
    int qos;
    const char *topic;
    const char *payload;
}IoTMsgT;

typedef struct {
    hi_bool  stop;
    hi_u32 conLost;
    hi_u32 queueID;
    hi_u32 iotTaskID;
    FnMsgCallBack msgCallBack;
    MQTTClient_deliveryToken tocken;
}IotAppCbT;
static IotAppCbT g_ioTAppCb;

static const char *g_defaultSubscribeTopic[] = {
    "$oc/devices/"CONFIG_DEVICE_ID"/sys/messages/down",
    "$oc/devices/"CONFIG_DEVICE_ID"/sys/properties/set/#",
    "$oc/devices/"CONFIG_DEVICE_ID"/sys/properties/get/#",
    "$oc/devices/"CONFIG_DEVICE_ID"/sys/shadow/get/response/#",
    "$oc/devices/"CONFIG_DEVICE_ID"/sys/events/down",
    "$oc/devices/"CONFIG_DEVICE_ID"/sys/commands/#"
};

#define CN_TOPIC_SUBSCRIBE_NUM    (sizeof(g_defaultSubscribeTopic) / sizeof(const char *))

static int MsgRcvCallBack(unsigned char *context, char *topic, int topicLen, MQTTClient_message *message)
{
    IoTMsgT *msg;
    char *buf;
    hi_u32 bufSize;
    int topicLenght = topicLen;

    if (topicLenght == 0) {
        topicLenght = strlen(topic);
    }
    bufSize = topicLenght + 1  + message->payloadlen + 1 + sizeof(IoTMsgT);
    buf = hi_malloc(0, bufSize);
    if (buf != NULL) {
        msg = (IoTMsgT *)buf;
        buf += sizeof(IoTMsgT);
        bufSize -= sizeof(IoTMsgT);
        msg->qos = message->qos;
        msg->type = EN_IOT_MSG_RECV;
        (void)memcpy_s(buf, bufSize, topic, topicLenght);
        buf[topicLenght] = '\0';
        msg->topic = buf;
        buf += topicLenght + 1;
        bufSize -= (topicLenght + 1);
        (void)memcpy_s(buf, bufSize, message->payload, message->payloadlen);
        buf[message->payloadlen] = '\0';
        msg->payload = buf;
        IOT_LOG_DEBUG("RCVMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n", msg->qos, msg->topic, msg->payload);
        if (IOT_SUCCESS != osMessageQueuePut(g_ioTAppCb.queueID, &msg, 0, CN_QUEUE_WAITTIMEOUT)) {
            IOT_LOG_ERROR("Wrie queue failed\r\n");
            hi_free(0, msg);
        }
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topic);
    return 1;
}

// when the connect lost and this callback will be called
static void ConnLostCallBack(unsigned char *context, char *cause)
{
    IOT_LOG_DEBUG("Connection lost:caused by:%s\r\n", cause == NULL ? "Unknown" : cause);
    return;
}

void IoTMsgProces(IoTMsgT *msg, MQTTClient_message pubmsg, MQTTClient client)
{
    hi_u32 ret;
    switch (msg->type) {
        case EN_IOT_MSG_PUBLISH:
            pubmsg.payload = (void *)msg->payload;
            pubmsg.payloadlen = (int)strlen(msg->payload);
            pubmsg.qos = msg->qos;
            pubmsg.retained = 0;
            ret = MQTTClient_publishMessage(client, msg->topic, &pubmsg, &g_ioTAppCb.tocken);
            if (ret != MQTTCLIENT_SUCCESS) {
                IOT_LOG_ERROR("MSGSEND:failed\r\n");
            }
            IOT_LOG_DEBUG("MSGSEND:SUCCESS\r\n");
            g_ioTAppCb.tocken++;
            break;
        case EN_IOT_MSG_RECV:
            if (g_ioTAppCb.msgCallBack != NULL) {
                g_ioTAppCb.msgCallBack(msg->qos, msg->topic, msg->payload);
            }
            break;
        default:
            break;
    }
    return;
}

// use this function to deal all the comming message
static int ProcessQueueMsg(MQTTClient client)
{
    printf("ProcessQueueMsg\r\n");
    hi_u32     ret;
    hi_u32     msgSize;
    IoTMsgT    *msg;
    hi_u32     timeout;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;

    timeout = CN_QUEUE_WAITTIMEOUT;
    do {
        msg = NULL;
        msgSize = sizeof(hi_pvoid);
        ret = osMessageQueueGet(g_ioTAppCb.queueID, &msg, &msgSize, timeout);
        if (ret != MQTTCLIENT_SUCCESS) {
            return HI_ERR_FAILURE;
        }
        if (msg != NULL) {
            IoTMsgProces(msg, pubmsg, client);
            hi_free(0, msg);
        }
        timeout = 0;  // continous to deal the message without wait here
    } while (ret == HI_ERR_SUCCESS);
    return;
}

void MqttProcess(MQTTClient client, char *clientID, char *userPwd, MQTTClient_connectOptions connOpts, int subQos[])
{
    int rc = MQTTClient_create(&client, CN_IOT_SERVER, clientID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        IOT_LOG_ERROR("Create Client failed,Please check the parameters--%d\r\n", rc);
        if (userPwd != NULL) {
            hi_free(0, userPwd);
            return;
        }
    }

    rc = MQTTClient_setCallbacks(client, NULL, ConnLostCallBack, MsgRcvCallBack, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        IOT_LOG_ERROR("Set the callback failed,Please check the callback paras\r\n");
        MQTTClient_destroy(&client);
        return;
    }

    rc = MQTTClient_connect(client, &connOpts);
    if (rc != MQTTCLIENT_SUCCESS) {
        IOT_LOG_ERROR("Connect IoT server failed,please check the network and parameters:%d\r\n", rc);
        MQTTClient_destroy(&client);
        return;
    }
    IOT_LOG_DEBUG("Connect success\r\n");

    rc = MQTTClient_subscribeMany(client, CN_TOPIC_SUBSCRIBE_NUM, (char* const*)g_defaultSubscribeTopic,
                                  (int *)&subQos[0]);
    if (rc != MQTTCLIENT_SUCCESS) {
        IOT_LOG_ERROR("Subscribe the default topic failed,Please check the parameters\r\n");
        MQTTClient_destroy(&client);
        return;
    }
    IOT_LOG_DEBUG("Subscribe success\r\n");
    while (MQTTClient_isConnected(client)) {
        ProcessQueueMsg(client); // do the job here
        int ret = ProcessQueueMsg(client); // do the job here
        if (ret == HI_ERR_SUCCESS) {
            return;
        }
        MQTTClient_yield(); // make the keepalive done
    }
    MQTTClient_disconnect(client, CONFIG_COMMAND_TIMEOUT);
    return;
}

static hi_void MainEntryProcess(hi_void)
{
    int subQos[CN_TOPIC_SUBSCRIBE_NUM] = {1};
    char *clientID = NULL;
    char *userID = NULL;
    char *userPwd = NULL;

    MQTTClient client = NULL;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    // make the clientID userID userPwd
    clientID = CONFIG_CLIENTID;
    userID = CONFIG_DEVICE_ID;
    userPwd = CONFIG_DEVICE_PWD;
    conn_opts.keepAliveInterval = CN_KEEPALIVE_TIME;
    conn_opts.cleansession = CN_CLEANSESSION;
    conn_opts.username = userID;
    conn_opts.password = userPwd;
    conn_opts.MQTTVersion = MQTTVERSION_3_1_1;
    // wait for the wifi connect ok
    printf("IOTSERVER:%s\r\n", CN_IOT_SERVER);
    MqttProcess(client, clientID, userPwd, conn_opts, subQos);
    return;
}

static hi_void *MainEntry(hi_void *arg)
{
    (void)arg;
    while (g_ioTAppCb.stop == HI_FALSE) {
        MainEntryProcess();
        IOT_LOG_DEBUG("The connection lost and we will try another connect\r\n");
        hi_sleep(1000*5); /* 延时5*1000ms */
    }
    return NULL;
}

int IoTMain(void)
{
    hi_u32 ret;
    hi_task_attr attr = {0};
    g_ioTAppCb.queueID = osMessageQueueNew(CN_QUEUE_MSGNUM, CN_QUEUE_MSGSIZE, NULL);
    attr.stack_size = CN_TASK_STACKSIZE;
    attr.task_prio = CN_TASK_PRIOR;
    attr.task_name = CN_TASK_NAME;
    ret = hi_task_create(&g_ioTAppCb.iotTaskID, &attr, MainEntry, NULL);
    if (ret != HI_ERR_SUCCESS) {
        IOT_LOG_ERROR("Create the Main Entry Failed\r\n");
    }

    return 0;
}

int IoTSetMsgCallback(FnMsgCallBack msgCallback)
{
    g_ioTAppCb.msgCallBack = msgCallback;
    return 0;
}

int IotSendMsg(int qos, const char *topic, const char *payload)
{
    int rc = -1;
    IoTMsgT *msg;
    char *buf;
    hi_u32 bufSize;

    bufSize = strlen(topic) + 1 + strlen(payload) + 1 + sizeof(IoTMsgT);
    buf = hi_malloc(0, bufSize);
    if (buf != NULL) {
        msg = (IoTMsgT *)buf;
        buf += sizeof(IoTMsgT);
        bufSize -= sizeof(IoTMsgT);
        msg->qos = qos;
        msg->type = EN_IOT_MSG_PUBLISH;
        (void)memcpy_s(buf, bufSize, topic, strlen(topic));
        buf[strlen(topic)] = '\0';
        msg->topic = buf;
        buf += strlen(topic) + 1;
        bufSize -= (strlen(topic) + 1);
        (void)memcpy_s(buf, bufSize, payload, strlen(payload));
        buf[strlen(payload)] = '\0';
        msg->payload = buf;
        IOT_LOG_DEBUG("SNDMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n", msg->qos, msg->topic, msg->payload);
        if (osMessageQueuePut(g_ioTAppCb.queueID, &msg, 0, CN_QUEUE_WAITTIMEOUT) != IOT_SUCCESS) {
            IOT_LOG_ERROR("Wrie queue failed\r\n");
            hi_free(0, msg);
            return;
        } else {
            rc = 0;
        }
    }
    return rc;
}