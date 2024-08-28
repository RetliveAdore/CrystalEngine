#ifndef _INCLUDE_CRYSTALTHREAD_H_
#define _INCLUDE_CRYSTALTHREAD_H_

#include "ThrDefs.h"

extern void** CRThreadFunList;

/**
 * 此模块需要额外的依赖项来就行初始化，
 * 将初始化后的CrystalAlgorithms的函数清单传入该函数来完成初始化
 */
typedef CRBOOL(*CRYSTALTHREADINIT)(void** alg);
#define CrystalThreadInit ((CRYSTALTHREADINIT)CRThreadFunList[0])


/**
 * 以毫秒为单位休眠一段时间
 */
typedef void(*CRSLEEP)(CRUINT64 ms);
#define CRSleep ((CRSLEEP)CRThreadFunList[2])


/**
 * 创建一个线程
 */
typedef CRTHREAD(*CRTHREADF)(CRThreadFunc func, CRLVOID data);
#define CRThread ((CRTHREADF)CRThreadFunList[4])
/**
 * 等待一个线程直到结束
 */
typedef void(*CRWAITTHREAD)(CRTHREAD thread);
#define CRWaitThread ((CRWAITTHREAD)CRThreadFunList[6])


/**
 * 创建一个锁
 */
typedef CRLOCK(*CRLOCKCREATE)(void);
#define CRLockCreate ((CRLOCKCREATE)CRThreadFunList[8])
/**
 * 释放一个锁
 */
typedef void(*CRLOCKRELEASE)(CRLOCK lock);
#define CRLockRelease ((CRLOCKRELEASE)CRThreadFunList[10])
/**
 * 加锁操作
 */
typedef void(*CRLOCKF)(CRLOCK lock);
#define CRLock ((CRLOCKF)CRThreadFunList[12])
/**
 * 解锁操作
 */
typedef void(*CRUNLOCK)(CRLOCK lock);
#define CRUnlock ((CRUNLOCK)CRThreadFunList[14])

#endif
