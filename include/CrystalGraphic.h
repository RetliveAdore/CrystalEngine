/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-06-24 16:49:08
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-08-06 23:34:38
 * @FilePath: \CrystalGraphic\include\CrystalGraphic.h
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#ifndef _INCLUDE_CRYSTALGRAPHIC_H_
#define _INCLUDE_CRYSTALGRAPHIC_H_

#include "GraphicDfs.h"

extern void** CRGraphicFunList;

/**
 * CrystalGraphic有额外的依赖项，需要单独初始化
 */
typedef CRBOOL(*CRYSTALGRAPHICINIT)(void** alg, void** thr);
#define CrystalGraphicInit ((CRYSTALGRAPHICINIT)CRGraphicFunList[0])

/**
 * 用于获取当前活动窗口数量，用以确保所有活动结束后再退出
 */
typedef CRINT64(*CRWINDOWCOUNTER)(void);
#define CRWindowCounter ((CRWINDOWCOUNTER)CRGraphicFunList[2])
/**
 * 创建一个窗口
 */
typedef void(*CRCREATEWINDOW)(PCRWindowProperties prop);
#define CRCreateWindow ((CRCREATEWINDOW)CRGraphicFunList[4])
/**
 * 关闭一个窗口
 */
typedef void(*CRCLOSEWINDOW)(CRWINDOW window);
#define CRCloseWindow ((CRCLOSEWINDOW)CRGraphicFunList[6])
/**
 * 设置窗口的事件回调函数
 */
typedef void(*CRSETWINDOWCBK)(CRWINDOW window, CRUINT8 type, CRWindowCallback cbk);
#define CRSetWindowCbk ((CRSETWINDOWCBK)CRGraphicFunList[8])

#endif