/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-06-10 18:01:51
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-06-24 02:01:36
 * @FilePath: \CrystalAlgorithms\core\cralgorithms.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include <CrystalAlgorithms.h>

void _cr_inner_do_nothing_alg_(void){};

void* CRAlgorithmsFunListArr[] =
{
    _cr_inner_do_nothing_alg_, "CRDynamic",
    _cr_inner_do_nothing_alg_, "CRTree",
    _cr_inner_do_nothing_alg_, "CRLinear",
    _cr_inner_do_nothing_alg_, "CRQuadtree",
    //6
    _cr_inner_do_nothing_alg_, "CRStructureSize",
    _cr_inner_do_nothing_alg_, "CRFreeStructure",
    //10
    _cr_inner_do_nothing_alg_, "CRDynPush",
    _cr_inner_do_nothing_alg_, "CRDynPop",
    _cr_inner_do_nothing_alg_, "CRDynSet",
    _cr_inner_do_nothing_alg_, "CRDynSeek",
    _cr_inner_do_nothing_alg_, "CRDynSetup",
    _cr_inner_do_nothing_alg_, "CRDynGetBits",
    _cr_inner_do_nothing_alg_, "CRDynSetBits",
    //24
    _cr_inner_do_nothing_alg_, "CRTreePut",
    _cr_inner_do_nothing_alg_, "CRTreeGet",
    _cr_inner_do_nothing_alg_, "CRTreeSeek",
    //30
    _cr_inner_do_nothing_alg_, "CRLinPut",
    _cr_inner_do_nothing_alg_, "CRLinSeek",
    _cr_inner_do_nothing_alg_, "CRLinGet",
    //36
    _cr_inner_do_nothing_alg_, "CRQuadtreePushin",
    _cr_inner_do_nothing_alg_, "CRQuadtreeRemove",
    _cr_inner_do_nothing_alg_, "CRQuadtreeSearch",
    _cr_inner_do_nothing_alg_, "CRQuadtreeCheck",
    //44
    _cr_inner_do_nothing_alg_, "CRStructureForEach",
    0 //检测到0表示清单结尾
};

CRAPI void** CRAlgorithmsFunList = CRAlgorithmsFunListArr;