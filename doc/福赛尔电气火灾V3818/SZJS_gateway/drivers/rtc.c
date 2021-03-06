/*
 * File      : rtc.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 * 2011-11-26     aozima       implementation time.
 */

#include <rtthread.h>
#include <stm32f2xx.h>
#include <time.h>
#include "sys_config.h"

__IO uint32_t AsynchPrediv = 0, SynchPrediv = 0;
RTC_TimeTypeDef RTC_TimeStructure;
RTC_InitTypeDef RTC_InitStructure;
RTC_AlarmTypeDef  RTC_AlarmStructure;
RTC_DateTypeDef RTC_DateStructure;

#define MINUTE   60
#define HOUR   (60*MINUTE)
#define DAY   (24*HOUR)
#define YEAR   (365*DAY)

static int month[12] =
{
    0,
    DAY*(31),
    DAY*(31+29),
    DAY*(31+29+31),
    DAY*(31+29+31+30),
    DAY*(31+29+31+30+31),
    DAY*(31+29+31+30+31+30),
    DAY*(31+29+31+30+31+30+31),
    DAY*(31+29+31+30+31+30+31+31),
    DAY*(31+29+31+30+31+30+31+31+30),
    DAY*(31+29+31+30+31+30+31+31+30+31),
    DAY*(31+29+31+30+31+30+31+31+30+31+30)
};
static struct rt_device rtc;

time_t rt_mktime(struct tm *tm)
{
	long res;
	int year;
	year = tm->tm_year - 70;

	res = YEAR * year + DAY * ((year + 1) / 4);
	res += month[tm->tm_mon];

	if (tm->tm_mon > 1 && ((year + 2) % 4))
	res -= DAY;
	res += DAY * (tm->tm_mday - 1);
	res += HOUR * tm->tm_hour;
	res += MINUTE * tm->tm_min;
	res += tm->tm_sec;
	return res;
}
static rt_err_t rt_rtc_open(rt_device_t dev, rt_uint16_t oflag)
{
    if (dev->rx_indicate != RT_NULL)
    {
        /* Open Interrupt */
    }

    return RT_EOK;
}

static rt_size_t rt_rtc_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
    return 0;
}

static rt_err_t rt_rtc_control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
    time_t *time;
	struct tm ti,*to;
    RT_ASSERT(dev != RT_NULL);

    switch (cmd)
    {
    case RT_DEVICE_CTRL_RTC_GET_TIME:
        time = (time_t *)args;
        /* read device */
		//RTC_GetTimeStamp(RTC_Format_BIN, &RTC_TimeStructure, &RTC_DateStructure);
		RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
		RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
		ti.tm_sec = RTC_TimeStructure.RTC_Seconds;
		ti.tm_min = RTC_TimeStructure.RTC_Minutes;
		ti.tm_hour = RTC_TimeStructure.RTC_Hours;
		//ti.tm_wday = (RTC_DateStructure.RTC_WeekDay==7)?0:RTC_DateStructure.RTC_WeekDay;
		ti.tm_mon = RTC_DateStructure.RTC_Month -1;
		ti.tm_mday = RTC_DateStructure.RTC_Date;
		ti.tm_year = RTC_DateStructure.RTC_Year + 70;
		*time = rt_mktime(&ti);
        //*time = RTC_GetCounter();

        break;

    case RT_DEVICE_CTRL_RTC_SET_TIME:
    {
        time = (time_t *)args;

        /* Enable the PWR clock */
	    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	    /* Allow access to RTC */
	    PWR_BackupAccessCmd(ENABLE);

        /* Wait until last write operation on RTC registers has finished */
        //RTC_WaitForLastTask();

        /* Change the current time */
        //RTC_SetCounter(*time);

		to = localtime(time);
		RTC_TimeStructure.RTC_Seconds = to->tm_sec;
		RTC_TimeStructure.RTC_Minutes = to->tm_min;
		RTC_TimeStructure.RTC_Hours	= to->tm_hour;
		//RTC_DateStructure.RTC_WeekDay =(ti->tm_wday==0)?7:ti->tm_wday;
		RTC_DateStructure.RTC_Month = to->tm_mon + 1;
		RTC_DateStructure.RTC_Date = to->tm_mday;
		RTC_DateStructure.RTC_Year = to->tm_year - 70;
		RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure);
		RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure);

        /* Wait until last write operation on RTC registers has finished */
        //RTC_WaitForLastTask();

        //RTC_WriteBackupRegister(SYS_RTC_SYNC_ADDR, 0xA5A5);
		//BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
    }
    break;
    }

    return RT_EOK;
}

/*******************************************************************************
* Function Name  : RTC_Configuration
* Description    : Configures the RTC.
* Input          : None
* Output         : None
* Return         : 0 reday,-1 error.
*******************************************************************************/
int RTC_Config(void)
{
	u32 count=0x200000;
    
	/* Enable the PWR clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);


	/* Allow access to RTC */
	PWR_BackupAccessCmd(ENABLE);

	RCC_LSEConfig(RCC_LSE_ON);

	/* Wait till LSE is ready */
	while ( (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) && (--count) );
    if ( count == 0 )
    {
        return -1;
    }

	/* Select the RTC Clock Source */
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

	SynchPrediv = 0xFF;
	AsynchPrediv = 0x7F;

	/* Enable the RTC Clock */
	RCC_RTCCLKCmd(ENABLE);

	/* Wait for RTC APB registers synchronisation */
	RTC_WaitForSynchro();

	/* Enable The TimeStamp */
	//RTC_TimeStampCmd(RTC_TimeStampEdge_Falling, ENABLE);

	return 0;
}



int RTC_alarm_cfg(void)
{
    RTC_AlarmTypeDef  RTC_AlarmStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;


        /* Enable the PWR clock */
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
        /* Enable BKPSRAM Clock */
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_BKPSRAM, ENABLE);

        /* Allow access to RTC */
        PWR_BackupAccessCmd(ENABLE);



    /* Wait for RTC APB registers synchronisation */
    RTC_WaitForSynchro();

    /* Clear the RTC Alarm Flag */
    RTC_ClearFlag(RTC_FLAG_ALRAF);


    RTC_AlarmCmd(RTC_Alarm_A, DISABLE);

    RTC_AlarmStructure.RTC_AlarmTime.RTC_Hours = 0x01;
    RTC_AlarmStructure.RTC_AlarmTime.RTC_Minutes = 0x00;
    RTC_AlarmStructure.RTC_AlarmTime.RTC_Seconds = 0x00;
    RTC_AlarmStructure.RTC_AlarmDateWeekDaySel = RTC_AlarmDateWeekDaySel_Date;
    RTC_AlarmStructure.RTC_AlarmDateWeekDay = 0x01;

    /* Set the alarmA Masks */
    RTC_AlarmStructure.RTC_AlarmMask = RTC_AlarmMask_All;
    RTC_SetAlarm(RTC_Format_BCD, RTC_Alarm_A, &RTC_AlarmStructure);
  

    /* Enable the RTC Alarm A Interrupt */
    RTC_ITConfig(RTC_IT_ALRA, ENABLE);
   
    /* Enable the alarm  A */
    RTC_AlarmCmd(RTC_Alarm_A, ENABLE);



    /* RTC Alarm A Interrupt Configuration */
    /* EXTI configuration *********************************************************/
    EXTI_ClearITPendingBit(EXTI_Line17);
    EXTI_InitStructure.EXTI_Line = EXTI_Line17;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
  
    /* Enable the RTC Alarm Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = RTC_Alarm_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

}


int RTC_Configuration(void)
{

	if(RTC_Config() < 0 )
		return -1;

	/* Set the Time */
	RTC_TimeStructure.RTC_Hours   = 0;
	RTC_TimeStructure.RTC_Minutes = 0;
	RTC_TimeStructure.RTC_Seconds = 0;

	/* Set the Date */
	RTC_DateStructure.RTC_Month = 1;
	RTC_DateStructure.RTC_Date = 1;
	RTC_DateStructure.RTC_Year = 0;
	RTC_DateStructure.RTC_WeekDay = 4;

	/* Calendar Configuration */
	RTC_InitStructure.RTC_AsynchPrediv = AsynchPrediv;
	RTC_InitStructure.RTC_SynchPrediv =  SynchPrediv;
	RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
	RTC_Init(&RTC_InitStructure);

	/* Set Current Time and Date */
	RTC_SetTime(RTC_Format_BCD, &RTC_TimeStructure);
	RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure);
	if (RTC_Init(&RTC_InitStructure) == ERROR)
		return -1;

    return 0;
}

void rt_hw_rtc_init(void)
{
    rtc.type	= RT_Device_Class_RTC;
    
    /* Enable the PWR clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    /* Enable BKPSRAM Clock */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_BKPSRAM, ENABLE);    
    /* Allow access to RTC */
    PWR_BackupAccessCmd(ENABLE);   
    
    if (RTC_ReadBackupRegister(SYS_RTC_SYNC_ADDR) != 0xA5A5)
    {
        rt_kprintf("rtc is not configured\n");
        rt_kprintf("please configure with set_date and set_time\n");
        if ( RTC_Configuration() != 0)
        {
            rt_kprintf("rtc configure fail...\r\n");
            return ;
        }
    }
    else
    {
        /* Wait for RTC registers synchronization */
        RTC_WaitForSynchro();
    }
    
    RTC_alarm_cfg();

    /* register rtc device */
    rtc.init 	= RT_NULL;
    rtc.open 	= rt_rtc_open;
    rtc.close	= RT_NULL;
    rtc.read 	= rt_rtc_read;
    rtc.write	= RT_NULL;
    rtc.control = rt_rtc_control;

    /* no private */
    rtc.user_data = RT_NULL;

    rt_device_register(&rtc, "rtc", RT_DEVICE_FLAG_RDWR);

    return;
}

#include <time.h>
#if defined (__IAR_SYSTEMS_ICC__) &&  (__VER__) >= 6020000   /* for IAR 6.2 later Compiler */
#pragma module_name = "?time"
time_t (__time32)(time_t *t)                                 /* Only supports 32-bit timestamp */
#else
// Song: modefied this line.
//time_t time(time_t* t)

time_t rt_time(time_t* t)
#endif
{
    rt_device_t device;
    time_t time=0;

    device = rt_device_find("rtc");
    if (device != RT_NULL)
    {
        rt_device_control(device, RT_DEVICE_CTRL_RTC_GET_TIME, &time);
        if (t != RT_NULL) *t = time;
    }

    return time;
}

#ifdef RT_USING_FINSH
#include <finsh.h>

void set_date(rt_uint32_t year, rt_uint32_t month, rt_uint32_t day)
{
    time_t now;
    struct tm* ti;
    rt_device_t device;

    ti = RT_NULL;
    /* get current time */
    rt_time(&now);

    ti = localtime(&now);
    if (ti != RT_NULL)
    {
        ti->tm_year = year - 1900;
        ti->tm_mon 	= month - 1; /* ti->tm_mon 	= month; */
        ti->tm_mday = day;
    }

    now = mktime(ti);

    device = rt_device_find("rtc");
    if (device != RT_NULL)
    {
        rt_rtc_control(device, RT_DEVICE_CTRL_RTC_SET_TIME, &now);
    }
    
    // Set the flag of date sync.
    RTC_WaitForSynchro();
    RTC_WriteBackupRegister(SYS_RTC_SYNC_ADDR, 0xA5A5);    
}
FINSH_FUNCTION_EXPORT(set_date, set date. e.g: set_date(2010,2,28))

void set_time(rt_uint32_t hour, rt_uint32_t minute, rt_uint32_t second)
{
    time_t now;
    struct tm* ti;
    rt_device_t device;

    ti = RT_NULL;
    /* get current time */
    rt_time(&now);

    ti = localtime(&now);
    if (ti != RT_NULL)
    {
        ti->tm_hour = hour;
        ti->tm_min 	= minute;
        ti->tm_sec 	= second;
    }

    now = mktime(ti);
    device = rt_device_find("rtc");
    if (device != RT_NULL)
    {
        rt_rtc_control(device, RT_DEVICE_CTRL_RTC_SET_TIME, &now);
    }
    
    // Set the flag of date sync.
    RTC_WaitForSynchro();
    RTC_WriteBackupRegister(SYS_RTC_SYNC_ADDR, 0xA5A5);
}
FINSH_FUNCTION_EXPORT(set_time, set time. e.g: set_time(23,59,59))




void set_time_date(time_t *time)
{
    rt_device_t device;

    device = rt_device_find("rtc");
    if (device != RT_NULL)
    {
        rt_rtc_control(device, RT_DEVICE_CTRL_RTC_SET_TIME, time);
    }
    
    // Set the flag of date sync.
    RTC_WaitForSynchro();
    RTC_WriteBackupRegister(SYS_RTC_SYNC_ADDR, 0xA5A5);

}

int time_BKR_record(uint32_t address)
{
    time_t now;
 
    if (IS_RTC_BKP(address) == 0)
    {
        return -1;
    }
    
    rt_time(&now);
    
    RTC_WaitForSynchro();
    RTC_WriteBackupRegister(address, now);
    
    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(time_BKR_record, time_record, record the date and time.)

int time_BKR_replay(uint32_t address, uint32_t *time)
{
    time_t now;
 
    if (IS_RTC_BKP(address) == 0)
    {
        return -1;
    }
   
    RTC_WaitForSynchro();
    *time = RTC_ReadBackupRegister(address);
    
    return 0;
}

int time_BKR_display(uint32_t address)
{
    time_t time;
 
    if (IS_RTC_BKP(address) == 0)
    {
        return -1;
    }
   
    RTC_WaitForSynchro();
    time = RTC_ReadBackupRegister(address);
    
    rt_kprintf("%s\n", ctime(&time));
    
    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(time_BKR_display, time_display, show the recorded date and time.)

void rt_set_UTC(time_t time)
{
    time += 8*60*60;  // East 8, add 8 hour.
    set_time_date(&time);
}

void list_date()
{
    time_t now;

    rt_time(&now);
    rt_kprintf("%s\n", ctime(&now));
}
FINSH_FUNCTION_EXPORT(list_date, show date and time.)
#endif
