/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-09-17     MurphyZhao   first implementation
 */

#ifndef __IOTB_EVENT__
#define __IOTB_EVENT__
#include <rtthread.h>

typedef enum
{
    IOTB_EVENT_SRC_NONE,		//空
    IOTB_EVENT_SRC_KEY0,		//按键0事件
    IOTB_EVENT_SRC_KEY1,		//按键1
    IOTB_EVENT_SRC_KEY2,		//按键2
    IOTB_EVENT_SRC_KEYWKUP,		//按键唤醒
    IOTB_EVENT_SRC_WIFI,		//wifi事件
    IOTB_EVENT_SRC_PM,			//内存管理事件
    IOTB_EVENT_SRC_MAX
} iotb_event_src_t;

typedef enum
{
    /* key */
    IOTB_EVENT_TYPE_NONE,					//
    IOTB_EVENT_TYPE_KEY_CLICK,				//按下
    IOTB_EVENT_TYPE_KEY_LONG_PRESSED,		//长按
    IOTB_EVENT_TYPE_KEY_LONG_PRESSED_UP,	//长按弹起

    /* wifi */
    IOTB_EVENT_TYPE_WIFI_UP,				//wifi连接
    IOTB_EVENT_TYPE_WIFI_DOWN,				//wifi断开
    IOTB_EVENT_TYPE_WIFI_AIRKISS_STARTED,	//开始配网
    IOTB_EVENT_TYPE_WIFI_AIRKISS_FAILED,	//配网失败
    IOTB_EVENT_TYPE_WIFI_AIRKISS_SUCCESS,	//配网成功

    /* PM */
    IOTB_EVENT_TYPE_PM_START,				//电源管理开启
    IOTB_EVENT_TYPE_PM_WKUP,				//电源管理结束
    IOTB_EVENT_TYPE_MAX
} iotb_event_type_t;

typedef struct _event
{
    iotb_event_src_t  event_src;			//事件源
    iotb_event_type_t event_type;			//事件类型
} iotb_event_t;

typedef struct _event_msg
{
    iotb_event_t event;
} iotb_event_msg_t;

#define IOTB_EVENT_THR_CYCLE (100)

rt_err_t iotb_event_start(void);
rt_err_t iotb_event_put(iotb_event_msg_t *msg);
rt_err_t iotb_event_get(iotb_event_msg_t *msg, uint32_t timeout);
void iotb_event_thr_set_cycle(uint16_t time);
void iotb_event_put_set_enable(void);
void iotb_event_put_set_disable(void);

#endif
