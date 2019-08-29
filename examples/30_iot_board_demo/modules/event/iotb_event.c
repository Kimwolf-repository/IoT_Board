/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-09-17     MurphyZhao   first implementation
 */

#include <rtthread.h>
#include <stdint.h>
#include "iotb_event.h"
#include "iotb_lcd_process.h"
#include "iotb_sensor.h"
#include <wlan_mgnt.h>
#include <rthw.h>

#define IOTB_EVENT_MSG_NUM 10

// #define IOTB_EVENT_DEBUG

#define DBG_ENABLE
#define DBG_TAG               "IOTB_EVENT"
#ifdef IOTB_EVENT_DEBUG
#define DBG_LVL                      DBG_LOG
#else
#define DBG_LVL                      DBG_INFO /* DBG_ERROR */
#endif 
#define DBG_COLOR
#include <rtdbg.h>

static struct rt_messagequeue iotb_event_mq;
iotb_event_msg_t event_msg[IOTB_EVENT_MSG_NUM];
static uint16_t iotb_event_thr_cycle = IOTB_EVENT_THR_CYCLE;

#ifdef IOTB_EVENT_DEBUG
#define enum2str(s)                    (#s)

char *iotb_event_src_string[IOTB_EVENT_SRC_MAX] = 
{
    enum2str(IOTB_EVENT_SRC_NONE),
    enum2str(IOTB_EVENT_SRC_KEY0),
    enum2str(IOTB_EVENT_SRC_KEY1),
    enum2str(IOTB_EVENT_SRC_KEY2),
    enum2str(IOTB_EVENT_SRC_KEYWKUP),
    enum2str(IOTB_EVENT_SRC_WIFI),
    enum2str(IOTB_EVENT_SRC_PM)
};

char *iotb_event_type_string[IOTB_EVENT_TYPE_MAX] = 
{
    enum2str(IOTB_EVENT_TYPE_NONE),
    enum2str(IOTB_EVENT_TYPE_KEY_CLICK),
    enum2str(IOTB_EVENT_TYPE_KEY_LONG_PRESSED),
    enum2str(IOTB_EVENT_TYPE_KEY_LONG_PRESSED_UP),
    enum2str(IOTB_EVENT_TYPE_WIFI_UP),
    enum2str(IOTB_EVENT_TYPE_WIFI_DOWN),
    enum2str(IOTB_EVENT_TYPE_WIFI_AIRKISS_STARTED),
    enum2str(IOTB_EVENT_TYPE_WIFI_AIRKISS_FAILED),
    enum2str(IOTB_EVENT_TYPE_WIFI_AIRKISS_SUCCESS),
    enum2str(IOTB_EVENT_TYPE_PM_START),
    enum2str(IOTB_EVENT_TYPE_PM_WKUP)
};
#endif

//初始化消息队列
static rt_err_t iotb_event_init(void)
{
    rt_mq_init(&iotb_event_mq, "event_mq", 	//初始化一个消息队列
		&event_msg[0],						//指向消息队列的缓冲区
        sizeof(iotb_event_msg_t),			//一条消息最大长度为这个
        sizeof(event_msg),					//缓冲区大小
        RT_IPC_FLAG_FIFO);

    return RT_EOK;
}

static rt_bool_t iotb_event_put_enable = RT_TRUE;

//设置开启事件发送
void iotb_event_put_set_enable(void)
{
    rt_base_t level = rt_hw_interrupt_disable();	//关中断
    iotb_event_put_enable = RT_TRUE;				//设置开启标志
    rt_hw_interrupt_enable(level);					//开中断
}

//设置关闭事件发送
void iotb_event_put_set_disable(void)
{
    rt_base_t level = rt_hw_interrupt_disable();	//关中断
    iotb_event_put_enable = RT_FALSE;				//设置标志
    rt_hw_interrupt_enable(level);					//开中断
}


//发送事件
rt_err_t iotb_event_put(iotb_event_msg_t *msg)
{
    rt_err_t rst = RT_EOK;
    if (iotb_event_put_enable == RT_FALSE)
    {
        LOG_I("event put is disable!");
        return rst;
    }

    rst = rt_mq_send(&iotb_event_mq, msg, sizeof(iotb_event_msg_t));
    if (rst == -RT_EFULL)
    {
        LOG_I("iotb event queue is full");
        rt_thread_mdelay(100);
        rst = -RT_EFULL;
    }
    return rst;
}

//获取一个事件
rt_err_t iotb_event_get(iotb_event_msg_t *msg, uint32_t timeout)
{
    rt_err_t rst = RT_EOK;
	//接收消息，消息队列对象，缓冲区地址，缓冲区大小，超时时间
    rst = rt_mq_recv(&iotb_event_mq, msg, sizeof(iotb_event_msg_t), timeout);
    if ((rst != RT_EOK) && (rst != -RT_ETIMEOUT))
    {
        LOG_E("iotb event get failed! Errno: %d", rst);
    }
    return rst;
}

#include <msh.h>
#include <finsh.h>
//事件处理函数
static rt_err_t iotb_event_process(iotb_event_msg_t *msg)
{
    rt_err_t rst = RT_EOK;

    if (iotb_pm_status_get())	//获取pm状态，休眠跳过所有事件
    {
        LOG_I("In pm mode. Skip all event!");
        return 0;
    }

    switch (msg->event.event_src)	//响应事件源
    {
        case IOTB_EVENT_SRC_KEY2:
            LOG_I("response KEY2 event!");
            iotb_lcd_event_put(IOTB_LCD_EVENT_KEY2);	//发送LCDkey2事件
            break;

        case IOTB_EVENT_SRC_KEY1:
            LOG_I("response KEY1 event!");
            iotb_lcd_event_put(IOTB_LCD_EVENT_KEY1);	//发送LCDkey1事件
            break;

        case IOTB_EVENT_SRC_KEY0:
            LOG_I("response KEY0 event!");
            iotb_lcd_event_put(IOTB_LCD_EVENT_NEXT);	//发送LCD下一个事件
            break;

        case IOTB_EVENT_SRC_KEYWKUP:
            if (msg->event.event_type == IOTB_EVENT_TYPE_KEY_CLICK)	//事件类型是唤醒键按下
            {
                iotb_lcd_event_put(IOTB_LCD_EVENT_WKUP);	//发送LED唤醒事件
            }
            else if (msg->event.event_type == IOTB_EVENT_TYPE_KEY_LONG_PRESSED)	//如果是长按事件
            {
                LOG_I("response WK_UP long press event!");
                LOG_I("Will reconfig network by smartconfig!");
                iotb_lcd_event_put(IOTB_LCD_EVENT_SMARTCONFIG_START);	//smart config 开始事件
            }
            break;

        case IOTB_EVENT_SRC_WIFI:	//事件源是WIFI
            if (msg->event.event_type == IOTB_EVENT_TYPE_WIFI_AIRKISS_STARTED)
            {
                LOG_I("response WiFi event!");
                iotb_lcd_event_put(IOTB_LCD_EVENT_SMARTCONFIG_STARTED);
            }
            else if (msg->event.event_type == IOTB_EVENT_TYPE_WIFI_AIRKISS_SUCCESS)
            {
                iotb_lcd_event_put(IOTB_LCD_EVENT_SMARTCONFIG_FINISH);
            }
            break;

        case IOTB_EVENT_SRC_PM:
            if (msg->event.event_type == IOTB_EVENT_TYPE_PM_START)
            {
                LOG_I("response PM START event!");
                iotb_lcd_event_put(IOTB_LCD_EVENT_PM_START);
            }
            else if (msg->event.event_type == IOTB_EVENT_TYPE_PM_WKUP)
            {
                LOG_I("response PM WKUP event!");
                iotb_pm_exit();
                iotb_lcd_event_put(IOTB_LCD_EVENT_PM_WKUP);
            }
            break;

        default:
            LOG_E("unhandled event!");
            break;
    }

    return rst;
}


//设置事件循环事件
void iotb_event_thr_set_cycle(uint16_t time)
{
    rt_base_t level = rt_hw_interrupt_disable();
    iotb_event_thr_cycle = time;
    rt_hw_interrupt_enable(level);
}

//event线程入口函数
static void iotb_event_handler(void* arg)
{
    rt_err_t rst = RT_EOK;
    iotb_event_msg_t msg;	//创建一个板级事件

    while(1)
    {
		//循环获取消息
        rst = iotb_event_get(&msg, iotb_event_thr_cycle);
        if (rst == RT_EOK)
        {
            LOG_D("event src: %s; event type: %s",
                iotb_event_src_string[msg.event.event_src],
                iotb_event_type_string[msg.event.event_type]);
            iotb_event_process(&msg);	//开始事件处理
        }
    }
}

//开始事件处理，创建一个线程运行iotb_event_handler
rt_err_t iotb_event_start(void)
{
    rt_thread_t iotb_event_tid;		//创建一个线程，这是个指针类型

    iotb_event_init();	//初始化消息队列

    iotb_event_tid = rt_thread_create("event_thr",	//创建线程，线程名
        iotb_event_handler,		//线程入口函数
        RT_NULL,				//入口函数参数
        1024, RT_THREAD_PRIORITY_MAX/2-4, 20);	//栈大小、优先级、时间片
    if (iotb_event_tid != RT_NULL)
        rt_thread_startup(iotb_event_tid);	//启动线程
    return RT_EOK;
}
