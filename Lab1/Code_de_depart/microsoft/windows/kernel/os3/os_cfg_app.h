/*
************************************************************************************************************************
*                                                      uC/OS-III
*                                                 The Real-Time Kernel
*
*                                  (c) Copyright 2009-2017; Micrium, Inc.; Weston, FL
*                           All rights reserved.  Protected by international copyright laws.
*
*                                       OS CONFIGURATION (APPLICATION SPECIFICS)
*
* File    : OS_CFG_APP.H
* By      : JJL
* Version : V3.06.02
*
* LICENSING TERMS:
* ---------------
*           uC/OS-III is provided in source form for FREE short-term evaluation, for educational use or 
*           for peaceful research.  If you plan or intend to use uC/OS-III in a commercial application/
*           product then, you need to contact Micrium to properly license uC/OS-III for its use in your 
*           application/product.   We provide ALL the source code for your convenience and to help you 
*           experience uC/OS-III.  The fact that the source is provided does NOT mean that you can use 
*           it commercially without paying a licensing fee.
*
*           Knowledge of the source code may NOT be used to develop a similar product.
*
*           Please help us continue to provide the embedded community with the finest software available.
*           Your honesty is greatly appreciated.
*
*           You can find our product's user manual, API reference, release notes and
*           more information at doc.micrium.com.
*           You can contact us at www.micrium.com.
************************************************************************************************************************
*/

#ifndef OS_CFG_APP_H
#define OS_CFG_APP_H

/*
************************************************************************************************************************
*                                                      CONSTANTS
************************************************************************************************************************
*/
                                                                /* ------------------ MISCELLANEOUS ------------------- */
#define  OS_CFG_ISR_STK_SIZE                         100u       /* Stack size of ISR stack (number of CPU_STK elements) */

#define  OS_CFG_MSG_POOL_SIZE                         512u       /* Maximum number of messages                           */

#define  OS_CFG_TASK_STK_LIMIT_PCT_EMPTY              10u       /* Stack limit position in percentage to empty          */


                                                                /* -------------------- IDLE TASK --------------------- */
#define  OS_CFG_IDLE_TASK_STK_SIZE                    64u       /* Stack size (number of CPU_STK elements)              */


                                                                /* ------------------ STATISTIC TASK ------------------ */
#define  OS_CFG_STAT_TASK_PRIO  ((OS_PRIO)(OS_CFG_PRIO_MAX-2u)) /* Priority                                             */
#define  OS_CFG_STAT_TASK_RATE_HZ                     10u       /* Rate of execution (1 to 10 Hz)                       */
#define  OS_CFG_STAT_TASK_STK_SIZE                   100u       /* Stack size (number of CPU_STK elements)              */


                                                                /* ---------------------- TICKS ----------------------- */
#define  OS_CFG_TICK_RATE_HZ                         100u         /* Tick rate in Hertz (10 to 1000 Hz)                   */
#define  OS_CFG_TICK_TASK_PRIO                       10u        /* Priority                                             */
#define  OS_CFG_TICK_TASK_STK_SIZE                   100u       /* Stack size (number of CPU_STK elements)              */


                                                                /* --------------------- TIMERS ----------------------- */
#define  OS_CFG_TMR_TASK_PRIO   ((OS_PRIO)(OS_CFG_PRIO_MAX-3u)) /* Priority of 'Timer Task'                             */
#define  OS_CFG_TMR_TASK_RATE_HZ                      10u       /* Rate for timers (10 Hz Typ.)                         */
#define  OS_CFG_TMR_TASK_STK_SIZE                    100u       /* Stack size (number of CPU_STK elements)              */

#endif
