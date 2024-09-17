/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-08-18 22:07:38
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-09-10 19:04:05
 * @FilePath: \CrystalEngine\src\CrystalGraphic\vk.h
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */

#include <GraphicDfs.h>
#include <vulkan/vulkan.h>

#ifdef CR_WINDOWS
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#elif defined CR_LINUX
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#endif

typedef CRLVOID cr_vk;

cr_vk _inner_create_vk_(
#ifdef CR_WINDOWS
    HWND window,
#elif defined CR_LINUX
    Display *dpy,
    Window win,
#endif
    CRUINT32 w, CRUINT32 h
);

void _inner_destroy_vk_(cr_vk vk);

void _inner_setsize_vk_(CRUINT32 w, CRUINT32 h);
void _inner_resize_vk_(cr_vk vk);

/**
 * UI层在content层之上，确保UI永远不会被图像挡住
 * ------------------UI      ^
 * ------------------Content |
 */

//使用此函数来绘制UI层
void _inner_paint_ui_(cr_vk vk);