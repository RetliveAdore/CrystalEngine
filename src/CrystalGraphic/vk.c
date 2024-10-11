/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-08-18 21:36:50
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-09-17 21:51:23
 * @FilePath: \CrystalEngine\src\CrystalGraphic\vk.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include "vk.h"
#include <stdio.h>
#include <string.h>

cr_vk _inner_create_vk_(
#ifdef CR_WINDOWS
    HWND window,
#elif defined CR_LINUX
    Display *dpy,
    Window win,
#endif
    CRUINT32 w, CRUINT32 h
)
{
    //
    cr_vk_inner *pInner = CRAlloc(NULL, sizeof(cr_vk_inner));
    if (!pInner)
    {
        CR_LOG_ERR("auto", "bad alloc");
        return NULL;
    }
    //创建Vulkan实例
    _inner_create_vk_instance_(pInner);
    //获取符合要求的GPU硬件
    _inner_search_gpu_(pInner);
    //创建逻辑设备
    _inner_create_logical_device_(pInner);
    //
    //
    #ifdef CR_WINDOWS
    _inner_create_swapchain_(pInner, window, w, h);
    #elif defined CR_LINUX

    #endif
    //
    _inner_create_image_view_(pInner);
    //
    _inner_create_render_pass_(pInner);
    //
    return pInner;
}

void _inner_destroy_vk_(cr_vk vk)
{
    //
    cr_vk_inner *pInner = (cr_vk_inner*)vk;
    if (!pInner)
    {
        CR_LOG_ERR("auto", "invalid vulkan structure");
        return;
    }
    //
    _inner_destroy_render_pass_(pInner);
    //
    _inner_destroy_image_view_(pInner);
    //
    _inner_destroy_swapchain_(pInner);
    //
    //
    _inner_destroy_logical_device_(pInner);
    //
    _inner_destroy_vk_instance_(pInner);
    //
    CRAlloc(pInner, 0);
}

void _inner_paint_ui_(cr_vk vk)
{
    //暂时什么都不执行
}