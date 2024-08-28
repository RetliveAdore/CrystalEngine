#ifndef _INCLUDE_CHRDEFS_H_
#define _INCLUDE_CHRDEFS_H_

#include <CrystalCore.h>

//一个线程对象
typedef CRLVOID CRTHREAD;
typedef void(*CRThreadFunction)(CRLVOID data, CRTHREAD idThis);
//用于异步加锁
typedef CRLVOID CRLOCK;

#endif
