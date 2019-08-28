/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-09-26     MurphyZhao   first implementation
 */

#include <rtthread.h>
#include <stdint.h>
#include <ipc/workqueue.h>

#define IOTB_WK_DEBUG

#define DBG_ENABLE
#define DBG_TAG               "IOTB_JOB"
#ifdef IOTB_WK_DEBUG
#define DBG_LVL                      DBG_LOG
#else
#define DBG_LVL                      DBG_INFO /* DBG_ERROR */
#endif 
#define DBG_COLOR
#include <rtdbg.h>

static struct rt_workqueue * iotb_work_hd = RT_NULL;

//工作队列结构体
typedef struct
{
    struct rt_work work;	//内核提供的工作结构体，包含函数指针和传入参数
    void (*fun)(void *parameter);	//额外的函数指针？
    void *parameter;				//额外的参数？
} iotb_work_t;

//板级工作队列函数
//work,内核提供的工作结构体指针
//work_data，传入参数
static void iotb_workqueue_fun(struct rt_work *work, void *work_data)
{
	//传入的work_data是iotb_work_t* ？
    iotb_work_t *iotb_work = work_data;

    iotb_work->fun(iotb_work->parameter);	//执行传入的iotb_work_t 的函数
    rt_free(iotb_work);		//这里应该是释放掉传入的work_data，只执行一次？
}

struct rt_workqueue *iotb_work_hd_get(void)
{
    return iotb_work_hd;
}

rt_err_t iotb_workqueue_dowork(void (*func)(void *parameter), void *parameter)
{
    iotb_work_t *iotb_work;
    rt_err_t err = RT_EOK;

    LOG_D("F:%s is run", __FUNCTION__);
    if (func == RT_NULL)
    {
        LOG_E("F:%s L:%d func is null", __FUNCTION__, __LINE__);
        return -RT_EINVAL;
    }

    if (iotb_work_hd == RT_NULL)
    {
        LOG_E("F:%s L:%d not init iotb work queue", __FUNCTION__, __LINE__);
        return -RT_ERROR;
    }

    iotb_work = rt_malloc(sizeof(iotb_work_t));
    if (iotb_work == RT_NULL)
    {
        LOG_E("F:%s L:%d create work failed, no memory", __FUNCTION__, __LINE__);
        return -RT_ENOMEM;
    }

    iotb_work->fun = func;
    iotb_work->parameter = parameter;

    rt_work_init(&iotb_work->work, iotb_workqueue_fun, iotb_work);
    err = rt_workqueue_dowork(iotb_work_hd, &iotb_work->work);
    if (err != RT_EOK)
    {
        LOG_E("F:%s L:%d do work failed", __FUNCTION__, __LINE__);
        rt_free(iotb_work);
        iotb_work = RT_NULL;
    }

    return err;
}


//初始化工作队列
rt_err_t iotb_workqueue_start(void)
{
    static rt_int8_t iotb_work_started = 0;	//static局部变量，只赋值一次

    if (iotb_work_started == 0)	//也就是这里只初始化一次
    {
		//创建一个工作队列
        iotb_work_hd = rt_workqueue_create("iotb_job", 2048, RT_THREAD_PRIORITY_MAX/2-2);
        if (iotb_work_hd == RT_NULL)
        {
            LOG_E("F:%s L:%d iotb work queue create failed", __FUNCTION__, __LINE__);
            return -RT_ERROR;
        }
        iotb_work_started = 1;
        return RT_EOK;
    }
    return RT_EOK;
}
