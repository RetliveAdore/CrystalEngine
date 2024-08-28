/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-06-10 16:11:31
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-08-06 23:28:31
 * @FilePath: \CrystalAlgorithms\include\CrystalAlgorithms.h
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#ifndef _INCLUDE_CRYSTALALGORITHMS_H_
#define _INCLUDE_CRYSTALALGORITHMS_H_

#include "AlgDefs.h"

extern void** CRAlgorithmsFunList;

/**
 * 创建一个动态数组
 */
typedef CRSTRUCTURE(*CRDYNAMIC)(CRUINT64 size);
#define CRDynamic ((CRDYNAMIC)CRAlgorithmsFunList[0])
/**
 * 创建一个键值树
 */
typedef CRSTRUCTURE(*CRTREE)(void);
#define CRTree ((CRTREE)CRAlgorithmsFunList[2])
/**
 * 创建一个线性表
 */
typedef CRSTRUCTURE(*CRLINEAR)(void);
#define CRLinear ((CRLINEAR)CRAlgorithmsFunList[4])
/**
 * 创建一个空间索引树，需要提供初始的空间长宽值
 */
typedef CRSTRUCTURE(*CRQUADTREE)(CRUINT64 w, CRUINT64 h, CRUINT8 max);
#define CRQuadtree ((CRQUADTREE)CRAlgorithmsFunList[6])


/**
 * 查询数据结构中元素数量
 */
typedef CRUINT64(*CRSTRUCTURESIZE)(CRSTRUCTURE s);
#define CRStructureSize ((CRSTRUCTURESIZE)CRAlgorithmsFunList[8])
/**
 * 销毁并释放数据结构
 */
typedef CRBOOL(*CRFREESTRUCTURE)(CRSTRUCTURE s, DSCallback cal);
#define CRFreeStructure ((CRFREESTRUCTURE)CRAlgorithmsFunList[10])


/**
 * 向动态数组中插入末尾元素
 */
typedef CRBOOL(*CRDYNPUSH)(CRSTRUCTURE dyn, void* data, CRDynEnum mode);
#define CRDynPush ((CRDYNPUSH)CRAlgorithmsFunList[12])
/**
 * 从动态数组中弹出末尾元素
 */
typedef CRBOOL(*CRDYNPOP)(CRSTRUCTURE dyn, void* data, CRDynEnum mode);
#define CRDynPop ((CRDYNPOP)CRAlgorithmsFunList[14])
/**
 * 设置动态数组中某一位置的元素，超出下标将设置在末尾
 */
typedef CRBOOL(*CRDYNSET)(CRSTRUCTURE dyn, CRUINT64 sub, CRDynEnum mode);
#define CRDynSet ((CRDYNSET)CRAlgorithmsFunList[16])
/**
 * 获取动态数组某一下标的值，但不删除
 */
typedef CRBOOL(*CRDYNSEEK)(CRSTRUCTURE dyn, void* data, CRUINT64 sub, CRDynEnum mode);
#define CRDynSeek ((CRDYNSEEK)CRAlgorithmsFunList[18])
/**
 * 使用已有的数据初始化动态数组
 */
typedef CRBOOL(*CRDYNSETUP)(CRSTRUCTURE dyn, void* buffer, CRUINT64 size);
#define CRDynSetup ((CRDYNSETUP)CRAlgorithmsFunList[20])
/**
 * 获取动态数组数据流中的某一段比特
 */
typedef CRUINT64(*CRDYNGETBITS)(CRSTRUCTURE dyn, CRUINT64 offset, CRUINT8 len);
#define CRDynGetBits ((CRDYNGETBITS)CRAlgorithmsFunList[22])
/**
 * 设置动态数组数据流中某一段比特的值
 */
typedef CRBOOL(*CRDYNSETBITS)(CRSTRUCTURE dyn, CRUINT64 offset, CRUINT8 len, CRUINT64 bits);
#define CRDynSetBits ((CRDYNSETBITS)CRAlgorithmsFunList[24])


/**
 * 向键值树中插入元素
 */
typedef CRBOOL(*CRTREEPUT)(CRSTRUCTURE tree, CRLVOID data, CRUINT64 key);
#define CRTreePut ((CRTREEPUT)CRAlgorithmsFunList[26])
/**
 * 从键值树中取出节点，并删除该节点
 */
typedef CRBOOL(*CRTREEGET)(CRSTRUCTURE tree, CRLVOID* data, CRUINT64 key);
#define CRTreeGet ((CRTREEGET)CRAlgorithmsFunList[28])
/**
 * 查询键值树节点的值，不删除
 */
typedef CRBOOL(*CRTREESEEK)(CRSTRUCTURE tree, CRLVOID* data, CRUINT64 key);
#define CRTreeSeek ((CRTREESEEK)CRAlgorithmsFunList[30])


/**
 * 向线性表中插入元素
 */
typedef CRBOOL(*CRLINPUT)(CRSTRUCTURE lin, CRLVOID data, CRINT64 seek);
#define CRLinPut ((CRLINPUT)CRAlgorithmsFunList[32])
/**
 * 查询线性表中的元素
 */
typedef CRBOOL(*CRLINSEEK)(CRSTRUCTURE lin, CRLVOID* data, CRINT64 seek);
#define CRLinSeek ((CRLINSEEK)CRAlgorithmsFunList[34])
/**
 * 从线性表中取出节点
 */
typedef CRBOOL(*CRLINGET)(CRSTRUCTURE lin, CRLVOID* data, CRINT64 seek);
#define CRLinGet ((CRLINGET)CRAlgorithmsFunList[36])


/**
 * 在空间中添一处索引
 */
typedef CRBOOL(*CRQUADTREEPUSHIN)(CRSTRUCTURE tree, CRRECTU range, CRLVOID key);
#define CRQuadtreePushin ((CRQUADTREEPUSHIN)CRAlgorithmsFunList[38])
/**
 * 从空间中移除一处索引
 */
typedef CRBOOL(*CRQUADTREEREMOVE)(CRSTRUCTURE tree, CRLVOID key);
#define CRQuadtreeRemove ((CRQUADTREEREMOVE)CRAlgorithmsFunList[40])
/**
 * 查询坐标命中的索引
 */
typedef CRBOOL(*CRQUADTREESEARCH)(CRSTRUCTURE tree, CRPOINTU p, CRSTRUCTURE dynOut);
#define CRQuadtreeSearch ((CRQUADTREESEARCH)CRAlgorithmsFunList[42])
/**
 * 检查key是否已经存在
 */
typedef CRBOOL(*CRQUADTREECHECK)(CRSTRUCTURE tree, CRLVOID key);
#define CRQuadtreeCheck ((CRQUADTREECHECK)CRAlgorithmsFunList[44])


/**
 * 遍历操作数据结构
 */
typedef CRBOOL(*CRSTRUCTUREFOREACH)(CRSTRUCTURE s, DSForeach cal, CRLVOID user);
#define CRStructureForEach ((CRSTRUCTUREFOREACH)CRAlgorithmsFunList[46])

#endif
