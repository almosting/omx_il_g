/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------
--
--  Abstract : Encoder Wrapper Layer for H2 without interrupts
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "base_type.h"
#include "ewl.h"
#include "ewl_linux_lock.h"
#include "ewl_x280_common.h"

#include "hx280enc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <errno.h>

#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#ifndef EWL_NO_HW_TIMEOUT
#define EWL_WAIT_HW_TIMEOUT 2   /* HW IRQ timeout in seconds */
#endif

extern volatile u32 asic_status;

/*******************************************************************************
 Function name   : EWLWaitHwRdy
 Description     : Poll the encoder interrupt register to notice IRQ
 Return type     : i32 
 Argument        : void
*******************************************************************************/
i32 EWLWaitHwRdy(const void *inst, u32 *slicesReady,u32 totalsliceNumber,u32* status_register)
{
    hx280ewl_t *enc = (hx280ewl_t *) inst;
    regMapping *reg = NULL;
    volatile u32 irq_stats;
    u32 prevSlicesReady = 0;
    i32 ret = EWL_HW_WAIT_TIMEOUT;
    struct timespec t;
    u32 timeout = 1000;     /* Polling interval in microseconds */
    int loop = 500;         /* How many times to poll before timeout */
    u32 wClr;
    u32 hwId = 0;
    u32 core_id = FIRST_CORE(enc);
    u32 core_type = EWLGetCoreTypeByClientType(enc->clientType);

    assert(enc != NULL);

    PTRACE("EWLWaitHwRdy\n");
    reg = &enc->reg_all_cores[core_id].core[core_type];
    hwId = reg->pRegBase[0];

    /* The function should return when a slice is ready */
    if (slicesReady)
        prevSlicesReady = *slicesReady;

    if(timeout == (u32) (-1) )
    {
        loop = -1;   /* wait forever (almost) */
        timeout = 1000; /* 1ms polling interval */
    }

    t.tv_sec = 0;
    t.tv_nsec = timeout - t.tv_sec * 1000;
    t.tv_nsec = 100 * 1000 * 1000;

    do
    {
        /* Get the number of completed slices from ASIC registers. */
        if (slicesReady)
            *slicesReady = (reg->pRegBase[7] >> 17) & 0xFF;

       #ifdef PCIE_FPGA_VERI_LINEBUF
        /* Only for verification purpose, to test input line buffer with hardware handshake mode. */
        if (pollInputLineBufTestFunc) pollInputLineBufTestFunc();
       #endif

        irq_stats = reg->pRegBase[1];
        PTRACE("EWLWaitHw: IRQ stat = %08x\n", irq_stats);

        if((irq_stats & ASIC_STATUS_ALL))
        {            
            /* clear all IRQ bits. */
            wClr = (HW_ID_MAJOR_NUMBER(hwId) >= 0x61 || (HW_ID_MAJOR_NUMBER(hwId) == 0x60 && HW_ID_MINOR_NUMBER(hwId) >= 1)) ? irq_stats: (irq_stats & (~(ASIC_STATUS_ALL|ASIC_IRQ_LINE)));
            EWLWriteBackReg(inst, 0x04, wClr);
            ret = EWL_OK;
            loop = 0;
        }

        if (slicesReady)
        {
            if (*slicesReady > prevSlicesReady)
            {
                ret = EWL_OK;
                /*loop = 0; */
            }
        }

        if (loop)
        {
            if(nanosleep(&t, NULL) != 0)
            {
                PTRACE("EWLWaitHw: Sleep interrupted!\n");
            }
        }
    }
    while(loop--);

    *status_register=irq_stats;

    asic_status = irq_stats; /* update the buffered asic status */

    if (slicesReady)
    {
        PTRACE("EWLWaitHw: slicesReady = %d\n", *slicesReady);
    }
    PTRACE("EWLWaitHw: asic_status = %x\n", asic_status);
    PTRACE("EWLWaitHw: OK!\n");

    return ret;
}
