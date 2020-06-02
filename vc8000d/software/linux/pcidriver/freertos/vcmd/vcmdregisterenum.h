/* Copyright 2019 Verisilicon(Beijing) Co.,Ltd. All Rights Reserved. */
/* Register interface based on the document version 1.1.2 */
    HWIF_VCMD_HW_ID,
    HWIF_VCMD_HW_VERSION,
    HWIF_VCMD_HW_BUILD_DATE,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCD_MMU,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCD_L2CACHE,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCD_DEC400,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCD,
    HWIF_VCMD_EXT_ABN_INT_SRC_CUTREE_MMU,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCE_MMU,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCE_L2CACHE,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCE_DEC400,
    HWIF_VCMD_EXT_ABN_INT_SRC_CUTREE,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCE,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCD_MMU,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCD_L2CACHE,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCD_DEC400,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCD,
    HWIF_VCMD_EXT_NORM_INT_SRC_CUTREE_MMU,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCE_MMU,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCE_L2CACHE,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCE_DEC400,
    HWIF_VCMD_EXT_NORM_INT_SRC_CUTREE,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCE,
    HWIF_VCMD_EXE_CMDBUF_COUNT,
    HWIF_VCMD_EXECUTING_CMD,
    HWIF_VCMD_EXECUTING_CMD_MSB,
    HWIF_VCMD_AXI_TOTAL_AR_LEN,
    HWIF_VCMD_AXI_TOTAL_R,
    HWIF_VCMD_AXI_TOTAL_AR,
    HWIF_VCMD_AXI_TOTAL_R_LAST,
    HWIF_VCMD_AXI_TOTAL_AW_LEN,
    HWIF_VCMD_AXI_TOTAL_W,
    HWIF_VCMD_AXI_TOTAL_AW,
    HWIF_VCMD_AXI_TOTAL_W_LAST,
    HWIF_VCMD_AXI_TOTAL_B,
    HWIF_VCMD_AXI_AR_VALID,
    HWIF_VCMD_AXI_AR_READY,
    HWIF_VCMD_AXI_R_VALID,
    HWIF_VCMD_AXI_R_READY,
    HWIF_VCMD_AXI_AW_VALID,
    HWIF_VCMD_AXI_AW_READY,
    HWIF_VCMD_AXI_W_VALID,
    HWIF_VCMD_AXI_W_READY,
    HWIF_VCMD_AXI_B_VALID,
    HWIF_VCMD_AXI_B_READY,
    HWIF_VCMD_WORK_STATE,
    HWIF_VCMD_AXI_CLK_GATE_DISABLE,
    HWIF_VCMD_MASTER_OUT_CLK_GATE_DISABLE,
    HWIF_VCMD_CORE_CLK_GATE_DISABLE,
    HWIF_VCMD_ABORT_MODE,
    HWIF_VCMD_RESET_CORE,
    HWIF_VCMD_RESET_ALL,
    HWIF_VCMD_START_TRIGGER,
    HWIF_VCMD_IRQ_INTCMD,
    HWIF_VCMD_IRQ_JMPP,
    HWIF_VCMD_IRQ_JMPD,
    HWIF_VCMD_IRQ_RESET,
    HWIF_VCMD_IRQ_ABORT,
    HWIF_VCMD_IRQ_CMDERR,
    HWIF_VCMD_IRQ_TIMEOUT,
    HWIF_VCMD_IRQ_BUSERR,
    HWIF_VCMD_IRQ_ENDCMD,
    HWIF_VCMD_IRQ_INTCMD_EN,
    HWIF_VCMD_IRQ_JMPP_EN,
    HWIF_VCMD_IRQ_JMPD_EN,
    HWIF_VCMD_IRQ_RESET_EN,
    HWIF_VCMD_IRQ_ABORT_EN,
    HWIF_VCMD_IRQ_CMDERR_EN,
    HWIF_VCMD_IRQ_TIMEOUT_EN,
    HWIF_VCMD_IRQ_BUSERR_EN,
    HWIF_VCMD_IRQ_ENDCMD_EN,
    HWIF_VCMD_TIMEOUT_EN,
    HWIF_VCMD_TIMEOUT_CYCLES,
    HWIF_VCMD_EXECUTING_CMD_ADDR,
    HWIF_VCMD_EXECUTING_CMD_ADDR_MSB,
    HWIF_VCMD_EXE_CMDBUF_LENGTH,
    HWIF_VCMD_CMD_SWAP,
    HWIF_VCMD_MAX_BURST_LEN,
    HWIF_VCMD_AXI_ID_RD,
    HWIF_VCMD_AXI_ID_WR,
    HWIF_VCMD_RDY_CMDBUF_COUNT,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCD_MMU_GATE,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCD_L2CACHE_GATE,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCD_DEC400_GATE,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCD_GATE,
    HWIF_VCMD_EXT_ABN_INT_SRC_CUTREE_MMU_GATE,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCE_MMU_GATE,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCE_L2CACHE_GATE,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCE_DEC400_GATE,
    HWIF_VCMD_EXT_ABN_INT_SRC_CUTREE_GATE,
    HWIF_VCMD_EXT_ABN_INT_SRC_VCE_GATE,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCD_MMU_GATE,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCD_L2CACHE_GATE,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCD_DEC400_GATE,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCD_GATE,
    HWIF_VCMD_EXT_NORM_INT_SRC_CUTREE_MMU_GATE,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCE_MMU_GATE,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCE_L2CACHE_GATE,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCE_DEC400_GATE,
    HWIF_VCMD_EXT_NORM_INT_SRC_CUTREE_GATE,
    HWIF_VCMD_EXT_NORM_INT_SRC_VCE_GATE,
    HWIF_VCMD_CMDBUF_EXECUTING_ID,
