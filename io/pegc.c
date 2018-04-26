/*----------------------------------------------------------------------*/
/* video256.c                                                           */
/* Graphic Video for 256color                                           */
/*----------------------------------------------------------------------*/
/*                                                                      */
/* Copyright (c) Satoshi Tomizawa All rights reserved.  2016            */
/*----------------------------------------------------------------------*/
/* Mod for NP2(pegc.c) by AZO                                           */
/*----------------------------------------------------------------------*/
#include "compiler.h"
#include <stdlib.h>
#include <string.h>
#include "vram.h"

typedef UINT32 (*fRead)(UINT32 address, int size);
typedef void   (*fWrite)(UINT32 address, UINT32 data, int size);
typedef UINT32 (*fEgcRop)(UINT32 dst, UINT32 ptn);
typedef void (*fPegcRopSub)(UINT32 *dst, int size);

#define GDCS_CHG_VRAM           (0x8000)

#define VMODE_256PLANE          (0x0010)

typedef struct _video_info {
    short  *pStatus;
    short  status[2];
    int    vMode;
    int    vSyncPhase;

    /* Pallete info */
    UINT8  palette256[256][4];
    /* 256 mode */
    INT8   mioBank[10];
    UINT8  *bankTop[3];
    /* PEGC */
    UINT32 pegcIO[8];
    UINT32 pegcPattern[8 + 1];
    UINT32 pegcMask[8];
    UINT8  pegcShift[64];
    UINT32 pegcFgc;
    UINT32 pegcBgc;
    UINT32 pegcCmpMask;
    UINT32 pegcApMask;
    int    pegcShfCnt;
    UINT8  pegcSrcSft;
    UINT8  pegcDstSft;
    int    pegcBlkSize;
    int    pegcBlkCnt;
    int    pegcDataSize;

    UINT8  *pegcSftTop;
    fEgcRop pegcFuncRop[2];
    fPegcRopSub pegcFuncRopSubR;
    fPegcRopSub pegcFuncRopSubL;
    fRead  pegcFuncInitRead;
    fWrite pegcFuncInitWrite;

    UINT8  *draw256;
    UINT8  *show256;
} STRVIDEO;

static const UINT32 g_256pSrcMask[16] = {
    0x00000000, 0xff000000, 0x00ff0000, 0xffff0000, 0x0000ff00, 0xff00ff00, 0x00ffff00, 0xffffff00,
    0x000000ff, 0xff0000ff, 0x00ff00ff, 0xffff00ff, 0x0000ffff, 0xff00ffff, 0x00ffffff, 0xffffffff };
static const UINT32 g_256pSftMaskR[4] = { 0, 0x000000ff, 0x0000ffff, 0x00ffffff };
static const UINT32 g_256pSftMaskL[4] = { 0, 0xff000000, 0xffff0000, 0xffffff00 };

static STRVIDEO *g_pVInfo = NULL;

const UINT32 dummy256data = 0;

/* bank switching for video RAM */
/*
void gvSwitchGVram(int mode)
{
    int type;

    g_arch.vramMode = mode;
    if((mode & VMODE_CMASK) == VMODE_256) {
        type = ((mode & VMODE_256PLANE) == 0) ? (mGvram256 | AC_RF | AC_WF)
                                              : (mGvram256plane | AC_RF | AC_WF);
        memset(&g_memInfo.index[0xa8], type, 8);
        if(g_arch.vramBank != 0) {
            memset(&g_memInfo.index[0xb0], mMain | AC_RP | AC_WP , 8);
        } else if((mode & (VMODE_256SMODE | VMODE_256PLANE)) == VMODE_256PLANE) {
            memset(&g_memInfo.index[0xb0], mNoAccess | AC_RF | AC_WF, 8);
        } else {
            memset(&g_memInfo.index[0xb0], type, 8);
        }
        memset(&g_memInfo.index[0xb8], mNoAccess | AC_RF | AC_WF, 8);
        memset(&g_memInfo.index[0xe0], mGvram256io | AC_RF | AC_WF, 8);
    } else {
        type = mGvram16 | AC_RF | AC_WF;
        if((mode & VMODE_GRCG) != 0) {
            if((mode & VMODE_EGC) != 0) {       /* EGC
                type = mGvram16egc | AC_RF | AC_WF;
            } else {                            /* GRCG
                type = (((mode & VMODE_GRCG_RMW) != 0) ? mGvram16rmw : mGvram16tcr) | AC_RF | AC_WF;
            }
        }
        memset(&g_memInfo.index[0xa8], type, 8);
        memset(&g_memInfo.index[0xb0], (g_arch.vramBank == 0) ? type : (mMain | AC_RP | AC_WP) , 16);
        memset(&g_memInfo.index[0xe0], type, 8);
    }
}

void gvChangePegcFunc(fRead egcRead, fWrite egcWrite)
{
    int type = mGvram256plane | AC_RF | AC_WF;

    if(egcRead != NULL) {
        g_memInfo.access[mGvram256plane].read.func  = egcRead;
    }
    if(egcWrite != NULL) {
        g_memInfo.access[mGvram256plane].write.func = egcWrite;
    }
    if((g_arch.vramMode & VMODE_256PLANE) != 0) {
        if(g_memInfo.index[0xa8] != type) {
            memset(&g_memInfo.index[0xa8], type, 8);
            if((g_arch.vramBank == 0) && (g_memInfo.index[0xa8] == g_memInfo.index[0xb0])) {
                memset(&g_memInfo.index[0xb0], type, 8);
            }
        }
    }
}

void gvSwitchUpperVram(int enable)
{
    if(enable) {
        //if((g_arch.port043b & 0x02) == 0) {
            g_memInfo.index[255 + 0x0f] = mGvram256f0 | AC_RF | AC_WF;
        //} else {
        //    g_memInfo.index[255 + 0x0f] = (g_arch.extMem >= 15) ? (mMain | AC_RP | AC_WP)
        //                                                        : (mNoAccess | AC_RF | AC_WF);
        //}
        g_memInfo.index[270 + 0xff] = mGvram256f0 | AC_RF | AC_WF;
    } else {
        g_memInfo.index[255 + 0x0f] = (g_arch.extMem >= 15) ? (mMain | AC_RP | AC_WP)
                                                            : (mNoAccess | AC_RF | AC_WF);
        g_memInfo.index[270 + 0xff] = (g_arch.extMem >= 4095) ? (mMain | AC_RP | AC_WP)
                                                              : (mNoAccess | AC_RF | AC_WF);
    }
}
*/

UINT32 gv256Read(UINT32 address, int size)
{
    int bank = (address & 0x00010000) >> 16;
    UINT8 *memTop = g_pVInfo->bankTop[bank];

    address &= 0x00007fff;
    if(size == 1) {
        return memTop[address];
    } else if(size == 2) {
        return *(UINT16 *)&memTop[address];
    }
    return *(UINT32 *)&memTop[address];
}

void gv256Write(UINT32 address, UINT32 data, int size)
{
    int bank = (address & 0x00010000) >> 16;
    UINT8 *memTop = g_pVInfo->bankTop[bank];

    address &= 0x00007fff;
    if(size == 1) {
        memTop[address] = (UINT8)data;
    } else if(size == 2) {
        *(UINT16 *)&memTop[address] = (UINT16)data;
    } else {
        *(UINT32 *)&memTop[address] = data;
    }
    *g_pVInfo->pStatus |= GDCS_CHG_VRAM;
}

//UINT32 readMainMemory(UINT32 address, int size);
//void writeMainMemory(UINT32 address, UINT32 data, int size);

UINT32 gv256ReadF0(UINT32 address, int size)
{
    if(((address + size) & 0x00ffffff) <= 0x00f80000) {
        address &= 0x0007ffff;
        if(size == 1) {
            return vramex[address];
        } else if(size == 2){
            return *(UINT16 *)&vramex[address];
        }
        return *(UINT32 *)&vramex[address];
    }
    if((address & 0x00ffffff) >= 0x00f80000) {
//        return readMainMemory(address, size);
        return 0;
    }
    if(size == 2) {
//        return (vramex[0x0007ffff] << 8) | (UINT8)readMainMemory(address + 1, 1);
        return (vramex[0x0007ffff] << 8);
    }
//    return (*(UINT32 *)&vramex[0x0007fffc] >> ((address & 3) * 8))
//         | (readMainMemory(address & 0xfffffffc, 4) << (32 - ((address & 3) * 8))) ;
    return (*(UINT32 *)&vramex[0x0007fffc] >> ((address & 3) * 8));
}

void gv256WriteF0(UINT32 address, UINT32 data, int size)
{
    if(((address + size) & 0x00ffffff) <= 0x00f80000) {
        address &= 0x0007ffff;
        if(size == 1) {
            vramex[address] = (UINT8)data;
        } else if (size == 2){
            *(UINT16 *)&vramex[address] = (UINT16)data;
        } else {
            *(UINT32 *)&vramex[address] = data;
        }
        *g_pVInfo->pStatus |= GDCS_CHG_VRAM;
    } else if((address & 0x00ffffff) >= 0x00f80000) {
//        writeMainMemory(address, data, size);
    } else if(size == 2) {
        vramex[0x0007ffff] = (UINT8)data;
//        writeMainMemory(address + 1, data >> 8, 1);
    } else {
        switch(address & 3) {
        case 1:
            vramex[0x0007fffd] = (UINT8)data;
            *(UINT16 *)&vramex[0x0007fffe] = (UINT16)(data >> 8);
//            writeMainMemory(address, data >> 24, 1);
            break;
        case 2:
            *(UINT16 *)&vramex[0x0007fffe] = (UINT16)data;
//            writeMainMemory(address, data >> 16, 2);
            break;
        default:
            vramex[0x0007ffff] = (UINT8)data;
//            writeMainMemory(address, data >> 8,  2);
//            writeMainMemory(address, data >> 24, 1);
        }
    }
}

static UINT32 _egcrop0(UINT32 dst, UINT32 ptn)
{
    return 0;
}
static UINT32 _egcrop1(UINT32 dst, UINT32 ptn)
{
    return  ~dst & ~ptn;
}
static UINT32 _egcrop2(UINT32 dst, UINT32 ptn)
{
    return  ~dst &  ptn;
}
static UINT32 _egcrop3(UINT32 dst, UINT32 ptn)
{
    return  ~dst;
}
static UINT32 _egcrop4(UINT32 dst, UINT32 ptn)
{
    return   dst & ~ptn;
}
static UINT32 _egcrop5(UINT32 dst, UINT32 ptn)
{
    return         ~ptn;
}
static UINT32 _egcrop6(UINT32 dst, UINT32 ptn)
{
    return ( dst & ~ptn) | (~dst & ptn);
}
static UINT32 _egcrop7(UINT32 dst, UINT32 ptn)
{
    return ( dst & ~ptn) |  ~dst;
}
static UINT32 _egcrop8(UINT32 dst, UINT32 ptn)
{
    return   dst &  ptn;
}
static UINT32 _egcrop9(UINT32 dst, UINT32 ptn)
{
    return ( dst &  ptn) | (~dst & ~ptn);
}
static UINT32 _egcropa(UINT32 dst, UINT32 ptn)
{
    return          ptn;
}
static UINT32 _egcropb(UINT32 dst, UINT32 ptn)
{
    return          ptn  | (~dst & ~ptn);
}
static UINT32 _egcropc(UINT32 dst, UINT32 ptn)
{
    return   dst;
}
static UINT32 _egcropd(UINT32 dst, UINT32 ptn)
{
    return   dst         | (~dst & ~ptn);
}
static UINT32 _egcrope(UINT32 dst, UINT32 ptn)
{
    return   dst         | (~dst &  ptn);
}
static UINT32 _egcropf(UINT32 dst, UINT32 ptn)
{
    return 0xffffffff;
}
const fEgcRop g_funcRop[16] = {
    _egcrop0, _egcrop1, _egcrop2, _egcrop3, _egcrop4, _egcrop5, _egcrop6, _egcrop7,
    _egcrop8, _egcrop9, _egcropa, _egcropb, _egcropc, _egcropd, _egcrope, _egcropf };

/* load to Shifter incremental diection first proc */
static void gv256ReadPRfsub(UINT32 address, int size)
{
    int shift = g_pVInfo->pegcSrcSft % size;
    UINT32 *src = (UINT32 *)&g_pVInfo->draw256[address + shift];
    UINT32 *dst = (UINT32 *)g_pVInfo->pegcSftTop;
    int i;

    size -= shift;
    for(i = size; i > 0; i -= 4) {
        *dst++ = *src++;
    }
    g_pVInfo->pegcShfCnt = size;
}
/* load to Shifter incremental diection continuous proc */
static void gv256ReadPRcsub(UINT32 address, int size)
{
    UINT32 *src = (UINT32 *)&g_pVInfo->draw256[address];
    UINT32 *dst = (UINT32 *)&g_pVInfo->pegcSftTop[g_pVInfo->pegcShfCnt];
    int i;

    if(g_pVInfo->pegcShfCnt + size > 63) {
        size = (64 - g_pVInfo->pegcShfCnt) & 0xf8;
    }
    for(i = size; i > 0; i -= 4) {
        *dst++ = *src++;
    }
    g_pVInfo->pegcShfCnt += size;
}
/* load to Shifter decremental diection first proc */
static void gv256ReadPLfsub(UINT32 address, int size)
{
    int shift = g_pVInfo->pegcSrcSft % size;
    UINT32 *src = (UINT32 *)&g_pVInfo->draw256[address + size - shift];
    UINT32 *dst = (UINT32 *)g_pVInfo->pegcSftTop;
    int i;

    size -= shift;
    for(i = size; i > 0; i -= 4) {
        *--dst = *--src;
    }
    g_pVInfo->pegcShfCnt = size;
}
/* load to Shifter decremental diection continuous proc */
static void gv256ReadPLcsub(UINT32 address, int size)
{
    UINT32 *src = (UINT32 *)&g_pVInfo->draw256[address + size];
    UINT32 *dst = (UINT32 *)&g_pVInfo->pegcSftTop[-g_pVInfo->pegcShfCnt];
    int i;

    if(g_pVInfo->pegcShfCnt + size > 63) {
        size = (64 - g_pVInfo->pegcShfCnt) & 0xf8;
    }
    for(i = size; i > 0; i -= 4) {
        *--dst = *--src;
    }
    g_pVInfo->pegcShfCnt += size;
}
/* compere read subroutine */
static UINT32 gv256checkCmp(UINT32 address, int size)
{
    UINT32 *src = (UINT32 *)&g_pVInfo->draw256[address];
    UINT32 ret = 0;
    UINT32 tmp;
    UINT32 data;
    int i;

    for(i = 0; i < size; i += 8) {
        data = ~((*src++ ^ g_pVInfo->pegcCmpMask) | g_pVInfo->pegcApMask);
        tmp  = ((data & 0xff000000) == 0) ? 0x80 : 0;
        tmp |= ((data & 0x00ff0000) == 0) ? 0x40 : 0;
        tmp |= ((data & 0x0000ff00) == 0) ? 0x20 : 0;
        tmp |= ((data & 0x000000ff) == 0) ? 0x10 : 0;
        data = ~((*src++ ^ g_pVInfo->pegcCmpMask) | g_pVInfo->pegcApMask);
        tmp |= ((data & 0xff000000) == 0) ? 0x08 : 0;
        tmp |= ((data & 0x00ff0000) == 0) ? 0x04 : 0;
        tmp |= ((data & 0x0000ff00) == 0) ? 0x02 : 0;
        tmp |= ((data & 0x000000ff) == 0) ? 0x01 : 0;
        ret |= tmp << i;
    }
    return ret;
}

static UINT32 _gv256ReadPRc(UINT32 address, int size)
{
    address = (address - 0xa8000) * 8;
    size *= 8;
    gv256ReadPRcsub(address, size);
    return 0xffffffff;
}

static UINT32 _gv256ReadPRf(UINT32 address, int size)
{
    address = (address - 0xa8000) * 8;
    size *= 8;
    gv256ReadPRfsub(address, size);
//    gvChangePegcFunc(_gv256ReadPRc, NULL);
    return 0xffffffff;
}

static UINT32 _gv256ReadPRCmpc(UINT32 address, int size)
{
    address = (address - 0xa8000) * 8;
    size *= 8;
    gv256ReadPRcsub(address, size);
    return gv256checkCmp(address, size);
}

static UINT32 _gv256ReadPRCmpf(UINT32 address, int size)
{
    address = (address - 0xa8000) * 8;
    size *= 8;
    gv256ReadPRfsub(address, size);
//    gvChangePegcFunc(_gv256ReadPRCmpc, NULL);
    return gv256checkCmp(address, size);
}

static UINT32 _gv256ReadPLc(UINT32 address, int size)
{
    address = (address - 0xa8000) * 8;
    size *= 8;
    gv256ReadPLcsub(address, size);
    return 0xffffffff;
}

static UINT32 _gv256ReadPLf(UINT32 address, int size)
{
    address = (address - 0xa8000) * 8;
    size *= 8;
    gv256ReadPLfsub(address, size);
//    gvChangePegcFunc(_gv256ReadPLc, NULL);
    return 0xffffffff;
}

static UINT32 _gv256ReadPLCmpc(UINT32 address, int size)
{
    address = (address - 0xa8000) * 8;
    size *= 8;
    gv256ReadPLcsub(address, size);
    return gv256checkCmp(address, size);
}

static UINT32 _gv256ReadPLCmpf(UINT32 address, int size)
{
    address = (address - 0xa8000) * 8;
    size *= 8;
    gv256ReadPLfsub(address, size);
//    gvChangePegcFunc(_gv256ReadPLCmpc, NULL);
    return gv256checkCmp(address, size);
}

static void gv256WritePRsub(UINT32 *dst, UINT32 data, int size)
{
    UINT32 mask;
    UINT32 tmp;
    UINT32 dData;
    int i;

    for(i = 28; i > 31 - size; i -= 4) {
        mask = g_256pSrcMask[(data >> i) & 0x0f];
        dData = *dst;
        tmp = ((dData & g_pVInfo->pegcApMask) & ~mask) | ((dData | ~g_pVInfo->pegcApMask) & mask);
        *dst++ = tmp;
    }
    if((size & 0x03) > 0) {
        mask = g_256pSrcMask[(data >> i) & 0x0f];
        dData = *dst;
        tmp = ((dData & g_pVInfo->pegcApMask) & ~mask) | ((dData | ~g_pVInfo->pegcApMask) & mask);
        mask = g_256pSftMaskR[size & 0x03];
        *dst = (tmp & mask) | (dData & ~mask);
    }
    g_pVInfo->pegcBlkCnt -= size;
    if(g_pVInfo->pegcBlkCnt == 0) {
        g_pVInfo->pegcBlkCnt = g_pVInfo->pegcBlkSize;
    }
}

static void gv256WritePLsub(UINT32 *dst, UINT32 data, int size)
{
    UINT32 mask;
    UINT32 tmp;
    UINT32 dData;
    int i;

    for(i = 0; i < size - 3; i += 4) {
        mask = g_256pSrcMask[(data >> i) & 0x0f];
        dData = *--dst;
        tmp = ((dData & g_pVInfo->pegcApMask) & ~mask) | ((dData | ~g_pVInfo->pegcApMask) & mask);
        *dst = tmp;
    }
    if((size & 0x03) > 0) {
        mask = g_256pSrcMask[(data >> i) & 0x0f];
        dData = *--dst;
        tmp = ((dData & g_pVInfo->pegcApMask) & ~mask) | ((dData | ~g_pVInfo->pegcApMask) & mask);
        mask = g_256pSftMaskL[size & 0x03];
        *dst = (tmp & mask) | (dData & ~mask);
    }
    g_pVInfo->pegcBlkCnt -= size;
    if(g_pVInfo->pegcBlkCnt == 0) {
        g_pVInfo->pegcBlkCnt = g_pVInfo->pegcBlkSize;
    }
}

static void _gv256WritePRc(UINT32 address, UINT32 data, int size)
{
    UINT32 *dst = (UINT32 *)&g_pVInfo->draw256[(address - 0xa8000) * 8];

    data = ((data <<  8) & 0xff00ff00) | ((data >>  8) & 0x00ff00ff);
    data = ((data << 16) & 0xffff0000) | ((data >> 16) & 0x0000ffff);
    gv256WritePRsub(dst, data, MIN(size * 8, g_pVInfo->pegcBlkCnt));
    *g_pVInfo->pStatus |= GDCS_CHG_VRAM;
}

static void _gv256WritePRf(UINT32 address, UINT32 data, int size)
{
    int shift = g_pVInfo->pegcDstSft % (size * 8);
    UINT32 *dst = (UINT32 *)&g_pVInfo->draw256[(address - 0xa8000) * 8 + shift];

    data = ((data <<  8) & 0xff00ff00) | ((data >>  8) & 0x00ff00ff);
    data = ((data << 16) & 0xffff0000) | ((data >> 16) & 0x0000ffff);
    gv256WritePRsub(dst, data, MIN(size * 8 - shift, g_pVInfo->pegcBlkCnt));
    *g_pVInfo->pStatus |= GDCS_CHG_VRAM;
//    gvChangePegcFunc(NULL, _gv256WritePRc);
}

static void _gv256WritePLc(UINT32 address, UINT32 data, int size)
{
    UINT32 *dst = (UINT32 *)&g_pVInfo->draw256[(address - 0xa8000 + size) * 8];

    data = ((data <<  8) & 0xff00ff00) | ((data >>  8) & 0x00ff00ff);
    data = ((data << 16) & 0xffff0000) | ((data >> 16) & 0x0000ffff);
    gv256WritePLsub(dst, data, MIN(size * 8, g_pVInfo->pegcBlkCnt));
    *g_pVInfo->pStatus |= GDCS_CHG_VRAM;
}

static void _gv256WritePLf(UINT32 address, UINT32 data, int size)
{
    int shift = g_pVInfo->pegcDstSft % (size * 8);
    UINT32 *dst = (UINT32 *)&g_pVInfo->draw256[(address - 0xa8000 + size) * 8 - shift];

    data = ((data <<  8) & 0xff00ff00) | ((data >>  8) & 0x00ff00ff);
    data = ((data << 16) & 0xffff0000) | ((data >> 16) & 0x0000ffff);
    gv256WritePLsub(dst, data, MIN(size * 8 - shift, g_pVInfo->pegcBlkCnt));
    *g_pVInfo->pStatus |= GDCS_CHG_VRAM;
//    gvChangePegcFunc(NULL, _gv256WritePLc);
}

static void gv256RopPostProcR(int size)
{
    UINT32 *src;
    UINT32 *dst;
    int i;

    g_pVInfo->pegcBlkCnt -= size;
    if(g_pVInfo->pegcBlkCnt == 0) {
        g_pVInfo->pegcShfCnt = 0;
        g_pVInfo->pegcBlkCnt = g_pVInfo->pegcBlkSize;
//        gvChangePegcFunc(g_pVInfo->pegcFuncInitRead, g_pVInfo->pegcFuncInitWrite);
    } else {
        src = (UINT32 *)&g_pVInfo->pegcSftTop[size];
        dst = (UINT32 *)g_pVInfo->pegcSftTop;
        g_pVInfo->pegcShfCnt -= size;
        for(i = g_pVInfo->pegcShfCnt; i > 0; i -= 4) {
            *dst++ = *src++;
        }
    }
}

static void gv256RopPostProcL(int size)
{
    UINT32 *src;
    UINT32 *dst;
    int i;

    g_pVInfo->pegcBlkCnt -= size;
    if(g_pVInfo->pegcBlkCnt == 0) {
        g_pVInfo->pegcShfCnt = 0;
        g_pVInfo->pegcBlkCnt = g_pVInfo->pegcBlkSize;
//        gvChangePegcFunc(g_pVInfo->pegcFuncInitRead, g_pVInfo->pegcFuncInitWrite);
    } else {
        src = (UINT32 *)&g_pVInfo->pegcSftTop[-size];
        dst = (UINT32 *)g_pVInfo->pegcSftTop;
        g_pVInfo->pegcShfCnt -= size;
        for(i = g_pVInfo->pegcShfCnt; i > 0; i -= 4) {
            *--dst = *--src;
        }
    }
}

static void _gv256WritePRRopsub(UINT32 *dst, int size)
{
    UINT32 mask;
    UINT32 tmp;
    UINT32 *pMask = g_pVInfo->pegcMask;
    UINT32 *src = (UINT32 *)g_pVInfo->pegcSftTop;
    UINT32 *ptn;
    UINT32 sData;
    UINT32 dData;
    int ptnMod = g_pVInfo->pegcDataSize;
    int ptnOfs = ((intptr_t)dst - (intptr_t)g_pVInfo->draw256) % ptnMod;
    int i;

    if(size > g_pVInfo->pegcShfCnt) {
        return;
    }
    for(i = 0; i < size - 3; i += 4) {
        sData = *src++;
        dData = *dst;
        ptn = (UINT32 *)&((UINT8 *)g_pVInfo->pegcPattern)[ptnOfs];
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, *ptn));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, *ptn));
        mask = *pMask++;
        *dst++ = (tmp & mask) | (dData & ~mask);
        ptnOfs = (ptnOfs + 4) % ptnMod;
    }
    if(i < size) {
        sData = *src;
        dData = *dst;
        ptn = (UINT32 *)&((UINT8 *)g_pVInfo->pegcPattern)[ptnOfs];
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, *ptn));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, *ptn));
        mask = *pMask & g_256pSftMaskR[size & 0x03];
        *dst = (tmp & mask) | (dData & ~mask);
    }
    gv256RopPostProcR(size);
}

static void _gv256WritePLRopsub(UINT32 *dst, int size)
{
    UINT32 mask;
    UINT32 tmp;
    UINT32 *pMask = g_pVInfo->pegcMask;
    UINT32 *src = (UINT32 *)g_pVInfo->pegcSftTop;
    UINT32 *ptn;
    UINT32 sData;
    UINT32 dData;
    int ptnMod = g_pVInfo->pegcDataSize;
    int ptnOfs = (((intptr_t)dst - (intptr_t)g_pVInfo->draw256) % ptnMod);
    int i;

    if(size > g_pVInfo->pegcShfCnt) {
        return;
    }
    for(i = 0; i < size - 3; i += 4) {
        sData = *--src;
        dData = *--dst;
        ptnOfs += (ptnOfs < 4) ? (ptnMod - 4) : -4;
        ptn = (UINT32 *)&((UINT8 *)g_pVInfo->pegcPattern)[ptnOfs];
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, *ptn));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, *ptn));
        mask = *pMask++;
        *dst = (tmp & mask) | (dData & ~mask);
    }
    if(i < size) {
        sData = *--src;
        dData = *--dst;
        ptnOfs += (ptnOfs < 4) ? (ptnMod - 4) : -4;
        ptn = (UINT32 *)&((UINT8 *)g_pVInfo->pegcPattern)[ptnOfs];
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, *ptn));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, *ptn));
        mask = *pMask & g_256pSftMaskL[size & 0x03];
        *dst = (tmp & mask) | (dData & ~mask);
    }
    gv256RopPostProcL(size);
}

static void _gv256WritePRFgcsub(UINT32 *dst, int size)
{
    UINT32 mask;
    UINT32 tmp;
    UINT32 fgc = g_pVInfo->pegcFgc;
    UINT32 *pMask = g_pVInfo->pegcMask;
    UINT32 *src = (UINT32 *)g_pVInfo->pegcSftTop;
    UINT32 sData;
    UINT32 dData;
    int i;

    if(size > g_pVInfo->pegcShfCnt) {
        return;
    }
    for(i = 0; i < size - 3; i += 4) {
        sData = *src++;
        dData = *dst;
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, fgc));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, fgc));
        mask = *pMask++;
        *dst++ = (tmp & mask) | (dData & ~mask);
    }
    if(i < size) {
        sData = *src;
        dData = *dst;
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, fgc));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, fgc));
        mask = *pMask & g_256pSftMaskR[size & 0x03];
        *dst = (tmp & mask) | (dData & ~mask);
    }
    gv256RopPostProcR(size);
}

static void _gv256WritePLFgcsub(UINT32 *dst, int size)
{
    UINT32 mask;
    UINT32 tmp;
    UINT32 fgc = g_pVInfo->pegcFgc;
    UINT32 *pMask = g_pVInfo->pegcMask;
    UINT32 *src = (UINT32 *)g_pVInfo->pegcSftTop;
    UINT32 sData;
    UINT32 dData;
    int i;

    if(size > g_pVInfo->pegcShfCnt) {
        return;
    }
    for(i = 0; i < size - 3; i += 4) {
        sData = *--src;
        dData = *--dst;
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, fgc));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, fgc));
        mask = *pMask++;
        *dst = (tmp & mask) | (dData & ~mask);
    }
    if(i < size) {
        sData = *--src;
        dData = *--dst;
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, fgc));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, fgc));
        mask = *pMask & g_256pSftMaskL[size & 0x03];
        *dst = (tmp & mask) | (dData & ~mask);
    }
    gv256RopPostProcL(size);
}

static void _gv256WritePRBgcsub(UINT32 *dst, int size)
{
    UINT32 mask;
    UINT32 tmp;
    UINT32 bgc = g_pVInfo->pegcBgc;
    UINT32 *pMask = g_pVInfo->pegcMask;
    UINT32 *src = (UINT32 *)g_pVInfo->pegcSftTop;
    UINT32 sData;
    UINT32 dData;
    int i;

    if(size > g_pVInfo->pegcShfCnt) {
        return;
    }
    for(i = 0; i < size - 3; i += 4) {
        sData = *src++;
        dData = *dst;
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, bgc));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, bgc));
        mask = *pMask++;
        *dst++ = (tmp & mask) | (dData & ~mask);
    }
    if(i < size) {
        sData = *src;
        dData = *dst;
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, bgc));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, bgc));
        mask = *pMask & g_256pSftMaskR[size & 0x03];
        *dst = (tmp & mask) | (dData & ~mask);
    }
    gv256RopPostProcR(size);
}

static void _gv256WritePLBgcsub(UINT32 *dst, int size)
{
    UINT32 mask;
    UINT32 tmp;
    UINT32 bgc = g_pVInfo->pegcBgc;
    UINT32 *pMask = g_pVInfo->pegcMask;
    UINT32 *src = (UINT32 *)g_pVInfo->pegcSftTop;
    UINT32 sData;
    UINT32 dData;
    int i;

    if(size > g_pVInfo->pegcShfCnt) {
        return;
    }
    for(i = 0; i < size - 3; i += 4) {
        sData = *--src;
        dData = *--dst;
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, bgc));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, bgc));
        mask = *pMask++;
        *dst = (tmp & mask) | (dData & ~mask);
    }
    if(i < size) {
        sData = *--src;
        dData = *--dst;
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, bgc));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, bgc));
        mask = *pMask & g_256pSftMaskL[size & 0x03];
        *dst = (tmp & mask) | (dData & ~mask);
    }
    gv256RopPostProcL(size);
}

static void _gv256WritePRFBgcsub(UINT32 *dst, int size)
{
    UINT32 mask;
    UINT32 tmp;
    UINT32 fgc = g_pVInfo->pegcFgc;
    UINT32 bgc = g_pVInfo->pegcBgc;
    UINT32 *pMask = g_pVInfo->pegcMask;
    UINT32 *src = (UINT32 *)g_pVInfo->pegcSftTop;
    UINT32 sData;
    UINT32 dData;
    int i;

    if(size > g_pVInfo->pegcShfCnt) {
        return;
    }
    for(i = 0; i < size - 3; i += 4) {
        sData = *src++;
        dData = *dst;
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, fgc));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, bgc));
        mask = *pMask++;
        *dst++ = (tmp & mask) | (dData & ~mask);
    }
    if(i < size) {
        sData = *src;
        dData = *dst;
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, fgc));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, bgc));
        mask = *pMask & g_256pSftMaskR[size & 0x03];
        *dst = (tmp & mask) | (dData & ~mask);
    }
    gv256RopPostProcR(size);
}

static void _gv256WritePLFBgcsub(UINT32 *dst, int size)
{
    UINT32 mask;
    UINT32 tmp;
    UINT32 fgc = g_pVInfo->pegcFgc;
    UINT32 bgc = g_pVInfo->pegcBgc;
    UINT32 *pMask = g_pVInfo->pegcMask;
    UINT32 *src = (UINT32 *)g_pVInfo->pegcSftTop;
    UINT32 sData;
    UINT32 dData;
    int i;

    if(size > g_pVInfo->pegcShfCnt) {
        return;
    }
    for(i = 0; i < size - 3; i += 4) {
        sData = *--src;
        dData = *--dst;
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, fgc));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, bgc));
        mask = *pMask++;
        *dst = (tmp & mask) | (dData & ~mask);
    }
    if(i < size) {
        sData = *--src;
        dData = *--dst;
        tmp  = ( sData & g_pVInfo->pegcFuncRop[0](dData, fgc));
        tmp |= (~sData & g_pVInfo->pegcFuncRop[1](dData, bgc));
        mask = *pMask & g_256pSftMaskL[size & 0x03];
        *dst = (tmp & mask) | (dData & ~mask);
    }
    gv256RopPostProcL(size);
}

static void gv256CputoSftPRfsub(UINT32 data, int size)
{
    int i;
    int shift = g_pVInfo->pegcSrcSft % size;
    UINT32 *dst = (UINT32 *)g_pVInfo->pegcSftTop;

    data <<= shift;
    for(i = size - 4; i >= shift; i -= 4) {
        *dst++ = g_256pSrcMask[(data >> i) & 0x0f];
    }
    g_pVInfo->pegcShfCnt = size - shift;
}

static void gv256CputoSftPRcsub(UINT32 data, int size)
{
    int i = size;
    UINT32 *dst = (UINT32 *)&g_pVInfo->pegcSftTop[g_pVInfo->pegcShfCnt];

    if(g_pVInfo->pegcShfCnt + size > 63) {
        return;
    }
    for(i = size - 4; i >= 0; i -= 4) {
        *dst++ = g_256pSrcMask[(data >> i) & 0x0f];
    }
    g_pVInfo->pegcShfCnt += size;
}

static void gv256CputoSftPLfsub(UINT32 data, int size)
{
    int i;
    int shift = g_pVInfo->pegcSrcSft % size;
    UINT32 *dst = (UINT32 *)g_pVInfo->pegcSftTop;

    data >>= shift;
    for(i = 0; i < size - shift; i += 4) {
        *--dst = g_256pSrcMask[(data >> i) & 0x0f];
    }
    g_pVInfo->pegcShfCnt = size - shift;
}

static void gv256CputoSftPLcsub(UINT32 data, int size)
{
    int i;
    UINT32 *dst = (UINT32 *)&g_pVInfo->pegcSftTop[-g_pVInfo->pegcShfCnt];

    if(g_pVInfo->pegcShfCnt + size > 63) {
        return;
    }
    for(i = 0; i < size; i += 4) {
        *--dst = g_256pSrcMask[(data >> i) & 0x0f];
    }
    g_pVInfo->pegcShfCnt += size;
}

static void _gv256WritePRVramRopc(UINT32 address, UINT32 data, int size)
{
    UINT32 *dst = (UINT32 *)&g_pVInfo->draw256[(address - 0xa8000) * 8];

    g_pVInfo->pegcDataSize = size * 8;
    g_pVInfo->pegcFuncRopSubR(dst, MIN(size * 8, g_pVInfo->pegcBlkCnt));
    *g_pVInfo->pStatus |= GDCS_CHG_VRAM;
}

static void _gv256WritePRVramRopf(UINT32 address, UINT32 data, int size)
{
    int shift = g_pVInfo->pegcDstSft % (size * 8);
    UINT32 *dst = (UINT32 *)&g_pVInfo->draw256[(address - 0xa8000) * 8 + shift];

    if((size * 8 - shift) <= g_pVInfo->pegcShfCnt) {
        g_pVInfo->pegcDataSize = size * 8;
        g_pVInfo->pegcFuncRopSubR(dst, MIN(size * 8 - shift, g_pVInfo->pegcBlkCnt));
    }
    if(g_pVInfo->pegcBlkCnt < g_pVInfo->pegcBlkSize) {
//        gvChangePegcFunc(NULL, _gv256WritePRVramRopc);
    }
    *g_pVInfo->pStatus |= GDCS_CHG_VRAM;
}

static void _gv256WritePLVramRopc(UINT32 address, UINT32 data, int size)
{
    UINT32 *dst = (UINT32 *)&g_pVInfo->draw256[(address - 0xa8000 + size) * 8];

    g_pVInfo->pegcFuncRopSubL(dst, MIN(size * 8, g_pVInfo->pegcBlkCnt));
    *g_pVInfo->pStatus |= GDCS_CHG_VRAM;
}

static void _gv256WritePLVramRopf(UINT32 address, UINT32 data, int size)
{
    int shift = g_pVInfo->pegcDstSft % (size * 8);
    UINT32 *dst = (UINT32 *)&g_pVInfo->draw256[(address - 0xa8000 + size) * 8 - shift];

    if((size * 8 - shift) <= g_pVInfo->pegcShfCnt) {
        g_pVInfo->pegcFuncRopSubL(dst, MIN(size * 8 - shift, g_pVInfo->pegcBlkCnt));
    }
    if(g_pVInfo->pegcBlkCnt < g_pVInfo->pegcBlkSize) {
//        gvChangePegcFunc(NULL, _gv256WritePLVramRopc);
    }
    *g_pVInfo->pStatus |= GDCS_CHG_VRAM;
}

static void _gv256WritePRCpuRopc(UINT32 address, UINT32 data, int size)
{
    UINT32 *dst = (UINT32 *)&g_pVInfo->draw256[(address - 0xa8000) * 8];

    if(size > 1) {
        data = ((data << 8) & 0xff00ff00) | ((data >> 8) & 0x00ff00ff);
        if(size > 2) {
            data = ((data << 16) & 0xffff0000) | ((data >> 16) & 0x0000ffff);
        }
    }
    size *= 8;
    g_pVInfo->pegcDataSize = size;
    gv256CputoSftPRcsub(data, size);
    size = MIN(size, g_pVInfo->pegcBlkCnt);
    g_pVInfo->pegcFuncRopSubR(dst, size);
    *g_pVInfo->pStatus |= GDCS_CHG_VRAM;
}

static void _gv256WritePRCpuRopf(UINT32 address, UINT32 data, int size)
{
    int shift = g_pVInfo->pegcDstSft % (size * 8);
    UINT32 *dst = (UINT32 *)&g_pVInfo->draw256[(address - 0xa8000) * 8 + shift];

    if(size > 1) {
        data = ((data << 8) & 0xff00ff00) | ((data >> 8) & 0x00ff00ff);
        if(size > 2) {
            data = ((data << 16) & 0xffff0000) | ((data >> 16) & 0x0000ffff);
        }
    }
    size *= 8;
    g_pVInfo->pegcDataSize = size;
    if(g_pVInfo->pegcShfCnt == 0) {
        gv256CputoSftPRfsub(data, size);
    } else {
        gv256CputoSftPRcsub(data, size);
    }
    if((size - shift) <= g_pVInfo->pegcShfCnt) {
        g_pVInfo->pegcFuncRopSubR(dst, MIN(size - shift, g_pVInfo->pegcBlkCnt));
    }
    if(g_pVInfo->pegcBlkCnt < g_pVInfo->pegcBlkSize) {
//        gvChangePegcFunc(NULL, _gv256WritePRCpuRopc);
    }
    *g_pVInfo->pStatus |= GDCS_CHG_VRAM;
}

static void _gv256WritePLCpuRopc(UINT32 address, UINT32 data, int size)
{
    UINT32 *dst = (UINT32 *)&g_pVInfo->draw256[(address - 0xa8000 + size) * 8];

    if(size > 1) {
        data = ((data << 8) & 0xff00ff00) | ((data >> 8) & 0x00ff00ff);
        if(size > 2) {
            data = ((data << 16) & 0xffff0000) | ((data >> 16) & 0x0000ffff);
        }
    }
    size *= 8;
    g_pVInfo->pegcDataSize = size;
    gv256CputoSftPLcsub(data, size);
    size = MIN(size, g_pVInfo->pegcBlkCnt);
    g_pVInfo->pegcFuncRopSubL(dst, size);
    *g_pVInfo->pStatus |= GDCS_CHG_VRAM;
}

static void _gv256WritePLCpuRopf(UINT32 address, UINT32 data, int size)
{
    int shift = g_pVInfo->pegcDstSft % (size * 8);
    UINT32 *dst = (UINT32 *)&g_pVInfo->draw256[(address - 0xa8000 + size) * 8 - shift];

    if(size > 1) {
        data = ((data << 8) & 0xff00ff00) | ((data >> 8) & 0x00ff00ff);
        if(size > 2) {
            data = ((data << 16) & 0xffff0000) | ((data >> 16) & 0x0000ffff);
        }
    }
    size *= 8;
    g_pVInfo->pegcDataSize = size;
    if(g_pVInfo->pegcShfCnt == 0) {
        gv256CputoSftPLfsub(data, size);
    } else {
        gv256CputoSftPLcsub(data, size);
    }
    if((size - shift) <= g_pVInfo->pegcShfCnt) {
        g_pVInfo->pegcFuncRopSubL(dst, MIN(size - shift, g_pVInfo->pegcBlkCnt));
    }
    if(g_pVInfo->pegcBlkCnt < g_pVInfo->pegcBlkSize) {
//        gvChangePegcFunc(NULL, _gv256WritePLCpuRopc);
    }
    *g_pVInfo->pStatus |= GDCS_CHG_VRAM;
}

static const fRead g_256pfuncRead[2][2] = {
    { _gv256ReadPRf,    _gv256ReadPLf }, { _gv256ReadPRCmpf, _gv256ReadPLCmpf }
};

static const fWrite g_256pfuncWrite[3][2] = {
    { _gv256WritePRf,        _gv256WritePLf        },
    { _gv256WritePRVramRopf, _gv256WritePLVramRopf },
    { _gv256WritePRCpuRopf,  _gv256WritePLCpuRopf  }
};

void gvSetPegcFunc()
{
    UINT32 rop = g_pVInfo->pegcIO[2];
    int isInc   = ((rop & 0x00000200) == 0) ? 0 : 1;
    int type;

    g_pVInfo->pegcShfCnt = 0;
    g_pVInfo->pegcBlkCnt = g_pVInfo->pegcBlkSize;

    g_pVInfo->pegcFuncInitRead = g_256pfuncRead[((rop & 0x00010000) == 0) ? 0 : 1][isInc];
    type = 0;
    if((rop & 0x00001000) != 0) {
        type = 1 + ((rop & 0x00000100) >> 8);
        switch(rop & 0x00000c00) {
        case 0x0000:
            g_pVInfo->pegcFuncRopSubR = _gv256WritePRRopsub;
            g_pVInfo->pegcFuncRopSubL = _gv256WritePLRopsub;
            break;
        case 0x0400:
            g_pVInfo->pegcFuncRopSubR = _gv256WritePRBgcsub;
            g_pVInfo->pegcFuncRopSubL = _gv256WritePLBgcsub;
            break;
        case 0x0800:
            g_pVInfo->pegcFuncRopSubR = _gv256WritePRFgcsub;
            g_pVInfo->pegcFuncRopSubL = _gv256WritePLFgcsub;
            break;
        default:
            g_pVInfo->pegcFuncRopSubR = _gv256WritePRFBgcsub;
            g_pVInfo->pegcFuncRopSubL = _gv256WritePLFBgcsub;
            break;
        }
    }
    g_pVInfo->pegcFuncInitWrite = g_256pfuncWrite[type][isInc];
//    gvChangePegcFunc(g_pVInfo->pegcFuncInitRead, g_pVInfo->pegcFuncInitWrite);
}

UINT32 gv256ReadIO(UINT32 address, int size)
{
    UINT8 *mio;
    UINT32 data;
    UINT32 src;
    int offset;
    int shift;
    int i;

    address &= 0x3ff;
    if(address < 0x100) {
        if(address < 8) {
            mio = (UINT8 *)g_pVInfo->mioBank;
            if(size == 1) {
                return mio[address];
            } else if (size == 2){
                return *(UINT16 *)&mio[address];
            }
            return *(UINT32 *)&mio[address];
        }
    } else if(address < 0x120) {
        offset = (address - 0x100) / 4;
        if(size == 1) {
            return (UINT8)(g_pVInfo->pegcIO[offset] >> (8 * (address & 0x03)));
        } else if (size == 2) {
            if((address & 0x01) == 0) {
                return (UINT16)(g_pVInfo->pegcIO[offset] >> (8 * (address & 0x03)));
            }
        } else if((address & 0x03) == 0) {
            return g_pVInfo->pegcIO[offset];
        }
    } else if((address & 0x03) == 0) {
        if((g_pVInfo->pegcIO[2] & 0x8000) == 0) {
            if(address < 0x140) {
                shift = 7 - ((address - 0x120) / 4);
                data = 0;
                for(i = 0; i < size * 2; i += 2) {
                    data <<= 8;
                    src = g_pVInfo->pegcPattern[i] << shift;
                    data |= (src      ) & 0x00000080;
                    data |= (src >>  9) & 0x00000040;
                    data |= (src >> 18) & 0x00000020;
                    data |= (src >> 27) & 0x00000010;
                    src = g_pVInfo->pegcPattern[i + 1] << shift;
                    data |= (src >>  4) & 0x00000008;
                    data |= (src >> 13) & 0x00000004;
                    data |= (src >> 22) & 0x00000002;
                    data |= (src >> 31) & 0x00000001;
                }
                return data;
            }
        } else {
            if(address < 0x1a0) {
                offset = (address - 0x120) / 4;
                return (g_pVInfo->pegcPattern[offset / 4] >> (8 * (offset & 0x03))) & 0xff;
            }
        }
    }
    return 0;
}

void gv256ResetRop()
{
    gvSetPegcFunc();
}

void gv256WriteIO(UINT32 address, UINT32 data, int size)
{
    static const INT8 mmiomask[10] = { 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x00, 0x00 };
    static const UINT32 iomask8[4]  = { 0xffffff00, 0xffff00ff, 0xff00ffff, 0x00ffffff };
    static const UINT32 iomask16[2] = { 0xffff0000, 0x0000ffff };
    static const UINT32 pconvert[16] = {
        0x00000000, 0x01000000, 0x00010000, 0x01010000, 0x00000100, 0x01000100, 0x00010100, 0x01010100,
        0x00000001, 0x01000001, 0x00010001, 0x01010001, 0x00000101, 0x01000101, 0x00010101, 0x01010101 };
    UINT32 src, prev;
    UINT32 mask;
    INT8 *mio;
    int offset;
    int i;
    int ropUpdate = FALSE;

    address &= 0x3ff;

    if(address < 0x100) {
        if(address <= 6) {
            mio = &g_pVInfo->mioBank[0];
            if(size == 1) {
                mio[address] = (INT8)data & mmiomask[address];
            } else if (size == 2){
                *(UINT16 *)&mio[address] = (UINT16)data & *(UINT16*)&mmiomask[address];
            } else {
                *(UINT32 *)&mio[address] = data & *(UINT32*)&mmiomask[address];
            }
            g_pVInfo->mioBank[4] &= 0x0f;
            g_pVInfo->mioBank[6] &= 0x0f;
            g_pVInfo->bankTop[0] = &vramex[g_pVInfo->mioBank[4] * 0x8000];
            g_pVInfo->bankTop[1] = &vramex[g_pVInfo->mioBank[6] * 0x8000];
        }
    } else if(address < 0x120) {
        offset = (address - 0x100) >> 2;
        prev = g_pVInfo->pegcIO[offset];
        if(size == 1) {
            src  = prev & iomask8[address & 0x03];
            src |= (data & 0xff) << (8 * (address & 0x03));
        } else if(size == 2) {
            address >>= 1;
            src  = prev & iomask16[address & 0x01];
            src |= (data & 0xffff) << (16 * (address & 0x01));
        } else {
            src = data;
        }
        switch(offset) {
        case 0: /* mode */
            src &= 0x00030001;
            if(((prev ^ src) & 0x00000001) != 0) {
                g_pVInfo->vMode &= ~VMODE_256PLANE;
                if((src & 0x00000001) != 0) {
                    ropUpdate = TRUE;
                    g_pVInfo->vMode |= VMODE_256PLANE;
                }
//                gvSwitchGVram(g_pVInfo->vMode);
            }
            if(((prev ^ src) & 0x00030000) != 0) {
//                gvSwitchUpperVram((src & 0x00010000) != 0);
            }
            break;
        case 1: /* access plane */
            src &= 0x000000ff;
            g_pVInfo->pegcApMask = src * 0x01010101;
            break;
        case 2: /* rop setting */
            src &= 0x0001ffff;
            g_pVInfo->pegcSftTop = &g_pVInfo->pegcShift[((src & 0x00000200) == 0) ? 0 : 64];
            g_pVInfo->pegcFuncRop[0] = g_funcRop[(src >> 4) & 0x0f];
            g_pVInfo->pegcFuncRop[1] = g_funcRop[src & 0x0f];
            ropUpdate = TRUE;
            break;
        case 3: /* mask */
            if(size == 1) {
                src = (src & 0xff) * 0x01010101;
            } else if(size == 2) {
                src = (src & 0xffff) * 0x00010001;
            }
            for(i = 0; i < 8; i += 2) {
                g_pVInfo->pegcMask[i    ] = g_256pSrcMask[(src >> (i * 4 + 4)) & 0x0f];
                g_pVInfo->pegcMask[i + 1] = g_256pSrcMask[(src >> (i * 4    )) & 0x0f];
            }
            break;
        case 4: /* block size / source shift / distination shift */
            src &= 0x1f1f0fff;
            g_pVInfo->pegcSrcSft = (UINT8)(src >> 16);
            g_pVInfo->pegcDstSft = (UINT8)(src >> 24);
            g_pVInfo->pegcBlkSize = (src & 0xfff) + 1;
            ropUpdate = TRUE;
            break;
        case 5: /* foreground color */
            src &= 0x000000ff;
            g_pVInfo->pegcCmpMask = ~(src * 0x01010101);
            g_pVInfo->pegcFgc = src * 0x01010101;
            break;
        case 6: /* background color */
            src &= 0x000000ff;
            g_pVInfo->pegcBgc = src * 0x01010101;
            break;
        default:
            break;
        }
        g_pVInfo->pegcIO[offset] = src;
        if(ropUpdate) {
            gv256ResetRop();
        }
    } else if((address & 0x03) == 0) {
        if((g_pVInfo->pegcIO[2] & 0x8000) == 0) {
            if(address < 0x140) {
                offset = (address - 0x120) / 4;
                mask = ~(0x01010101 << offset);
                for(i = size * 2 - 1; i >= 0; i--) {
                    src = g_pVInfo->pegcPattern[i] & mask;
                    g_pVInfo->pegcPattern[i] = src | (pconvert[data & 0x0f] << offset);
                    data >>= 4;
                }
            }
        } else {
            if(address < 0x1a0) {
                offset = (address - 0x120) / 4;
                src  = g_pVInfo->pegcPattern[offset / 4] & iomask8[offset & 0x03];
                src |= (data & 0xff) << (8 * (offset & 0x03));
                g_pVInfo->pegcPattern[offset / 4] = src;
            }
        }
        g_pVInfo->pegcPattern[8] = g_pVInfo->pegcPattern[0];
        return;
    }
}

static const UINT8 gv256_initialPalette[] = {
    204,  12, 144, 0,  252, 235, 239, 0,  152, 245, 255, 0,   17,  38, 145, 0,
      0,   6,  33, 0,  109, 167, 255, 0,   99, 255, 255, 0,  135,   0, 100, 0,
    146, 136, 197, 0,   29, 255, 247, 0,  234, 255,  57, 0,  149, 149,  16, 0,
    231,  49,  38, 0,   94, 255,  63, 0,   29, 126, 127, 0,  251, 197, 130, 0,
    253,  67,  48, 0,  188,  95, 255, 0,   33, 244, 247, 0,  111,  92,  84, 0,
    123,  74, 228, 0,  132, 253, 255, 0,   50, 255, 243, 0,  180,   5,   1, 0,
    239,  86, 121, 0,  127, 239, 255, 0,   93, 247, 219, 0,  149,  17,   6, 0,
    178,   2, 117, 0,   54, 254, 251, 0,   48, 253, 126, 0,   64,  65,   4, 0,
    109, 222,   5, 0,  134, 251, 254, 0,  207, 127, 253, 0,  135,  65,  32, 0,
     56,  46,  80, 0,  255, 119, 119, 0,  187, 223, 254, 0,  187, 132,  68, 0,
    137, 193,   0, 0,  174, 255, 119, 0,  171, 190, 191, 0,  142, 104,  84, 0,
    213,  81, 224, 0,  252, 255, 255, 0,  171, 126, 255, 0,   29, 161,   8, 0,
    175,   7, 192, 0,  125, 255, 189, 0,  154, 239, 247, 0,  220,   1,  12, 0,
     93,  59, 176, 0,  205,  63, 251, 0,   85, 239, 127, 0,  236, 193, 214, 0,
    251,  66,  96, 0,  160, 253, 127, 0,   60, 231, 247, 0,   81, 100,  98, 0,
    114,  50,  48, 0,  204, 251, 223, 0,  251, 126, 239, 0,   13,  94, 100, 0,
    191,  32, 207, 0,   95, 255, 255, 0,  219, 255, 251, 0,  175, 163,  14, 0,
     27,  68, 238, 0,  254, 254, 191, 0,  193, 251, 254, 0,   59,   2, 217, 0,
    223,   4,  35, 0,  254, 191, 191, 0,  120, 251, 255, 0,   64,  50, 188, 0,
     15, 131,   4, 0,  182, 251, 254, 0,  224, 239, 239, 0,  187, 162,  68, 0,
    111,  22, 160, 0,   90, 255, 206, 0,  186, 255, 251, 0,  158, 216,  84, 0,
    238, 114,  48, 0,  103, 255, 117, 0,   40, 234, 255, 0,   25,  40,   3, 0,
    255,   2, 179, 0,  191, 250, 251, 0,   20, 235, 239, 0,   89, 216,  19, 0,
    214,  36, 104, 0,   86, 191, 247, 0,   43, 255, 238, 0,  240,   5,  41, 0,
    230,  76, 135, 0,  175, 246, 253, 0,  166, 255, 239, 0,   33, 129, 144, 0,
    250, 192, 179, 0,  157, 174, 231, 0,  164, 255,  63, 0,  237, 136,  34, 0,
    177, 248, 122, 0,  243, 250, 231, 0,  191, 255, 250, 0,   61, 162,  26, 0,
    183,   2, 162, 0,  255,  63, 255, 0,  223, 255, 255, 0,  138, 167,   4, 0,
     88,  52,  24, 0,  152, 235, 255, 0,  116, 255, 223, 0,   99, 128, 104, 0,
    191,  41, 240, 0,  253, 254, 254, 0,   47, 253, 255, 0,   13, 160, 130, 0,
    181, 136,  21, 0,    3, 120, 187, 0,  208, 255, 231, 0,  163,  88, 134, 0,
      3,  86,  75, 0,  115,  94, 251, 0,  127, 255, 239, 0,  146, 226,  10, 0,
    185,   9,   0, 0,  255, 253, 223, 0,  184, 223, 187, 0,  212,  66,  75, 0,
     25, 212,  37, 0,   95, 125, 111, 0,  249, 255, 255, 0,  125, 192, 224, 0,
    245, 154,  59, 0,  255, 247, 125, 0,  252, 127, 255, 0,  154, 100,  37, 0,
    141,   6,  66, 0,  255, 252, 255, 0,  192, 255, 253, 0,   53,   0, 216, 0,
    175,  88,  40, 0,  170, 107, 239, 0,  198, 253, 255, 0,   86, 107,  36, 0,
     46,  28, 204, 0,   51, 255, 253, 0,  114, 238, 255, 0,   19,  23, 194, 0,
    206,  11,  49, 0,  191, 127, 239, 0,  129, 255, 251, 0,   86,   3,  23, 0,
    254,   3, 253, 0,  113,  95, 111, 0,  166, 255, 191, 0,  138,  85, 144, 0,
    178,  64, 221, 0,  199,  95, 223, 0,  103, 239, 239, 0,  252, 129,  16, 0,
     31,  48,   0, 0,  105, 253, 189, 0,  194, 191, 127, 0,  175,  38,   0, 0,
     44,  32, 208, 0,   79, 191, 255, 0,  111, 107, 255, 0,   85, 132,   1, 0,
     54,   1, 209, 0,  223, 231, 253, 0,    8, 253, 247, 0,   22,  75,  89, 0,
    248,   5, 205, 0,  113, 123, 215, 0,   95, 255, 251, 0,  113,   0,  74, 0,
    139,  61,  93, 0,  178, 255, 231, 0,  139, 253, 191, 0,  223,  68,  57, 0,
    158,  12, 126, 0,    8, 189, 127, 0,  241, 127, 253, 0,  145, 130,  28, 0,
     23,  53,  33, 0,    5, 215, 255, 0,  255, 255, 215, 0,  189,   6,  16, 0,
    100, 155,  28, 0,   63, 232, 255, 0,  203, 255, 255, 0,   11,  45,  10, 0,
    242,  40,   8, 0,  157, 251, 253, 0,    8, 255, 127, 0,   95, 214, 120, 0,
    232,   0,  69, 0,  130, 255,  47, 0,  106, 171, 254, 0,   42,  17, 172, 0,
    253, 216,   0, 0,   85, 189, 215, 0,   10, 125, 255, 0,   44,  44, 157, 0,
     61, 162,  12, 0,   36, 127, 183, 0,  104, 255,  85, 0,  221,  32,  12, 0,
    119,  80,  14, 0,    0, 179, 255, 0,  137, 221, 250, 0,  253,  34,  64, 0,
    154, 198,  40, 0,  243, 251,  62, 0,  240, 255, 251, 0,   47, 136,  72, 0,
    124,  36,  88, 0,  107, 111, 117, 0,  170,  63, 125, 0,   72, 144,  13, 0,
    151,  66,  16, 0,   59, 255, 255, 0,  239,  63, 239, 0,  250, 135,  10, 0,
     95,   6, 232, 0,  168, 255, 237, 0,   54, 255, 254, 0,  144, 236, 162, 0,
     43, 163, 229, 0,  219, 255, 190, 0,   50, 191, 255, 0,  243, 190,  78, 0,
    243,  40,  53, 0,   93, 227, 255, 0,  216, 251, 247, 0,  195, 185, 128, 0,
    214,  45, 218, 0,  255, 239, 235, 0,   64, 247, 254, 0,  203,   0,  40, 0,
    164,  20,  36, 0,   69, 254, 254, 0,   98, 183,  58, 0,  142,  21, 133, 0,
    137,  64,  14, 0,   12, 231, 255, 0,  104, 127, 245, 0,  129,  14, 132, 0,
    170,   1,  36, 0,  233, 255, 119, 0,   74, 251, 255, 0,   88, 193,  40, 0
};

void gv256_initPalette()
{
    memcpy(g_pVInfo->palette256, gv256_initialPalette, 256 * 4);
}

void gv256_initPEGC()
{
    if(!g_pVInfo)
        g_pVInfo = (STRVIDEO*)calloc(1, sizeof(STRVIDEO));

    g_pVInfo->draw256 = vramex;
    g_pVInfo->show256 = vramex;
    gv256_initPalette();
    g_pVInfo->bankTop[0] = vramex;
    g_pVInfo->bankTop[1] = vramex;
    g_pVInfo->bankTop[2] = (UINT8*)&dummy256data;
    memcpy(g_pVInfo->palette256, gv256_initialPalette, 256 * 4);
}

void gv256_uninitPEGC()
{
    if(g_pVInfo) {
        free(g_pVInfo);
        g_pVInfo = NULL;
    }
}

