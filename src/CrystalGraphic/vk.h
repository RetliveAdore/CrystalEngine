/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-08-18 22:07:38
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-09-04 21:17:26
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

void _inner_init_vk_();
void _inner_uninit_vk_();

#ifdef CR_WINDOWS
cr_vk _inner_create_vk_(HWND hWnd, CRUINT32 w, CRUINT32 h);
#elif defined CR_LINUX
cr_vk _inner_create_vk_(Display *dpy, Window win, CRUINT32 w, CRUINT32 h);
#endif

void _inner_release_vk_(cr_vk vk);