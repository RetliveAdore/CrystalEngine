/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-08-18 21:36:50
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-09-17 18:00:25
 * @FilePath: \CrystalEngine\src\CrystalGraphic\vk.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include "vk.h"
#include <resources.h>
#include <stdio.h>
#include <string.h>

//用于控制是否启用验证层
#define CRVK_NO_DBG

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

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

VKAPI_ATTR VkBool32 VKAPI_CALL
dbgFunc(
    VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
    uint64_t srcObject, size_t location, int32_t msgCode,
    const char *pLayerPrefix, const char *pMsg, void *pUserData
)
{
    char *message = (char *)CRAlloc(NULL, strlen(pMsg) + 100);

    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        sprintf(message, "ERROR: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
    }
    else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        // We know that we're submitting queues without fences, ignore this
        // warning
        if (strstr(pMsg, "vkQueueSubmit parameter, VkFence fence, is null pointer"))
        {
            return VK_FALSE;
        }
        sprintf(message, "WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
    }
    else
    {
        return VK_FALSE;
    }

    CRPrint(CR_TC_RED, "%s", message);
    CRAlloc(message, 0);

    /*
     * false indicates that layer should not bail-out of an
     * API call that had validation failures. This may mean that the
     * app dies inside the driver due to invalid parameter(s).
     * That's what would happen without validation layers, so we'll
     * keep that behavior here.
     */
    return VK_FALSE;
}

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
    VkDescriptorPool decs_pool;
    VkDescriptorSet desc_set;
    //
    VkFramebuffer *frameBuffers;
    CRUINT32 curFrame;
    CRUINT32 frameCount;
    CRBOOL validate;
    CRBOOL use_break;
    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
    VkDebugReportCallbackEXT msg_callback;
    PFN_vkDebugReportMessageEXT DebugReportMessage;
    //
    CRUINT32 current_buffer;
    CRUINT32 queue_count;
}cr_vk_inner;

/**
 * 检查内存特性是否符合
 */
static CRBOOL _inner_memory_type_from_properties_(cr_vk_inner *pInner, CRUINT32 typeBits, VkFlags requirements_mask, uint32_t *typeIndex)
{
    for (uint32_t i = 0; i < 32; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((pInner->memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
            {
                *typeIndex = i;
                return CRTRUE;
            }
        }
        typeBits >>= 1;
    }
    return CRFALSE;
}

static void _inner_flush_init_cmd_(cr_vk_inner *pInner)
{
    //
}

/*
 * Return 1 (true) if all layer names specified in check_names
 * can be found in given layer properties.
 */
static CRBOOL _inner_check_layers_(
    uint32_t check_count, char **check_names,
    uint32_t layer_count,
    VkLayerProperties *layers
)
{
    for (uint32_t i = 0; i < check_count; i++) {
        CRBOOL found = 0;
        for (uint32_t j = 0; j < layer_count; j++) {
            if (!strcmp(check_names[i], layers[j].layerName)) {
                found = 1;
                break;
            }
        }
        if (!found) {
            CR_LOG_ERR("auto", "Cannot find layer: %s", check_names[i]);
            return CRFALSE;
        }
    }
    return CRTRUE;
}

static void _inner_create_device_(cr_vk_inner *pInner)
{
    VkResult err;
    float queue_priorities[1] = {0.0f};
    const VkDeviceQueueCreateInfo queue = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueFamilyIndex = pInner->graphics_queue_node_index,
        .queueCount = 1,
        .pQueuePriorities = queue_priorities
    };
    VkDeviceCreateInfo device = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue,
        .enabledLayerCount = pInner->enabled_layer_count,
        .ppEnabledLayerNames = (const char *const *)((pInner->validate) ? pInner->device_validation_layers : NULL),
        .enabledExtensionCount = pInner->enabled_extension_count,
        .ppEnabledExtensionNames = (const char *const *)pInner->extension_names,
        .pEnabledFeatures = NULL
    };
    err = vkCreateDevice(pInner->gpu, &device, NULL, &pInner->device);
    if (err) CR_LOG_ERR("auto", "failed to create logic device!");
}

static void _inner_destroy_device_(cr_vk_inner *pInner)
{
    vkDestroyDevice(pInner->device, NULL);
}

static PFN_vkGetDeviceProcAddr g_gdpa = NULL;
#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                                  \
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

static void _inner_init_swapchain_(
    cr_vk_inner *pInner,
#ifdef CR_WINDOWS
    HWND window
#elif defined CR_LINUX
    Display *dpy,
    Window win
#endif
)
{
    VkResult err;
    CRUINT32 i;
    //首先要创建图像面
    #ifdef CR_WINDOWS
    VkWin32SurfaceCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .hinstance = GetModuleHandle(NULL),
        .hwnd = window
    };
    err = vkCreateWin32SurfaceKHR(pInner->inst, &createInfo, NULL, &pInner->surface);
    #elif defined CR_LINUX
    VkXlibSurfaceCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .dpy = dpy,
        .window = win
    };
    err = vkCreateXlibSurfaceKHR(pInner->inst, &createInfo, NULL, &pInner->surface);
    #endif
    if (err) CR_LOG_ERR("auto", "failed to create surface");
    VkBool32 *supportPresent = (VkBool32*)CRAlloc(NULL, pInner->queue_count * sizeof(VkBool32));
    if (!supportPresent) CR_LOG_ERR("auto", "bad alloc");
    for (i = 0; i < pInner->queue_count; i++)
        pInner->fpGetPhysicalDeviceSurfaceSupportKHR(pInner->gpu, i, pInner->surface, &supportPresent[i]);
    CRUINT32 graphicsQueueNodeIndex = UINT32_MAX;
    CRUINT32 presentQueueNodeIndex = UINT32_MAX;
    for (i = 0; i < pInner->queue_count; i++)
    {
        if (pInner->queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            if (graphicsQueueNodeIndex = UINT32_MAX)
                graphicsQueueNodeIndex = i;
        }
        if (supportPresent[i] == VK_TRUE)
        {
            graphicsQueueNodeIndex = i;
            presentQueueNodeIndex = i;
            break;
        }
    }
    if (presentQueueNodeIndex == UINT32_MAX)
    {
        //并未找到同时支持
        for (i = 0; i < pInner->queue_count; i++)
        {
            if (supportPresent[i] == VK_TRUE)
                presentQueueNodeIndex = i;
            break;
        }
    }
    CRAlloc(supportPresent, 0);
    if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)
        CR_LOG_ERR("auto", "cound not find a graphic or a present queue");
    if (graphicsQueueNodeIndex != presentQueueNodeIndex)
        CR_LOG_ERR("auto", "cound not find a common graphic and a present queue");
    pInner->graphics_queue_node_index = graphicsQueueNodeIndex;
    //创建逻辑设备
    _inner_create_device_(pInner);
    //
    GET_DEVICE_PROC_ADDR(pInner->device, CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(pInner->device, DestroySwapchainKHR);
    GET_DEVICE_PROC_ADDR(pInner->device, GetSwapchainImagesKHR);
    GET_DEVICE_PROC_ADDR(pInner->device, AcquireNextImageKHR);
    GET_DEVICE_PROC_ADDR(pInner->device, QueuePresentKHR);
    //
    vkGetDeviceQueue(pInner->device, pInner->graphics_queue_node_index, 0, &pInner->queue);
    CRUINT32 formatCount;
    err = pInner->fpGetPhysicalDeviceSurfaceFormatsKHR(pInner->gpu, pInner->surface, &formatCount, NULL);
    if (err) CR_LOG_ERR("auto", "failed to get gpu surface formats");
    VkSurfaceFormatKHR *surfFormats = (VkSurfaceFormatKHR*)CRAlloc(NULL, sizeof(VkSurfaceFormatKHR) * formatCount);
    if (!surfFormats) CR_LOG_ERR("auto", "bad alloc");
    err = pInner->fpGetPhysicalDeviceSurfaceFormatsKHR(pInner->gpu, pInner->surface, &formatCount, surfFormats);
    if (err) CR_LOG_ERR("auto", "failed to get gpu surface formats");
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED)
        pInner->format = VK_FORMAT_B8G8R8A8_UNORM;
    else pInner->format = surfFormats[0].format;
    pInner->color_space = surfFormats[0].colorSpace;
    pInner->curFrame = 0;
    //获取内存配置
    vkGetPhysicalDeviceMemoryProperties(pInner->gpu, &pInner->memory_properties);
}

static void _inner_uninit_swapchain_(cr_vk_inner *pInner)
{
    _inner_destroy_device_(pInner);
    vkDestroySurfaceKHR(pInner->inst, pInner->surface, NULL);
}

static void _inner_set_image_layout_(
    cr_vk_inner *pInner,
    VkImage image,
    VkImageAspectFlags aspectMask,
    VkImageLayout old_image_layout,
    VkImageLayout new_image_layout
)
{
    VkResult err;
    if (pInner->cmd == VK_NULL_HANDLE)
    {
        const VkCommandBufferAllocateInfo cmd = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = NULL,
            .commandPool = pInner->cmd_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };
        err = vkAllocateCommandBuffers(pInner->device, &cmd, &pInner->cmd);
        if (err) CR_LOG_ERR("auto", "failed to allocate command buffer!");
        VkCommandBufferInheritanceInfo cmd_buf_hinfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
            .pNext = NULL,
            .renderPass = VK_NULL_HANDLE,
            .subpass = 0,
            .framebuffer = VK_NULL_HANDLE,
            .occlusionQueryEnable = VK_FALSE,
            .queryFlags = 0,
            .pipelineStatistics = 0
        };
        VkCommandBufferBeginInfo cmd_buf_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = NULL,
            .flags = 0,
            .pInheritanceInfo = &cmd_buf_hinfo
        };
        err = vkBeginCommandBuffer(pInner->cmd, &cmd_buf_info);
        if (err) CR_LOG_ERR("auto", "failed to begin command buffer!");
    }
    VkImageMemoryBarrier image_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
        .oldLayout = old_image_layout,
        .newLayout = new_image_layout,
        .image = image,
        .subresourceRange = {aspectMask, 0, 1, 0, 1}
    };
    if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    else if (new_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    else if (new_image_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    else if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;
    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    vkCmdPipelineBarrier(pInner->cmd, src_stages, dst_stages, 0, 0, NULL, 0, NULL, 1, pmemory_barrier);
}

static void _inner_prepare_buffers_(cr_vk_inner *pInner)
{
    VkResult err;
    VkSwapchainKHR oldSwapchain = pInner->swapchain;
    //检查表层功能和格式
    VkSurfaceCapabilitiesKHR surfCapabilities;
    err = pInner->fpGetPhysicalDeviceSurfaceCapabilitiesKHR(pInner->gpu, pInner->surface, &surfCapabilities);
    if (err) CR_LOG_ERR("auto", "failed to get surface capabilities!");
    CRUINT32 presentModeCount;
    err = pInner->fpGetPhysicalDeviceSurfacePresentModesKHR(pInner->gpu, pInner->surface, &presentModeCount, NULL);
    if (err) CR_LOG_ERR("auto", "failed to get present mode!");
    VkPresentModeKHR *presentModes = CRAlloc(NULL, sizeof(VkPresentModeKHR) * presentModeCount);
    if (!presentModes) CR_LOG_ERR("auto", "bad alloc");
    err = pInner->fpGetPhysicalDeviceSurfacePresentModesKHR(pInner->gpu, pInner->surface, &presentModeCount, presentModes);
    if (err) CR_LOG_ERR("auto", "failed to get present mode!");
    //
    VkExtent2D swapchainExtent;
    if (surfCapabilities.currentExtent.width == (CRUINT32)-1)
    {
        swapchainExtent.width = pInner->width;
        swapchainExtent.height = pInner->height;
    }
    else
    {
        swapchainExtent = surfCapabilities.currentExtent;
        pInner->width = surfCapabilities.currentExtent.width;
        pInner->height = surfCapabilities.currentExtent.height;
    }
    //消息模式优先级为：MAILBOX > IMMEDATE > FIFO
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (CRUINT64 i = 0; i < presentModeCount; i++)
    {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
        else if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
            swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
    //
    CRUINT32 desiredNumberOfSwapchainImages = surfCapabilities.minImageCount + 1;
    if ((surfCapabilities.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCapabilities.maxImageCount))
        desiredNumberOfSwapchainImages - surfCapabilities.maxImageCount;
    VkSurfaceTransformFlagsKHR preTransform;
    if (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    else preTransform = surfCapabilities.currentTransform;
    const VkSwapchainCreateInfoKHR swapchain = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = pInner->surface,
        .minImageCount = desiredNumberOfSwapchainImages,
        .imageFormat = pInner->format,
        .imageColorSpace = pInner->color_space,
        .imageExtent = {
            .width = swapchainExtent.width, .height = swapchainExtent.height
        },
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = preTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .imageArrayLayers = 1,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .presentMode = swapchainPresentMode,
        .oldSwapchain = oldSwapchain,
        .clipped = VK_TRUE
    };
    CRUINT32 i;
    err = pInner->fpCreateSwapchainKHR(pInner->device, &swapchain, NULL, &pInner->swapchain);
    if (err) CR_LOG_ERR("auto", "failed to create new swapchain!");
    if (oldSwapchain != VK_NULL_HANDLE) pInner->fpDestroySwapchainKHR(pInner->device, oldSwapchain, NULL);
    //
    err = pInner->fpGetSwapchainImagesKHR(pInner->device, pInner->swapchain, &pInner->swapchainImageCount, NULL);
    if (err) CR_LOG_ERR("auto", "failed to get swap chain images!");
    //此内存会转移到pInner->buffers中，不需要在此处释放
    VkImage *swapchainImages = CRAlloc(NULL, sizeof(VkImage) * pInner->swapchainImageCount);
    err = pInner->fpGetSwapchainImagesKHR(pInner->device, pInner->swapchain, &pInner->swapchainImageCount, swapchainImages);
    if (err) CR_LOG_ERR("auto", "failed to get swap chain images!");
    //
    pInner->buffers = CRAlloc(NULL, sizeof(SwapchainBuffers) * pInner->swapchainImageCount);
    if (!pInner->buffers) CR_LOG_ERR("auto", "bad alloc");
    for (int i = 0; i < pInner->swapchainImageCount; i++)
    {
        VkImageViewCreateInfo color_image_view = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .format = pInner->format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_R,
                .g = VK_COMPONENT_SWIZZLE_G,
                .b = VK_COMPONENT_SWIZZLE_B,
                .a = VK_COMPONENT_SWIZZLE_A,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .viewType = VK_IMAGE_VIEW_TYPE_2D
        };
        pInner->buffers[i].image = swapchainImages[i];
        //
        _inner_set_image_layout_(pInner, pInner->buffers[i].image,
            VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        //
        color_image_view.image = pInner->buffers[i].image;
        err = vkCreateImageView(pInner->device, &color_image_view, NULL, &pInner->buffers[i].view);
        if (err) CR_LOG_ERR("auto", "failed to create image view");
    }
    //
    if (presentModes) CRAlloc(presentModes, 0);
}

static void _inner_deprepare_buffers_(cr_vk_inner *pInner)
{
    for (int i = 0; i < pInner->swapchainImageCount; i++)
        vkDestroyImageView(pInner->device, pInner->buffers[i].view, NULL);
    CRAlloc(pInner->buffers, 0);
    pInner->fpDestroySwapchainKHR(pInner->device, pInner->swapchain, NULL);
}

static void _inner_prepare_depth_(cr_vk_inner *pInner)
{
    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    const VkImageCreateInfo image = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depth_format,
        .extent= {pInner->width, pInner->height},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    };
    VkImageViewCreateInfo view = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = VK_NULL_HANDLE,
        .format = depth_format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .viewType = VK_IMAGE_VIEW_TYPE_2D
    };
    VkMemoryRequirements mem_reqs;
    VkResult err;
    CRBOOL pass;
    pInner->depth.format = depth_format;
    //创建图像
    err = vkCreateImage(pInner->device, &image, NULL, &pInner->depth.image);
    if (err) CR_LOG_ERR("auto", "failed to create image!");
    vkGetImageMemoryRequirements(pInner->device, pInner->depth.image, &mem_reqs);
    //申请图像内存（显存中申请）
    pInner->depth.mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    pInner->depth.mem_alloc.pNext = NULL;
    pInner->depth.mem_alloc.allocationSize = mem_reqs.size;
    pInner->depth.mem_alloc.memoryTypeIndex = 0;
    if (!_inner_memory_type_from_properties_(pInner, mem_reqs.memoryTypeBits, 0, &pInner->depth.mem_alloc.memoryTypeIndex))
        CR_LOG_ERR("auto", "No memory types matched");
    err = vkAllocateMemory(pInner->device, &pInner->depth.mem_alloc, NULL, &pInner->depth.mem);
    if (err) CR_LOG_ERR("auto", "bad alloc vulkan");
    //绑定内存到深度缓冲图像
    err = vkBindImageMemory(pInner->device, pInner->depth.image, pInner->depth.mem, 0);
    if (err) CR_LOG_ERR("auto", "failed to bind memory to image!");
    //
    _inner_set_image_layout_(pInner, pInner->depth.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    view.image = pInner->depth.image;
    err = vkCreateImageView(pInner->device, &view, NULL, &pInner->depth.view);
    if (err) CR_LOG_ERR("auto", "failed to create image view!");
}

static void _inner_deprepare_depth_(cr_vk_inner *pInner)
{
    vkDestroyImageView(pInner->device, pInner->depth.view, NULL);
    vkFreeMemory(pInner->device, pInner->depth.mem, NULL);
    vkDestroyImage(pInner->device, pInner->depth.image, NULL);
}

static void _inner_prepare_textures_(cr_vk_inner *pInner)
{
    CR_LOG_DBG("auto", "prepare textures");
}

static void _inner_deprepare_textures_(cr_vk_inner *pInner)
{

}

static void _inner_create_pipeline_(cr_vk_inner *pInner)
{
    VkResult err;
    const VkCommandPoolCreateInfo cmd_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueFamilyIndex = pInner->graphics_queue_node_index
    };
    err = vkCreateCommandPool(pInner->device, &cmd_pool_info, NULL, &pInner->cmd_pool);
    if (err) CR_LOG_ERR("auto", "failed to create command pool!");
    const VkCommandBufferAllocateInfo cmd = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = pInner->cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    //
    _inner_prepare_buffers_(pInner);
    _inner_prepare_depth_(pInner);
    _inner_prepare_textures_(pInner);
}

static void _inner_destroy_pipeline_(cr_vk_inner *pInner)
{
    _inner_deprepare_textures_(pInner);
    _inner_deprepare_depth_(pInner);
    _inner_deprepare_buffers_(pInner);
    vkDestroyCommandPool(pInner->device, pInner->cmd_pool, NULL);
}

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                               \
    {                                                                          \
        pInner->fp##entrypoint =                                               \
            (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint); \
        if (pInner->fp##entrypoint == NULL) {                                  \
            CR_LOG_ERR("auto", "vkGetInstanceProcAddr failed to find vk %s",   \
             #entrypoint);                                                     \
        }                                                                      \
    }

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
    cr_vk_inner *pInner = CRAlloc(NULL, sizeof(cr_vk_inner));
    if (!pInner) CR_LOG_ERR("auto", "bad alloc");
    memset(pInner, 0, sizeof(*pInner));
    pInner->width = w;
    pInner->height = h;
    //
    #ifndef CRVK_NO_DBG
    pInner->validate = CRTRUE;
    #else
    pInner->validate = CRFALSE;
    #endif
    pInner->use_break = CRFALSE;
    //
    VkResult err;
    char *extension_names[64];
    CRUINT32 instance_extension_count = 0;
    CRUINT32 instance_layer_count = 0;
    CRUINT32 device_validation_layer_count = 0;
    CRUINT32 enabled_extension_count = 0;
    CRUINT32 enabled_layer_count = 0;
    //
    char *instance_validation_layers[] ={
        "VK_LAYER_LUNARG_threading",      "VK_LAYER_LUNARG_mem_tracker",
        "VK_LAYER_LUNARG_object_tracker", "VK_LAYER_LUNARG_draw_state",
        "VK_LAYER_LUNARG_param_checker",  "VK_LAYER_LUNARG_swapchain",
        "VK_LAYER_LUNARG_device_limits",  "VK_LAYER_LUNARG_image",
        "VK_LAYER_GOOGLE_unique_objects"
    };
    pInner->device_validation_layers[0] = "VK_LAYER_LUNARG_threading";
    pInner->device_validation_layers[1] = "VK_LAYER_LUNARG_mem_tracker";
    pInner->device_validation_layers[2] = "VK_LAYER_LUNARG_object_tracker";
    pInner->device_validation_layers[3] = "VK_LAYER_LUNARG_draw_state";
    pInner->device_validation_layers[4] = "VK_LAYER_LUNARG_param_checker";
    pInner->device_validation_layers[5] = "VK_LAYER_LUNARG_swapchain";
    pInner->device_validation_layers[6] = "VK_LAYER_LUNARG_device_limits";
    pInner->device_validation_layers[7] = "VK_LAYER_LUNARG_image";
    pInner->device_validation_layers[8] = "VK_LAYER_GOOGLE_unique_objects";
    device_validation_layer_count = 9;
    //
    CRBOOL validation_found = CRFALSE;
    err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
    if (err) CR_LOG_ERR("auto", "failed to enumerate instance layer properties!");
    if (instance_layer_count)
    {
        VkLayerProperties *instance_layers = CRAlloc(NULL, sizeof(VkLayerProperties) * instance_layer_count);
        if (!instance_layers) CR_LOG_ERR("auto", "bad alloc");
        err = vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers);
        if (err) CR_LOG_ERR("auto", "failed to enumerate instance layer properties!");
        if (pInner->validate)
        {
            validation_found = _inner_check_layers_(
                ARRAY_SIZE(instance_validation_layers),
                instance_validation_layers, instance_layer_count,
                instance_layers
            );
            enabled_layer_count = ARRAY_SIZE(instance_validation_layers);
        }
        CRAlloc(instance_layers, 0);
    }
    if (pInner->validate && !validation_found) CR_LOG_ERR("auto", "failed to find required validation layer!");
    //查看实例扩展
    CRBOOL surfaceExtFound = CRFALSE;
    CRBOOL platformSurfaceExtFound = CRFALSE;
    memset(extension_names, 0, sizeof(extension_names));
    err = vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL);
    if (err) CR_LOG_ERR("auto", "failed to enumerate instance extension properties");
    if (instance_extension_count)
    {
        VkExtensionProperties *instance_extensions = CRAlloc(NULL, sizeof(VkExtensionProperties) * instance_extension_count);
        if (!instance_extensions) CR_LOG_ERR("auto", "bad alloc");
        err = vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, instance_extensions);
        if (err) CR_LOG_ERR("auto", "failed to enumerate instance extension properties");
        for (uint32_t i = 0; i < instance_extension_count; i++)
        {
            if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName))
            {
                surfaceExtFound = CRTRUE;
                extension_names[enabled_extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
            }
            if (!strcmp(
                #ifdef CR_WINDOWS
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME
                #elif defined CR_LINUX
                VK_KHR_XLIB_SURFACE_EXTENSION_NAME
                #endif
                ,
                instance_extensions[i].extensionName))
            {
                platformSurfaceExtFound = CRTRUE;
                #ifdef CR_WINDOWS
                extension_names[enabled_extension_count++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
                #elif defined CR_LINUX
                extension_names[enabled_extension_count++] = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
                #endif
            }
            if (!strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, instance_extensions[i].extensionName))
            {
                if (pInner->validate)
                {
                    extension_names[enabled_extension_count++] =
                        VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
                }
            }
        }
        CRAlloc(instance_extensions, 0);
    }
    if (!surfaceExtFound) CR_LOG_ERR("auto", "failed to find: %s", VK_KHR_SURFACE_EXTENSION_NAME);
    if (!platformSurfaceExtFound)
        CR_LOG_ERR("auto", "failed to find: %s",
            #ifdef CR_WINDOWS
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
            #elif defined CR_LINUX
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME
            #endif
        );
    //
    const VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "CrystalApp",
        .applicationVersion = 0,
        .pEngineName = "CrystalEngine",
        .engineVersion = 0,
        .apiVersion = VK_MAKE_VERSION(1, 2, 0),
    };
    VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pApplicationInfo = &app,
        .enabledLayerCount = enabled_layer_count,
        .ppEnabledLayerNames = (const char *const *)((pInner->validate) ? instance_validation_layers : NULL),
        .enabledExtensionCount = enabled_extension_count,
        .ppEnabledExtensionNames = (const char *const *)extension_names,
    };
    CRUINT32 gpu_count;
    err = vkCreateInstance(&inst_info, NULL, &pInner->inst);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) CR_LOG_ERR("auto", "cannot find a compatible Vulkan installable client driver!");
    else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) CR_LOG_ERR("auto", "cannot find a specified extension library");
    else if (err) CR_LOG_ERR("auto", "failed to create vulkan instance");
    //开始枚举显卡
    err = vkEnumeratePhysicalDevices(pInner->inst, &gpu_count, NULL);
    if (err || !gpu_count) CR_LOG_ERR("auto", "failed to enumerate gpu(s)");
    CRPrint(CR_TC_GREEN, "显卡数量：%d\n", gpu_count);
    if (gpu_count)
    {
        VkPhysicalDevice *physical_devices = CRAlloc(NULL, sizeof(VkPhysicalDevice) * gpu_count);
        if (!physical_devices) CR_LOG_ERR("auto", "bad alloc");
        err = vkEnumeratePhysicalDevices(pInner->inst, &gpu_count, physical_devices);
        if (err) CR_LOG_ERR("auto", "failed to enumerate gpu(s)");
        pInner->gpu = physical_devices[0];
        for (int i = 0; i < gpu_count; i++)
        {
            VkPhysicalDeviceProperties prop;
            vkGetPhysicalDeviceProperties(physical_devices[i], &prop);
            CRPrint(CR_TC_GREEN, "显卡%d型号：%s", i, prop.deviceName);
            if (i == 0) CRPrint(CR_TC_GREEN, "（已选取）\n");
            else CRPrint(CR_TC_GREEN, "\n");
        }
        //
        CRAlloc(physical_devices, 0);
    }
    else CR_LOG_ERR("auto", "reports zero accessible devices!");
    //
    validation_found = 0;
    pInner->enabled_layer_count = 0;
    CRUINT32 device_layer_count = 0;
    err = vkEnumerateDeviceLayerProperties(pInner->gpu, &device_layer_count, NULL);
    if (err) CR_LOG_ERR("auto", "failed to enumerate device layer properties");
    if (device_layer_count)
    {
        VkLayerProperties *device_layers = CRAlloc(NULL, sizeof(VkLayerProperties) * device_layer_count);
        err = vkEnumerateDeviceLayerProperties(pInner->gpu, &device_layer_count, device_layers);
        if (err) CR_LOG_ERR("auto", "failed to enumerate device layer properties");
        if (pInner->validate)
        {
            validation_found = _inner_check_layers_(
                device_validation_layer_count,
                pInner->device_validation_layers,
                device_layer_count,
                device_layers
            );
            pInner->enabled_layer_count = device_validation_layer_count;
        }
        CRAlloc(device_layers, 0);
    }
    if (pInner->validate && !validation_found) CR_LOG_ERR("auto", "failed to find a required validation layer!");
    //
    CRUINT32 device_extension_count = 0;
    CRBOOL swapchainExtFound = 0;
    pInner->enabled_extension_count = 0;
    memset(extension_names, 0, sizeof(extension_names));
    err = vkEnumerateDeviceExtensionProperties(pInner->gpu, NULL, &device_extension_count, NULL);
    if (err) CR_LOG_ERR("auto", "failed to enumerate device extension properties");
    if (device_extension_count)
    {
        VkExtensionProperties *device_extensions = CRAlloc(NULL, sizeof(VkExtensionProperties) * device_extension_count);
        err = vkEnumerateDeviceExtensionProperties(pInner->gpu, NULL, &device_extension_count, device_extensions);
        if (err) CR_LOG_ERR("auto", "failed to enumerate device extension properties");
        for (CRUINT32 i = 0; i < device_extension_count; i++) {
            if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, device_extensions[i].extensionName))
            {
                swapchainExtFound = 1;
                pInner->extension_names[pInner->enabled_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
            }
        }
        CRAlloc(device_extensions, 0);
    }
    if (!swapchainExtFound) CR_LOG_ERR("auto", "failed to find the %s extension!", VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    if (pInner->validate)
    {
        pInner->CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(pInner->inst, "vkCreateDebugReportCallbackEXT");
        pInner->DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(pInner->inst, "vkDestroyDebugReportCallbackEXT");
        if (!pInner->CreateDebugReportCallback)
            CR_LOG_ERR("auto", "Unable to find vkCreateDebugReportCallbackEXT");
        if (!pInner->DestroyDebugReportCallback)
            CR_LOG_ERR("auto", "Unable to find vkDestroyDebugReportCallbackEXT");
        pInner->DebugReportMessage = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(pInner->inst, "vkDebugReportMessageEXT");
        if (!pInner->DebugReportMessage)
            CR_LOG_ERR("auto", "Unable to find vkDebugReportMessageEXT");
             PFN_vkDebugReportCallbackEXT callback;
        if (!pInner->use_break)
        {
            callback = dbgFunc;
        }
        else
        {
            callback = dbgFunc;
            // TODO add a break callback defined locally since there is no
            // longer
            // one included in the loader
        }
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.pNext = NULL;
        dbgCreateInfo.pfnCallback = callback;
        dbgCreateInfo.pUserData = NULL;
        dbgCreateInfo.flags =
            VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        err = pInner->CreateDebugReportCallback(pInner->inst, &dbgCreateInfo, NULL, &pInner->msg_callback);
        switch (err)
        {
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            CR_LOG_ERR("auto", "CreateDebugReportCallback: out of host memory");
            break;
        default:
            CR_LOG_ERR("auto", "CreateDebugReportCallback: unknown failure");
            break;
        }
    }
    //
    vkGetPhysicalDeviceProperties(pInner->gpu, &pInner->gpu_props);
    vkGetPhysicalDeviceQueueFamilyProperties(pInner->gpu, &pInner->queue_count, NULL);
    if (!pInner->queue_count) CR_LOG_ERR("auto", "failed to get queue family properties!");
    pInner->queue_props = (VkQueueFamilyProperties*)CRAlloc(NULL, sizeof(VkQueueFamilyProperties) * pInner->queue_count);
    if (!pInner->queue_props) CR_LOG_ERR("auto", "bad alloc");
    vkGetPhysicalDeviceQueueFamilyProperties(pInner->gpu, &pInner->queue_count, pInner->queue_props);
    CRUINT32 gfx_queue_index = 0;
    for (;gfx_queue_index < pInner->queue_count; gfx_queue_index++)
        if(pInner->queue_props[gfx_queue_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) goto Succeed;
Failed:
    CR_LOG_ERR("auto", "graphic queue doesn't support");
Succeed:
    VkPhysicalDeviceFeatures physFeatures;
    vkGetPhysicalDeviceFeatures(pInner->gpu, &physFeatures);
    GET_INSTANCE_PROC_ADDR(pInner->inst, GetPhysicalDeviceSurfaceSupportKHR);
    GET_INSTANCE_PROC_ADDR(pInner->inst, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    GET_INSTANCE_PROC_ADDR(pInner->inst, GetPhysicalDeviceSurfaceFormatsKHR);
    GET_INSTANCE_PROC_ADDR(pInner->inst, GetPhysicalDeviceSurfacePresentModesKHR);
    GET_INSTANCE_PROC_ADDR(pInner->inst, GetSwapchainImagesKHR);
    //
    #ifdef CR_WINDOWS
    _inner_init_swapchain_(pInner, window);
    #elif defined CR_LINUX
    _inner_init_swapchain_(pInner, dpy, win);
    #endif
    //
    _inner_create_pipeline_(pInner);
    //
    return pInner;
}

void _inner_destroy_vk_(cr_vk vk)
{
    cr_vk_inner *pInner = (cr_vk_inner*)vk;
    //
    _inner_destroy_pipeline_(pInner);
    //
    _inner_uninit_swapchain_(pInner);
    //
    CRAlloc(pInner->queue_props, 0);
    vkDestroyInstance(pInner->inst, NULL);
    //
    CRAlloc(pInner, 0);
}