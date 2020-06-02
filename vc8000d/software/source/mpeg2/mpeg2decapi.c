/*------------------------------------------------------------------------------
--       Copyright (c) 2015-2017, VeriSilicon Inc. All rights reserved        --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
--                                                                            --
-- This software is confidential and proprietary and may be used only as      --
--   expressly authorized by VeriSilicon in a written licensing agreement.    --
--                                                                            --
--         This entire notice must be reproduced on all copies                --
--                       and may not be removed.                              --
--                                                                            --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
--   * Redistributions of source code must retain the above copyright notice, --
--       this list of conditions and the following disclaimer.                --
--   * Redistributions in binary form must reproduce the above copyright      --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
--   * Neither the names of Google nor the names of its contributors may be   --
--       used to endorse or promote products derived from this software       --
--       without specific prior written permission.                           --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

#include <string.h>
#include "version.h"
#include "basetype.h"
#include "mpeg2hwd_cfg.h"
#include "mpeg2hwd_container.h"
#include "mpeg2hwd_utils.h"
#include "mpeg2hwd_strm.h"
#include "mpeg2decapi.h"
#include "mpeg2decapi_internal.h"
#include "dwl.h"
#include "regdrv.h"
#include "mpeg2hwd_headers.h"
#include "deccfg.h"
#include "refbuffer.h"
#include "workaround.h"
#include "bqueue.h"
#include "tiledref.h"
#include "errorhandling.h"
#include "commonconfig.h"
#include "vpufeature.h"
#include "sw_util.h"
#include "ppu.h"
#include "mpeg2hwd_debug.h"
#ifdef MPEG2_ASIC_TRACE
#include "mpeg2asicdbgtrace.h"
#endif

#ifdef MPEG2DEC_TRACE
#define MPEG2_API_TRC(str)    Mpeg2DecTrace((str))
#else
#define MPEG2_API_TRC(str)
#endif

#ifndef TRACE_PP_CTRL
#define TRACE_PP_CTRL(...)          do{}while(0)
#else
#undef TRACE_PP_CTRL
#define TRACE_PP_CTRL(...)          printf(__VA_ARGS__)
#endif

#ifdef USE_RANDOM_TEST
#include "string.h"
#include "stream_corrupt.h"
#endif

#define MPEG2_BUFFER_UNDEFINED    16

#define ID8170_DEC_TIMEOUT        0xFFU
#define ID8170_DEC_SYSTEM_ERROR   0xFEU
#define ID8170_DEC_HW_RESERVED    0xFDU

#define MPEG2DEC_UPDATE_POUTPUT \
    dec_cont->MbSetDesc.out_data.data_left = \
    DEC_STRM.p_strm_buff_start - dec_cont->MbSetDesc.out_data.strm_curr_pos; \
    (void) DWLmemcpy(output, &dec_cont->MbSetDesc.out_data, \
                             sizeof(Mpeg2DecOutput))
#define NON_B_BUT_B_ALLOWED \
   !dec_cont->Hdrs.low_delay && dec_cont->FrameDesc.pic_coding_type != BFRAME

#define MPEG2DEC_IS_FIELD_OUTPUT \
    dec_cont->Hdrs.interlaced && !dec_cont->pp_config_query.deinterlace

#define MPEG2DEC_NON_PIPELINE_AND_B_PICTURE \
    ((!dec_cont->pp_config_query.pipeline_accepted || dec_cont->Hdrs.interlaced) \
    && dec_cont->FrameDesc.pic_coding_type == BFRAME)
void mpeg2RefreshRegs(Mpeg2DecContainer * dec_cont);
void mpeg2FlushRegs(Mpeg2DecContainer * dec_cont);
static u32 mpeg2HandleVlcModeError(Mpeg2DecContainer * dec_cont, u32 pic_num);
static void mpeg2HandleFrameEnd(Mpeg2DecContainer * dec_cont);
static u32 RunDecoderAsic(Mpeg2DecContainer * dec_cont, addr_t strm_bus_address);
static void Mpeg2FillPicStruct(Mpeg2DecPicture * picture,
                               Mpeg2DecContainer * dec_cont, u32 pic_index);
static u32 Mpeg2SetRegs(Mpeg2DecContainer * dec_cont, addr_t strm_bus_address);
static u32 Mp2CheckFormatSupport(void);
static void Mpeg2CheckReleasePpAndHw(Mpeg2DecContainer *dec_cont);
static Mpeg2DecRet Mpeg2DecNextPicture_INTERNAL(Mpeg2DecInst dec_inst,
    Mpeg2DecPicture * picture, u32 end_of_stream);

static void Mpeg2SetExternalBufferInfo(Mpeg2DecInst dec_inst);

static void Mpeg2EnterAbortState(Mpeg2DecContainer *dec_cont);
static void Mpeg2ExistAbortState(Mpeg2DecContainer *dec_cont);
static void Mpeg2EmptyBufferQueue(Mpeg2DecContainer *dec_cont);
static void Mpeg2CheckBufferRealloc(Mpeg2DecContainer *dec_cont);
/*------------------------------------------------------------------------------
       Version Information - DO NOT CHANGE!
------------------------------------------------------------------------------*/

#define MPEG2DEC_MAJOR_VERSION 1
#define MPEG2DEC_MINOR_VERSION 2

#define DEC_DPB_NOT_INITIALIZED      -1
/*------------------------------------------------------------------------------

    Function: Mpeg2DecGetAPIVersion

        Functional description:
            Return version information of API

        Inputs:
            none

        Outputs:
            none

        Returns:
            API version

------------------------------------------------------------------------------*/

Mpeg2DecApiVersion Mpeg2DecGetAPIVersion() {
  Mpeg2DecApiVersion ver;

  ver.major = MPEG2DEC_MAJOR_VERSION;
  ver.minor = MPEG2DEC_MINOR_VERSION;

  return (ver);
}

/*------------------------------------------------------------------------------

    Function: Mpeg2DecGetBuild

        Functional description:
            Return build information of SW and HW

        Returns:
            Mpeg2DecBuild

------------------------------------------------------------------------------*/

Mpeg2DecBuild Mpeg2DecGetBuild(void) {
  Mpeg2DecBuild build_info;

  (void)DWLmemset(&build_info, 0, sizeof(build_info));

  build_info.sw_build = HANTRO_DEC_SW_BUILD;
  build_info.hw_build = DWLReadAsicID(DWL_CLIENT_TYPE_MPEG2_DEC);

  DWLReadAsicConfig(build_info.hw_config, DWL_CLIENT_TYPE_MPEG2_DEC);

  MPEG2_API_TRC("Mpeg2DecGetBuild# OK\n");

  return build_info;
}

/*------------------------------------------------------------------------------

    Function: Mpeg2DecInit()

        Functional description:
            Initialize decoder software. Function reserves memory for the
            decoder instance.

        Inputs:

        Outputs:
            dec_inst        pointer to initialized instance is stored here

        Returns:
            MPEG2DEC_OK       successfully initialized the instance
            MPEG2DEC_MEM_FAIL memory allocation failed

------------------------------------------------------------------------------*/
Mpeg2DecRet Mpeg2DecInit(Mpeg2DecInst * dec_inst,
                         const void *dwl,
                         enum DecErrorHandling error_handling,
                         u32 num_frame_buffers,
                         enum DecDpbFlags dpb_flags,
                         u32 use_adaptive_buffers,
                         u32 n_guard_size) {
  /*@null@ */ Mpeg2DecContainer *dec_cont;
  u32 i = 0;
  u32 asic_id, hw_build_id;
  struct DecHwFeatures hw_feature;

  DWLHwConfig config;
  u32 reference_frame_format;
  MPEG2_API_TRC("Mpeg2DecInit#");
  MPEG2DEC_API_DEBUG(("Mpeg2API_DecoderInit#"));
  MPEG2FLUSH;

  /* check that right shift on negative numbers is performed signed */
  /*lint -save -e* following check causes multiple lint messages */
  if(((-1) >> 1) != (-1)) {
    MPEG2DEC_API_DEBUG(("MPEG2DecInit# ERROR: Right shift is not signed"));
    MPEG2FLUSH;
    return (MPEG2DEC_INITFAIL);
  }
  /*lint -restore */

  if(dec_inst == NULL) {
    MPEG2DEC_API_DEBUG(("MPEG2DecInit# ERROR: dec_inst == NULL"));
    MPEG2FLUSH;
    return (MPEG2DEC_PARAM_ERROR);
  }

  *dec_inst = NULL;

  /* check that MPEG-2 decoding supported in HW */
  if(Mp2CheckFormatSupport()) {
    MPEG2DEC_API_DEBUG(("MPEG2DecInit# ERROR: MPEG-2 not supported in HW\n"));
    return MPEG2DEC_FORMAT_NOT_SUPPORTED;
  }

  dec_cont = (Mpeg2DecContainer *) DWLmalloc(sizeof(Mpeg2DecContainer));
  MPEG2FLUSH;

  if(dec_cont == NULL) {
    return (MPEG2DEC_MEMFAIL);
  }

  /* set everything initially zero */
  (void) DWLmemset(dec_cont, 0, sizeof(Mpeg2DecContainer));

  pthread_mutex_init(&dec_cont->protect_mutex, NULL);
  dec_cont->dwl = dwl;

  mpeg2API_InitDataStructures(dec_cont);

  dec_cont->ApiStorage.DecStat = INITIALIZED;
  dec_cont->ApiStorage.first_field = 1;

  *dec_inst = (Mpeg2DecContainer *) dec_cont;

  if( num_frame_buffers > 16 )
    num_frame_buffers = 16;
  dec_cont->StrmStorage.max_num_buffers = num_frame_buffers;

  asic_id = DWLReadAsicID(DWL_CLIENT_TYPE_MPEG2_DEC);
  if((asic_id >> 16) == 0x8170U)
    error_handling = 0;

  dec_cont->mpeg2_regs[0] = asic_id;
  for(i = 1; i < TOTAL_X170_REGISTERS; i++)
    dec_cont->mpeg2_regs[i] = 0;

  SetCommonConfigRegs(dec_cont->mpeg2_regs);

  (void) DWLmemset(&config, 0, sizeof(DWLHwConfig));

  DWLReadAsicConfig(&config, DWL_CLIENT_TYPE_MPEG2_DEC);
  hw_build_id = DWLReadHwBuildID(DWL_CLIENT_TYPE_MPEG2_DEC);
  GetReleaseHwFeaturesByID(hw_build_id, &hw_feature);

  if(!hw_feature.addr64_support && sizeof(addr_t) == 8) {
    MPEG2DEC_API_DEBUG(("MPEG2DecInit# ERROR: HW not support 64bit address!\n"));
    return (MPEG2DEC_PARAM_ERROR);
  }

  if (hw_feature.ref_frame_tiled_only)
    dpb_flags = DEC_REF_FRM_TILED_DEFAULT | DEC_DPB_ALLOW_FIELD_ORDERING;

  dec_cont->ref_buf_support = hw_feature.ref_buf_support;
  reference_frame_format = dpb_flags & DEC_REF_FRM_FMT_MASK;
  if(reference_frame_format == DEC_REF_FRM_TILED_DEFAULT) {
    /* Assert support in HW before enabling.. */
    if(!hw_feature.tiled_mode_support) {
      return MPEG2DEC_FORMAT_NOT_SUPPORTED;
    }
    dec_cont->tiled_mode_support = hw_feature.tiled_mode_support;
  } else
    dec_cont->tiled_mode_support = 0;
  if (hw_feature.strm_len_32bits)
    dec_cont->max_strm_len = DEC_X170_MAX_STREAM_VC8000D;
  else
    dec_cont->max_strm_len = DEC_X170_MAX_STREAM;
#if 0
  /* Down scaler ratio */
  if ((dscale_cfg->down_scale_x == 0) || (dscale_cfg->down_scale_y == 0)) {
    dec_cont->pp_enabled = 0;
    dec_cont->down_scale_x = 0;   /* Meaningless when pp not enabled. */
    dec_cont->down_scale_y = 0;
  } else if ((dscale_cfg->down_scale_x != 1 &&
              dscale_cfg->down_scale_x != 2 &&
              dscale_cfg->down_scale_x != 4 &&
              dscale_cfg->down_scale_x != 8 ) ||
             (dscale_cfg->down_scale_y != 1 &&
              dscale_cfg->down_scale_y != 2 &&
              dscale_cfg->down_scale_y != 4 &&
              dscale_cfg->down_scale_y != 8 )) {
    return (MPEG2DEC_PARAM_ERROR);
  } else {
    u32 scale_table[9] = {0, 0, 1, 0, 2, 0, 0, 0, 3};

    dec_cont->pp_enabled = 1;
    dec_cont->down_scale_x = dscale_cfg->down_scale_x;
    dec_cont->down_scale_y = dscale_cfg->down_scale_y;
    dec_cont->dscale_shift_x = scale_table[dscale_cfg->down_scale_x];
    dec_cont->dscale_shift_y = scale_table[dscale_cfg->down_scale_y];
  }
#endif
  dec_cont->pp_buffer_queue = InputQueueInit(0);
  if (dec_cont->pp_buffer_queue == NULL) {
    return (MPEG2DEC_MEMFAIL);
  }
  dec_cont->StrmStorage.release_buffer = 0;

  /* Custom DPB modes require tiled support >= 2 */
  dec_cont->allow_dpb_field_ordering = 0;
  dec_cont->dpb_mode = DEC_DPB_NOT_INITIALIZED;
  if( dpb_flags & DEC_DPB_ALLOW_FIELD_ORDERING )
    dec_cont->allow_dpb_field_ordering = hw_feature.field_dpb_support;

  dec_cont->StrmStorage.intra_freeze = error_handling == DEC_EC_VIDEO_FREEZE;
  if (error_handling == DEC_EC_PARTIAL_FREEZE)
    dec_cont->StrmStorage.partial_freeze = 1;
  else if (error_handling == DEC_EC_PARTIAL_IGNORE)
    dec_cont->StrmStorage.partial_freeze = 2;
  dec_cont->StrmStorage.picture_broken = HANTRO_FALSE;

  InitWorkarounds(MPEG2_DEC_X170_MODE_MPEG2, &dec_cont->workarounds);
  /* take top/botom fields into consideration */
  if (FifoInit(32, &dec_cont->fifo_display) != FIFO_OK)
    return MPEG2DEC_MEMFAIL;

  dec_cont->use_adaptive_buffers = use_adaptive_buffers;
  dec_cont->n_guard_size = n_guard_size;

  /* If tile mode is enabled, should take DTRC minimum size(96x48) into consideration */
  if (dec_cont->tiled_mode_support) {
    dec_cont->min_dec_pic_width = MPEG2_MIN_WIDTH_EN_DTRC;
    dec_cont->min_dec_pic_height = MPEG2_MIN_HEIGHT_EN_DTRC;
  }
  else {
    dec_cont->min_dec_pic_width = MPEG2_MIN_WIDTH;
    dec_cont->min_dec_pic_height = MPEG2_MIN_HEIGHT;
  }

#ifdef USE_RANDOM_TEST
  /*********************************************************/
  /** Developers can change below parameters to generate  **/
  /** different kinds of error stream.                    **/
  /*********************************************************/
  dec_cont->error_params.seed = 88;
  strcpy(dec_cont->error_params.truncate_stream_odds , "1 : 5");
  strcpy(dec_cont->error_params.swap_bit_odds, "1 : 100000");
  strcpy(dec_cont->error_params.packet_loss_odds, "1 : 5");
  /*********************************************************/

  if (strcmp(dec_cont->error_params.swap_bit_odds, "0") != 0)
    dec_cont->error_params.swap_bits_in_stream = 0;

  if (strcmp(dec_cont->error_params.packet_loss_odds, "0") != 0)
    dec_cont->error_params.lose_packets = 1;

  if (strcmp(dec_cont->error_params.truncate_stream_odds, "0") != 0)
    dec_cont->error_params.truncate_stream = 1;

  if (dec_cont->error_params.swap_bits_in_stream ||
      dec_cont->error_params.lose_packets ||
      dec_cont->error_params.truncate_stream) {
    dec_cont->error_params.random_error_enabled = 1;
    InitializeRandom(dec_cont->error_params.seed);
  }
#endif
#ifdef DUMP_INPUT_STREAM
  dec_cont->ferror_stream = fopen("random_error.mpeg2", "wb");
  if(dec_cont->ferror_stream == NULL) {
    MPEG2DEC_DEBUG(("Unable to open file error.mpeg2\n"));
    return MPEG2DEC_MEMFAIL;
  }
#endif

  MPEG2DEC_API_DEBUG(("Container %p\n", (void *)dec_cont));
  MPEG2FLUSH;
  MPEG2_API_TRC("Mpeg2DecInit: OK\n");

  return (MPEG2DEC_OK);
}

/*------------------------------------------------------------------------------

    Function: Mpeg2DecGetInfo()

        Functional description:
            This function provides read access to decoder information. This
            function should not be called before Mpeg2DecDecode function has
            indicated that headers are ready.

        Inputs:
            dec_inst     decoder instance

        Outputs:
            dec_info    pointer to info struct where data is written

        Returns:
            MPEG2DEC_OK            success
            MPEG2DEC_PARAM_ERROR     invalid parameters

------------------------------------------------------------------------------*/
Mpeg2DecRet Mpeg2DecGetInfo(Mpeg2DecInst dec_inst, Mpeg2DecInfo * dec_info) {

#define API_STOR ((Mpeg2DecContainer *)dec_inst)->ApiStorage
#define DEC_FRAMED ((Mpeg2DecContainer *)dec_inst)->FrameDesc
#define DEC_STRM ((Mpeg2DecContainer *)dec_inst)->StrmDesc
#define DEC_STST ((Mpeg2DecContainer *)dec_inst)->StrmStorage
#define DEC_HDRS ((Mpeg2DecContainer *)dec_inst)->Hdrs
#define DEC_REGS ((Mpeg2DecContainer *)dec_inst)->mpeg2_regs

  MPEG2_API_TRC("Mpeg2DecGetInfo#");

  if(dec_inst == NULL || dec_info == NULL) {
    return MPEG2DEC_PARAM_ERROR;
  }

  if(API_STOR.DecStat == UNINIT || API_STOR.DecStat == INITIALIZED) {
    return MPEG2DEC_HDRS_NOT_RDY;
  }

  dec_info->frame_width = DEC_FRAMED.frame_width << 4;
  dec_info->frame_height = DEC_FRAMED.frame_height << 4;
  dec_info->coded_width = DEC_HDRS.horizontal_size;
  dec_info->coded_height = DEC_HDRS.vertical_size;

  dec_info->profile_and_level_indication = DEC_HDRS.profile_and_level_indication;
  dec_info->colour_description_present_flag = DEC_HDRS.color_description;
  dec_info->video_range = DEC_HDRS.video_range;
  dec_info->colour_primaries = DEC_HDRS.color_primaries;
  dec_info->transfer_characteristics = DEC_HDRS.transfer_characteristics;
  dec_info->matrix_coefficients = DEC_HDRS.matrix_coefficients;
  dec_info->video_format = DEC_HDRS.video_format;
  dec_info->stream_format = DEC_HDRS.mpeg2_stream;
  dec_info->interlaced_sequence = DEC_HDRS.interlaced;
  dec_info->pic_buff_size = 3;
  /*dec_info->multi_buff_pp_size = DEC_HDRS.interlaced ? 1 : 2;*/
  dec_info->multi_buff_pp_size = 2;

  mpeg2DecAspectRatio((Mpeg2DecContainer *) dec_inst, dec_info);

  dec_info->dpb_mode = ((Mpeg2DecContainer *)dec_inst)->dpb_mode;

  if(((Mpeg2DecContainer *)dec_inst)->tiled_mode_support) {
    if(DEC_HDRS.interlaced &&
        (dec_info->dpb_mode != DEC_DPB_INTERLACED_FIELD)) {
      dec_info->output_format = MPEG2DEC_SEMIPLANAR_YUV420;
    } else {
      dec_info->output_format = MPEG2DEC_TILED_YUV420;
    }
  } else {
    dec_info->output_format = MPEG2DEC_SEMIPLANAR_YUV420;
  }

  MPEG2_API_TRC("Mpeg2DecGetInfo: OK");
  return (MPEG2DEC_OK);

#undef API_STOR
#undef DEC_STRM
#undef DEC_FRAMED
#undef DEC_STST
#undef DEC_HDRS
}

/*------------------------------------------------------------------------------

    Function: Mpeg2DecDecode

        Functional description:
            Decode stream data. Calls StrmDec_Decode to do the actual decoding.

        Input:
            dec_inst     decoder instance
            input      pointer to input struct

        Outputs:
            output     pointer to output struct

        Returns:
            MPEG2DEC_NOT_INITIALIZED   decoder instance not initialized yet
            MPEG2DEC_PARAM_ERROR       invalid parameters

            MPEG2DEC_STRM_PROCESSED    stream buffer decoded
            MPEG2DEC_HDRS_RDY          headers decoded
            MPEG2DEC_PIC_DECODED       decoding of a picture finished
            MPEG2DEC_STRM_ERROR        serious error in decoding, no
                                       valid parameter sets available
                                       to decode picture data

------------------------------------------------------------------------------*/

Mpeg2DecRet Mpeg2DecDecode(Mpeg2DecInst dec_inst,
                           Mpeg2DecInput * input, Mpeg2DecOutput * output) {
#define API_STOR ((Mpeg2DecContainer *)dec_inst)->ApiStorage
#define DEC_STRM ((Mpeg2DecContainer *)dec_inst)->StrmDesc
#define DEC_FRAMED ((Mpeg2DecContainer *)dec_inst)->FrameDesc

  Mpeg2DecContainer *dec_cont;
  Mpeg2DecRet internal_ret;
  u32 strm_dec_result;
  u32 asic_status;
  i32 ret = 0;
  u32 field_rdy = 0;
  u32 error_concealment = 0;
  u32 input_data_len;
  u32 hw_build_id = DWLReadHwBuildID(DWL_CLIENT_TYPE_MPEG2_DEC);
  struct DecHwFeatures hw_feature;

  GetReleaseHwFeaturesByID(hw_build_id, &hw_feature);

  MPEG2_API_TRC("\nMpeg2_dec_decode#");

  if(input == NULL || output == NULL || dec_inst == NULL) {
    MPEG2_API_TRC("Mpeg2DecDecode# PARAM_ERROR\n");
    return MPEG2DEC_PARAM_ERROR;
  }

  dec_cont = ((Mpeg2DecContainer *) dec_inst);

  /*
   *  Check if decoder is in an incorrect mode
   */
  if(API_STOR.DecStat == UNINIT) {

    MPEG2_API_TRC("Mpeg2DecDecode: NOT_INITIALIZED\n");
    return MPEG2DEC_NOT_INITIALIZED;
  }

  input_data_len = input->data_len;

#ifdef USE_RANDOM_TEST
  if (dec_cont->error_params.random_error_enabled) {
    // error type: lose packets;
    if (dec_cont->error_params.lose_packets && !dec_cont->stream_not_consumed) {
      u8 lose_packet = 0;
      if (RandomizePacketLoss(dec_cont->error_params.packet_loss_odds,
                              &lose_packet)) {
        MPEG2DEC_DEBUG(("Packet loss simulator error (wrong config?)\n"));
      }
      if (lose_packet) {
        input_data_len = 0;
        output->data_left = 0;
        dec_cont->stream_not_consumed = 0;
        return MPEG2DEC_STRM_PROCESSED;
      }
    }

    // error type: truncate stream(random len for random packet);
    if (dec_cont->error_params.truncate_stream && !dec_cont->stream_not_consumed) {
      u8 truncate_stream = 0;
      if (RandomizePacketLoss(dec_cont->error_params.truncate_stream_odds,
                              &truncate_stream)) {
        MPEG2DEC_DEBUG(("Truncate stream simulator error (wrong config?)\n"));
      }
      if (truncate_stream) {
        u32 random_size = input_data_len;
        if (RandomizeU32(&random_size)) {
          MPEG2DEC_DEBUG(("Truncate randomizer error (wrong config?)\n"));
        }
        input_data_len = random_size;
      }

      dec_cont->prev_input_len = input_data_len;

      if (input_data_len == 0) {
        output->data_left = 0;
        dec_cont->stream_not_consumed = 0;
        return MPEG2DEC_STRM_PROCESSED;
      }
    }

    /*  stream is truncated but not consumed at first time, the same truncated length
        at the second time */
    if (dec_cont->error_params.truncate_stream && dec_cont->stream_not_consumed)
      input_data_len = dec_cont->prev_input_len;

    // error type: swap bits;
    if (dec_cont->error_params.swap_bits_in_stream && !dec_cont->stream_not_consumed) {
      u8 *p_tmp = (u8 *)input->stream;
      /* mpeg2 don't have ringbuffer, just send zero/null parameters*/
      if (RandomizeBitSwapInStream(p_tmp, NULL, 0, input_data_len,
                                   dec_cont->error_params.swap_bit_odds, 0)) {
        MPEG2DEC_DEBUG(("Bitswap randomizer error (wrong config?)\n"));
      }
    }
  }
#endif

  if(input->data_len == 0 ||
      input->data_len > dec_cont->max_strm_len ||
      input->stream == NULL || input->stream_bus_address == 0) {
    MPEG2_API_TRC("Mpeg2DecDecode# PARAM_ERROR\n");
    return MPEG2DEC_PARAM_ERROR;
  }
  /* If we have set up for delayed resolution change, do it here */
  if(dec_cont->StrmStorage.new_headers_change_resolution) {
#ifndef USE_OMXIL_BUFFER
    BqueueWaitNotInUse(&dec_cont->StrmStorage.bq);
    if (dec_cont->pp_enabled)
      InputQueueWaitNotUsed(dec_cont->pp_buffer_queue);
#endif
    dec_cont->StrmStorage.new_headers_change_resolution = 0;
    dec_cont->Hdrs.horizontal_size = dec_cont->tmp_hdrs.horizontal_size;
    dec_cont->Hdrs.vertical_size = dec_cont->tmp_hdrs.vertical_size;
    /* Set rest of parameters just in case */
    dec_cont->Hdrs.aspect_ratio_info = dec_cont->tmp_hdrs.aspect_ratio_info;
    dec_cont->Hdrs.frame_rate_code = dec_cont->tmp_hdrs.frame_rate_code;
    dec_cont->Hdrs.bit_rate_value = dec_cont->tmp_hdrs.bit_rate_value;
    dec_cont->Hdrs.vbv_buffer_size = dec_cont->tmp_hdrs.vbv_buffer_size;
    dec_cont->Hdrs.constr_parameters = dec_cont->tmp_hdrs.constr_parameters;
    dec_cont->Hdrs.frame_rate_code = dec_cont->tmp_hdrs.frame_rate_code;
    dec_cont->Hdrs.aspect_ratio_info = dec_cont->tmp_hdrs.aspect_ratio_info;
    dec_cont->FrameDesc.frame_width = (dec_cont->tmp_hdrs.horizontal_size + 15) >> 4;
    dec_cont->FrameDesc.frame_height = (dec_cont->tmp_hdrs.vertical_size + 15) >> 4;
    dec_cont->FrameDesc.total_mb_in_frame =
      dec_cont->FrameDesc.frame_width * dec_cont->FrameDesc.frame_height;

  }

  if (API_STOR.DecStat == HEADERSDECODED) {
    /* check if buffer need to be realloced, both external buffer and internal buffer */
    Mpeg2CheckBufferRealloc(dec_cont);
    if (!dec_cont->pp_enabled) {
      if (dec_cont->realloc_ext_buf) {
#ifndef USE_OMXIL_BUFFER
        BqueueWaitNotInUse(&dec_cont->StrmStorage.bq);
#endif
        if (dec_cont->StrmStorage.ext_buffer_added) {
          dec_cont->StrmStorage.release_buffer = 1;
          ret = MPEG2DEC_WAITING_FOR_BUFFER;
        }

        mpeg2FreeBuffers(dec_cont);
        if (!dec_cont->ApiStorage.p_qtable_base.virtual_address) {
          MPEG2DEC_DEBUG(("Allocate buffers\n"));
          MPEG2FLUSH;
          internal_ret = mpeg2AllocateBuffers(dec_cont);
          /* Reset frame number to ensure PP doesn't run in non-pipeline
           * mode during 1st pic of new headers. */
          dec_cont->FrameDesc.frame_number = 0;
          if (internal_ret != MPEG2DEC_OK) {
            MPEG2DEC_DEBUG(("ALLOC BUFFER FAIL\n"));
            MPEG2_API_TRC("Mpeg2DecDecode# MEMFAIL\n");
            return (internal_ret);
          }
        }
      }
    } else {
      if (dec_cont->realloc_ext_buf) {
#ifndef USE_OMXIL_BUFFER
        InputQueueWaitNotUsed(dec_cont->pp_buffer_queue);
#endif
        if (dec_cont->StrmStorage.ext_buffer_added) {
          dec_cont->StrmStorage.release_buffer = 1;
          ret = MPEG2DEC_WAITING_FOR_BUFFER;
        }
      }

      if (dec_cont->realloc_int_buf) {
        mpeg2FreeBuffers(dec_cont);
        if(!dec_cont->ApiStorage.p_qtable_base.virtual_address) {
          MPEG2DEC_DEBUG(("Allocate buffers\n"));
          MPEG2FLUSH;
          internal_ret = mpeg2AllocateBuffers(dec_cont);
          /* Reset frame number to ensure PP doesn't run in non-pipeline
           * mode during 1st pic of new headers. */
          dec_cont->FrameDesc.frame_number = 0;
          if(internal_ret != MPEG2DEC_OK) {
            MPEG2DEC_DEBUG(("ALLOC BUFFER FAIL\n"));
            MPEG2_API_TRC("Mpeg2DecDecode# MEMFAIL\n");
            return (internal_ret);
          }
        }
      }
    }
  }

  /*
   *  Update stream structure
   */
  DEC_STRM.p_strm_buff_start = input->stream;
  DEC_STRM.strm_curr_pos = input->stream;
  DEC_STRM.bit_pos_in_word = 0;
  DEC_STRM.strm_buff_size = input_data_len;
  DEC_STRM.strm_buff_read_bits = 0;

#ifdef _DEC_PP_USAGE
  dec_cont->StrmStorage.latest_id = input->pic_id;
#endif
  do {
    MPEG2DEC_API_DEBUG(("Start Decode\n"));
    /* run SW if HW is not in the middle of processing a picture
     * (indicated as HW_PIC_STARTED decoder status) */
    if(API_STOR.DecStat == HEADERSDECODED) {
      API_STOR.DecStat = STREAMDECODING;
      if (dec_cont->realloc_ext_buf) {
        dec_cont->buffer_index = 0;
        Mpeg2SetExternalBufferInfo(dec_cont);
        ret =  MPEG2DEC_WAITING_FOR_BUFFER;
      }
    } else if (dec_cont->buffer_index < dec_cont->ext_min_buffer_num) {
      ret = MPEG2DEC_WAITING_FOR_BUFFER;
    } else if (API_STOR.DecStat != HW_PIC_STARTED) {
      strm_dec_result = mpeg2StrmDec_Decode(dec_cont);
      if (dec_cont->unpaired_field ||
          (strm_dec_result != DEC_PIC_HDR_RDY &&
           strm_dec_result != DEC_END_OF_STREAM)) {
        Mpeg2CheckReleasePpAndHw(dec_cont);
        dec_cont->unpaired_field = 0;
      }

      switch (strm_dec_result) {
      case DEC_PIC_HDR_RDY:
        /* if type inter predicted and no reference -> error */
        if((dec_cont->Hdrs.picture_coding_type == PFRAME &&
            dec_cont->ApiStorage.first_field &&
            dec_cont->StrmStorage.work0 == INVALID_ANCHOR_PICTURE) ||
            (dec_cont->Hdrs.picture_coding_type == BFRAME &&
             (dec_cont->StrmStorage.work0 == INVALID_ANCHOR_PICTURE ||
              (dec_cont->StrmStorage.work1 == INVALID_ANCHOR_PICTURE &&
               dec_cont->Hdrs.closed_gop == 0) ||
              dec_cont->StrmStorage.skip_b ||
              input->skip_non_reference)) ||
            (dec_cont->Hdrs.picture_coding_type == PFRAME &&
             dec_cont->StrmStorage.picture_broken &&
             dec_cont->StrmStorage.intra_freeze)) {
          if(dec_cont->StrmStorage.skip_b ||
              input->skip_non_reference) {
            MPEG2_API_TRC("Mpeg2DecDecode# MPEG2DEC_NONREF_PIC_SKIPPED\n");
          }
          if (!dec_cont->ApiStorage.first_field && dec_cont->pp_enabled)
            InputQueueReturnBuffer(dec_cont->pp_buffer_queue, dec_cont->StrmStorage.p_pic_buf[dec_cont->StrmStorage.work_out].pp_data->virtual_address);
          ret = mpeg2HandleVlcModeError(dec_cont, input->pic_id);
          error_concealment = HANTRO_TRUE;
          Mpeg2CheckReleasePpAndHw(dec_cont);
        } else
          API_STOR.DecStat = HW_PIC_STARTED;

        /* Initialize DPB mode */
        if ((!dec_cont->Hdrs.progressive_sequence ||
             !dec_cont->Hdrs.frame_pred_frame_dct
            ) && dec_cont->allow_dpb_field_ordering)
          dec_cont->dpb_mode = DEC_DPB_INTERLACED_FIELD;
        else
          dec_cont->dpb_mode = DEC_DPB_FRAME;
        break;

      case DEC_PIC_SUPRISE_B:
        /* Handle suprise B */
        internal_ret = mpeg2DecAllocExtraBPic(dec_cont);
        if(internal_ret != MPEG2DEC_OK) {
          MPEG2_API_TRC
          ("Mpeg2DecDecode# MEMFAIL Mpeg2DecAllocExtraBPic\n");
          return (internal_ret);
        }

        dec_cont->Hdrs.low_delay = 0;

        mpeg2DecBufferPicture(dec_cont,
                              input->pic_id, 1, 0,
                              MPEG2DEC_PIC_DECODED, 0);

        ret = mpeg2HandleVlcModeError(dec_cont, input->pic_id);
        error_concealment = HANTRO_TRUE;
        /* copy output parameters for this PIC */
        MPEG2DEC_UPDATE_POUTPUT;
        break;

      case DEC_PIC_HDR_RDY_ERROR:
        if (!dec_cont->ApiStorage.first_field && dec_cont->pp_enabled)
          InputQueueReturnBuffer(dec_cont->pp_buffer_queue, dec_cont->StrmStorage.p_pic_buf[dec_cont->StrmStorage.work_out].pp_data->virtual_address);
        ret = mpeg2HandleVlcModeError(dec_cont, input->pic_id);
        error_concealment = HANTRO_TRUE;
        /* copy output parameters for this PIC */
        MPEG2DEC_UPDATE_POUTPUT;
        break;

      case DEC_HDRS_RDY:
        internal_ret = mpeg2DecCheckSupport(dec_cont);
        if(internal_ret != MPEG2DEC_OK) {
          dec_cont->StrmStorage.strm_dec_ready = FALSE;
          dec_cont->StrmStorage.valid_sequence = 0;
          API_STOR.DecStat = INITIALIZED;
#ifdef DUMP_INPUT_STREAM
          fwrite(input->stream, 1, input_data_len, dec_cont->ferror_stream);
#endif
#ifdef USE_RANDOM_TEST
          dec_cont->stream_not_consumed = 0;
#endif
          return internal_ret;
        }
        /* reset after DEC_HDRS_RDY */
        dec_cont->StrmStorage.error_in_hdr = 0;

        if(dec_cont->ApiStorage.first_headers) {
          dec_cont->ApiStorage.first_headers = 0;

          if (!hw_feature.pic_size_reg_unified) {
            SetDecRegister(dec_cont->mpeg2_regs, HWIF_PIC_MB_WIDTH,
                           dec_cont->FrameDesc.frame_width);
          } else {
            SetDecRegister(dec_cont->mpeg2_regs, HWIF_PIC_WIDTH_IN_CBS,
                           dec_cont->FrameDesc.frame_width << 1);
          }

          /* check the decoding mode */
          if(dec_cont->Hdrs.mpeg2_stream) {
            SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_MODE,
                           MPEG2_DEC_X170_MODE_MPEG2);
          } else {
            SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_MODE,
                           MPEG2_DEC_X170_MODE_MPEG1);
          }

          if(dec_cont->ref_buf_support) {
            RefbuInit(&dec_cont->ref_buffer_ctrl,
                      MPEG2_DEC_X170_MODE_MPEG2,
                      dec_cont->FrameDesc.frame_width,
                      dec_cont->FrameDesc.frame_height,
                      dec_cont->ref_buf_support);
          }
        }

        /* Initialize DPB mode */
        if( (!dec_cont->Hdrs.progressive_sequence ||
             !dec_cont->Hdrs.frame_pred_frame_dct
            ) && dec_cont->allow_dpb_field_ordering )
          dec_cont->dpb_mode = DEC_DPB_INTERLACED_FIELD;
        else
          dec_cont->dpb_mode = DEC_DPB_FRAME;

        /* Initialize tiled mode */
        if( dec_cont->tiled_mode_support) {
          /* Check mode validity */
          if(DecCheckTiledMode( dec_cont->tiled_mode_support,
                                dec_cont->dpb_mode,
                                !dec_cont->Hdrs.progressive_sequence ||
                                  !dec_cont->Hdrs.frame_pred_frame_dct
                              ) != HANTRO_OK ) {
            MPEG2_API_TRC("Mpeg2DecDecode# ERROR: DPB mode does not "\
                          "support tiled reference pictures");
            return MPEG2DEC_PARAM_ERROR;
          }
        }

        /* Handle MPEG-1 parameters */
        if(dec_cont->Hdrs.mpeg2_stream == MPEG1) {
          mpeg2HandleMpeg1Parameters(dec_cont);
        }

        API_STOR.DecStat = HEADERSDECODED;
        if (dec_cont->pp_enabled) {
          dec_cont->prev_pp_width = dec_cont->ppu_cfg[0].scale.width;
          dec_cont->prev_pp_height = dec_cont->ppu_cfg[0].scale.height;
        }

        MPEG2DEC_API_DEBUG(("HDRS_RDY\n"));
        FifoPush(dec_cont->fifo_display, (FifoObject)-2, FIFO_EXCEPTION_DISABLE);
        ret = MPEG2DEC_HDRS_RDY;
        break;

      default:
        ASSERT(strm_dec_result == DEC_END_OF_STREAM);
        if(dec_cont->StrmStorage.new_headers_change_resolution) {
          ret = MPEG2DEC_PIC_DECODED;
        } else if (field_rdy) {
          ret = MPEG2DEC_BUF_EMPTY;
        } else {
          ret = MPEG2DEC_STRM_PROCESSED;
        }
        break;
      }
    }

    /* picture header properly decoded etc -> start HW */
    if(API_STOR.DecStat == HW_PIC_STARTED) {
      if(dec_cont->ApiStorage.first_field &&
          !dec_cont->asic_running) {
        dec_cont->StrmStorage.work_out_prev =
          dec_cont->StrmStorage.work_out;
        dec_cont->StrmStorage.work_out = BqueueNext2(
                                           &dec_cont->StrmStorage.bq,
                                           dec_cont->StrmStorage.work0,
                                           dec_cont->Hdrs.low_delay ? BQUEUE_UNUSED : dec_cont->StrmStorage.work1,
                                           BQUEUE_UNUSED,
                                           dec_cont->FrameDesc.pic_coding_type == BFRAME );

        if(dec_cont->StrmStorage.work_out == (u32)0xFFFFFFFFU) {
          if (dec_cont->abort)
            return MPEG2DEC_ABORTED;
#ifdef GET_FREE_BUFFER_NON_BLOCK
          else {
            ret = MPEG2DEC_NO_DECODING_BUFFER;
            break;
          }
#endif
        }
        dec_cont->StrmStorage.p_pic_buf[dec_cont->StrmStorage.work_out].first_show = 1;

        if (dec_cont->pp_enabled) {
          dec_cont->StrmStorage.p_pic_buf[dec_cont->StrmStorage.work_out].pp_data =
            InputQueueGetBuffer(dec_cont->pp_buffer_queue, 1);
          if (dec_cont->StrmStorage.p_pic_buf[dec_cont->StrmStorage.work_out].pp_data == NULL)
            return MPEG2DEC_ABORTED;
        }

        if (dec_cont->workarounds.mpeg.start_code) {
          PrepareStartCodeWorkaround(
            (u8*)dec_cont->StrmStorage.p_pic_buf[
              dec_cont->StrmStorage.work_out].data.virtual_address,
            dec_cont->FrameDesc.frame_width,
            dec_cont->FrameDesc.frame_height,
            dec_cont->Hdrs.picture_structure == TOPFIELD,
            dec_cont->dpb_mode);
        }
      }
      if (!dec_cont->asic_running && dec_cont->StrmStorage.partial_freeze)
        PreparePartialFreeze(
          (u8*)dec_cont->StrmStorage.p_pic_buf[
            dec_cont->StrmStorage.work_out].data.virtual_address,
          dec_cont->FrameDesc.frame_width,
          dec_cont->FrameDesc.frame_height);

      asic_status = RunDecoderAsic(dec_cont, input->stream_bus_address);

      /* check start code workout if applicable: if timeout interrupt
       * from HW, but all macroblocks written to output -> assume
       * picture finished -> change to pic rdy. Stream end address
       * indicated by HW is not properly updated, but is handled in
       * mpeg2HandleFrameEnd() */
      if ( (asic_status & MPEG2_DEC_X170_IRQ_TIMEOUT) &&
           dec_cont->workarounds.mpeg.start_code ) {
        if ( ProcessStartCodeWorkaround(
               (u8*)dec_cont->StrmStorage.p_pic_buf[
                 dec_cont->StrmStorage.work_out].data.virtual_address,
               dec_cont->FrameDesc.frame_width,
               dec_cont->FrameDesc.frame_height,
               dec_cont->Hdrs.picture_structure == TOPFIELD,
               dec_cont->dpb_mode) ==
             HANTRO_TRUE ) {
          asic_status &= ~MPEG2_DEC_X170_IRQ_TIMEOUT;
          asic_status |= MPEG2_DEC_X170_IRQ_DEC_RDY;
        }
      }

      if(asic_status == ID8170_DEC_TIMEOUT) {
        ret = MPEG2DEC_HW_TIMEOUT;
      } else if(asic_status == ID8170_DEC_SYSTEM_ERROR) {
        ret = MPEG2DEC_SYSTEM_ERROR;
      } else if(asic_status == ID8170_DEC_HW_RESERVED) {
        ret = MPEG2DEC_HW_RESERVED;
      } else if(asic_status & MPEG2_DEC_X170_IRQ_BUS_ERROR) {
        ret = MPEG2DEC_HW_BUS_ERROR;
      } else if( (asic_status & MPEG2_DEC_X170_IRQ_STREAM_ERROR) ||
                 (asic_status & MPEG2_DEC_X170_IRQ_TIMEOUT) ||
                 (asic_status & DEC_8190_IRQ_ABORT)) {

        if (!dec_cont->StrmStorage.partial_freeze ||
            !ProcessPartialFreeze(
              (u8*)dec_cont->StrmStorage.p_pic_buf[
                dec_cont->StrmStorage.work_out].data.virtual_address,
              dec_cont->StrmStorage.work0 != INVALID_ANCHOR_PICTURE ?
              (u8*)dec_cont->StrmStorage.p_pic_buf[
                (i32)dec_cont->StrmStorage.work0].data.virtual_address :
              NULL,
              dec_cont->FrameDesc.frame_width,
              dec_cont->FrameDesc.frame_height,
              dec_cont->StrmStorage.partial_freeze == 1)) {
          if (asic_status & MPEG2_DEC_X170_IRQ_STREAM_ERROR) {
            MPEG2DEC_API_DEBUG(("STREAM ERROR IN HW\n"));
          } else if (asic_status & MPEG2_DEC_X170_IRQ_TIMEOUT) {
            MPEG2DEC_API_DEBUG(("IRQ TIMEOUT IN HW\n"));
          } else {
            MPEG2DEC_API_DEBUG(("IRQ ABORT IN HW\n"));
          }
          MPEG2FLUSH;

          if (dec_cont->pp_enabled) {
            InputQueueReturnBuffer(dec_cont->pp_buffer_queue, dec_cont->StrmStorage.p_pic_buf[dec_cont->StrmStorage.work_out].pp_data->virtual_address);
          }
          ret = mpeg2HandleVlcModeError(dec_cont, input->pic_id);
          error_concealment = HANTRO_TRUE;
          MPEG2DEC_UPDATE_POUTPUT;
        } else {
          asic_status &= ~MPEG2_DEC_X170_IRQ_STREAM_ERROR;
          asic_status &= ~MPEG2_DEC_X170_IRQ_TIMEOUT;
          asic_status |= MPEG2_DEC_X170_IRQ_DEC_RDY;
          error_concealment = HANTRO_FALSE;
        }
      } else if(asic_status & MPEG2_DEC_X170_IRQ_BUFFER_EMPTY) {
        mpeg2DecPreparePicReturn(dec_cont);
        ret = MPEG2DEC_BUF_EMPTY;

      }
      /* HW finished decoding a picture */
      else if(asic_status & MPEG2_DEC_X170_IRQ_DEC_RDY) {
      } else {
        ASSERT(0);
      }

      if(asic_status & MPEG2_DEC_X170_IRQ_DEC_RDY) {
        if(dec_cont->Hdrs.picture_structure == FRAMEPICTURE ||
            !dec_cont->ApiStorage.first_field) {
          dec_cont->FrameDesc.frame_number++;

          dec_cont->field_rdy = 0;

          mpeg2HandleFrameEnd(dec_cont);

          mpeg2DecBufferPicture(dec_cont,
                                input->pic_id,
                                dec_cont->Hdrs.
                                picture_coding_type == BFRAME,
                                dec_cont->Hdrs.
                                picture_coding_type == PFRAME,
                                MPEG2DEC_PIC_DECODED, 0);

          ret = MPEG2DEC_PIC_DECODED;

          dec_cont->ApiStorage.first_field = 1;
          if(dec_cont->Hdrs.picture_coding_type != BFRAME) {
            /*if(dec_cont->Hdrs.low_delay == 0)*/
            {
              dec_cont->StrmStorage.work1 =
                dec_cont->StrmStorage.work0;
            }
            dec_cont->StrmStorage.work0 =
              dec_cont->StrmStorage.work_out;
            if(dec_cont->StrmStorage.skip_b)
              dec_cont->StrmStorage.skip_b--;
          }

          if(dec_cont->Hdrs.picture_coding_type == IFRAME)
            dec_cont->StrmStorage.picture_broken = HANTRO_FALSE;
        } else {
          field_rdy = 1;
          dec_cont->field_rdy = 1;

          /* Store the buffer index and pic header info for single field output */
          dec_cont->field_hdrs = dec_cont->Hdrs;
          dec_cont->StrmStorage.work_out_field = dec_cont->StrmStorage.work_out;

          mpeg2HandleFrameEnd(dec_cont);
          dec_cont->ApiStorage.first_field = 0;
          dec_cont->ApiStorage.field_pic_id = input->pic_id;
          if((u32) (dec_cont->StrmDesc.strm_curr_pos -
                    dec_cont->StrmDesc.p_strm_buff_start) >=
              input_data_len) {
            ret = MPEG2DEC_BUF_EMPTY;
          }
        }
        dec_cont->StrmStorage.valid_pic_header = FALSE;
        dec_cont->StrmStorage.valid_pic_ext_header = FALSE;

        /* handle first field indication */
        if(dec_cont->Hdrs.interlaced) {
          if(dec_cont->Hdrs.picture_structure != FRAMEPICTURE)
            dec_cont->Hdrs.field_index++;
          else
            dec_cont->Hdrs.field_index = 1;

          dec_cont->Hdrs.first_field_in_frame++;
        }

        mpeg2DecPreparePicReturn(dec_cont);
      }

      if((ret != MPEG2DEC_STRM_PROCESSED && ret != MPEG2DEC_BUF_EMPTY) || field_rdy) {
        API_STOR.DecStat = STREAMDECODING;
      }

      if(ret == MPEG2DEC_PIC_DECODED || ret == MPEG2DEC_STRM_PROCESSED ||
          ret == MPEG2DEC_BUF_EMPTY) {
        /* copy output parameters for this PIC (excluding stream pos) */
        dec_cont->MbSetDesc.out_data.strm_curr_pos =
          output->strm_curr_pos;
        MPEG2DEC_UPDATE_POUTPUT;
      }
    }
  } while(ret == 0);

  if(error_concealment && dec_cont->Hdrs.picture_coding_type != BFRAME) {
    dec_cont->StrmStorage.picture_broken = HANTRO_TRUE;
  }

  MPEG2_API_TRC("Mpeg2DecDecode: Exit\n");
  output->strm_curr_pos = dec_cont->StrmDesc.strm_curr_pos;
  output->strm_curr_bus_address = input->stream_bus_address +
                                  (dec_cont->StrmDesc.strm_curr_pos - dec_cont->StrmDesc.p_strm_buff_start);
  output->data_left = dec_cont->StrmDesc.strm_buff_size -
                      (output->strm_curr_pos - DEC_STRM.p_strm_buff_start);

#ifdef DUMP_INPUT_STREAM
  fwrite(input->stream, 1, (input_data_len - output->data_left), dec_cont->ferror_stream);
#endif
#ifdef USE_RANDOM_TEST
  if (output->data_left == input_data_len)
    dec_cont->stream_not_consumed = 1;
  else
    dec_cont->stream_not_consumed = 0;
#endif


  u32 tmpret;
  Mpeg2DecPicture tmp_output;
  do {
    tmpret = Mpeg2DecNextPicture_INTERNAL(dec_cont, &tmp_output, 0);
    if(tmpret == MPEG2DEC_ABORTED)
      return (MPEG2DEC_ABORTED);
  } while( tmpret == MPEG2DEC_PIC_RDY);

  if(dec_cont->abort)
    return(MPEG2DEC_ABORTED);
  else
    return ((Mpeg2DecRet) ret);

#undef API_STOR
#undef DEC_STRM
#undef DEC_FRAMED

}

/*------------------------------------------------------------------------------

    Function: Mpeg2DecRelease()

        Functional description:
            Release the decoder instance.

        Inputs:
            dec_inst     Decoder instance

        Outputs:
            none

        Returns:
            none

------------------------------------------------------------------------------*/

void Mpeg2DecRelease(Mpeg2DecInst dec_inst) {
#define API_STOR ((Mpeg2DecContainer *)dec_inst)->ApiStorage
  Mpeg2DecContainer *dec_cont = NULL;
  const void *dwl;

  MPEG2DEC_DEBUG(("1\n"));
  MPEG2_API_TRC("Mpeg2DecRelease#");
  if(dec_inst == NULL) {
    MPEG2_API_TRC("Mpeg2DecRelease# ERROR: dec_inst == NULL");
    return;
  }

  dec_cont = ((Mpeg2DecContainer *) dec_inst);
  dwl = dec_cont->dwl;

  pthread_mutex_destroy(&dec_cont->protect_mutex);

  if (dec_cont->asic_running) {
    /* Release HW */
    DWLDisableHw(dec_cont->dwl, dec_cont->core_id, 4, 0);
    DWLReleaseHw(dec_cont->dwl, dec_cont->core_id);
  }

#ifdef DUMP_INPUT_STREAM
  if (dec_cont->ferror_stream)
    fclose(dec_cont->ferror_stream);
#endif

  for (u32 i = 0; i < DEC_MAX_PPU_COUNT; i++) {
    if (dec_cont->ppu_cfg[i].lanczos_table.virtual_address) {
      DWLFreeLinear(dec_cont->dwl, &dec_cont->ppu_cfg[i].lanczos_table);
      dec_cont->ppu_cfg[i].lanczos_table.virtual_address = NULL;
    }
  }
  if (dec_cont->fifo_display)
    FifoRelease(dec_cont->fifo_display);

  mpeg2FreeBuffers(dec_cont);

  if (dec_cont->pp_buffer_queue) InputQueueRelease(dec_cont->pp_buffer_queue);

  DWLfree(dec_cont);

  MPEG2_API_TRC("Mpeg2DecRelease: OK");
#undef API_STOR
  (void)dwl;
}


/*------------------------------------------------------------------------------
    Function name   : mpeg2RefreshRegs
    Description     :
    Return type     : void
    Argument        : Mpeg2DecContainer *dec_cont
------------------------------------------------------------------------------*/
void mpeg2RefreshRegs(Mpeg2DecContainer * dec_cont) {
  addr_t i;
  u32 *pp_regs = dec_cont->mpeg2_regs;

  for(i = 0; i < DEC_X170_REGISTERS; i++) {
    pp_regs[i] = DWLReadReg(dec_cont->dwl, dec_cont->core_id, 4 * i);
  }
}

/*------------------------------------------------------------------------------
    Function name   : mpeg2FlushRegs
    Description     :
    Return type     : void
    Argument        : Mpeg2DecContainer *dec_cont
------------------------------------------------------------------------------*/
void mpeg2FlushRegs(Mpeg2DecContainer * dec_cont) {
  i32 i;
  u32 *pp_regs = dec_cont->mpeg2_regs;

  for(i = 2; i < DEC_X170_REGISTERS; i++) {
    DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 4 * i, pp_regs[i]);
    pp_regs[i] = 0;
  }
#if 0
#ifdef USE_64BIT_ENV
  offset = TOTAL_X170_ORIGIN_REGS * 0x04;
  pp_regs =  dec_cont->mpeg2_regs + TOTAL_X170_ORIGIN_REGS;
  for(i = DEC_X170_EXPAND_REGS; i > 0; --i) {
    DWLWriteReg(dec_cont->dwl, dec_cont->core_id, offset, *pp_regs);
    pp_regs++;
    offset += 4;
  }
#endif
  offset = 314 * 0x04;
  pp_regs =  dec_cont->mpeg2_regs + 314;
  DWLWriteReg(dec_cont->dwl, dec_cont->core_id, offset, *pp_regs);
  offset = PP_START_UNIFIED_REGS * 0x04;
  pp_regs =  dec_cont->mpeg2_regs + PP_START_UNIFIED_REGS;
  for(i = PP_UNIFIED_REGS; i > 0; --i) {
    DWLWriteReg(dec_cont->dwl, dec_cont->core_id, offset, *pp_regs);
    pp_regs++;
    offset += 4;
  }
#endif
}

/*------------------------------------------------------------------------------
    Function name   : mpeg2HandleVlcModeError
    Description     :
    Return type     : u32
    Argument        : Mpeg2DecContainer *dec_cont
------------------------------------------------------------------------------*/
u32 mpeg2HandleVlcModeError(Mpeg2DecContainer * dec_cont, u32 pic_num) {
  u32 ret = 0, tmp;

  ASSERT(dec_cont->StrmStorage.strm_dec_ready);

  tmp = mpeg2StrmDec_NextStartCode(dec_cont);
  if(tmp != END_OF_STREAM) {
    dec_cont->StrmDesc.strm_curr_pos -= 4;
    dec_cont->StrmDesc.strm_buff_read_bits -= 32;
  }

  /* error in first picture -> set reference to grey */
  if(!dec_cont->FrameDesc.frame_number) {
    /* 1st picture, take care of false header data */
    dec_cont->StrmStorage.error_in_hdr = 1;
    /* Don't do it if first field has been decoded successfully */
    if (!dec_cont->field_rdy) {
      if (!dec_cont->tiled_stride_enable) {
        (void) DWLmemset(dec_cont->StrmStorage.
                         p_pic_buf[dec_cont->StrmStorage.work_out].data.
                         virtual_address, 128,
                         384 * dec_cont->FrameDesc.total_mb_in_frame);
      } else {
        u32 out_w, out_h, size;
        out_w = NEXT_MULTIPLE(4 * dec_cont->FrameDesc.frame_width * 16, ALIGN(dec_cont->align));
        out_h = dec_cont->FrameDesc.frame_height * 4;
        size = out_w * out_h * 3 / 2;
        (void) DWLmemset(dec_cont->StrmStorage.
                         p_pic_buf[dec_cont->StrmStorage.work_out].data.
                         virtual_address, 128, size);
      }
    }
    mpeg2DecPreparePicReturn(dec_cont);

    /* no pictures finished -> return STRM_PROCESSED */
    if(tmp == END_OF_STREAM) {
      ret = MPEG2DEC_STRM_PROCESSED;
    } else
      ret = 0;
    dec_cont->StrmStorage.work0 = dec_cont->StrmStorage.work_out;
    dec_cont->StrmStorage.skip_b = 2;
  } else {
    if(dec_cont->Hdrs.picture_coding_type != BFRAME) {
      dec_cont->FrameDesc.frame_number++;

      dec_cont->StrmStorage.work_out_prev = dec_cont->StrmStorage.work_out;
      dec_cont->StrmStorage.work_out = dec_cont->StrmStorage.work0;

      mpeg2DecBufferPicture(dec_cont,
                            pic_num,
                            dec_cont->Hdrs.picture_coding_type == BFRAME,
                            1, (Mpeg2DecRet) FREEZED_PIC_RDY,
                            dec_cont->FrameDesc.total_mb_in_frame);

      ret = MPEG2DEC_PIC_DECODED;

      /*if(dec_cont->Hdrs.low_delay == 0)*/
      {
        dec_cont->StrmStorage.work1 = dec_cont->StrmStorage.work0;
      }
      dec_cont->StrmStorage.skip_b = 2;
    } else {
      if(dec_cont->StrmStorage.intra_freeze) {
        dec_cont->FrameDesc.frame_number++;
        mpeg2DecBufferPicture(dec_cont,
                              pic_num,
                              dec_cont->Hdrs.picture_coding_type ==
                              BFRAME, 1, (Mpeg2DecRet) FREEZED_PIC_RDY,
                              dec_cont->FrameDesc.total_mb_in_frame);

        ret = MPEG2DEC_PIC_DECODED;

      } else {
        ret = MPEG2DEC_NONREF_PIC_SKIPPED;
      }
      dec_cont->StrmStorage.work_out_prev = dec_cont->StrmStorage.work0;
    }
  }

  /* picture freezed due to error in first field -> skip/ignore 2. field */
  if(dec_cont->Hdrs.picture_structure != FRAMEPICTURE &&
      dec_cont->Hdrs.picture_coding_type != BFRAME &&
      dec_cont->ApiStorage.first_field) {
    dec_cont->ApiStorage.ignore_field = 1;
    dec_cont->ApiStorage.first_field = 0;
  } else
    dec_cont->ApiStorage.first_field = 1;

  dec_cont->ApiStorage.DecStat = STREAMDECODING;
  dec_cont->StrmStorage.valid_pic_header = FALSE;
  dec_cont->StrmStorage.valid_pic_ext_header = FALSE;
  dec_cont->Hdrs.picture_structure = FRAMEPICTURE;

  return ret;
}

/*------------------------------------------------------------------------------
    Function name   : mpeg2HandleFrameEnd
    Description     :
    Return type     : u32
    Argument        : Mpeg2DecContainer *dec_cont
------------------------------------------------------------------------------*/
void mpeg2HandleFrameEnd(Mpeg2DecContainer * dec_cont) {

  u32 tmp;

  dec_cont->StrmDesc.strm_buff_read_bits =
    8 * (dec_cont->StrmDesc.strm_curr_pos -
         dec_cont->StrmDesc.p_strm_buff_start);
  dec_cont->StrmDesc.bit_pos_in_word = 0;

  do {
    tmp = mpeg2StrmDec_ShowBits(dec_cont, 32);
    if((tmp >> 8) == 0x1)
      break;
  } while(mpeg2StrmDec_FlushBits(dec_cont, 8) == HANTRO_OK);

}

/*------------------------------------------------------------------------------

         Function name: RunDecoderAsic

         Purpose:       Set Asic run lenght and run Asic

         Input:         Mpeg2DecContainer *dec_cont

         Output:        void

------------------------------------------------------------------------------*/
u32 RunDecoderAsic(Mpeg2DecContainer * dec_cont, addr_t strm_bus_address) {
  i32 ret;
  addr_t tmp = 0;
  u32 asic_status = 0;
  u32 hw_build_id;
  struct DecHwFeatures hw_feature;
  addr_t mask;

  hw_build_id = DWLReadHwBuildID(DWL_CLIENT_TYPE_MPEG2_DEC);
  GetReleaseHwFeaturesByID(hw_build_id, &hw_feature);

  MPEG2FLUSH;

  ASSERT(dec_cont->StrmStorage.
         p_pic_buf[dec_cont->StrmStorage.work_out].data.bus_address != 0);
  ASSERT(strm_bus_address != 0);

  if (hw_feature.g1_strm_128bit_align)
    mask = 15;
  else
    mask = 7;

  /* q-tables to buffer */
  mpeg2HandleQTables(dec_cont);

  /* Save frameDesc/Hdr/dpb_mode info for current picture. */
  dec_cont->StrmStorage.p_pic_buf[dec_cont->StrmStorage.work_out].FrameDesc
    = dec_cont->FrameDesc;
  dec_cont->StrmStorage.p_pic_buf[dec_cont->StrmStorage.work_out].Hdrs
    = dec_cont->Hdrs;
  dec_cont->StrmStorage.p_pic_buf[dec_cont->StrmStorage.work_out].dpb_mode
    = dec_cont->dpb_mode;

  if(!dec_cont->asic_running) {
    tmp = Mpeg2SetRegs(dec_cont, strm_bus_address);
    if(tmp == HANTRO_NOK)
      return 0;

    if (!dec_cont->keep_hw_reserved)
      (void) DWLReserveHw(dec_cont->dwl, &dec_cont->core_id, DWL_CLIENT_TYPE_MPEG2_DEC);

    SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_OUT_DIS, 0);
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_FILTERING_DIS, 1);

    dec_cont->asic_running = 1;

    DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 0x4, 0);

    mpeg2FlushRegs(dec_cont);

    DWLReadPpConfigure(dec_cont->dwl, dec_cont->ppu_cfg, NULL, 0);
    /* Enable HW */
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_E, 1);
    DWLEnableHw(dec_cont->dwl, dec_cont->core_id, 4 * 1,
                dec_cont->mpeg2_regs[1]);
  } else { /* in the middle of decoding, continue decoding */
    /* tmp is strm_bus_address + number of bytes decoded by SW */
    tmp = dec_cont->StrmDesc.strm_curr_pos -
          dec_cont->StrmDesc.p_strm_buff_start;
    tmp = strm_bus_address + tmp;

    /* pointer to start of the stream, mask to get the pointer to
     * previous 64/128-bit aligned position */
    if(!(tmp & ~(mask))) {
      return 0;
    }

    SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_RLC_VLC_BASE, tmp & ~(mask));
    /* amount of stream (as seen by the HW), obtained as amount of stream
     * given by the application subtracted by number of bytes decoded by
     * SW (if strm_bus_address is not 64/128-bit aligned -> adds number of bytes
     * from previous 64/128-bit aligned boundary) */
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_STREAM_LEN,
                   dec_cont->StrmDesc.strm_buff_size -
                   ((tmp & ~(mask)) - strm_bus_address));

    SetDecRegister(dec_cont->mpeg2_regs, HWIF_STRM_BUFFER_LEN,
                   dec_cont->StrmDesc.strm_buff_size -
                   ((tmp & ~(mask)) - strm_bus_address));
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_STRM_START_OFFSET, 0);

    SetDecRegister(dec_cont->mpeg2_regs, HWIF_STRM_START_BIT,
                   dec_cont->StrmDesc.bit_pos_in_word + 8 * (tmp & (mask)));

    /* This depends on actual register allocation */
    DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 4 * 5,
                dec_cont->mpeg2_regs[5]);
    DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 4 * 6,
                dec_cont->mpeg2_regs[6]);
    DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 4 * 258,
                dec_cont->mpeg2_regs[258]);
    DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 4 * 259,
                dec_cont->mpeg2_regs[259]);
    if (IS_LEGACY(dec_cont->mpeg2_regs[0]))
      DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 4 * 12, dec_cont->mpeg2_regs[12]);
    else
      DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 4 * 169, dec_cont->mpeg2_regs[169]);
    if (sizeof(addr_t) == 8) {
      if(hw_feature.addr64_support) {
        if (IS_LEGACY(dec_cont->mpeg2_regs[0]))
          DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 4 * 122, dec_cont->mpeg2_regs[122]);
        else
          DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 4 * 168, dec_cont->mpeg2_regs[168]);
      } else {
        ASSERT(dec_cont->mpeg2_regs[122] == 0);
        ASSERT(dec_cont->mpeg2_regs[168] == 0);
      }
    } else {
      if(hw_feature.addr64_support) {
        if (IS_LEGACY(dec_cont->mpeg2_regs[0]))
          DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 4 * 122, 0);
        else
          DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 4 * 168, 0);
      }
    }
    DWLEnableHw(dec_cont->dwl, dec_cont->core_id, 4 * 1,
                dec_cont->mpeg2_regs[1]);
  }

  /* Wait for HW ready */
  MPEG2DEC_API_DEBUG(("Wait for Decoder\n"));
  ret = DWLWaitHwReady(dec_cont->dwl, dec_cont->core_id,
                       (u32) DEC_X170_TIMEOUT_LENGTH);

  mpeg2RefreshRegs(dec_cont);

  if(ret == DWL_HW_WAIT_OK) {
    asic_status =
      GetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_IRQ_STAT);
  } else if(ret == DWL_HW_WAIT_TIMEOUT) {
    asic_status = ID8170_DEC_TIMEOUT;
  } else {
    asic_status = ID8170_DEC_SYSTEM_ERROR;
  }

  if(!(asic_status & MPEG2_DEC_X170_IRQ_BUFFER_EMPTY) ||
      (asic_status & MPEG2_DEC_X170_IRQ_STREAM_ERROR) ||
      (asic_status & MPEG2_DEC_X170_IRQ_BUS_ERROR) ||
      (asic_status == ID8170_DEC_TIMEOUT) ||
      (asic_status == ID8170_DEC_SYSTEM_ERROR)) {
    /* reset HW */
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_IRQ_STAT, 0);
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_IRQ, 0);

    DWLDisableHw(dec_cont->dwl, dec_cont->core_id, 4 * 1,
                 dec_cont->mpeg2_regs[1]);

    dec_cont->keep_hw_reserved = 0;

    dec_cont->asic_running = 0;
    if (!dec_cont->keep_hw_reserved)
      (void) DWLReleaseHw(dec_cont->dwl, dec_cont->core_id);
  }

  /* if HW interrupt indicated either BUFFER_EMPTY or
   * DEC_RDY -> read stream end pointer and update StrmDesc structure */
  if((asic_status &
      (MPEG2_DEC_X170_IRQ_BUFFER_EMPTY | MPEG2_DEC_X170_IRQ_DEC_RDY |
       MPEG2_DEC_X170_IRQ_STREAM_ERROR | MPEG2_DEC_X170_IRQ_TIMEOUT))) {
    tmp = GET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_RLC_VLC_BASE);

    if ( asic_status &
         (MPEG2_DEC_X170_IRQ_STREAM_ERROR | MPEG2_DEC_X170_IRQ_TIMEOUT) ) {
      if (tmp > strm_bus_address + 8)
        tmp -= 8;
      else
        tmp = strm_bus_address;
    }

    if((tmp - strm_bus_address) <= dec_cont->StrmDesc.strm_buff_size) {
      dec_cont->StrmDesc.strm_curr_pos =
        dec_cont->StrmDesc.p_strm_buff_start + (tmp - strm_bus_address);
    } else {
      dec_cont->StrmDesc.strm_curr_pos =
        dec_cont->StrmDesc.p_strm_buff_start +
        dec_cont->StrmDesc.strm_buff_size;
    }
    /* if timeout interrupt and no bytes consumed by HW -> advance one
     * byte to prevent processing current slice again (may only happen
     * in slice-by-slice mode) */
    if ( (asic_status & MPEG2_DEC_X170_IRQ_TIMEOUT) &&
         tmp == strm_bus_address &&
         dec_cont->StrmDesc.strm_curr_pos <
         (dec_cont->StrmDesc.p_strm_buff_start +
          dec_cont->StrmDesc.strm_buff_size) )
      dec_cont->StrmDesc.strm_curr_pos++;
  }

  dec_cont->StrmDesc.strm_buff_read_bits =
    8 * (dec_cont->StrmDesc.strm_curr_pos -
         dec_cont->StrmDesc.p_strm_buff_start);

  dec_cont->StrmDesc.bit_pos_in_word = 0;

  if( dec_cont->Hdrs.picture_coding_type != BFRAME &&
      dec_cont->ref_buf_support &&
      (asic_status & MPEG2_DEC_X170_IRQ_DEC_RDY) &&
      dec_cont->asic_running == 0) {
    RefbuMvStatistics( &dec_cont->ref_buffer_ctrl,
                       dec_cont->mpeg2_regs,
                       NULL,
                       HANTRO_FALSE,
                       dec_cont->Hdrs.picture_coding_type == IFRAME );
  }

  SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_IRQ_STAT, 0);

  return asic_status;

}

/*------------------------------------------------------------------------------

    Function name: Mpeg2DecNextPicture

    Functional description:
        Retrieve next decoded picture

    Input:
        dec_inst     Reference to decoder instance.
        picture    Pointer to return value struct
        end_of_stream Indicates whether end of stream has been reached

    Output:
        picture Decoder output picture.

    Return values:
        MPEG2DEC_OK         No picture available.
        MPEG2DEC_PIC_RDY    Picture ready.

------------------------------------------------------------------------------*/
Mpeg2DecRet Mpeg2DecNextPicture(Mpeg2DecInst dec_inst,
                                Mpeg2DecPicture * picture, u32 end_of_stream) {
  /* Variables */
  Mpeg2DecContainer *dec_cont;
  i32 ret;

  /* Code */
  MPEG2_API_TRC("\nMpeg2_dec_next_picture#");

  /* Check that function input parameters are valid */
  if(picture == NULL) {
    MPEG2_API_TRC("Mpeg2DecNextPicture# ERROR: picture is NULL");
    return (MPEG2DEC_PARAM_ERROR);
  }

  dec_cont = (Mpeg2DecContainer *) dec_inst;

  /* Check if decoder is in an incorrect mode */
  if(dec_inst == NULL || dec_cont->ApiStorage.DecStat == UNINIT) {
    MPEG2_API_TRC("Mpeg2DecNextPicture# ERROR: Decoder not initialized");
    return (MPEG2DEC_NOT_INITIALIZED);
  }

  addr_t i;
  if ((ret = FifoPop(dec_cont->fifo_display, (FifoObject *)&i,
#ifdef GET_OUTPUT_BUFFER_NON_BLOCK
          FIFO_EXCEPTION_ENABLE
#else
          FIFO_EXCEPTION_DISABLE
#endif
          )) != FIFO_ABORT) {

#ifdef GET_OUTPUT_BUFFER_NON_BLOCK
    if (ret == FIFO_EMPTY) return MPEG2DEC_OK;
#endif

    if ((i32)i == -1) {
      MPEG2_API_TRC("Mpeg2DecNextPicture# MPEG2DEC_END_OF_STREAM\n");
      return MPEG2DEC_END_OF_STREAM;
    }
    if ((i32)i == -2) {
      MPEG2_API_TRC("Mpeg2DecNextPicture# MPEG2DEC_FLUSHED\n");
      return MPEG2DEC_FLUSHED;
    }

    *picture = dec_cont->StrmStorage.picture_info[i];

    MPEG2_API_TRC("Mpeg2DecNextPicture# MPEG2DEC_PIC_RDY\n");
    return (MPEG2DEC_PIC_RDY);
  } else
    return MPEG2DEC_ABORTED;
}

/*------------------------------------------------------------------------------

    Function name: Mpeg2DecNextPicture_INTERNAL

    Functional description:
        Push next picture in display order into output fifo if any available.

    Input:
        dec_inst     Reference to decoder instance.
        picture    Pointer to return value struct
        end_of_stream Indicates whether end of stream has been reached

    Output:
        picture Decoder output picture.

    Return values:
        MPEG2DEC_OK         No picture available.
        MPEG2DEC_PIC_RDY    Picture ready.
        MPEG2DEC_PARAM_ERROR     invalid parameters
        MPEG2DEC_NOT_INITIALIZED   decoder instance not initialized yet

------------------------------------------------------------------------------*/
Mpeg2DecRet Mpeg2DecNextPicture_INTERNAL(Mpeg2DecInst dec_inst,
    Mpeg2DecPicture * picture, u32 end_of_stream) {
  /* Variables */
  Mpeg2DecRet return_value = MPEG2DEC_PIC_RDY;
  Mpeg2DecContainer *dec_cont;
  u32 pic_index = MPEG2_BUFFER_UNDEFINED;
  u32 min_count;
  /* Code */
  MPEG2_API_TRC("\nMpeg2_dec_next_picture_INTERNAL#");

  /* Check that function input parameters are valid */
  if(picture == NULL) {
    MPEG2_API_TRC("Mpeg2DecNextPicture_INTERNAL# ERROR: picture is NULL");
    return (MPEG2DEC_PARAM_ERROR);
  }

  dec_cont = (Mpeg2DecContainer *) dec_inst;

  /* Check if decoder is in an incorrect mode */
  if(dec_inst == NULL || dec_cont->ApiStorage.DecStat == UNINIT) {
    MPEG2_API_TRC("Mpeg2DecNextPicture_INTERNAL# ERROR: Decoder not initialized");
    return (MPEG2DEC_NOT_INITIALIZED);
  }

  min_count = 0;
  memset(picture, 0, sizeof(Mpeg2DecPicture));
  if(dec_cont->Hdrs.low_delay == 0 && !end_of_stream &&
      !dec_cont->StrmStorage.new_headers_change_resolution)
    min_count = 1;

  /* this is to prevent post-processing of non-finished pictures in the
   * end of the stream */
  if(end_of_stream && dec_cont->FrameDesc.pic_coding_type == BFRAME) {
    dec_cont->FrameDesc.pic_coding_type = PFRAME;
  }

  /* Nothing to send out */
  if(dec_cont->StrmStorage.out_count <= min_count) {
    (void) DWLmemset(picture, 0, sizeof(Mpeg2DecPicture));
    picture->pictures[0].output_picture = NULL;
    picture->interlaced = dec_cont->Hdrs.interlaced;
    /* print nothing to send out */
    return_value = MPEG2DEC_OK;
  } else {
    pic_index = dec_cont->StrmStorage.out_index;
    pic_index = dec_cont->StrmStorage.out_buf[pic_index];

    Mpeg2FillPicStruct(picture, dec_cont, pic_index);

    /* field output */
    //if(MPEG2DEC_IS_FIELD_OUTPUT)
    if (!dec_cont->StrmStorage.p_pic_buf[pic_index].Hdrs.progressive_sequence &&
        !dec_cont->StrmStorage.p_pic_buf[pic_index].Hdrs.progressive_frame) {
      picture->interlaced = 1;
      picture->field_picture = 1;

      if(!dec_cont->ApiStorage.output_other_field) {
        picture->top_field =
          dec_cont->StrmStorage.p_pic_buf[pic_index].tf ? 1 : 0;
        dec_cont->ApiStorage.output_other_field = 1;
      } else {
        picture->top_field =
          dec_cont->StrmStorage.p_pic_buf[pic_index].tf ? 0 : 1;
        dec_cont->ApiStorage.output_other_field = 0;
        dec_cont->StrmStorage.out_count--;
        dec_cont->StrmStorage.out_index++;
        dec_cont->StrmStorage.out_index &= 15;
      }
    } else {
      /* progressive or deinterlaced frame output */
      picture->interlaced = 0; //dec_cont->StrmStorage.p_pic_buf[pic_index].Hdrs.interlaced;
      picture->top_field = 0;
      picture->field_picture = 0;
      dec_cont->StrmStorage.out_count--;
      dec_cont->StrmStorage.out_index++;
      dec_cont->StrmStorage.out_index &= 15;
    }

    picture->output_other_field = dec_cont->ApiStorage.output_other_field;

    picture->single_field =
      dec_cont->StrmStorage.p_pic_buf[pic_index].sf ? 1 : 0;

    if (picture->single_field)
      picture->top_field = dec_cont->StrmStorage.p_pic_buf[pic_index].ps;


#ifdef USE_PICTURE_DISCARD
    if (dec_cont->StrmStorage.p_pic_buf[pic_index].first_show)
#endif
    {
      /* wait this buffer as unused */
      if(BqueueWaitBufNotInUse(&dec_cont->StrmStorage.bq, pic_index) != HANTRO_OK)
        return MPEG2DEC_ABORTED;

      if(dec_cont->pp_enabled) {
        InputQueueWaitBufNotUsed(dec_cont->pp_buffer_queue,dec_cont->StrmStorage.p_pic_buf[pic_index].pp_data->virtual_address);
      }
      /* set this buffer as used */
      if((!dec_cont->ApiStorage.output_other_field &&
          picture->interlaced) || !picture->interlaced) {
        BqueueSetBufferAsUsed(&dec_cont->StrmStorage.bq, pic_index);
        dec_cont->StrmStorage.p_pic_buf[pic_index].first_show = 0;
        if(dec_cont->pp_enabled)
          InputQueueSetBufAsUsed(dec_cont->pp_buffer_queue,dec_cont->StrmStorage.p_pic_buf[pic_index].pp_data->virtual_address);
      }

      dec_cont->StrmStorage.picture_info[dec_cont->fifo_index] = *picture;
      FifoPush(dec_cont->fifo_display, (FifoObject)(addr_t)dec_cont->fifo_index, FIFO_EXCEPTION_DISABLE);
      dec_cont->fifo_index++;
      if(dec_cont->fifo_index == 32)
        dec_cont->fifo_index = 0;
      if (dec_cont->pp_enabled) {
        BqueuePictureRelease(&dec_cont->StrmStorage.bq, pic_index);
      }
    }
  }

  return return_value;
}

/*------------------------------------------------------------------------------

    Function name: Mpeg2DecPictureConsumed

    Functional description:
        release specific decoded picture

    Input:
        dec_inst     Reference to decoder instance.
        picture    Pointer to  picture struct


    Return values:
        MPEG2DEC_PARAM_ERROR         Decoder instance or picture is null
        MPEG2DEC_NOT_INITIALIZED     Decoder instance isn't initialized
        MPEG2DEC_OK                          picture release success
------------------------------------------------------------------------------*/
Mpeg2DecRet Mpeg2DecPictureConsumed(Mpeg2DecInst dec_inst, Mpeg2DecPicture * picture) {
  /* Variables */
  Mpeg2DecContainer *dec_cont;
  u32 i;
  u8 *output_picture = NULL;
  PpUnitIntConfig *ppu_cfg;

  /* Code */
  MPEG2_API_TRC("\nMpeg2_dec_picture_consumed#");

  /* Check that function input parameters are valid */
  if(picture == NULL) {
    MPEG2_API_TRC("Mpeg2DecPictureConsumed# ERROR: picture is NULL");
    return (MPEG2DEC_PARAM_ERROR);
  }

  dec_cont = (Mpeg2DecContainer *) dec_inst;

  /* Check if decoder is in an incorrect mode */
  if(dec_inst == NULL || dec_cont->ApiStorage.DecStat == UNINIT) {
    MPEG2_API_TRC("Mpeg2DecPictureConsumed# ERROR: Decoder not initialized");
    return (MPEG2DEC_NOT_INITIALIZED);
  }

  if (!dec_cont->pp_enabled) {
    for(i = 0; i < dec_cont->StrmStorage.num_buffers; i++) {
      if(picture->pictures[0].output_picture_bus_address == dec_cont->StrmStorage.p_pic_buf[i].data.bus_address
          && (addr_t)picture->pictures[0].output_picture
          == (addr_t)dec_cont->StrmStorage.p_pic_buf[i].data.virtual_address) {
        BqueuePictureRelease(&dec_cont->StrmStorage.bq, i);
        return (MPEG2DEC_OK);
      }
    }
  } else {
    ppu_cfg = dec_cont->ppu_cfg;
    for (i = 0; i < DEC_MAX_PPU_COUNT; i++, ppu_cfg++) {
      if (!ppu_cfg->enabled)
        continue;
      else {
        output_picture = picture->pictures[i].output_picture;
        break;
      }
    }
    InputQueueReturnBuffer(dec_cont->pp_buffer_queue, (const u32 *)output_picture);
    return (MPEG2DEC_OK);
  }
  return (MPEG2DEC_PARAM_ERROR);
}


Mpeg2DecRet Mpeg2DecEndOfStream(Mpeg2DecInst dec_inst, u32 strm_end_flag) {
  Mpeg2DecContainer *dec_cont = (Mpeg2DecContainer *) dec_inst;
  Mpeg2DecPicture output;
  Mpeg2DecRet ret;

  MPEG2_API_TRC("Mpeg2DecEndOfStream#\n");

  /* Check if decoder is in an incorrect mode */
  if(dec_inst == NULL || dec_cont->ApiStorage.DecStat == UNINIT) {
    MPEG2_API_TRC("Mpeg2DecPictureConsumed# ERROR: Decoder not initialized");
    return (MPEG2DEC_NOT_INITIALIZED);
  }

  pthread_mutex_lock(&dec_cont->protect_mutex);
  /* Do not do it twice */
  if(dec_cont->dec_stat == MPEG2DEC_END_OF_STREAM) {
    pthread_mutex_unlock(&dec_cont->protect_mutex);
    return (MPEG2DEC_OK);
  }

  if(dec_cont->asic_running) {
    /* stop HW */
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_IRQ_STAT, 0);
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_IRQ, 0);
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_E, 0);
    DWLDisableHw(dec_cont->dwl, dec_cont->core_id, 4 * 1,
                 dec_cont->mpeg2_regs[1] | DEC_IRQ_DISABLE);
    DWLReleaseHw(dec_cont->dwl, dec_cont->core_id);  /* release HW lock */
    dec_cont->asic_running = 0;
  } else if(dec_cont->keep_hw_reserved) {
    DWLReleaseHw(dec_cont->dwl, dec_cont->core_id);  /* release HW lock */
    dec_cont->keep_hw_reserved = 0;
  }

#if 1
  if (dec_cont->field_rdy) {
    // Single field in current buffer
    dec_cont->StrmStorage.work_out = dec_cont->StrmStorage.work_out_field;
    dec_cont->Hdrs = dec_cont->field_hdrs;
    dec_cont->StrmStorage.p_pic_buf[dec_cont->StrmStorage.work_out].sf = 1;
    mpeg2DecBufferPicture(dec_cont,
                          dec_cont->ApiStorage.field_pic_id,
                          dec_cont->Hdrs.
                          picture_coding_type == BFRAME,
                          dec_cont->Hdrs.
                          picture_coding_type == PFRAME,
                          MPEG2DEC_PIC_DECODED, 0);
  }
#endif

  while((ret = Mpeg2DecNextPicture_INTERNAL(dec_inst, &output, 1)) == MPEG2DEC_PIC_RDY);
  if(ret == MPEG2DEC_ABORTED) {
    pthread_mutex_unlock(&dec_cont->protect_mutex);
    return (MPEG2DEC_ABORTED);
  }

  if(strm_end_flag) {
    dec_cont->dec_stat = MPEG2DEC_END_OF_STREAM;
    FifoPush(dec_cont->fifo_display, (FifoObject)-1, FIFO_EXCEPTION_DISABLE);
  }

  /* Wait all buffers as unused */
  if(!strm_end_flag)
    BqueueWaitNotInUse(&dec_cont->StrmStorage.bq);

  dec_cont->StrmStorage.work0 =
    dec_cont->StrmStorage.work1 = INVALID_ANCHOR_PICTURE;

  pthread_mutex_unlock(&dec_cont->protect_mutex);
  MPEG2_API_TRC("Mpeg2DecEndOfStream# MPEG2DEC_OK\n");
  return (MPEG2DEC_OK);
}

/*----------------------=-------------------------------------------------------

    Function name: Mpeg2FillPicStruct

    Functional description:
        Fill data to output pic description

    Input:
        dec_cont    Decoder container
        picture    Pointer to return value struct

    Return values:
        void

------------------------------------------------------------------------------*/
static void Mpeg2FillPicStruct(Mpeg2DecPicture * picture,
                               Mpeg2DecContainer * dec_cont, u32 pic_index) {
  picture_t *p_pic;

  p_pic = (picture_t *) dec_cont->StrmStorage.p_pic_buf;
  PpUnitIntConfig *ppu_cfg = dec_cont->ppu_cfg;
  u32 i;

  if (!dec_cont->pp_enabled) {
    picture->pictures[0].frame_width = p_pic[pic_index].FrameDesc.frame_width << 4;
    picture->pictures[0].frame_height = p_pic[pic_index].FrameDesc.frame_height << 4;
    picture->pictures[0].coded_width = p_pic[pic_index].Hdrs.horizontal_size;
    picture->pictures[0].coded_height = p_pic[pic_index].Hdrs.vertical_size;
    if (dec_cont->tiled_stride_enable)
      picture->pictures[0].pic_stride = NEXT_MULTIPLE(picture->pictures[0].frame_width * 4,
                                                      ALIGN(dec_cont->align));
    else
      picture->pictures[0].pic_stride = picture->pictures[0].frame_width * 4;
    picture->pictures[0].pic_stride_ch = picture->pictures[0].pic_stride;
    picture->pictures[0].output_picture = (u8 *) p_pic[pic_index].data.virtual_address;
    picture->pictures[0].output_picture_bus_address = p_pic[pic_index].data.bus_address;
    picture->pictures[0].output_format = p_pic[pic_index].tiled_mode ?
                           DEC_OUT_FRM_TILED_4X4 : DEC_OUT_FRM_RASTER_SCAN;
  } else {
    for (i = 0; i < DEC_MAX_PPU_COUNT; i++, ppu_cfg++) {
      if (!ppu_cfg->enabled) continue;
      picture->pictures[i].frame_width = NEXT_MULTIPLE(dec_cont->ppu_cfg[i].scale.width, ALIGN(ppu_cfg->align));
      picture->pictures[i].frame_height = dec_cont->ppu_cfg[i].scale.height;
      picture->pictures[i].coded_width = dec_cont->ppu_cfg[i].scale.width;
      picture->pictures[i].coded_height = dec_cont->ppu_cfg[i].scale.height;
      picture->pictures[i].pic_stride = dec_cont->ppu_cfg[i].ystride;
      picture->pictures[i].pic_stride_ch = dec_cont->ppu_cfg[i].cstride;
      picture->pictures[i].output_picture = (u8 *) ((addr_t)p_pic[pic_index].pp_data->virtual_address + ppu_cfg->luma_offset);
      picture->pictures[i].output_picture_bus_address = p_pic[pic_index].pp_data->bus_address + ppu_cfg->luma_offset;
      CheckOutputFormat(dec_cont->ppu_cfg, &picture->pictures[i].output_format, i);
    }
  }
  picture->interlaced = p_pic[pic_index].Hdrs.interlaced;

  picture->key_picture = p_pic[pic_index].pic_type;
  picture->pic_id = p_pic[pic_index].pic_id;
  picture->decode_id = p_pic[pic_index].pic_id;
  picture->pic_coding_type[0] = p_pic[pic_index].pic_code_type[0];
  picture->pic_coding_type[1] = p_pic[pic_index].pic_code_type[1];

  /* handle first field indication */
  if(p_pic[pic_index].Hdrs.interlaced) {
    if(p_pic[pic_index].Hdrs.field_out_index)
      p_pic[pic_index].Hdrs.field_out_index = 0;
    else
      p_pic[pic_index].Hdrs.field_out_index = 1;
  }

  picture->first_field = p_pic[pic_index].ff[p_pic[pic_index].Hdrs.field_out_index];
  picture->repeat_first_field = p_pic[pic_index].rff;
  picture->repeat_frame_count = p_pic[pic_index].rfc;
  picture->number_of_err_mbs = p_pic[pic_index].nbr_err_mbs;
  picture->dpb_mode = p_pic[pic_index].dpb_mode;
  (void) DWLmemcpy(&picture->time_code,
                   &p_pic[pic_index].time_code, sizeof(Mpeg2DecTime));
}

/*------------------------------------------------------------------------------

    Function name: Mpeg2SetRegs

    Functional description:
        Set registers

    Input:
        container

    Return values:
        void

------------------------------------------------------------------------------*/
static u32 Mpeg2SetRegs(Mpeg2DecContainer * dec_cont, addr_t strm_bus_address) {
  addr_t tmp = 0;
  u32 tmp_fwd, tmp_curr;
  u32 hw_build_id = DWLReadHwBuildID(DWL_CLIENT_TYPE_MPEG2_DEC);
  struct DecHwFeatures hw_feature;
  addr_t mask;

  GetReleaseHwFeaturesByID(hw_build_id, &hw_feature);

#ifdef _DEC_PP_USAGE
  Mpeg2DecPpUsagePrint(dec_cont, DECPP_UNSPECIFIED,
                       dec_cont->StrmStorage.work_out, 1,
                       dec_cont->StrmStorage.latest_id);
#endif

  /*
  if(dec_cont->Hdrs.interlaced)
      SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_OUT_TILED_E, 0);
  */

  if (hw_feature.g1_strm_128bit_align)
    mask = 15;
  else
    mask = 7;

  MPEG2DEC_API_DEBUG(("Decoding to index %d \n",
                      dec_cont->StrmStorage.work_out));

  /* swReg3 */
  SetDecRegister(dec_cont->mpeg2_regs, HWIF_PIC_INTERLACE_E,
                 dec_cont->Hdrs.interlaced);

  if(dec_cont->Hdrs.picture_structure == FRAMEPICTURE) {
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_PIC_FIELDMODE_E, 0);
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_TOPFIELDFIRST_E,
                   dec_cont->Hdrs.top_field_first);
  } else {
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_PIC_FIELDMODE_E, 1);
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_PIC_TOPFIELD_E,
                   dec_cont->Hdrs.picture_structure == 1);
#ifdef ASIC_TRACE_SUPPORT
    /* TopFieldFirst will be used for system model to clear PP output buffer at
       the first field in top simulation. */
    if (dec_cont->ApiStorage.first_field)
      SetDecRegister(dec_cont->mpeg2_regs, HWIF_TOPFIELDFIRST_E,
                     dec_cont->Hdrs.picture_structure == 1);
    else
      SetDecRegister(dec_cont->mpeg2_regs, HWIF_TOPFIELDFIRST_E,
                     dec_cont->Hdrs.picture_structure != 1);
#else
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_TOPFIELDFIRST_E,
                   dec_cont->Hdrs.top_field_first);
#endif
  }
  if (!hw_feature.pic_size_reg_unified) {
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_PIC_MB_HEIGHT_P,
                   dec_cont->FrameDesc.frame_height);
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_PIC_MB_H_EXT,
                   dec_cont->FrameDesc.frame_height >> 8);
  } else {
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_PIC_HEIGHT_IN_CBS,
                   dec_cont->FrameDesc.frame_height << 1);
  }

  if(dec_cont->Hdrs.picture_coding_type == BFRAME || dec_cont->Hdrs.picture_coding_type == DFRAME)  /* ? */
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_PIC_B_E, 1);
  else
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_PIC_B_E, 0);

  SetDecRegister(dec_cont->mpeg2_regs, HWIF_PIC_INTER_E,
                 dec_cont->Hdrs.picture_coding_type == PFRAME ||
                 dec_cont->Hdrs.picture_coding_type == BFRAME ? 1 : 0);

  SetDecRegister(dec_cont->mpeg2_regs, HWIF_FWD_INTERLACE_E, 0);  /* ??? */

  /* Never write out mvs, as SW doesn't allocate any memory for them */
  SetDecRegister(dec_cont->mpeg2_regs, HWIF_WRITE_MVS_E, 0 );

  if (!hw_feature.pic_size_reg_unified)
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_ALT_SCAN_E_V0,
                   dec_cont->Hdrs.alternate_scan);
  else
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_ALT_SCAN_E,
                   dec_cont->Hdrs.alternate_scan);


  /* swReg5 */

  /* tmp is strm_bus_address + number of bytes decoded by SW */
  tmp = dec_cont->StrmDesc.strm_curr_pos -
        dec_cont->StrmDesc.p_strm_buff_start;
  tmp = strm_bus_address + tmp;

  /* bus address must not be zero */
  if(!(tmp & ~(mask))) {
    return 0;
  }

  /* pointer to start of the stream, mask to get the pointer to
   * previous 64/128-bit aligned position */
  SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_RLC_VLC_BASE, tmp & ~(mask));

  /* amount of stream (as seen by the HW), obtained as amount of
   * stream given by the application subtracted by number of bytes
   * decoded by SW (if strm_bus_address is not 64/128-bit aligned -> adds
   * number of bytes from previous 64/128-bit aligned boundary) */
  SetDecRegister(dec_cont->mpeg2_regs, HWIF_STREAM_LEN,
                 dec_cont->StrmDesc.strm_buff_size -
                 ((tmp & ~(mask)) - strm_bus_address));

  SetDecRegister(dec_cont->mpeg2_regs, HWIF_STRM_BUFFER_LEN,
                 dec_cont->StrmDesc.strm_buff_size -
                 ((tmp & ~(mask)) - strm_bus_address));
  SetDecRegister(dec_cont->mpeg2_regs, HWIF_STRM_START_OFFSET, 0);

  SetDecRegister(dec_cont->mpeg2_regs, HWIF_STRM_START_BIT,
                 dec_cont->StrmDesc.bit_pos_in_word + 8 * (tmp & (mask)));

  SetDecRegister(dec_cont->mpeg2_regs, HWIF_CH_QP_OFFSET, 0); /* ? */

  SetDecRegister(dec_cont->mpeg2_regs, HWIF_QSCALE_TYPE,
                 dec_cont->Hdrs.quant_type);

  SetDecRegister(dec_cont->mpeg2_regs, HWIF_CON_MV_E, dec_cont->Hdrs.concealment_motion_vectors);  /* ? */

  SetDecRegister(dec_cont->mpeg2_regs, HWIF_INTRA_DC_PREC,
                 dec_cont->Hdrs.intra_dc_precision);

  SetDecRegister(dec_cont->mpeg2_regs, HWIF_INTRA_VLC_TAB,
                 dec_cont->Hdrs.intra_vlc_format);

  SetDecRegister(dec_cont->mpeg2_regs, HWIF_FRAME_PRED_DCT,
                 dec_cont->Hdrs.frame_pred_frame_dct);

  /* swReg6 */

  SetDecRegister(dec_cont->mpeg2_regs, HWIF_INIT_QP, 1);

  /* swReg12 */
  /*SetDecRegister(dec_cont->mpeg2_regs, HWIF_RLC_VLC_BASE, 0);     */

  /* swReg13 */
  if(dec_cont->Hdrs.picture_structure == TOPFIELD ||
      dec_cont->Hdrs.picture_structure == FRAMEPICTURE) {
    SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_DEC_OUT_BASE,
                 dec_cont->StrmStorage.p_pic_buf[dec_cont->
                     StrmStorage.work_out].
                 data.bus_address);
  } else {
    /* start of bottom field line */
    if(dec_cont->dpb_mode == DEC_DPB_FRAME ) {
      SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_DEC_OUT_BASE,
                   (dec_cont->StrmStorage.p_pic_buf[dec_cont->
                       StrmStorage.work_out].
                    data.bus_address +
                    ((dec_cont->FrameDesc.frame_width << 4))));
    } else if( dec_cont->dpb_mode == DEC_DPB_INTERLACED_FIELD ) {
      SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_DEC_OUT_BASE,
                   dec_cont->StrmStorage.p_pic_buf[dec_cont->
                       StrmStorage.work_out].
                   data.bus_address);
    }
  }
  SetDecRegister( dec_cont->mpeg2_regs, HWIF_DPB_ILACE_MODE,
                  dec_cont->dpb_mode );

  SetDecRegister(dec_cont->mpeg2_regs, HWIF_PP_CR_FIRST, dec_cont->cr_first);
  SetDecRegister( dec_cont->mpeg2_regs, HWIF_PP_OUT_E_U, dec_cont->pp_enabled );
  if (dec_cont->pp_enabled &&
      hw_feature.pp_support && hw_feature.pp_version) {
    PpUnitIntConfig *ppu_cfg = &dec_cont->ppu_cfg[0];
    addr_t ppu_out_bus_addr = dec_cont->StrmStorage.p_pic_buf[dec_cont->
                              StrmStorage.work_out].pp_data->bus_address;
    u32 bottom_flag = !(dec_cont->Hdrs.picture_structure == FRAMEPICTURE ||
                        dec_cont->Hdrs.picture_structure == 1);
    PPSetRegs(dec_cont->mpeg2_regs, &hw_feature, ppu_cfg, ppu_out_bus_addr,
              0, bottom_flag);
#if 0
    u32 ppw, pph;
#define TOFIX(d, q) ((u32)( (d) * (u32)(1<<(q)) ))
#define FDIVI(a, b) ((a)/(b))
    u32 pp_field_offset = 0;  // offset to bottom field
    if (hw_feature.pp_version == FIXED_DS_PP) {
        /* G1V8_1 only supports fixed ratio (1/2/4/8) */
      ppw = NEXT_MULTIPLE((dec_cont->FrameDesc.frame_width * 16 >> dec_cont->dscale_shift_x), ALIGN(dec_cont->align));
      pph = (dec_cont->FrameDesc.frame_height * 16 >> dec_cont->dscale_shift_y);
      if (dec_cont->dscale_shift_x == 0) {
        SetDecRegister(dec_cont->mpeg2_regs, HWIF_HOR_SCALE_MODE_U, 0);
        SetDecRegister(dec_cont->mpeg2_regs, HWIF_WSCALE_INVRA_U, 0);
      } else {
        /* down scale */
        SetDecRegister(dec_cont->mpeg2_regs, HWIF_HOR_SCALE_MODE_U, 2);
        SetDecRegister(dec_cont->mpeg2_regs, HWIF_WSCALE_INVRA_U,
                       1<<(16-dec_cont->dscale_shift_x));
      }

      if (dec_cont->dscale_shift_y == 0) {
        SetDecRegister(dec_cont->mpeg2_regs, HWIF_VER_SCALE_MODE_U, 0);
        SetDecRegister(dec_cont->mpeg2_regs, HWIF_HSCALE_INVRA_U, 0);
      } else {
        /* down scale */
        SetDecRegister(dec_cont->mpeg2_regs, HWIF_VER_SCALE_MODE_U, 2);
        SetDecRegister(dec_cont->mpeg2_regs, HWIF_HSCALE_INVRA_U,
                       1<<(16-dec_cont->dscale_shift_y));
      }
    } else {
      /* flexible scale ratio */
      u32 in_width = dec_cont->crop_width;
      u32 in_height = dec_cont->crop_height;
      u32 out_width = dec_cont->scaled_width;
      u32 out_height = dec_cont->scaled_height;

      ppw = NEXT_MULTIPLE(dec_cont->scaled_width,  ALIGN(dec_cont->align));
      pph = dec_cont->scaled_height;
      SetDecRegister(dec_cont->mpeg2_regs, HWIF_CROP_STARTX_U,
                     dec_cont->crop_startx >> hw_feature.crop_step_rshift);
      SetDecRegister(dec_cont->mpeg2_regs, HWIF_CROP_STARTY_U,
                     dec_cont->crop_starty >> hw_feature.crop_step_rshift);
      SetDecRegister(dec_cont->mpeg2_regs, HWIF_PP_IN_WIDTH_U,
                     dec_cont->crop_width >> hw_feature.crop_step_rshift);
      SetDecRegister(dec_cont->mpeg2_regs, HWIF_PP_IN_HEIGHT_U,
                     dec_cont->crop_height >> hw_feature.crop_step_rshift);
      SetDecRegister(dec_cont->mpeg2_regs, HWIF_PP_OUT_WIDTH_U,
                     dec_cont->scaled_width);
      SetDecRegister(dec_cont->mpeg2_regs, HWIF_PP_OUT_HEIGHT_U,
                     dec_cont->scaled_height);

      if(in_width < out_width) {
        /* upscale */
        u32 W, inv_w;

        SetDecRegister(dec_cont->mpeg2_regs, HWIF_HOR_SCALE_MODE_U, 1);

        W = FDIVI(TOFIX((out_width - 1), 16), (in_width - 1));
        inv_w = FDIVI(TOFIX((in_width - 1), 16), (out_width - 1));

        SetDecRegister(dec_cont->mpeg2_regs, HWIF_SCALE_WRATIO_U, W);
        SetDecRegister(dec_cont->mpeg2_regs, HWIF_WSCALE_INVRA_U, inv_w);
      } else if(in_width > out_width) {
        /* downscale */
        u32 hnorm;

        SetDecRegister(dec_cont->mpeg2_regs, HWIF_HOR_SCALE_MODE_U, 2);

        hnorm = FDIVI(TOFIX(out_width, 16), in_width);
        SetDecRegister(dec_cont->mpeg2_regs, HWIF_WSCALE_INVRA_U, hnorm);
      } else {
        SetDecRegister(dec_cont->mpeg2_regs, HWIF_WSCALE_INVRA_U, 0);
        SetDecRegister(dec_cont->mpeg2_regs, HWIF_HOR_SCALE_MODE_U, 0);
      }

      if(in_height < out_height) {
        /* upscale */
        u32 H, inv_h;

        SetDecRegister(dec_cont->mpeg2_regs, HWIF_VER_SCALE_MODE_U, 1);

        H = FDIVI(TOFIX((out_height - 1), 16), (in_height - 1));

        SetDecRegister(dec_cont->mpeg2_regs, HWIF_SCALE_HRATIO_U, H);

        inv_h = FDIVI(TOFIX((in_height - 1), 16), (out_height - 1));

        SetDecRegister(dec_cont->mpeg2_regs, HWIF_HSCALE_INVRA_U, inv_h);
      } else if(in_height > out_height) {
        /* downscale */
        u32 Cv;

        Cv = FDIVI(TOFIX(out_height, 16), in_height) + 1;

        SetDecRegister(dec_cont->mpeg2_regs, HWIF_VER_SCALE_MODE_U, 2);

        SetDecRegister(dec_cont->mpeg2_regs, HWIF_HSCALE_INVRA_U, Cv);
      } else {
        SetDecRegister(dec_cont->mpeg2_regs, HWIF_HSCALE_INVRA_U, 0);
        SetDecRegister(dec_cont->mpeg2_regs, HWIF_VER_SCALE_MODE_U, 0);
      }
    }
    if (hw_feature.pp_stride_support) {
      SetDecRegister(dec_cont->mpeg2_regs, HWIF_PP_OUT_Y_STRIDE, ppw);
      SetDecRegister(dec_cont->mpeg2_regs, HWIF_PP_OUT_C_STRIDE, ppw);
    }
    if ((dec_cont->dpb_mode == DEC_DPB_FRAME ) &&
        !(dec_cont->Hdrs.picture_structure == TOPFIELD ||
          dec_cont->Hdrs.picture_structure == FRAMEPICTURE)) {
      pp_field_offset = ppw;
    }
    SET_ADDR64_REG(dec_cont->mpeg2_regs, HWIF_PP_OUT_LU_BASE_U,
                   dec_cont->StrmStorage.p_pic_buf[dec_cont->
                       StrmStorage.work_out].
                   pp_data->bus_address + pp_field_offset);
    SET_ADDR64_REG(dec_cont->mpeg2_regs, HWIF_PP_OUT_CH_BASE_U,
                   dec_cont->StrmStorage.p_pic_buf[dec_cont->
                       StrmStorage.work_out].
                   pp_data->bus_address + ppw * pph + pp_field_offset);
#endif
    SetPpRegister(dec_cont->mpeg2_regs, HWIF_PP_IN_FORMAT_U, 1);
  }
  if (hw_feature.dec_stride_support) {
    /* Stride registers only available since g1v8_2 */
    if (dec_cont->tiled_stride_enable) {
      SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_OUT_Y_STRIDE,
                     NEXT_MULTIPLE(dec_cont->FrameDesc.frame_width * 4 * 16, ALIGN(dec_cont->align)));
      SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_OUT_C_STRIDE,
                     NEXT_MULTIPLE(dec_cont->FrameDesc.frame_width * 4 * 16, ALIGN(dec_cont->align)));
    } else {
      SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_OUT_Y_STRIDE,
                     dec_cont->FrameDesc.frame_width * 64);
      SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_OUT_C_STRIDE,
                     dec_cont->FrameDesc.frame_width * 64);
    }
  }
  if(dec_cont->Hdrs.picture_structure == FRAMEPICTURE) {
    if(dec_cont->Hdrs.picture_coding_type == BFRAME) { /* ? */
      /* past anchor set to future anchor if past is invalid (second
       * picture in sequence is B) */
      tmp_fwd =
        dec_cont->StrmStorage.work1 != INVALID_ANCHOR_PICTURE ?
        dec_cont->StrmStorage.work1 :
        dec_cont->StrmStorage.work0;

      SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_REFER0_BASE,
                   dec_cont->StrmStorage.p_pic_buf[tmp_fwd].data.
                   bus_address);
      SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_REFER1_BASE,
                   dec_cont->StrmStorage.p_pic_buf[tmp_fwd].data.
                   bus_address);
      SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_REFER2_BASE,
                   dec_cont->StrmStorage.
                   p_pic_buf[(i32)dec_cont->StrmStorage.work0].data.
                   bus_address);
      SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_REFER3_BASE,
                   dec_cont->StrmStorage.
                   p_pic_buf[(i32)dec_cont->StrmStorage.work0].data.
                   bus_address);
    } else {
      SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_REFER0_BASE,
                   dec_cont->StrmStorage.
                   p_pic_buf[(i32)dec_cont->StrmStorage.work0].data.
                   bus_address);
      SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_REFER1_BASE,
                   dec_cont->StrmStorage.
                   p_pic_buf[(i32)dec_cont->StrmStorage.work0].data.
                   bus_address);
    }
  } else {
    if(dec_cont->Hdrs.picture_coding_type == BFRAME)
      /* past anchor set to future anchor if past is invalid */
      tmp_fwd =
        dec_cont->StrmStorage.work1 != INVALID_ANCHOR_PICTURE ?
        dec_cont->StrmStorage.work1 :
        dec_cont->StrmStorage.work0;
    else
      tmp_fwd = dec_cont->StrmStorage.work0;
    tmp_curr = dec_cont->StrmStorage.work_out;
    /* past anchor not available -> use current (this results in using the
     * same top or bottom field as reference and output picture base,
     * output is probably corrupted) */
    if(tmp_fwd == INVALID_ANCHOR_PICTURE)
      tmp_fwd = tmp_curr;

    if(dec_cont->ApiStorage.first_field ||
        dec_cont->Hdrs.picture_coding_type == BFRAME)
      /*
       * if ((dec_cont->Hdrs.picture_structure == 1 &&
       * dec_cont->Hdrs.top_field_first) ||
       * (dec_cont->Hdrs.picture_structure == 2 &&
       * !dec_cont->Hdrs.top_field_first))
       */
    {
      SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_REFER0_BASE,
                   dec_cont->StrmStorage.p_pic_buf[tmp_fwd].data.
                   bus_address);
      SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_REFER1_BASE,
                   dec_cont->StrmStorage.p_pic_buf[tmp_fwd].data.
                   bus_address);
    } else if(dec_cont->Hdrs.picture_structure == 1) {
      SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_REFER0_BASE,
                   dec_cont->StrmStorage.p_pic_buf[tmp_fwd].data.
                   bus_address);
      SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_REFER1_BASE,
                   dec_cont->StrmStorage.p_pic_buf[tmp_curr].data.
                   bus_address);
    } else if(dec_cont->Hdrs.picture_structure == 2) {
      SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_REFER0_BASE,
                   dec_cont->StrmStorage.p_pic_buf[tmp_curr].data.
                   bus_address);
      SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_REFER1_BASE,
                   dec_cont->StrmStorage.p_pic_buf[tmp_fwd].data.
                   bus_address);
    }
    SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_REFER2_BASE,
                 dec_cont->StrmStorage.p_pic_buf[(i32)dec_cont->
                     StrmStorage.
                     work0].data.
                 bus_address);
    SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_REFER3_BASE,
                 dec_cont->StrmStorage.p_pic_buf[(i32)dec_cont->
                     StrmStorage.
                     work0].data.
                 bus_address);
  }

  /* swReg18 */
  SetDecRegister(dec_cont->mpeg2_regs, HWIF_ALT_SCAN_FLAG_E,
                 dec_cont->Hdrs.alternate_scan);

  /* ? */
  SetDecRegister(dec_cont->mpeg2_regs, HWIF_FCODE_FWD_HOR,
                 dec_cont->Hdrs.f_code_fwd_hor);
  SetDecRegister(dec_cont->mpeg2_regs, HWIF_FCODE_FWD_VER,
                 dec_cont->Hdrs.f_code_fwd_ver);
  SetDecRegister(dec_cont->mpeg2_regs, HWIF_FCODE_BWD_HOR,
                 dec_cont->Hdrs.f_code_bwd_hor);
  SetDecRegister(dec_cont->mpeg2_regs, HWIF_FCODE_BWD_VER,
                 dec_cont->Hdrs.f_code_bwd_ver);

  if(!dec_cont->Hdrs.mpeg2_stream && dec_cont->Hdrs.f_code[0][0]) {
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_MV_ACCURACY_FWD, 0);
  } else
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_MV_ACCURACY_FWD, 1);

  if(!dec_cont->Hdrs.mpeg2_stream && dec_cont->Hdrs.f_code[1][0]) {
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_MV_ACCURACY_BWD, 0);
  } else
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_MV_ACCURACY_BWD, 1);

  /* swReg40 */
  SET_ADDR_REG(dec_cont->mpeg2_regs, HWIF_QTABLE_BASE,
               dec_cont->ApiStorage.p_qtable_base.bus_address);

  /* swReg48 */
  SetDecRegister(dec_cont->mpeg2_regs, HWIF_STARTMB_X, 0);
  SetDecRegister(dec_cont->mpeg2_regs, HWIF_STARTMB_Y, 0);

  SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_OUT_DIS, 0);
  SetDecRegister(dec_cont->mpeg2_regs, HWIF_FILTERING_DIS, 1);

  /* Setup reference picture buffer */
  if(dec_cont->ref_buf_support) {
    RefbuSetup(&dec_cont->ref_buffer_ctrl, dec_cont->mpeg2_regs,
               (dec_cont->Hdrs.picture_structure == BOTTOMFIELD ||
                dec_cont->Hdrs.picture_structure == TOPFIELD) ?
               REFBU_FIELD : REFBU_FRAME,
               dec_cont->Hdrs.picture_coding_type == IFRAME,
               dec_cont->Hdrs.picture_coding_type == BFRAME, 0, 2,
               0 );
  }

  if( dec_cont->tiled_mode_support) {
    dec_cont->tiled_reference_enable =
      DecSetupTiledReference( dec_cont->mpeg2_regs,
                              dec_cont->tiled_mode_support,
                              dec_cont->dpb_mode,
                              !dec_cont->Hdrs.progressive_sequence ||
                              !dec_cont->Hdrs.frame_pred_frame_dct
                               );
  } else {
    dec_cont->tiled_reference_enable = 0;
  }
  return HANTRO_OK;
}


/*------------------------------------------------------------------------------

    Function name: Mp2CheckFormatSupport

    Functional description:
        Check if mpeg2 supported

    Input:
        container

    Return values:
        return zero for OK

------------------------------------------------------------------------------*/
u32 Mp2CheckFormatSupport(void) {
  u32 hw_build_id;
  struct DecHwFeatures hw_feature;

  hw_build_id = DWLReadHwBuildID(DWL_CLIENT_TYPE_MPEG2_DEC);
  GetReleaseHwFeaturesByID(hw_build_id, &hw_feature);
  return (hw_feature.mpeg2_support == MPEG2_NOT_SUPPORTED);
}


/*------------------------------------------------------------------------------

    Function name: Mpeg2CheckReleasePpAndHw

    Functional description:
        Function handles situations when PP was left running and decoder
        reserved after decoding of first field of a frame was finished but
        error occurred and second field of the picture in question is lost
        or corrupted

    Input:
        container

    Return values:
        void

------------------------------------------------------------------------------*/
void Mpeg2CheckReleasePpAndHw(Mpeg2DecContainer *dec_cont) {
  if (dec_cont->keep_hw_reserved) {
    (void) DWLReleaseHw(dec_cont->dwl, dec_cont->core_id);
    dec_cont->keep_hw_reserved = 0;
  }
}


/*------------------------------------------------------------------------------

    Function name: Mpeg2DecPeek

    Functional description:
        Retrieve last decoded picture

    Input:
        dec_inst     Reference to decoder instance.
        picture    Pointer to return value struct

    Output:
        picture Decoder output picture.

    Return values:
        MPEG2DEC_OK         No picture available.
        MPEG2DEC_PIC_RDY    Picture ready.

------------------------------------------------------------------------------*/
Mpeg2DecRet Mpeg2DecPeek(Mpeg2DecInst dec_inst, Mpeg2DecPicture * picture) {
  /* Variables */
  Mpeg2DecRet return_value = MPEG2DEC_PIC_RDY;
  Mpeg2DecContainer *dec_cont;
  picture_t *p_pic;
  u32 pic_index = MPEG2_BUFFER_UNDEFINED;

  /* Code */
  MPEG2_API_TRC("\nMpeg2_dec_peek#");

  /* Check that function input parameters are valid */
  if(picture == NULL) {
    MPEG2_API_TRC("Mpeg2DecPeek# ERROR: picture is NULL");
    return (MPEG2DEC_PARAM_ERROR);
  }

  dec_cont = (Mpeg2DecContainer *) dec_inst;

  /* Check if decoder is in an incorrect mode */
  if(dec_inst == NULL || dec_cont->ApiStorage.DecStat == UNINIT) {
    MPEG2_API_TRC("Mpeg2DecPeek# ERROR: Decoder not initialized");
    return (MPEG2DEC_NOT_INITIALIZED);
  }

  /* no output pictures available */
  /* when output release thread enabled, Mpeg2DecNextPicture_INTERNAL() called in
     Mpeg2DecDecode(), and "dec_cont->StrmStorage.out_count--" may called in
     Mpeg2DecNextPicture() before Mpeg2DecPeek() called, so dec_cont->fullness
     used to sample the real out_count in case of Mpeg2DecNextPicture() called
     before than Mpeg2DecPeek() */
  u32 tmp = dec_cont->fullness;
  if(!tmp || dec_cont->StrmStorage.new_headers_change_resolution) {
    (void) DWLmemset(picture, 0, sizeof(Mpeg2DecPicture));
    picture->pictures[0].output_picture = NULL;
    picture->interlaced = dec_cont->Hdrs.interlaced;
    /* print nothing to send out */
    return_value = MPEG2DEC_OK;
  } else {
    /* output current (last decoded) picture */
    pic_index = dec_cont->StrmStorage.work_out;
    if (!dec_cont->pp_enabled) {
      picture->pictures[0].frame_width = dec_cont->FrameDesc.frame_width << 4;
      picture->pictures[0].frame_height = dec_cont->FrameDesc.frame_height << 4;
      picture->pictures[0].coded_width = dec_cont->Hdrs.horizontal_size;
      picture->pictures[0].coded_height = dec_cont->Hdrs.vertical_size;
    } else {
      picture->pictures[0].frame_width = (dec_cont->FrameDesc.frame_width << 4) >> dec_cont->dscale_shift_x;
      picture->pictures[0].frame_height = (dec_cont->FrameDesc.frame_height << 4) >> dec_cont->dscale_shift_y;
      picture->pictures[0].coded_width = dec_cont->Hdrs.horizontal_size >> dec_cont->dscale_shift_x;
      picture->pictures[0].coded_height = dec_cont->Hdrs.vertical_size >> dec_cont->dscale_shift_y;
    }

    picture->interlaced = dec_cont->Hdrs.interlaced;

    p_pic = dec_cont->StrmStorage.p_pic_buf + pic_index;
    if (!dec_cont->pp_enabled) {
      picture->pictures[0].output_picture = (u8 *) p_pic->data.virtual_address;
      picture->pictures[0].output_picture_bus_address = p_pic->data.bus_address;
    } else {
      picture->pictures[0].output_picture = (u8 *) p_pic->pp_data->virtual_address;
      picture->pictures[0].output_picture_bus_address = p_pic->pp_data->bus_address;
    }
    picture->key_picture = p_pic->pic_type;
    picture->pic_id = p_pic->pic_id;
    picture->decode_id = p_pic->pic_id;
    picture->pic_coding_type[0] = p_pic->pic_code_type[0];
    picture->pic_coding_type[1] = p_pic->pic_code_type[1];

    picture->repeat_first_field = p_pic->rff;
    picture->repeat_frame_count = p_pic->rfc;
    picture->number_of_err_mbs = p_pic->nbr_err_mbs;

    (void) DWLmemcpy(&picture->time_code,
                     &p_pic->time_code, sizeof(Mpeg2DecTime));

    /* frame output */
    picture->field_picture = 0;
    picture->top_field = 0;
    picture->first_field = 0;
    picture->pictures[0].output_format = p_pic->tiled_mode ?
                             DEC_OUT_FRM_TILED_4X4 : DEC_OUT_FRM_RASTER_SCAN;
  }

  return return_value;
}

void Mpeg2SetExternalBufferInfo(Mpeg2DecInst dec_inst) {
  Mpeg2DecContainer *dec_cont = (Mpeg2DecContainer *)dec_inst;
  u32 ext_buffer_size;
  u32 buffers = 3;

  u32 ref_buff_size = mpeg2GetRefFrmSize(dec_cont);
  ext_buffer_size = ref_buff_size;

  buffers = dec_cont->StrmStorage.max_num_buffers;
  if( buffers < 3 )
    buffers = 3;

  if (dec_cont->pp_enabled) {
    PpUnitIntConfig *ppu_cfg = dec_cont->ppu_cfg;
    ext_buffer_size = CalcPpUnitBufferSize(ppu_cfg, 0);
  }

  dec_cont->ext_min_buffer_num = dec_cont->buf_num =  buffers;
  dec_cont->next_buf_size = ext_buffer_size;
}

Mpeg2DecRet Mpeg2DecGetBufferInfo(Mpeg2DecInst dec_inst, Mpeg2DecBufferInfo *mem_info) {
  Mpeg2DecContainer  * dec_cont = (Mpeg2DecContainer *)dec_inst;

  struct DWLLinearMem empty = {0, 0, 0};

  struct DWLLinearMem *buffer = NULL;

  if(dec_cont == NULL || mem_info == NULL) {
    return MPEG2DEC_PARAM_ERROR;
  }

  if (dec_cont->StrmStorage.release_buffer) {
    /* Release old buffers from input queue. */
    //buffer = InputQueueGetBuffer(decCont->pp_buffer_queue, 0);
    buffer = NULL;
    if (dec_cont->ext_buffer_num) {
      buffer = &dec_cont->ext_buffers[dec_cont->ext_buffer_num - 1];
      dec_cont->ext_buffer_num--;
    }
    if (buffer == NULL) {
      /* All buffers have been released. */
      dec_cont->StrmStorage.release_buffer = 0;
      InputQueueRelease(dec_cont->pp_buffer_queue);
      dec_cont->pp_buffer_queue = InputQueueInit(0);
      if (dec_cont->pp_buffer_queue == NULL) {
        return (MPEG2DEC_MEMFAIL);
      }
      dec_cont->StrmStorage.ext_buffer_added = 0;
      mem_info->buf_to_free = empty;
      mem_info->next_buf_size = 0;
      mem_info->buf_num = 0;
      return MPEG2DEC_OK;
    } else {
      mem_info->buf_to_free = *buffer;
      mem_info->next_buf_size = 0;
      mem_info->buf_num = 0;
      return MPEG2DEC_WAITING_FOR_BUFFER;
    }
  }

  if(dec_cont->buf_to_free == NULL && dec_cont->next_buf_size == 0) {
    /* External reference buffer: release done. */
    mem_info->buf_to_free = empty;
    mem_info->next_buf_size = dec_cont->next_buf_size;
    mem_info->buf_num = dec_cont->buf_num + dec_cont->n_guard_size;
    return MPEG2DEC_OK;
  }

  if(dec_cont->buf_to_free) {
    mem_info->buf_to_free = *dec_cont->buf_to_free;
    dec_cont->buf_to_free->virtual_address = NULL;
    dec_cont->buf_to_free = NULL;
  } else
    mem_info->buf_to_free = empty;

  mem_info->next_buf_size = dec_cont->next_buf_size;
  mem_info->buf_num = dec_cont->buf_num + dec_cont->n_guard_size;

  ASSERT((mem_info->buf_num && mem_info->next_buf_size) ||
         (mem_info->buf_to_free.virtual_address != NULL));

  return MPEG2DEC_WAITING_FOR_BUFFER;
}

Mpeg2DecRet Mpeg2DecAddBuffer(Mpeg2DecInst dec_inst, struct DWLLinearMem *info) {
  Mpeg2DecContainer *dec_cont = (Mpeg2DecContainer *)dec_inst;
  Mpeg2DecRet dec_ret = MPEG2DEC_OK;
  u32 i = dec_cont->buffer_index;

  if(dec_inst == NULL || info == NULL ||
      X170_CHECK_VIRTUAL_ADDRESS(info->virtual_address) ||
      X170_CHECK_BUS_ADDRESS_AGLINED(info->bus_address) ||
      info->size < dec_cont->next_buf_size) {
    return MPEG2DEC_PARAM_ERROR;
  }

  if (dec_cont->buffer_index >= 16)
    /* Too much buffers added. */
    return MPEG2DEC_EXT_BUFFER_REJECTED;

  dec_cont->ext_buffers[dec_cont->ext_buffer_num] = *info;
  dec_cont->ext_buffer_num++;
  dec_cont->buffer_index++;
  dec_cont->n_ext_buf_size = info->size;

  /* buffer is not enoughm, return WAITING_FOR_BUFFER */
  if (dec_cont->buffer_index < dec_cont->ext_min_buffer_num)
    dec_ret = MPEG2DEC_WAITING_FOR_BUFFER;

  if (dec_cont->pp_enabled == 0) {
    dec_cont->StrmStorage.p_pic_buf[i].data = *info;
    if(dec_cont->buffer_index > dec_cont->ext_min_buffer_num) {
      /* Adding extra buffers. */
      dec_cont->StrmStorage.bq.queue_size++;
      dec_cont->StrmStorage.num_buffers++;
    }
  } else {
    /* Add down scale buffer. */
    InputQueueAddBuffer(dec_cont->pp_buffer_queue, info);
  }
  dec_cont->StrmStorage.ext_buffer_added = 1;
  return dec_ret;
}

void Mpeg2EnterAbortState(Mpeg2DecContainer *dec_cont) {
  dec_cont->abort = 1;
  BqueueSetAbort(&dec_cont->StrmStorage.bq);
#ifdef USE_OMXIL_BUFFER
  FifoSetAbort(dec_cont->fifo_display);
#endif
  if (dec_cont->pp_enabled)
    InputQueueSetAbort(dec_cont->pp_buffer_queue);

}

void Mpeg2ExistAbortState(Mpeg2DecContainer *dec_cont) {
  dec_cont->abort = 0;
  BqueueClearAbort(&dec_cont->StrmStorage.bq);
#ifdef USE_OMXIL_BUFFER
  FifoClearAbort(dec_cont->fifo_display);
#endif
  if (dec_cont->pp_enabled)
    InputQueueClearAbort(dec_cont->pp_buffer_queue);
}

void Mpeg2EmptyBufferQueue(Mpeg2DecContainer *dec_cont) {
  BqueueEmpty(&dec_cont->StrmStorage.bq);
  dec_cont->StrmStorage.work_out_prev = 0;
  dec_cont->StrmStorage.work_out = 0;
  dec_cont->StrmStorage.work0 =
    dec_cont->StrmStorage.work1 = INVALID_ANCHOR_PICTURE;
}

void Mpeg2StateReset(Mpeg2DecContainer *dec_cont) {
  u32 buffers = 3;

  buffers = dec_cont->StrmStorage.max_num_buffers;
  if( buffers < 3 )
    buffers = 3;

  /* Clear parameters in decContainer */
  dec_cont->keep_hw_reserved = 0;
  dec_cont->unpaired_field = 0;

#ifdef USE_OMXIL_BUFFER
  dec_cont->ext_min_buffer_num = buffers;
  dec_cont->buffer_index = 0;
  dec_cont->ext_buffer_num = 0;
#endif
  dec_cont->realloc_ext_buf = 0;
  dec_cont->realloc_int_buf = 0;
  dec_cont->fullness = 0;
#ifdef USE_OMXIL_BUFFER
  dec_cont->fifo_index = 0;
  dec_cont->ext_buffer_num = 0;
#endif
  dec_cont->dec_stat = MPEG2DEC_OK;
  dec_cont->field_rdy = 0;

  /* Clear parameters in DecStrmStorage */
#ifdef CLEAR_HDRINFO_IN_SEEK
  dec_cont->StrmStorage.strm_dec_ready = 0;
  dec_cont->StrmStorage.valid_pic_header = 0;
  dec_cont->StrmStorage.valid_pic_ext_header = 0;
  dec_cont->StrmStorage.valid_sequence = 0;
#endif
  dec_cont->StrmStorage.error_in_hdr = 0;
#ifdef USE_OMXIL_BUFFER
  dec_cont->StrmStorage.bq.queue_size = buffers;
  dec_cont->StrmStorage.num_buffers = buffers;
#endif
  dec_cont->StrmStorage.out_index = 0;
  dec_cont->StrmStorage.out_count = 0;
  dec_cont->StrmStorage.skip_b = 0;
  dec_cont->StrmStorage.prev_pic_coding_type = 0;
  dec_cont->StrmStorage.prev_pic_structure = 0;
  dec_cont->StrmStorage.picture_broken = 0;
  dec_cont->StrmStorage.new_headers_change_resolution = 0;
  dec_cont->StrmStorage.release_buffer = 0;
  dec_cont->StrmStorage.ext_buffer_added = 0;

  /* Clear parameters in DecApiStorage */
  dec_cont->ApiStorage.DecStat = INITIALIZED;
  dec_cont->ApiStorage.first_field = 1;
  dec_cont->ApiStorage.output_other_field = 0;
  dec_cont->ApiStorage.ignore_field = 0;
  dec_cont->ApiStorage.parity = 0;

  /* Clear parameters in DecFrameDesc */
  dec_cont->FrameDesc.frame_number = 0;
  dec_cont->FrameDesc.pic_coding_type = 0;
  dec_cont->FrameDesc.field_coding_type[0] = 0;
  dec_cont->FrameDesc.field_coding_type[1] = 0;
#ifdef CLEAR_HDRINFO_IN_SEEK
  dec_cont->FrameDesc.frame_time_pictures = 0;
  dec_cont->FrameDesc.time_code_hours = 0;
  dec_cont->FrameDesc.time_code_minutes = 0;
  dec_cont->FrameDesc.time_code_minutes = 0;
#endif

  (void) DWLmemset(&dec_cont->MbSetDesc, 0, sizeof(DecMbSetDesc));
  (void) DWLmemset(&dec_cont->StrmDesc, 0, sizeof(DecStrmDesc));
  (void) DWLmemset(dec_cont->StrmStorage.out_buf, 0, 16 * sizeof(u32));
#ifdef USE_OMXIL_BUFFER
  if (!dec_cont->pp_enabled)
    (void) DWLmemset(dec_cont->StrmStorage.p_pic_buf, 0, 16 * sizeof(picture_t));
  (void) DWLmemset(dec_cont->StrmStorage.picture_info, 0, 32 * sizeof(Mpeg2DecPicture));
#endif
#ifdef CLEAR_HDRINFO_IN_SEEK
  (void) DWLmemset(&(dec_cont->Hdrs), 0, sizeof(DecHdrs));
  (void) DWLmemset(&(dec_cont->tmp_hdrs), 0, sizeof(DecHdrs));
  (void) DWLmemset(&(dec_cont->field_hdrs), 0, sizeof(DecHdrs));
  mpeg2API_InitDataStructures(dec_cont);
#endif

#ifdef USE_OMXIL_BUFFER
  if (dec_cont->fifo_display)
    FifoRelease(dec_cont->fifo_display);
  FifoInit(32, &dec_cont->fifo_display);
#endif
  if (dec_cont->pp_enabled)
    InputQueueReset(dec_cont->pp_buffer_queue);
}

Mpeg2DecRet Mpeg2DecAbort(Mpeg2DecInst dec_inst) {
  Mpeg2DecContainer *dec_cont = (Mpeg2DecContainer *) dec_inst;

  MPEG2_API_TRC("Mpeg2DecAbort#\n");

  /* Check if decoder is in an incorrect mode */
  if(dec_inst == NULL || dec_cont->ApiStorage.DecStat == UNINIT) {
    MPEG2_API_TRC("Mpeg2DecAbort# ERROR: Decoder not initialized");
    return (MPEG2DEC_NOT_INITIALIZED);
  }

  pthread_mutex_lock(&dec_cont->protect_mutex);
  /* Abort frame buffer waiting */
  Mpeg2EnterAbortState(dec_cont);
  pthread_mutex_unlock(&dec_cont->protect_mutex);

  MPEG2_API_TRC("Mpeg2DecAbort# MPEG2DEC_OK\n");
  return (MPEG2DEC_OK);
}

Mpeg2DecRet Mpeg2DecAbortAfter(Mpeg2DecInst dec_inst) {
  Mpeg2DecContainer *dec_cont = (Mpeg2DecContainer *) dec_inst;

  MPEG2_API_TRC("Mpeg2DecAbortAfter#\n");

  /* Check if decoder is in an incorrect mode */
  if(dec_inst == NULL || dec_cont->ApiStorage.DecStat == UNINIT) {
    MPEG2_API_TRC("Mpeg2DecAbortAfter# ERROR: Decoder not initialized");
    return (MPEG2DEC_NOT_INITIALIZED);
  }

  pthread_mutex_lock(&dec_cont->protect_mutex);

#if 0
  /* If normal EOS is waited, return directly */
  if(dec_cont->dec_stat == MPEG2DEC_END_OF_STREAM) {
    pthread_mutex_unlock(&dec_cont->protect_mutex);
    return (MPEG2DEC_OK);
  }
#endif

  /* Stop and release HW */
  if(dec_cont->asic_running) {
    /* stop HW */
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_IRQ_STAT, 0);
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_IRQ, 0);
    SetDecRegister(dec_cont->mpeg2_regs, HWIF_DEC_E, 0);
    DWLDisableHw(dec_cont->dwl, dec_cont->core_id, 4 * 1,
                 dec_cont->mpeg2_regs[1] | DEC_IRQ_DISABLE);
    DWLReleaseHw(dec_cont->dwl, dec_cont->core_id);  /* release HW lock */
    dec_cont->asic_running = 0;
  } else if(dec_cont->keep_hw_reserved) {
    DWLReleaseHw(dec_cont->dwl, dec_cont->core_id);  /* release HW lock */
    dec_cont->keep_hw_reserved = 0;
  }

  /* Clear any remaining pictures from DPB */
  Mpeg2EmptyBufferQueue(dec_cont);

  Mpeg2StateReset(dec_cont);

  Mpeg2ExistAbortState(dec_cont);

  pthread_mutex_unlock(&dec_cont->protect_mutex);

  MPEG2_API_TRC("Mpeg2DecAbortAfter# MPEG2DEC_OK\n");
  return (MPEG2DEC_OK);
}

Mpeg2DecRet Mpeg2DecSetInfo(Mpeg2DecInst dec_inst,
                            struct Mpeg2DecConfig *dec_cfg) {
#define DEC_FRAMED ((Mpeg2DecContainer *)dec_inst)->FrameDesc
  /*@null@ */ Mpeg2DecContainer *dec_cont = (Mpeg2DecContainer *)dec_inst;
  u32 pic_width = DEC_FRAMED.frame_width << 4;
  u32 pic_height = DEC_FRAMED.frame_height << 4;
  u32 hw_build_id = DWLReadHwBuildID(DWL_CLIENT_TYPE_MPEG2_DEC);
  struct DecHwFeatures hw_feature;
  PpUnitConfig *ppu_cfg = dec_cfg->ppu_config;

  GetReleaseHwFeaturesByID(hw_build_id, &hw_feature);

  PpUnitSetIntConfig(dec_cont->ppu_cfg, ppu_cfg, 8, !dec_cont->Hdrs.interlaced, 0);
  for (u32 i = 0; i < DEC_MAX_PPU_COUNT; i++) {
    if (hw_feature.pp_lanczos[i] && (dec_cont->ppu_cfg[i].lanczos_table.virtual_address == NULL)) {
      u32 size = LANCZOS_BUFFER_SIZE * sizeof(i32);
      i32 ret = DWLMallocLinear(dec_cont->dwl, size, &dec_cont->ppu_cfg[i].lanczos_table);
      if (ret != 0)
        return(MPEG2DEC_MEMFAIL);
    }
  }
  if (CheckPpUnitConfig(&hw_feature, pic_width, pic_height, dec_cont->Hdrs.interlaced, dec_cont->ppu_cfg))
    return MPEG2DEC_PARAM_ERROR;
  if (!hw_feature.dec_stride_support) {
    /* For verion earlier than g1v8_2, reset alignment to 16B. */
    dec_cont->align = DEC_ALIGN_16B;
  } else {
    dec_cont->align = dec_cfg->align;
  }
  dec_cont->ppu_cfg[0].pixel_width = dec_cont->ppu_cfg[1].pixel_width =
  dec_cont->ppu_cfg[2].pixel_width = dec_cont->ppu_cfg[3].pixel_width = 
  dec_cont->ppu_cfg[4].pixel_width = 8;
  dec_cont->cr_first = dec_cfg->cr_first;
  dec_cont->pp_enabled = dec_cont->ppu_cfg[0].enabled |
                         dec_cont->ppu_cfg[1].enabled |
                         dec_cont->ppu_cfg[2].enabled |
                         dec_cont->ppu_cfg[3].enabled |
                         dec_cont->ppu_cfg[4].enabled;
#if 0
  dec_cont->crop_enabled = dec_cfg->crop.enabled;
  if (dec_cont->crop_enabled) {
    dec_cont->crop_startx = dec_cfg->crop.x;
    dec_cont->crop_starty = dec_cfg->crop.y;
    dec_cont->crop_width  = dec_cfg->crop.width;
    dec_cont->crop_height = dec_cfg->crop.height;
  } else {
    dec_cont->crop_startx = 0;
    dec_cont->crop_starty = 0;
    dec_cont->crop_width  = pic_width;
    dec_cont->crop_height = pic_height;
  }

  dec_cont->scale_enabled = dec_cfg->scale.enabled;
  if (dec_cont->scale_enabled) {
    dec_cont->scaled_width = dec_cfg->scale.width;
    dec_cont->scaled_height = dec_cfg->scale.height;
  } else {
    dec_cont->scaled_width = dec_cont->crop_width;
    dec_cont->scaled_height = dec_cont->crop_height;
  }
  if (dec_cont->crop_enabled || dec_cont->scale_enabled)
    dec_cont->pp_enabled = 1;
  else
    dec_cont->pp_enabled = 0;

  // x -> 1.5 ->  3  ->  6
  //    1  |  2   |  4   |  8
  if (dec_cont->scaled_width * 6 <= pic_width)
    dec_cont->dscale_shift_x = 3;
  else if (dec_cont->scaled_width * 3 <= pic_width)
    dec_cont->dscale_shift_x = 2;
  else if (dec_cont->scaled_width * 3 / 2 <= pic_width)
    dec_cont->dscale_shift_x = 1;
  else
    dec_cont->dscale_shift_x = 0;

  if (dec_cont->scaled_height * 6 <= pic_height)
    dec_cont->dscale_shift_y = 3;
  else if (dec_cont->scaled_height * 3 <= pic_height)
    dec_cont->dscale_shift_y = 2;
  else if (dec_cont->scaled_height * 3 / 2 <= pic_height)
    dec_cont->dscale_shift_y = 1;
  else
    dec_cont->dscale_shift_y = 0;
  if (hw_feature.pp_version == FIXED_DS_PP) {
    dec_cont->scaled_width = pic_width >> dec_cont->dscale_shift_x;
    dec_cont->scaled_height = pic_height >> dec_cont->dscale_shift_y;
  }
#endif
  return (MPEG2DEC_OK);
}



void Mpeg2CheckBufferRealloc(Mpeg2DecContainer *dec_cont) {
  dec_cont->realloc_int_buf = 0;
  dec_cont->realloc_ext_buf = 0;
  /* tile output */
  if (!dec_cont->pp_enabled) {
    if (dec_cont->use_adaptive_buffers) {
      /* Check if external buffer size is enouth */
      if (mpeg2GetRefFrmSize(dec_cont) > dec_cont->n_ext_buf_size)
        dec_cont->realloc_ext_buf = 1;
    }

    if (!dec_cont->use_adaptive_buffers) {
      if (dec_cont->Hdrs.prev_horizontal_size != dec_cont->Hdrs.horizontal_size ||
          dec_cont->Hdrs.prev_vertical_size != dec_cont->Hdrs.vertical_size)
        dec_cont->realloc_ext_buf = 1;
    }

    dec_cont->realloc_int_buf = 0;

  } else { /* PP output*/
    if (dec_cont->use_adaptive_buffers) {
      if (CalcPpUnitBufferSize(dec_cont->ppu_cfg, 0) > dec_cont->n_ext_buf_size)
        dec_cont->realloc_ext_buf = 1;
      if (mpeg2GetRefFrmSize(dec_cont) > dec_cont->n_int_buf_size)
        dec_cont->realloc_int_buf = 1;
    }

    if (!dec_cont->use_adaptive_buffers) {
      if (dec_cont->ppu_cfg[0].scale.width != dec_cont->prev_pp_width ||
          dec_cont->ppu_cfg[0].scale.height != dec_cont->prev_pp_height)
        dec_cont->realloc_ext_buf = 1;
      if (mpeg2GetRefFrmSize(dec_cont) != dec_cont->n_int_buf_size)
        dec_cont->realloc_int_buf = 1;
    }
  }
}
