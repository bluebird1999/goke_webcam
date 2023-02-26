#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/prctl.h>
#include "../../../common/tools_interface.h"
#include "../../../global/global_interface.h"
#include "hisi.h"

#define IVE_ALIGN 16

HI_U16 ive_calculate_stride(HI_U32 u32Width, HI_U8 u8Align) {
    return (u32Width + (u8Align - u32Width % u8Align) % u8Align);
}

HI_VOID ive_blob_to_rect(IVE_CCBLOB_S *pstBlob, rect_array_t *pstRect,
                                   HI_U16 u16RectMaxNum,HI_U16 u16AreaThrStep,
                                   HI_U32 u32SrcWidth, HI_U32 u32SrcHeight,
                                   HI_U32 u32DstWidth,HI_U32 u32DstHeight)
{
    HI_U16 u16Num;
    HI_U16 i,j,k;
    HI_U16 u16Thr= 0;
    HI_BOOL bValid;

    if(pstBlob->u8RegionNum > u16RectMaxNum)
    {
        u16Thr = pstBlob->u16CurAreaThr;
        do
        {
            u16Num = 0;
            u16Thr += u16AreaThrStep;
            for(i = 0;i < 254;i++)
            {
                if(pstBlob->astRegion[i].u32Area > u16Thr)
                {
                    u16Num++;
                }
            }
        }while(u16Num > u16RectMaxNum);
    }

    u16Num = 0;

    for(i = 0;i < 254;i++)
    {
        if(pstBlob->astRegion[i].u32Area > u16Thr)
        {
            pstRect->astRect[u16Num].points[0].s32X = (HI_U32)((HI_FLOAT)pstBlob->astRegion[i].u16Left / (HI_FLOAT)u32SrcWidth * (HI_FLOAT)u32DstWidth) & (~1) ;
            pstRect->astRect[u16Num].points[0].s32Y = (HI_U32)((HI_FLOAT)pstBlob->astRegion[i].u16Top / (HI_FLOAT)u32SrcHeight * (HI_FLOAT)u32DstHeight) & (~1);

            pstRect->astRect[u16Num].points[1].s32X = (HI_U32)((HI_FLOAT)pstBlob->astRegion[i].u16Right/ (HI_FLOAT)u32SrcWidth * (HI_FLOAT)u32DstWidth) & (~1);
            pstRect->astRect[u16Num].points[1].s32Y = (HI_U32)((HI_FLOAT)pstBlob->astRegion[i].u16Top / (HI_FLOAT)u32SrcHeight * (HI_FLOAT)u32DstHeight) & (~1);

            pstRect->astRect[u16Num].points[2].s32X = (HI_U32)((HI_FLOAT)pstBlob->astRegion[i].u16Right / (HI_FLOAT)u32SrcWidth * (HI_FLOAT)u32DstWidth) & (~1);
            pstRect->astRect[u16Num].points[2].s32Y = (HI_U32)((HI_FLOAT)pstBlob->astRegion[i].u16Bottom / (HI_FLOAT)u32SrcHeight * (HI_FLOAT)u32DstHeight) & (~1);

            pstRect->astRect[u16Num].points[3].s32X = (HI_U32)((HI_FLOAT)pstBlob->astRegion[i].u16Left / (HI_FLOAT)u32SrcWidth * (HI_FLOAT)u32DstWidth) & (~1);
            pstRect->astRect[u16Num].points[3].s32Y = (HI_U32)((HI_FLOAT)pstBlob->astRegion[i].u16Bottom / (HI_FLOAT)u32SrcHeight * (HI_FLOAT)u32DstHeight) & (~1);

            bValid = HI_TRUE;
            for(j = 0; j < 3;j++)
            {
                for (k = j + 1; k < 4;k++)
                {
                    if ((pstRect->astRect[u16Num].points[j].s32X == pstRect->astRect[u16Num].points[k].s32X)
                        &&(pstRect->astRect[u16Num].points[j].s32Y == pstRect->astRect[u16Num].points[k].s32Y))
                    {
                        bValid = HI_FALSE;
                        break;
                    }
                }
            }
            if (HI_TRUE == bValid)
            {
                u16Num++;
            }
        }
    }

    pstRect->u16Num = u16Num;
}

HI_S32 ive_create_image(IVE_IMAGE_S *pstImg, IVE_IMAGE_TYPE_E enType, HI_U32 u32Width, HI_U32 u32Height) {
    HI_U32 u32Size = 0;
    HI_S32 s32Ret;
    if (NULL == pstImg) {
        log_goke(DEBUG_SERIOUS, "pstImg is null！");
        return HI_FAILURE;
    }

    pstImg->enType = enType;
    pstImg->u32Width = u32Width;
    pstImg->u32Height = u32Height;
    pstImg->au32Stride[0] = ive_calculate_stride(pstImg->u32Width, IVE_ALIGN);

    switch (enType) {
        case IVE_IMAGE_TYPE_U8C1:
        case IVE_IMAGE_TYPE_S8C1: {
            u32Size = pstImg->au32Stride[0] * pstImg->u32Height;
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (HI_VOID **) &pstImg->au64VirAddr[0], NULL, HI_NULL,
                                         u32Size);
            if (s32Ret != HI_SUCCESS) {
                log_goke(DEBUG_SERIOUS, "Mmz Alloc fail,Error(%#x)", s32Ret);
                return s32Ret;
            }
        }
            break;
        case IVE_IMAGE_TYPE_YUV420SP: {
            u32Size = pstImg->au32Stride[0] * pstImg->u32Height * 3 / 2;
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (HI_VOID **) &pstImg->au64VirAddr[0], NULL, HI_NULL,
                                         u32Size);
            if (s32Ret != HI_SUCCESS) {
                log_goke(DEBUG_SERIOUS, "Mmz Alloc fail,Error(%#x)", s32Ret);
                return s32Ret;
            }
            pstImg->au32Stride[1] = pstImg->au32Stride[0];
            pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
            pstImg->au64VirAddr[1] = pstImg->au64VirAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;

        }
            break;
        case IVE_IMAGE_TYPE_YUV422SP: {
            u32Size = pstImg->au32Stride[0] * pstImg->u32Height * 2;
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (HI_VOID **) &pstImg->au64VirAddr[0], NULL, HI_NULL,
                                         u32Size);
            if (s32Ret != HI_SUCCESS) {
                log_goke(DEBUG_SERIOUS, "Mmz Alloc fail,Error(%#x)", s32Ret);
                return s32Ret;
            }
            pstImg->au32Stride[1] = pstImg->au32Stride[0];
            pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
            pstImg->au64VirAddr[1] = pstImg->au64VirAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;

        }
            break;
        case IVE_IMAGE_TYPE_YUV420P:
            break;
        case IVE_IMAGE_TYPE_YUV422P:
            break;
        case IVE_IMAGE_TYPE_S8C2_PACKAGE:
            break;
        case IVE_IMAGE_TYPE_S8C2_PLANAR:
            break;
        case IVE_IMAGE_TYPE_S16C1:
        case IVE_IMAGE_TYPE_U16C1: {

            u32Size = pstImg->au32Stride[0] * pstImg->u32Height * sizeof(HI_U16);
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (HI_VOID **) &pstImg->au64VirAddr[0], NULL, HI_NULL,
                                         u32Size);
            if (s32Ret != HI_SUCCESS) {
                log_goke(DEBUG_SERIOUS, "Mmz Alloc fail,Error(%#x)", s32Ret);
                return s32Ret;
            }
        }
            break;
        case IVE_IMAGE_TYPE_U8C3_PACKAGE: {
            u32Size = pstImg->au32Stride[0] * pstImg->u32Height * 3;
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (HI_VOID **) &pstImg->au64VirAddr[0], NULL, HI_NULL,
                                         u32Size);
            if (s32Ret != HI_SUCCESS) {
                log_goke(DEBUG_SERIOUS, "Mmz Alloc fail,Error(%#x)", s32Ret);
                return s32Ret;
            }
            pstImg->au64VirAddr[1] = pstImg->au64VirAddr[0] + 1;
            pstImg->au64VirAddr[2] = pstImg->au64VirAddr[1] + 1;
            pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[0] + 1;
            pstImg->au64PhyAddr[2] = pstImg->au64PhyAddr[1] + 1;
            pstImg->au32Stride[1] = pstImg->au32Stride[0];
            pstImg->au32Stride[2] = pstImg->au32Stride[0];
        }
            break;
        case IVE_IMAGE_TYPE_U8C3_PLANAR:
            break;
        case IVE_IMAGE_TYPE_S32C1:
        case IVE_IMAGE_TYPE_U32C1: {
            u32Size = pstImg->au32Stride[0] * pstImg->u32Height * sizeof(HI_U32);
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (HI_VOID **) &pstImg->au64VirAddr[0], NULL, HI_NULL,
                                         u32Size);
            if (s32Ret != HI_SUCCESS) {
                log_goke(DEBUG_SERIOUS, "Mmz Alloc fail,Error(%#x)", s32Ret);
                return s32Ret;
            }
        }
            break;
        case IVE_IMAGE_TYPE_S64C1:
        case IVE_IMAGE_TYPE_U64C1: {

            u32Size = pstImg->au32Stride[0] * pstImg->u32Height * sizeof(HI_U64);
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (HI_VOID **) &pstImg->au64VirAddr[0], NULL, HI_NULL,
                                         u32Size);
            if (s32Ret != HI_SUCCESS) {
                log_goke(DEBUG_SERIOUS, "Mmz Alloc fail,Error(%#x)", s32Ret);
                return s32Ret;
            }
        }
            break;
        default:
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ive_create_memory_info(IVE_MEM_INFO_S *pstMemInfo, HI_U32 u32Size) {
    HI_S32 s32Ret;

    if (NULL == pstMemInfo) {
        log_goke(DEBUG_SERIOUS, "pstMemInfo is null");
        return HI_FAILURE;
    }
    pstMemInfo->u32Size = u32Size;
    s32Ret = HI_MPI_SYS_MmzAlloc(&pstMemInfo->u64PhyAddr, (HI_VOID **) &pstMemInfo->u64VirAddr,
                                 NULL, HI_NULL, u32Size);
    if (s32Ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "Mmz Alloc fail,Error(%#x)", s32Ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

HI_VOID hisi_uninit_md(md_config_t *pstMd) {
    HI_S32 i;
    HI_S32 s32Ret = HI_SUCCESS;

    for (i = 0; i < IVE_MD_IMG_NUM; i++) {
        IVE_MMZ_FREE(pstMd->ast_img[i].au64PhyAddr[0], pstMd->ast_img[i].au64VirAddr[0]);
    }
    IVE_MMZ_FREE(pstMd->blob.u64PhyAddr, pstMd->blob.u64VirAddr);
    s32Ret = HI_IVS_MD_Exit();
    if (s32Ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "HI_IVS_MD_Exit fail,Error(%#x)", s32Ret);
        return;
    }
}

HI_S32 hisi_init_md(md_config_t *pstMd, int sad_thresh, HI_U32 u32Width, HI_U32 u32Height) {
    HI_S32 s32Ret = HI_SUCCESS;
    HI_S32 i;
    HI_U32 u32Size;
    HI_U8 u8WndSz;

    for (i = 0; i < IVE_MD_IMG_NUM; i++) {
        s32Ret = ive_create_image(&pstMd->ast_img[i], IVE_IMAGE_TYPE_U8C1, u32Width, u32Height);
        if (s32Ret != HI_SUCCESS) {
            log_goke(DEBUG_WARNING, "Error(%#x),Create img[%d] image failed!", s32Ret, i);
            goto MD_INIT_FAIL;
        }
    }
    u32Size = sizeof(IVE_CCBLOB_S);
    s32Ret = ive_create_memory_info(&pstMd->blob, u32Size);
    if (s32Ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "Error(%#x),Create blob mem info failed!", s32Ret);
        goto MD_INIT_FAIL;
    }

    //Set attr info
    pstMd->md_attr.enAlgMode = MD_ALG_MODE_BG;
    pstMd->md_attr.enSadMode = IVE_SAD_MODE_MB_4X4;
    pstMd->md_attr.enSadOutCtrl = IVE_SAD_OUT_CTRL_THRESH;
    pstMd->md_attr.u16SadThr = sad_thresh;
    pstMd->md_attr.u32Width = u32Width;
    pstMd->md_attr.u32Height = u32Height;
    pstMd->md_attr.stAddCtrl.u0q16X = 32768;
    pstMd->md_attr.stAddCtrl.u0q16Y = 32768;
    pstMd->md_attr.stCclCtrl.enMode = IVE_CCL_MODE_4C;
    u8WndSz = (1 << (2 + pstMd->md_attr.enSadMode));
    pstMd->md_attr.stCclCtrl.u16InitAreaThr = u8WndSz * u8WndSz;
    pstMd->md_attr.stCclCtrl.u16Step = u8WndSz;

    s32Ret = HI_IVS_MD_Init();
    if (s32Ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "Error(%#x),HI_IVS_MD_Init failed!", s32Ret);
        goto MD_INIT_FAIL;
    }
    MD_INIT_FAIL:
    if (HI_SUCCESS != s32Ret) {
        hisi_uninit_md(pstMd);
    }
    return s32Ret;
}

HI_S32 hisi_md_dma_image(VIDEO_FRAME_INFO_S *pstFrameInfo, IVE_DST_IMAGE_S *pstDst, HI_BOOL bInstant) {
    HI_S32 s32Ret;
    IVE_HANDLE hIveHandle;
    IVE_SRC_DATA_S stSrcData;
    IVE_DST_DATA_S stDstData;
    IVE_DMA_CTRL_S stCtrl = {IVE_DMA_MODE_DIRECT_COPY, 0};
    HI_BOOL bFinish = HI_FALSE;
    HI_BOOL bBlock = HI_TRUE;

    //fill src
    //stSrcData.u64VirAddr = pstFrameInfo->stVFrame.u64VirAddr[0];
    stSrcData.u64PhyAddr = pstFrameInfo->stVFrame.u64PhyAddr[0];
    stSrcData.u32Width = pstFrameInfo->stVFrame.u32Width;
    stSrcData.u32Height = pstFrameInfo->stVFrame.u32Height;
    stSrcData.u32Stride = pstFrameInfo->stVFrame.u32Stride[0];

    //fill dst
    //stDstData.u64VirAddr = pstDst->au64VirAddr[0];
    stDstData.u64PhyAddr = pstDst->au64PhyAddr[0];
    stDstData.u32Width = pstDst->u32Width;
    stDstData.u32Height = pstDst->u32Height;
    stDstData.u32Stride = pstDst->au32Stride[0];

    s32Ret = HI_MPI_IVE_DMA(&hIveHandle, &stSrcData, &stDstData, &stCtrl, bInstant);
    if( HI_SUCCESS != s32Ret ) {
        log_goke(DEBUG_WARNING, "Error(%#x),HI_MPI_IVE_DMA failed!", s32Ret);
    }

    if (HI_TRUE == bInstant) {
        s32Ret = HI_MPI_IVE_Query(hIveHandle, &bFinish, bBlock);
        while (HI_ERR_IVE_QUERY_TIMEOUT == s32Ret) {
            usleep(100);
            s32Ret = HI_MPI_IVE_Query(hIveHandle, &bFinish, bBlock);
        }
        if( HI_SUCCESS != s32Ret ) {
            log_goke( DEBUG_WARNING, s32Ret, "Error(%#x),HI_MPI_IVE_Query failed!", s32Ret);
        }
    }

    return HI_SUCCESS;
}