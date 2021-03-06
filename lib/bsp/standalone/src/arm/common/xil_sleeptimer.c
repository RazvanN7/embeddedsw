/******************************************************************************
*
* Copyright (C) 2017 - 2019 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
*
*
******************************************************************************/

/*****************************************************************************/
/**
*
* @file xil_sleeptimer.c
*
* This file provides the common helper routines for the sleep API's
*
* <pre>
* MODIFICATION HISTORY :
*
* Ver   Who  Date	 Changes
* ----- ---- -------- -------------------------------------------------------
* 6.6	srm  10/18/17 First Release.
* 6.6   srm  04/20/18 Fixed compilation warning in Xil_SleepTTCCommon API
* 7.0   mus  03/27/19 Updated XTime_StartTTCTimer to skip IOU slcr address
*                     space access, if processor is nonsecure and IOU slcr
*                     address space is secure. CR#1015725.
*
* </pre>
*****************************************************************************/

/****************************  Include Files  ********************************/

#include "xil_io.h"
#include "xil_sleeptimer.h"
#include "xtime_l.h"

/****************************  Constant Definitions  *************************/


/* Function definitions are applicable only when TTC3 is present*/
#if defined (SLEEP_TIMER_BASEADDR)
/****************************************************************************/
/**
*
* This is a helper function used by sleep/usleep APIs to
* have delay in sec/usec
*
* @param            delay - delay time in seconds/micro seconds
*
* @param            frequency - Number of counts per second/micro second
*
* @return           none
*
* @note             none
*
*****************************************************************************/
void Xil_SleepTTCCommon(u32 delay, u64 frequency)
{
	u64 tEnd = 0U;
	u64 tCur = 0U;
	XCntrVal TimeHighVal = 0U;
	XCntrVal TimeLowVal1 = 0U;
	XCntrVal TimeLowVal2 = 0U;

	TimeLowVal1 = XSleep_ReadCounterVal(SLEEP_TIMER_BASEADDR +
			XSLEEP_TIMER_TTC_COUNT_VALUE_OFFSET);
	tEnd = (u64)TimeLowVal1 + ((u64)(delay) * frequency);
	do
	{
		TimeLowVal2 = XSleep_ReadCounterVal(SLEEP_TIMER_BASEADDR +
				                  XSLEEP_TIMER_TTC_COUNT_VALUE_OFFSET);
		if (TimeLowVal2 < TimeLowVal1) {
			TimeHighVal++;
		}
		TimeLowVal1 = TimeLowVal2;
		tCur = (((u64) TimeHighVal) << XSLEEP_TIMER_REG_SHIFT) |
								(u64)TimeLowVal2;
	}while (tCur < tEnd);
}


/*****************************************************************************/
/**
*
* This API starts the Triple Timer Counter
*
* @param            none
*
* @return           none
*
* @note             none
*
*****************************************************************************/
void XTime_StartTTCTimer()
{
	u32 TimerPrescalar;
	u32 TimerCntrl;

#if (defined (__aarch64__) && EL3==1) || (defined (ARMR5) && (PROCESSOR_ACCESS_VALUE & IOU_SLCR_TZ_MASK)) || defined (ARMA53_32)
	u32 LpdRst;

#if defined (versal)
	u32 RstAddr = CRL_TTC_RST;
	u32 RstMask = CRL_TTC_BASE_RST_MASK << XSLEEP_TTC_INSTANCE;
#else
	u32 RstAddr = RST_LPD_IOU2;
	u32 RstMask = RST_LPD_IOU2_TTC_BASE_RESET_MASK << XSLEEP_TTC_INSTANCE;
#endif
	/* check if the timer is reset */
    LpdRst = XSleep_ReadCounterVal(RstAddr);
    if ((LpdRst & RstMask) != 0 ) {
    	LpdRst = LpdRst & (~RstMask);
    	Xil_Out32(RstAddr, LpdRst);
	} else {
#endif
		TimerCntrl = XSleep_ReadCounterVal(SLEEP_TIMER_BASEADDR +
					XSLEEP_TIMER_TTC_CNT_CNTRL_OFFSET);
		/* check if Timer is disabled */
		if ((TimerCntrl & XSLEEP_TIMER_TTC_CNT_CNTRL_DIS_MASK) == 0) {
		    TimerPrescalar = XSleep_ReadCounterVal(SLEEP_TIMER_BASEADDR +
					       XSLEEP_TIMER_TTC_CLK_CNTRL_OFFSET);
		/* check if Timer is configured with proper functionalty for sleep */
		   if ((TimerPrescalar & XSLEEP_TIMER_TTC_CLK_CNTRL_PS_EN_MASK) == 0)
						return;
		}
#if (defined (__aarch64__) && EL3==1) || (defined (ARMR5) && (PROCESSOR_ACCESS_VALUE & IOU_SLCR_TZ_MASK))  || defined (ARMA53_32)
	}
#endif
	/* Disable the timer to configure */
	TimerCntrl = XSleep_ReadCounterVal(SLEEP_TIMER_BASEADDR +
					XSLEEP_TIMER_TTC_CNT_CNTRL_OFFSET);
	TimerCntrl = TimerCntrl | XSLEEP_TIMER_TTC_CNT_CNTRL_DIS_MASK;
	Xil_Out32(SLEEP_TIMER_BASEADDR + XSLEEP_TIMER_TTC_CNT_CNTRL_OFFSET,
			                 TimerCntrl);
	/* Disable the prescalar */
	TimerPrescalar = XSleep_ReadCounterVal(SLEEP_TIMER_BASEADDR +
			XSLEEP_TIMER_TTC_CLK_CNTRL_OFFSET);
	TimerPrescalar = TimerPrescalar & (~XSLEEP_TIMER_TTC_CLK_CNTRL_PS_EN_MASK);
	Xil_Out32(SLEEP_TIMER_BASEADDR + XSLEEP_TIMER_TTC_CLK_CNTRL_OFFSET,
								TimerPrescalar);
	/* Enable the Timer */
	TimerCntrl = TimerCntrl & (~XSLEEP_TIMER_TTC_CNT_CNTRL_DIS_MASK);
	Xil_Out32(SLEEP_TIMER_BASEADDR + XSLEEP_TIMER_TTC_CNT_CNTRL_OFFSET,
								TimerCntrl);
}
#endif
