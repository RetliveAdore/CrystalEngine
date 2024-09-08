/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-06-23 00:38:41
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-09-08 23:22:08
 * @FilePath: \CrystalEngine\src\CrystalGraphic\windows.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include <GraphicDfs.h>
#include "vk.h"

#ifdef CR_WINDOWS
#include <windows.h>
#include <windowsx.h>
#include <CrystalThread.h>

static CRINT64 windowCounter = 0;
CRLOCK lock = NULL;

typedef struct crwindow_inner
{
    HWND hWnd;
    CRUINT32 fps;
    PCRWindowProperties prop;
    CRPOINTU cursor;
    cr_vk vkui;
    cr_vk vkpaint;
    //
    CRWindowCallback funcs[CALLBACK_FUNCS_NUM];
    CRBOOL onProcess;
    CRBOOL drag;
    CRBOOL preClose;
    //
    CRTHREAD eventThread;
    CRTHREAD uiThread;
    CRTHREAD paintThread;
    //
    CRUINT64 w, h;
    CRBOOL resized;
}CRWINDOWINNER, *PCRWINDOWINNER;

static CRCODE _inner_empty_callback_(PCRWINDOWMSG msg)
{
    return 0;
}

static LRESULT AfterProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, PCRWINDOWINNER pInner)
{
    CRWINDOWMSG inf = {0};
    inf.window = (CRWINDOW)pInner;
    inf.x = GET_X_LPARAM(lParam);
    inf.y = GET_Y_LPARAM(lParam) - CRUI_TITLEBAR_PIXEL;
    inf.keycode = wParam & 0xff;
    inf.status = CRUI_STAT_OTHER;
    switch (msg)
    {
        case WM_PAINT:
            _inner_paint_ui_(pInner->vkui);
            break;
        case WM_MOUSEMOVE:
            if (inf.y > 0)
            {
                inf.status = CRUI_STAT_MOVE;
                pInner->funcs[CRWINDOW_MOUSE_CB](&inf);
            }
            pInner->cursor.x = inf.x;
            pInner->cursor.y = inf.y;
            return 0;
        case WM_SETCURSOR:
            if (pInner->cursor.y < 0 && pInner->cursor.x < CRUI_TITLEBAR_PIXEL)
                SetCursor(LoadCursor(NULL, IDC_HAND));
            else
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;
        case WM_LBUTTONDOWN:
            if (inf.y < 0)
            {
                if (inf.x > CRUI_TITLEBAR_PIXEL * 3)
                {
                    SendMessage(hWnd, WM_SYSCOMMAND, SC_MOVE|HTCAPTION, 0);
                    pInner->drag = CRTRUE;
                }
                else if (inf.x > CRUI_TITLEBAR_PIXEL * 2)
                {}
                else if (inf.x > CRUI_TITLEBAR_PIXEL)
                {}
                else
                    pInner->preClose = CRTRUE;
            }
            else
            {
                inf.status = CRUI_STAT_DOWN | CRUI_STAT_LEFT;
                pInner->funcs[CRWINDOW_MOUSE_CB](&inf);
                //
            }
            return 0;
        case WM_LBUTTONUP:
            if (inf.y < 0)
            {
                if (inf.x > CRUI_TITLEBAR_PIXEL * 3)
                {}
                else if (inf.x > CRUI_TITLEBAR_PIXEL * 2)
                {}
                else if (inf.x > CRUI_TITLEBAR_PIXEL)
                {}
                else if (pInner->preClose)
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
            }
            else
            {
                inf.status = CRUI_STAT_UP | CRUI_STAT_LEFT;
                pInner->funcs[CRWINDOW_MOUSE_CB](&inf);
            }
            pInner->preClose = CRFALSE;
            pInner->drag = CRFALSE;
            return 0;
        case WM_KEYDOWN:
            inf.status = CRUI_STAT_DOWN;
            if (pInner->funcs[CRWINDOW_KEY_CB](&inf))
                return 0;
            break;
        case WM_KEYUP:
            inf.status = CRUI_STAT_UP;
            if (pInner->funcs[CRWINDOW_KEY_CB](&inf))
                return 0;
            break;
        case WM_SIZE:
            pInner->w = inf.w;
            pInner->h = inf.h;
            pInner->resized = CRTRUE;
            if (pInner->funcs[CRWINDOW_SIZE_CB](&inf))
                return 0;
            break;
        case WM_MOVE:
            pInner->drag = CRTRUE;
            break;
        default:
            break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PCRWINDOWINNER pInner;
    CRWINDOWMSG inf = {0};
    if (msg == WM_CREATE)
    {
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)(lpcs->lpCreateParams));
        pInner = lpcs->lpCreateParams;
        pInner->vkui = _inner_create_vk_(hWnd, pInner->w, pInner->h);
        return 0;
    }
    pInner = (PCRWINDOWINNER)(CRUINT64)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (!pInner) return DefWindowProc(hWnd, msg, wParam, lParam);
    inf.window = (CRWINDOW)pInner;
    if (msg == WM_CLOSE)
    {
        if (!pInner->funcs[CRWINDOW_QUIT_CB](&inf))
            DestroyWindow(hWnd);
        CR_LOG_IFO("auto", "Close window");
        return 0;
    }
    else if (msg == WM_DESTROY)
    {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)NULL);
        pInner->onProcess = CRFALSE;
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return AfterProc(hWnd, msg, wParam, lParam, pInner);
}
//
static void _inner_paint_ui_thread_(CRLVOID data, CRTHREAD idThis);
static void _inner_paint_thread_(CRLVOID data, CRTHREAD idThis);
//
static void _inner_window_thread_(CRLVOID data, CRTHREAD idThis)
{
    PCRWINDOWINNER pInner = data;
    DWORD style = WS_POPUP;
    style = WS_OVERLAPPEDWINDOW;
    pInner->hWnd = CreateWindow(
        CR_WNDCLASS_NAME,
        pInner->prop->title, style,
        pInner->prop->x, pInner->prop->y,
        pInner->prop->w, pInner->prop->h,
        NULL, NULL, GetModuleHandle(NULL), pInner
    );
    UpdateWindow(pInner->hWnd);
    ShowWindow(pInner->hWnd, SW_SHOWDEFAULT);
    //
    CR_LOG_IFO("auto", "Create Window");
    MSG msg = {0};
    pInner->drag = CRFALSE;
    //
    pInner->uiThread = CRThread(_inner_paint_ui_thread_, pInner);
    pInner->paintThread = CRThread(_inner_paint_thread_, pInner);
    //
    while (pInner->onProcess)
    {
        if (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    pInner->prop->window = (CRWINDOW)NULL;
    CRAlloc(pInner, 0);
    //
    CRLock(lock);
    windowCounter--;
    CRUnlock(lock);
}

static void _inner_paint_ui_thread_(CRLVOID data, CRTHREAD idThis)
{
    PCRWINDOWINNER pInner = (PCRWINDOWINNER)data;
    while (pInner->onProcess)
    {
        SendMessage(pInner->hWnd, WM_PAINT, 0, 0);
        CRSleep(100);
    }
}

static void _inner_paint_thread_(CRLVOID data, CRTHREAD idThis)
{
    PCRWINDOWINNER pInner = (PCRWINDOWINNER)data;
    //pInner->vkpaint = _inner_create_vk_(pInner->hWnd, pInner->w, pInner->h);
    while (pInner->onProcess)
    {
        CRSleep(1);
    }
    //_inner_release_vk_(pInner->vkpaint);
    pInner->vkpaint = NULL;
}

CRAPI CRINT64 CRWindowCounter(void)
{
    return windowCounter;
}

CRAPI void CRCreateWindow(PCRWindowProperties prop)
{
    if (!prop)
    {
        CR_LOG_ERR("auto", "nullptr");
        return;
    }
    prop->window = (CRWINDOW)NULL;
    PCRWINDOWINNER pInner = CRAlloc(NULL, sizeof(CRWINDOWINNER));
    if (!pInner)
    {
        CR_LOG_ERR("auto", "bad alloc");
        return;
    }
    for (int i = 0; i < CALLBACK_FUNCS_NUM; i++)
        pInner->funcs[i] = _inner_empty_callback_;
    pInner->onProcess = CRTRUE;
    pInner->preClose = CRFALSE;
    pInner->prop = prop;
    pInner->vkui = NULL;
    pInner->vkpaint = NULL;
    pInner->w = prop->w;
    pInner->h = prop->h;
    pInner->eventThread = CRThread(_inner_window_thread_, pInner);
    //
    //多线程操作需要加锁
    CRLock(lock);
    windowCounter++;
    CRUnlock(lock);
    //
    prop->window = (CRWINDOW)pInner;
}

CRAPI void CRCloseWindow(CRWINDOW window)
{
    if (!window)
    {
        CR_LOG_WAR("auto", "invalid window");
        return;
    }
    PCRWINDOWINNER pInner = (PCRWINDOWINNER)window;
    CloseWindow(pInner->hWnd);
}

CRAPI void CRSetWindowCbk(CRWINDOW window, CRUINT8 type, CRWindowCallback cbk)
{
    if (!window)
    {
        CR_LOG_WAR("auto", "invalid window");
        return;
    }
    PCRWINDOWINNER pInner = (PCRWINDOWINNER)window;
    if (cbk)
        pInner->funcs[type] = cbk;
    else
        pInner->funcs[type] = _inner_empty_callback_;
}

#endif