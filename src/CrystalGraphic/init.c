/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-06-23 00:42:19
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-09-09 19:13:01
 * @FilePath: \CrystalEngine\src\CrystalGraphic\init.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include <GraphicDfs.h>
#include <CrystalThread.h>
#include "vk.h"

void **CRCoreFunList = NULL;
void **CRThreadFunList = NULL;
void **CRAlgorithmsFunList= NULL;
extern CRLOCK lock;

#ifdef CR_WINDOWS
#include <Windows.h>

extern LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static CRBOOL _inner_init_wndclass_(void)
{
    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_VREDRAW | CS_HREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hIcon = NULL;
    wcex.hCursor = NULL;
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = CR_WNDCLASS_NAME;
    wcex.hIconSm = NULL;
    if (!RegisterClassEx(&wcex))
    {
        CR_LOG_ERR("auto", "Error registering window class");
        return CRFALSE;
    }
    return CRTRUE;
}
static void _inner_uninit_wndclass_(void)
{
    if (!UnregisterClass(CR_WNDCLASS_NAME, GetModuleHandle(NULL))) CR_LOG_ERR("auto", "Failed unregister class");
}
#elif defined CR_LINUX
extern CRBOOL _inner_x_init_();
extern void _inner_x_uninit_();
#endif

CRAPI CRBOOL CRModInit(void **ptr)
{
    if (ptr[0] == ptr[1])
        return CRFALSE;
    CRCoreFunList = ptr;
    CRBOOL back;
    #ifdef CR_WINDOWS
    back = _inner_init_wndclass_();
    #elif defined CR_LINUX
    back = _inner_x_init_();
    #endif
    return back;
}

CRAPI void CRModUninit(void)
{
    #ifdef CR_WINDOWS
    _inner_uninit_wndclass_();
    #elif defined CR_LINUX
    _inner_x_uninit_();
    #endif
    if (lock) CRLockRelease(lock);
}

CRAPI CRBOOL CrystalGraphicInit(void** alg, void** thr)
{
    if (!alg || !thr)
    {
        CR_LOG_ERR("auto", "nullptr");
        return CRFALSE;
    }
    if (alg[0] == alg[2] || thr[0] == thr[2])
    {
        CR_LOG_ERR("auto", "CRAlgorithmsFunList or CRThreadFunList not inited correctly");
        return CRFALSE;
    }
    CRAlgorithmsFunList = alg;
    CRThreadFunList = thr;
    lock = CRLockCreate();
    return CRTRUE;
}