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
--  Abstract :
--
------------------------------------------------------------------------------*/

#ifndef MEMALLOC_H
#define MEMALLOC_H

#include <linux/ioctl.h>

#undef PDEBUG   /* undef it, just in case */
#ifdef MEMALLOC_DEBUG
#  ifdef __KERNEL__
    /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_INFO "memalloc: " fmt, ## args)
#  else
    /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...)  /* not debugging: nothing */
#endif
/*
 * Ioctl definitions
 */
/* Use 'k' as magic number */
#define MEMALLOC_IOC_MAGIC  'k'
/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */
#define MEMALLOC_IOCXGETBUFFER    _IOWR(MEMALLOC_IOC_MAGIC, 1, MemallocParams*)
#define MEMALLOC_IOCSFREEBUFFER   _IOW(MEMALLOC_IOC_MAGIC, 2, unsigned long*)
#define MEMALLOC_IOCGMEMBASE	  _IOR(MEMALLOC_IOC_MAGIC, 3, unsigned long *)

/* ... more to come */
#define MEMALLOC_IOCHARDRESET       _IO(MEMALLOC_IOC_MAGIC, 15) /* debugging tool */
#define MEMALLOC_IOC_MAXNR 15


typedef struct {
        unsigned long busAddress;
        unsigned int size;
        unsigned long translationOffset;
} MemallocParams;

#endif /* MEMALLOC_H */
