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
    VkImage *images;
    VkCommandBuffer cmd;
    VkImageView *views;
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
    VkInstance instance;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue queue;
    CRUINT32 graphics_queue_node_index;
    VkPhysicalDeviceProperties gpu_props;
    VkQueueFamilyProperties *queue_props;
    VkPhysicalDeviceMemoryProperties memory_properties;
    //
    CRUINT32 extension_count;
    CRUINT32 enabled_layer_count;
    const char* extensions[64];
    const char* device_validation_layers[64];
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

/**
 * 创建vulkan实例
 */
void _inner_create_vk_instance_(cr_vk_inner *pInner);
void _inner_destroy_vk_instance_(cr_vk_inner *pInner);
/**
 * 选择物理设备GPU
 */
void _inner_search_gpu_(cr_vk_inner *pInner);
/**
 * 创建逻辑设备
 */
void _inner_create_logical_device_(cr_vk_inner *pInner);
void _inner_destroy_logical_device_(cr_vk_inner *pInner);
/**
 * 创建交换链
 * 交换链是一个队列，包含一系列用于在屏幕上呈现的图像
 */
void _inner_create_swapchain_(
    cr_vk_inner *pInner,
#ifdef CR_WINDOWS
    HWND window,
#elif defined CR_LINUX
    Display *dpy,
    Window win,
#endif
    CRUINT32 w, CRUINT32 h
);
void _inner_destroy_swapchain_(cr_vk_inner *pInner);
/**
 * 创建图像视图
 */
void _inner_create_image_view_(cr_vk_inner *pInner);
void _inner_destroy_image_view_(cr_vk_inner *pInner);
/**
 * 创建渲染通道
 * 定义渲染过程中使用的附件
 */
void _inner_create_render_pass_(cr_vk_inner *pInner);
void _inner_destroy_render_pass_(cr_vk_inner *pInner);