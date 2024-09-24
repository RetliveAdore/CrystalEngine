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
#include <resources.h>

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

//

//

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define NUM_DYNAMIC_STATS 2
#define CR_TEXTURE_COUNT 0

/**
 * 用于获取vulkan接口
 * 通常随版本快速变化的接口要通过这种方式手动获取
 */
#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                              \
{                                                                          \
    if (!g_gdpa)                                                           \
        g_gdpa = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(           \
            pInner->inst, "vkGetDeviceProcAddr");                          \
    pInner->fp##entrypoint =                                               \
        (PFN_vk##entrypoint)g_gdpa(dev, "vk" #entrypoint);                 \
    if (pInner->fp##entrypoint == NULL) {                                  \
        CR_LOG_ERR("auto", "vkGetDeviceProcAddr failed to find vk %s",     \
         #entrypoint);                                                     \
    }                                                                      \
}
#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                           \
{                                                                          \
    pInner->fp##entrypoint =                                               \
        (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint); \
    if (pInner->fp##entrypoint == NULL) {                                  \
        CR_LOG_ERR("auto", "vkGetInstanceProcAddr failed to find vk %s",   \
         #entrypoint);                                                     \
    }                                                                      \
}

/**
 * 追踪所有纹理相关的实体
 */
typedef struct{
    VkSampler sampler;
    //
    VkImage image;
    VkImageLayout imageLayout;
    //
    VkMemoryAllocateInfo mem_alloc;
    VkDeviceMemory mem;
    VkImageView view;
    CRUINT32 tex_width, tex_height;
}texture_object;

typedef struct{
    VkImage image;
    VkCommandBuffer cmd;
    VkImageView view;
}SwapchainBuffers;

typedef struct vk_inner{
    #ifdef CR_WINDOWS
    HINSTANCE hInst;
    HWND window;
    #elif defined CR_LINUIX
    #endif
    VkSurfaceKHR surface;
    CRBOOL inited;
    CRBOOL using_staging_buffer;
    //
    VkInstance inst;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue queue;
    CRUINT32 graphics_queue_node_index;
    VkPhysicalDeviceProperties gpu_props;
    VkQueueFamilyProperties *queue_props;
    VkPhysicalDeviceMemoryProperties memory_properties;
    //
    CRUINT32 enabled_extension_count;
    CRUINT32 enabled_layer_count;
    char* extension_names[64];
    char* device_validation_layers[64];
    //
    CRUINT32 width, height;
    VkFormat format;
    VkColorSpaceKHR color_space;
    //
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
    PFN_vkQueuePresentKHR fpQueuePresentKHR;
    CRUINT32 swapchainImageCount;
    VkSwapchainKHR swapchain;
    SwapchainBuffers *buffers;
    //
    VkCommandPool cmd_pool;
    struct{
        VkFormat format;
        VkImage image;
        VkMemoryAllocateInfo mem_alloc;
        VkDeviceMemory mem;
        VkImageView view;
    }depth;
    //
    texture_object *pTextures;
    struct{
        VkBuffer buf;
        VkMemoryAllocateInfo mem_alloc;
        VkDeviceMemory mem;
        VkDescriptorBufferInfo buffer_info;
    }uniform_data;
    //
    VkCommandBuffer cmd;
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout desc_layout;
    VkPipelineCache pipelineCache;
    VkRenderPass render_pass;
    VkPipeline pipeline;
    //
    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;
    //
    VkDescriptorPool desc_pool;
    VkDescriptorSet desc_set;
    //
    VkFramebuffer *frameBuffers;
    CRUINT32 curFrame;
    CRUINT32 frameCount;
    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
    VkDebugReportCallbackEXT msg_callback;
    PFN_vkDebugReportMessageEXT DebugReportMessage;
    //
    CRUINT32 current_buffer;
    CRUINT32 queue_count;
}cr_vk_inner;

//utility functions, for init

//检测类函数

/**
 * 检查内存特性是否符合要求
 */
CRBOOL _inner_memory_type_from_properties_(cr_vk_inner *pInner, CRUINT32 typeBits, VkFlags requirements_mask, uint32_t *typeIndex);
/**
 * 用于刷新绘制命令池
 */
void _inner_flush_init_cmd_(cr_vk_inner *pInner);
/**
 * 检查层描述符，判定是否支持全部特性
 * 只有全部特性支持时才返回CRTRUE
 */
CRBOOL _inner_check_layers_(uint32_t check_count, char **check_names, uint32_t layer_count, VkLayerProperties *layers);

//初始化类函数

/**
 * 创建vulkan句柄
 */
void _inner_create_instance_(cr_vk_inner *pInner);
void _inner_destroy_instance_(cr_vk_inner *pInner);
/**
 * 创建逻辑设备
 */
void _inner_create_device_(cr_vk_inner *pInner);
void _inner_destroy_device_(cr_vk_inner *pInner);
/**
 * 创建交换链
 */
void _inner_init_swapchain_(
    cr_vk_inner *pInner,
#ifdef CR_WINDOWS
    HWND window
#elif defined CR_LINUX
    Display *dpy,
    Window win
#endif
);
void _inner_uninit_swapchain_(cr_vk_inner *pInner);

/**
 * 创建交换链的图像缓存
 * 交换链中每一帧都由一张图像保存，
 * 绘制完毕后将此张图像切换到当前显示，
 * 然后在交换链中的下一张图像上绘制
 */
void _inner_prepare_buffers_(cr_vk_inner *pInner);
void _inner_deprepare_buffers_(cr_vk_inner *pInner);
/**
 * 创建深度缓冲图像
 * 用于深度检测以便后期可能会剔除一些面片
 */
void _inner_prepare_depth_(cr_vk_inner *pInner);
void _inner_deprepare_depth_(cr_vk_inner *pInner);
/**
 * 创建纹理（留空）
 */
void _inner_prepare_textures_(cr_vk_inner *pInner);
void _inner_deprepare_textures_(cr_vk_inner *pInner);
/**
 * 创建描述符接口
 * 此接口用于描述一个资源，
 * 资源会像游戏机卡带一样插到vulkan中以供使用，
 * 此接口便提供一个插入的方式，描述符不包含数据，
 * 仅描述数据是如何组织的
 */
void _inner_set_image_layout_(
    cr_vk_inner *pInner,
    VkImage image,
    VkImageAspectFlags aspectMask,
    VkImageLayout old_image_layout,
    VkImageLayout new_image_layout
);
void _inner_prepare_descriptor_layout_(cr_vk_inner *pInner);
void _inner_deprepare_descriptor_layout_(cr_vk_inner *pInner);
/**
 * 创建渲染通道
 * 分块渲染特性
 */
void _inner_prepare_render_pass_(cr_vk_inner *pInner);
void _inner_deprepare_render_pass_(cr_vk_inner *pInner);
/**
 * 创建渲染管线
 * 主要工作是将shader导入到gpu，
 * 然后设置一些基本的渲染设置，如视口、裁切等
 */
VkShaderModule _inner_prepare_shader_(cr_vk_inner *pInner, void* code, CRUINT64 size, VkShaderModule *pModule);
void _inner_prepare_pipeline_(cr_vk_inner *pInner);
void _inner_deprepare_pipeline_(cr_vk_inner *pInner);
/**
 * 创建符号池，可以保存复数的描述符
 */
void _inner_prepare_descriptor_pool_(cr_vk_inner *pInner);
void _inner_deprepare_descriptor_pool_(cr_vk_inner *pInner);
/**
 * 创建帧缓冲，用于离屏渲染
 */
void _inner_prepare_framebuffers_(cr_vk_inner *pInner);
void _inner_deprepare_framebuffers_(cr_vk_inner *pInner);
