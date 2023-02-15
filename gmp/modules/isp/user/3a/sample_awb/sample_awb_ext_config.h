/*
 * Copyright (c) Hunan Goke,Chengdu Goke,Shandong Goke. 2021. All rights reserved.
 */
#ifndef __ISP_AWB_EXT_CONFIG_H__
#define __ISP_AWB_EXT_CONFIG_H__

#include "isp_vreg.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/*
 * ------------------------------------------------------------------------------
 *          these two entity illustrate how to use a ext register
 *
 * 1. Awb Mode
 * 2. Detected color temperature
 *
 * ------------------------------------------------------------------------------
 */
// ------------------------------------------------------------------------------ //
// Register: ext_system_WB_type
// ------------------------------------------------------------------------------ //
// white balance type, 0 auto, 1 manual
// ------------------------------------------------------------------------------ //
#define ISP_EXT_SYSTEM_WB_TYPE_DEFAULT         (0x0)
#define ISP_EXT_SYSTEM_WB_TYPE_DATASIZE        (1)

// args: data (2-bit)
static __inline GK_VOID ext_system_wb_type_write(GK_U8 id, GK_U8 data)
{
    IOWR_8DIRECT((AWB_LIB_VREG_BASE(id)), data);
}
static __inline GK_U8 ext_system_wb_type_read(GK_U8 id)
{
    return (IORD_8DIRECT(AWB_LIB_VREG_BASE(id)) & 0x1);
}


// ------------------------------------------------------------------------------ //
// Register: ext_system_wb_detect_temp
// ------------------------------------------------------------------------------ //
// the detected color temperature
// ------------------------------------------------------------------------------ //

#define ISP_EXT_SYSTEM_WB_DETECT_TEMP_DEFAULT  (5000)
#define ISP_EXT_SYSTEM_WB_DETECT_TEMP_DATASIZE (16)

// args: data (16-bit)
static __inline GK_VOID ext_system_wb_detect_temp_write(GK_U8 id, GK_U16 data)
{
    IOWR_16DIRECT((AWB_LIB_VREG_BASE(id) + 0x2), data);
}

static __inline GK_U16 ext_system_wb_detect_temp_read(GK_U8 id)
{
    return IORD_16DIRECT(AWB_LIB_VREG_BASE(id) + 0x2);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif
