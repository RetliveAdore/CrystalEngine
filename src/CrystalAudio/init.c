/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-07-12 20:50:10
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-07-31 23:24:06
 * @FilePath: \Crystal-Audio\src\init.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include <AudioDfs.h>

void **CRCoreFunList = NULL;
void **CRAlgorithmsFunList = NULL;
void **CRThreadFunList = NULL;

extern CRBOOL _inner_craudio_init_();
extern void _inner_craudio_uninit_();

CRAPI CRBOOL CRModInit(void** ptr)
{
    if (ptr[0] == ptr[1])
        return CRFALSE;
    CRCoreFunList = ptr;
    return _inner_craudio_init_();
}

CRAPI void CRModUninit(void)
{
    _inner_craudio_uninit_();
}

CRAPI CRBOOL CrystalAudioInit(void** alg, void** thr)
{
    if (!alg || !thr)
    {
        CR_LOG_ERR("auto", "nullptr");
        return CRFALSE;
    }
    if (alg[0] == alg[2] || thr[0] == thr[2])
    {
        CR_LOG_ERR("auto", "CRAlgorithmsFunList or CRThreadFunList is not inited correctly");
        return CRFALSE;
    }
    CRAlgorithmsFunList = alg;
    CRThreadFunList = thr;
    return CRTRUE;
}