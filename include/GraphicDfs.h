/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-06-22 21:47:27
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-08-27 19:42:55
 * @FilePath: \CrystalMods\include\GraphicDfs.h
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#ifndef _INCLUDE_GRAPHICDFS_H_
#define _INCLUDE_GRAPHICDFS_H_

#include <CrystalCore.h>
#include <CrystalAlgorithms.h>

#define CR_WNDCLASS_NAME "CrystalWindow"
#define CRUI_TITLEBAR_PIXEL 25

typedef struct crcoloru
{
    CRUINT8 r;
    CRUINT8 g;
    CRUINT8 b;
    CRUINT8 a;
}CRCOLORU;

typedef struct crwindow_properties
{
    CRCHAR* title;
    CRINT64 x, y;
    CRINT64 w, h;
    //
    CRBOOL modified;
    //
    CRWINDOW window;
}CRWindowProperties, *PCRWindowProperties;

typedef struct crwindow_msg
{
    CRWINDOW window;
    union
    {
        CRINT64 x;
        CRUINT64 w;
    };
    union
    {
        CRINT64 y;
        CRUINT64 h;
    };
    CRCHAR keycode;
    //CRW_STAT_XX
    CRUINT8 status;
    CRLVOID lp;
}CRWINDOWMSG, *PCRWINDOWMSG;
#define CRUI_STAT_OTHER 0x00
#define CRUI_STAT_UP    0x01
#define CRUI_STAT_DOWN  0x02
#define CRUI_STAT_MOVE  0x04
#define CRUI_STAT_LEFT  0x10
#define CRUI_STAT_NIDD  0x20
#define CRUI_STAT_RIGHT 0x30

#define CRWINDOW_QUIT_CB   0
#define CRWINDOW_PAINT_CB  1
#define CRWINDOW_MOUSE_CB  2
#define CRWINDOW_KEY_CB    3
#define CRWINDOW_FOCUS_CB  4
#define CRWINDOW_SIZE_CB   5
#define CRWINDOW_MOVE_CB   6
#define CRWINDOW_ENTITY_CB 7
#define CALLBACK_FUNCS_NUM 8

/**
 * 由事件触发的回调函数
 */
typedef CRCODE(*CRWindowCallback)(PCRWINDOWMSG msg);

#endif
