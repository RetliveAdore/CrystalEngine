/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-06-23 14:35:41
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-06-24 01:54:59
 * @FilePath: \Crystal-Thread\src\init.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include <ThrDefs.h>
#include <CrystalAlgorithms.h>

void** CRCoreFunList = NULL;
void** CRAlgorithmsFunList = NULL;

CRLVOID currentID = (CRLVOID)1;
CRSTRUCTURE threadTree = NULL;  //tree
CRSTRUCTURE availableID = NULL;  //linear

CRAPI CRBOOL CRModInit(void** ptr)
{
    if (ptr[0] == ptr[1])
        return CRFALSE;
    CRCoreFunList = ptr;
    return CRTRUE;
}

CRAPI void CRModUninit(void)
{
    if (threadTree) CRFreeStructure(threadTree, NULL);
    if (availableID) CRFreeStructure(availableID, NULL);
}

//额外的依赖项要自行初始化
CRAPI CRBOOL CrystalThreadInit(void** alg)
{
    if (!alg)
    {
        CR_LOG_ERR("auto", "nullptr");
        return CRFALSE;
    }
    if (alg[0] == alg[2])
        return CRFALSE;
    CRAlgorithmsFunList = alg;
    threadTree = CRTree();
    availableID = CRLinear();
    return CRTRUE;
}
