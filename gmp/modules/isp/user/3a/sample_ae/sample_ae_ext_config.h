/*
 * Copyright (c) Hunan Goke,Chengdu Goke,Shandong Goke. 2021. All rights reserved.
 */
#ifndef __ISP_AE_EXT_CONFIG_H__
#define __ISP_AE_EXT_CONFIG_H__

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
 * 1. Ae Mode
 * 2. Analog Gain
 *
 * ------------------------------------------------------------------------------
 */
// -----------------------------------------------------------------------s------- //
// Register: AE_MODE
// ------------------------------------------------------------------------------ //
#define ISP_EXT_SYSTEM_AE_MODE_DEFAULT             (0x0)
#define ISP_EXT_SYSTEM_AE_MODE_DATASIZE            (1)

// args: data (1-bit)
static __inline GK_VOID ext_system_ae_mode_write(GK_U8 id, GK_U8 data)
{
    IOWR_8DIRECT((AE_LIB_VREG_BASE(id) + 0x0), data & 0x01);
}
static __inline GK_U8 ext_system_ae_mode_read(GK_U8 id)
{
    return (IORD_8DIRECT(AE_LIB_VREG_BASE(id) + 0x0) & 0x01);
}


// ------------------------------------------------------------------------------ //
// Register: AGAIN
// ------------------------------------------------------------------------------ //

#define ISP_EXT_SYSTEM_QUERY_EXPOSURE_ISO_DEFAULT  (0x00)
#define ISP_EXT_SYSTEM_QUERY_EXPOSURE_ISO_DATASIZE (32)

// args: data (32-bit)
static __inline GK_VOID ext_system_query_exposure_again_write(GK_U8 id, GK_U32 data)
{
    IOWR_32DIRECT((AE_LIB_VREG_BASE(id) + 0x4), data);
}
static __inline GK_U32 ext_system_query_exposure_again_read(GK_U8 id)
{
    return  IORD_32DIRECT(AE_LIB_VREG_BASE(id) + 0x4);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif
