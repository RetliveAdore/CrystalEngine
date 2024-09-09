/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-08-18 21:36:50
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-09-09 22:18:58
 * @FilePath: \CrystalEngine\src\CrystalGraphic\vk.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include "vk.h"
#include <resources.h>
#include <stdio.h>

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
    CRUINT32 enabled_extensions_count;
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

cr_vk _inner_create_vk_(
    #ifdef CR_WINDOWS
    HWND window,
    #elif defined CR_LINUX
    Display *dpy,
    Window win
    #endif
    CRUINT32 w, CRUINT32 h
)
{
    cr_vk_inner *pInner = CRAlloc(NULL, sizeof(cr_vk_inner));
    if (!pInner) CR_LOG_ERR("auto", "bad alloc");
    memset(pInner, 0, sizeof(*pInner));
    //
    #ifndef CRVK_NO_DBG
    pInner->validate = CRTRUE;
    #else
    pInner->validate = CRFALSE;
    #endif
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
        
    }
    //
    return pInner;
}

void _inner_destroy_vk_(cr_vk vk)
{
    cr_vk_inner *pInner = (cr_vk_inner*)vk;
    //
    CRAlloc(pInner, 0);
}