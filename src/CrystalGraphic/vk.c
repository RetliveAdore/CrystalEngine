/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-08-18 21:36:50
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-08-28 22:18:06
 * @FilePath: \CrystalEngine\src\CrystalGraphic\vk.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include "vk.h"

//单例，每个进程只需要初始化一次
static struct vk_init
{
    VkInstance instance;
    VkPhysicalDevice physical_device;
}CR_VKINIT;

//实例，每个进程可以创建多个
typedef struct vk_struct
{
    VkSurfaceKHR surface;
    VkDevice device;
    struct{
        VkSwapchainKHR swapchain;
        CRUINT32 image_count;
        VkFormat image_format;
        VkColorSpaceKHR color_space;
    }swapchain;
    #ifdef CR_WINDOWS
    HWND hWnd;
    #elif defined CR_LINUX
    Display *pDisplay;
    Window window;
    #endif
}CR_VKSTRUCT, *PCR_VKSTRUCT;

static void _inner_device_initialization_()
{
    VkApplicationInfo appInfo = {0};
    appInfo.sType =VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "CrystalVulkan";
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    //
    #ifdef CR_WINDOWS
    CRUINT32 extensionCount = 2;
    const char* extensions[2] = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface"
    };
    #elif defined CR_LINUX
    CRUINT32 extensionCount = 2;
    const char* extensions[2] = {
        "VK_KHR_surface",
        "VK_KHR_xlib_surface"
    };
    #endif
    VkInstanceCreateInfo instanceCreateInfo = {0};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = extensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = extensions;
    instanceCreateInfo.enabledLayerCount = 0;
    if (vkCreateInstance(&instanceCreateInfo, NULL, &(CR_VKINIT.instance)) != VK_SUCCESS)
    CR_LOG_ERR("auto", "failed to create instance!");
}

void _inner_init_vk_()
{
    #ifdef CR_WINDOWS
    VkWin32SurfaceCreateInfoKHR inf;
    #elif defined CR_LINUX
    #endif
    _inner_device_initialization_();
    //枚举物理设备
    CRUINT32 deviceCount = 0;
    vkEnumeratePhysicalDevices(CR_VKINIT.instance, &deviceCount, NULL);
    VkPhysicalDevice *pDevice = (VkPhysicalDevice*)CRAlloc(NULL,sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(CR_VKINIT.instance, &deviceCount, pDevice);
    CR_VKINIT.physical_device = pDevice[0];
    VkPhysicalDeviceProperties prop;
    vkGetPhysicalDeviceProperties(CR_VKINIT.physical_device, &prop);
    CRUINT32 extensionCounts = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCounts, NULL);
    CR_LOG_IFO("auto", "Default graphic card: %s, %d extensions supported", prop.deviceName, extensionCounts);
    CRAlloc(pDevice, 0);
}

void _inner_uninit_vk_()
{
    vkDestroyInstance(CR_VKINIT.instance, NULL);
    CR_VKINIT.instance = VK_NULL_HANDLE;
}

#ifdef CR_WINDOWS
cr_vk _inner_create_vk_(HWND hWnd)
{
    PCR_VKSTRUCT pInner = CRAlloc(NULL, sizeof(CR_VKSTRUCT));
    if (!pInner)
    {
        CR_LOG_ERR("auto", "bad alloc");
        return NULL;
    }
    pInner->hWnd = hWnd;
    VkWin32SurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = pInner->hWnd;
    createInfo.hinstance = GetModuleHandle(NULL);
    if (vkCreateWin32SurfaceKHR(CR_VKINIT.instance, &createInfo, NULL, &(pInner->surface)) != VK_SUCCESS)
    {
        CR_LOG_WAR("auto", "failed to create surface");
        CRAlloc(pInner, 0);
        return NULL;
    }
    CR_LOG_DBG("auto", "create surface success");
    //
    return pInner;
}
#elif defined CR_LINUX
cr_vk _inner_create_vk_(Display* dpy, Window win)
{
    PCR_VKSTRUCT pInner = CRAlloc(NULL, sizeof(CR_VKSTRUCT));
    if (!pInner)
    {
        CR_LOG_ERR("auto", "bad alloc");
        return NULL;
    }
    pInner->pDisplay = dpy;
    pInner->window = win;
    VkXlibSurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.dpy = pInner->pDisplay;
    createInfo.window = pInner->window;
    if (vkCreateXlibSurfaceKHR(CR_VKINIT.instance, &createInfo, NULL, &(pInner->surface)) != VK_SUCCESS)
    {
        CR_LOG_WAR("auto", "failed to create surface");
        CRAlloc(pInner, 0);
        return NULL;
    }
    CR_LOG_DBG("auto", "create surface success");
    //
    return pInner;
}
#endif

void _inner_release_vk_(cr_vk vk)
{
    PCR_VKSTRUCT pInner = (PCR_VKSTRUCT)vk;
    vkDestroyDevice(pInner->device, NULL);
    vkDestroySurfaceKHR(CR_VKINIT.instance, pInner->surface, NULL);
}