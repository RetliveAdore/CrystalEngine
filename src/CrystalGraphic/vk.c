/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-08-18 21:36:50
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-09-08 23:21:29
 * @FilePath: \CrystalEngine\src\CrystalGraphic\vk.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include "vk.h"
#include <resources.h>

//虚拟机中不支持调试层扩展
#define NDEBUG
#ifdef NDEBUG
const int enableValidationLayers = 0;
#else
const int enableValidationLayers = 1;
#endif
const char* validation_layer_name[] = {"VK_LAYER_KHRONOS_validation"};

/**
 * vulkan可以多帧并行渲染，
 * 此处定义多可以多少帧并行渲染
 */
#define MAX_FRAMES_IN_FLIGHT 2

static VKAPI_ATTR VkBool32 VKAPI_CALL _inner_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
)
{
    CR_LOG_WAR("auto", "validation layer: %s", pCallbackData->pMessage);
    return VK_FALSE;
}

const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//单例，每个进程只需要初始化一次
static struct vk_init
{
    VkInstance instance;
    VkPhysicalDevice physical_device;
    CRUINT32 queue_family_count;
    CRUINT32 graphics_family_index;
    VkDebugUtilsMessengerEXT debugMessenger;
}CR_VKINIT;

//实例，每个进程可以创建多个
typedef struct vk_struct
{
    VkSurfaceKHR surface;
    VkDevice device;
    CRUINT32 present_family_index;
    struct{
        VkSwapchainKHR swapchain;
        CRUINT32 image_count;
        VkFormat image_format;
        VkColorSpaceKHR color_space;
        VkImageUsageFlags image_usage_flags;
        VkExtent2D extent;
        CRUINT32 requested_min_image_count;
        VkPresentModeKHR present_mode;
        VkImage *swapchain_images;
        VkImageView *swapchain_image_views;
    }swapchain;
    struct{
        VkQueue graphics_queue;
        VkQueue present_queue;
        VkFramebuffer *framebuffers;
        VkRenderPass render_pass;
        VkPipelineLayout pipeline_layout;
        VkPipeline graphics_pipeline;
        VkCommandPool command_pool;
        VkCommandBuffer *command_buffers;
        VkSemaphore *available_semaphores;
        VkSemaphore *finished_semaphores;
        VkFence *in_flight_fences;
        VkFence *image_in_flight;
        size_t current_frame;
    }data;
    #ifdef CR_WINDOWS
    HWND hWnd;
    #elif defined CR_LINUX
    Display *pDisplay;
    Window window;
    #endif
}CR_VKSTRUCT, *PCR_VKSTRUCT;

#ifdef CR_WINDOWS
static CRUINT32 extensionCountB = 2;
static const char* extensionsB[2] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
};
#elif defined CR_LINUX
static CRUINT32 extensionCountB = 2;
static const char* extensionsB[2] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME
};
#endif
static const char** extensions = NULL;
static uint32_t extensionCount = 0;

static void _inner_device_initialization_()
{
    VkApplicationInfo appInfo = {0};
    appInfo.sType =VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "CrystalVulkan";
    appInfo.pEngineName = "Crystal Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;
    //
    if (enableValidationLayers) {
        extensionCount = extensionCountB + 1;
        extensions = (const char**)CRAlloc(NULL, sizeof(const char*) * extensionCount);
        if (!extensions) CR_LOG_ERR("auto", "bad alloc");
        for (int i = 0; i < extensionCountB; i++) {
            extensions[i] = extensionsB[i];
        }
        extensions[extensionCount - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    } else {
        extensionCount = extensionCountB;
        extensions = extensionsB;
    }

    VkInstanceCreateInfo instanceCreateInfo = {0};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = extensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = extensions;
    if (enableValidationLayers)
    {
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = _inner_debug_callback;

        instanceCreateInfo.enabledLayerCount = 1;
        instanceCreateInfo.ppEnabledLayerNames = validation_layer_name;
        instanceCreateInfo.pNext = &debug_create_info;
    }
    else
    {
        instanceCreateInfo.enabledLayerCount = 0;
    }
    if (vkCreateInstance(&instanceCreateInfo, NULL, &(CR_VKINIT.instance)) != VK_SUCCESS)
        CR_LOG_ERR("auto", "failed to create instance!");
    // setup debug messenger
    if (enableValidationLayers) {
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = _inner_debug_callback;

        PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(CR_VKINIT.instance, "vkCreateDebugUtilsMessengerEXT");
        if (func == NULL)
            CR_LOG_WAR("auto", "vkCreateDebugUtilsMessengerEXT not found!");
        else if (func(CR_VKINIT.instance, &debug_create_info, NULL, &(CR_VKINIT.debugMessenger)) != VK_SUCCESS)
            CR_LOG_WAR("auto", "failed to set up debug messenger!");
    }
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
    if (!pDevice) CR_LOG_ERR("auto", "bad alloc");
    vkEnumeratePhysicalDevices(CR_VKINIT.instance, &deviceCount, pDevice);
    CR_VKINIT.physical_device = pDevice[0];
    VkPhysicalDeviceProperties prop;
    vkGetPhysicalDeviceProperties(CR_VKINIT.physical_device, &prop);
    CRUINT32 extensionCounts = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCounts, NULL);
    CR_LOG_IFO("auto", "Default graphic card: %s, %d extensions supported", prop.deviceName, extensionCounts);
    //检查是否支持图形命令队列
    CR_VKINIT.queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(CR_VKINIT.physical_device, &(CR_VKINIT.queue_family_count), NULL);
    VkQueueFamilyProperties *pQueueProp = CRAlloc(NULL, sizeof(VkQueueFamilyProperties) * CR_VKINIT.queue_family_count);
    if (!pQueueProp) CR_LOG_ERR("auto", "bad alloc");
    vkGetPhysicalDeviceQueueFamilyProperties(CR_VKINIT.physical_device, &(CR_VKINIT.queue_family_count), pQueueProp);
    for (int i = 0; i < CR_VKINIT.queue_family_count; i++)
    {
        if (pQueueProp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            CR_VKINIT.graphics_family_index = i;
            goto CRSuccess;
        } 
    }
Failed:
    CR_LOG_WAR("auto", "failed to found graphics family index");
CRSuccess:
    CRAlloc(pQueueProp, 0);
    CRAlloc(pDevice, 0);
}

void _inner_uninit_vk_()
{
    if (enableValidationLayers)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(CR_VKINIT.instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != NULL) func(CR_VKINIT.instance, CR_VKINIT.debugMessenger, NULL);
    }
    vkDestroyInstance(CR_VKINIT.instance, NULL);
    CR_VKINIT.instance = VK_NULL_HANDLE;
    if (enableValidationLayers)
        CRAlloc(extensions, 0);
}

//
//
//////////
//

static void _inner_init_data_(PCR_VKSTRUCT pInner, CRUINT32 w, CRUINT32 h)
{
    pInner->swapchain.swapchain = VK_NULL_HANDLE;
    pInner->swapchain.image_count = 0;
    pInner->swapchain.image_format = VK_FORMAT_B8G8R8A8_SRGB;
    pInner->swapchain.color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    pInner->swapchain.image_usage_flags = 0;
    pInner->swapchain.extent.width = w;
    pInner->swapchain.extent.height = h;
    pInner->swapchain.requested_min_image_count = 0;
    pInner->swapchain.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    //
    pInner->data.current_frame = 0;
}

static void _inner_create_logical_device_(PCR_VKSTRUCT pInner)
{
    float queuePriority = 1.0f;
    VkPhysicalDeviceFeatures device_features = {0};
    VkDeviceCreateInfo device_create_info = {0};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    VkDeviceQueueCreateInfo queue_create_infos[2] = {
        {0},
        {0},
    };
    queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[0].queueFamilyIndex = CR_VKINIT.graphics_family_index;
    queue_create_infos[0].queueCount = 1;
    queue_create_infos[0].pQueuePriorities = &queuePriority;
    queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[1].queueFamilyIndex = pInner->present_family_index;
    queue_create_infos[1].queueCount = 1;
    queue_create_infos[1].pQueuePriorities = &queuePriority;
    //
    if (CR_VKINIT.graphics_family_index != pInner->present_family_index)
        device_create_info.queueCreateInfoCount = 2;
    else
        device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = queue_create_infos;
    device_create_info.enabledExtensionCount = 1;
    device_create_info.ppEnabledExtensionNames = device_extensions;
    if (enableValidationLayers)
    {
        device_create_info.enabledLayerCount = 1;
        device_create_info.ppEnabledLayerNames = validation_layer_name;
    }
    else
    {
        device_create_info.enabledLayerCount = 0;
    }
    device_create_info.pEnabledFeatures = &device_features;
    if (vkCreateDevice(CR_VKINIT.physical_device, &device_create_info, NULL, &(pInner->device)) != VK_SUCCESS)
        CR_LOG_WAR("auto", "failed to create logical device!");
}

static void _inner_create_swapchain_(PCR_VKSTRUCT pInner)
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(CR_VKINIT.physical_device, pInner->surface, &capabilities);
    pInner->swapchain.image_count = capabilities.minImageCount + 1;
    //对图像数量进行校正
    if (capabilities.maxImageCount > 0 &&pInner->swapchain.image_count > capabilities.maxImageCount)
        pInner->swapchain.image_count = capabilities.maxImageCount;
    //
    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = pInner->surface,
        .minImageCount = pInner->swapchain.image_count,
        .imageFormat = pInner->swapchain.image_format,
        .imageColorSpace = pInner->swapchain.color_space,
        .imageExtent = pInner->swapchain.extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = pInner->swapchain.present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };
    CRUINT32 queue_family_indicies[] = { CR_VKINIT.graphics_family_index, pInner->present_family_index};
    swapchain_create_info.pQueueFamilyIndices = queue_family_indicies;
    if (queue_family_indicies[0] != queue_family_indicies[1])
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
    }
    else
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 1;
    }
    if (swapchain_create_info.imageExtent.width < capabilities.minImageExtent.width || swapchain_create_info.imageExtent.height < capabilities.minImageExtent.height)
    {
        swapchain_create_info.imageExtent.width = capabilities.minImageExtent.width;
        swapchain_create_info.imageExtent.height = capabilities.minImageExtent.height;
    }
    else if (swapchain_create_info.imageExtent.width > capabilities.maxImageExtent.width || swapchain_create_info.imageExtent.height > capabilities.maxImageExtent.height)
    {
        swapchain_create_info.imageExtent.width = capabilities.maxImageExtent.width;
        swapchain_create_info.imageExtent.height = capabilities.maxImageExtent.height;
    }
    if (vkCreateSwapchainKHR(pInner->device, &swapchain_create_info, NULL, &(pInner->swapchain.swapchain)) != VK_SUCCESS)
        CR_LOG_WAR("auto", "failed to create swap chain!");

    vkGetSwapchainImagesKHR(pInner->device, pInner->swapchain.swapchain, &(pInner->swapchain.image_count), NULL);
    pInner->swapchain.swapchain_images = (VkImage*)CRAlloc(NULL, sizeof(VkImage) * pInner->swapchain.image_count);
    if (!pInner->swapchain.swapchain_images) CR_LOG_ERR("auto", "bad alloc");
    vkGetSwapchainImagesKHR(pInner->device, pInner->swapchain.swapchain, &(pInner->swapchain.image_count), pInner->swapchain.swapchain_images);
    
    pInner->swapchain.swapchain_image_views = (VkImageView*)CRAlloc(NULL, sizeof(VkImageView) * pInner->swapchain.image_count);
    if (!pInner->swapchain.swapchain_image_views) CR_LOG_ERR("auto", "bad alloc");
    for (CRUINT32 i = 0; i < pInner->swapchain.image_count; i++)
    {
        VkImageViewCreateInfo image_view_create_infos =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = pInner->swapchain.swapchain_images[i],
            .viewType = VK_IMAGE_TYPE_2D,
            .format = pInner->swapchain.image_format,
            //
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            //
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1
        };
        if (vkCreateImageView(pInner->device, &image_view_create_infos, NULL, &(pInner->swapchain.swapchain_image_views[i])) != VK_SUCCESS)
            CR_LOG_WAR("auto", "failed to create image views!");
    }
}

static void _inner_get_queues_(PCR_VKSTRUCT pInner)
{
    vkGetDeviceQueue(pInner->device, CR_VKINIT.graphics_family_index, 0, &(pInner->data.graphics_queue));
    vkGetDeviceQueue(pInner->device, pInner->present_family_index, 0, &(pInner->data.present_queue));
}

//创建图形管线
static void _inner_create_render_pass_(PCR_VKSTRUCT pInner)
{
    VkAttachmentDescription color_attachment = {
        .flags = 0,
        .format = pInner->swapchain.image_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };
    VkAttachmentReference color_attachment_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;
    VkSubpassDependency dependency = {
        .dependencyFlags = 0,
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstSubpass = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };
    VkRenderPassCreateInfo renderpass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .flags = 0,
        .pNext = NULL,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };
    if (vkCreateRenderPass(pInner->device, &renderpass_info, NULL, &(pInner->data.render_pass)) != VK_SUCCESS)
        CR_LOG_WAR("auto", "failed to create render pass!");
}

static VkShaderModule _inner_create_shader_module_(VkDevice device, const uint32_t* code, size_t code_size)
{
    VkShaderModuleCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code_size;
    create_info.pCode = code;
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &create_info, NULL, &shaderModule) != VK_SUCCESS)
    {
        CR_LOG_WAR("auto", "failed to create shader module!");
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

static void _inner_create_pipeline_(PCR_VKSTRUCT pInner)
{
    CRUINT64 size_vert = (CRUINT64)&_binary_out_objs_shader_defaultshader_vert_spv_size;
    CRUINT64 size_frag = (CRUINT64)&_binary_out_objs_shader_defaultshader_frag_spv_size;
    const char* vert_code = &_binary_out_objs_shader_defaultshader_vert_spv_start;
    const char* frag_code = &_binary_out_objs_shader_defaultshader_frag_spv_start;
    VkShaderModule vert_module = _inner_create_shader_module_(pInner->device, (const uint32_t*)vert_code, size_vert);
    VkShaderModule frag_module = _inner_create_shader_module_(pInner->device, (const uint32_t*)frag_code, size_frag);

    VkPipelineShaderStageCreateInfo vert_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_module,
        .pName = "main",
        .pSpecializationInfo = NULL
    };
    VkPipelineShaderStageCreateInfo frag_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_module,
        .pName = "main",
        .pSpecializationInfo = NULL
    };
    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_stage_info, frag_stage_info};
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {  //顶点输入设置
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .vertexAttributeDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .pVertexAttributeDescriptions = NULL
    };
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)pInner->swapchain.extent.width,
        .height = (float)pInner->swapchain.extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    VkRect2D scissor = {  //剪裁：不剪裁，直接输出全部
        .offset.x = 0,
        .offset.y = 0,
        .extent = pInner->swapchain.extent
    };
    VkPipelineViewportStateCreateInfo viewport_stat = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth= 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f
    };
    VkPipelineMultisampleStateCreateInfo multisampling = {  //多重采样，需要抗锯齿时启用
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_TRUE,
        .colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,  //采用透明度混合的算法
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,  //
        .colorBlendOp = VK_BLEND_OP_ADD,  //采用加性混合
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD
    };
    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants[0] = 0.0f,
        .blendConstants[1] = 0.0f,
        .blendConstants[2] = 0.0f,
        .blendConstants[3] = 0.0f
    };
    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0
    };
    if (vkCreatePipelineLayout(pInner->device, &pipeline_layout_info, NULL, &(pInner->data.pipeline_layout)) != VK_SUCCESS)
        CR_LOG_WAR("auto", "failed to create pipeline layout");
    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamic_states
    };
    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stageCount =2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_stat,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_info,
        .layout = pInner->data.pipeline_layout,
        .renderPass = pInner->data.render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE
    };
    if (vkCreateGraphicsPipelines(pInner->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &(pInner->data.graphics_pipeline)) != VK_SUCCESS)
        CR_LOG_WAR("auto", "failed to create graphics pipeline!");
    //
    vkDestroyShaderModule(pInner->device, vert_module, NULL);
    vkDestroyShaderModule(pInner->device, frag_module, NULL);
}

static void _inner_create_frame_bffers_(PCR_VKSTRUCT pInner)
{
    pInner->data.framebuffers = (VkFramebuffer*)CRAlloc(NULL, sizeof(VkFramebuffer) * pInner->swapchain.image_count);
    if (!pInner->data.framebuffers) CR_LOG_ERR("auto", "bad alloc");
    for (CRUINT32 i = 0; i < pInner->swapchain.image_count; i++)
    {
        VkImageView attachments[] = {pInner->swapchain.swapchain_image_views[i]};
        VkFramebufferCreateInfo framebuffer_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .renderPass = pInner->data.render_pass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = pInner->swapchain.extent.width,
            .height = pInner->swapchain.extent.height,
            .layers = 1
        };
        if (vkCreateFramebuffer(pInner->device, &framebuffer_info, NULL, &(pInner->data.framebuffers[i])) != VK_SUCCESS)
            CR_LOG_WAR("auto", "failed to create framebuffer");
    }
}

static void _inner_create_command_pool_(PCR_VKSTRUCT pInner)
{
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueFamilyIndex = 0
    };
    if (vkCreateCommandPool(pInner->device, &pool_info, NULL, &(pInner->data.command_pool)) != VK_SUCCESS)
        CR_LOG_WAR("auto", "failedto create command pool");
}

static void _inner_create_command_buffers_(PCR_VKSTRUCT pInner)
{
    pInner->data.command_buffers = (VkCommandBuffer*)CRAlloc(NULL, sizeof(VkCommandBuffer) * pInner->swapchain.image_count);
    if (!pInner->data.command_buffers) CR_LOG_ERR("auto", "bad alloc");
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext= NULL,
        .commandBufferCount = (uint32_t)pInner->swapchain.image_count,
        .commandPool = pInner->data.command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
    };
    if (vkAllocateCommandBuffers(pInner->device, &allocInfo, pInner->data.command_buffers) != VK_SUCCESS)
        CR_LOG_WAR("auto", "failed to create command buffers");
    for (CRUINT32 i = 0; i < pInner->swapchain.image_count; i++)
    {
        VkCommandBufferBeginInfo begin_info = {0};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(pInner->data.command_buffers[i], &begin_info) != VK_SUCCESS)
            CR_LOG_WAR("auto", "failed to begin recording command buffer!");
        VkClearValue clearColor;
        clearColor.color.float32[0] = 0.5f;
        clearColor.color.float32[1] = 0.3f;
        clearColor.color.float32[2] = 0.7f;
        clearColor.color.float32[3] = 1.0f;
        VkRenderPassBeginInfo render_pass_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = NULL,
            .framebuffer = pInner->data.framebuffers[i],
            .renderPass = pInner->data.render_pass,
            .renderArea.offset.x = 0,
            .renderArea.offset.y = 0,
            .renderArea.extent = pInner->swapchain.extent,
            .clearValueCount = 1,
            .pClearValues = &clearColor
        };
        VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float)pInner->swapchain.extent.width,
            .height = (float)pInner->swapchain.extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
        VkRect2D scissor = {
            .offset.x = 0,
            .offset.y = 0,
            .extent = pInner->swapchain.extent
        };
        //录入指令，之后可以直接执行缓冲中已有的指令
        vkCmdSetViewport(pInner->data.command_buffers[i], 0, 1, &viewport);
        vkCmdSetScissor(pInner->data.command_buffers[i], 0, 1, &scissor);
        //
        vkCmdBeginRenderPass(pInner->data.command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(pInner->data.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pInner->data.graphics_pipeline);
        vkCmdDraw(pInner->data.command_buffers[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(pInner->data.command_buffers[i]);
        //
        if (vkEndCommandBuffer(pInner->data.command_buffers[i]) != VK_SUCCESS)
            CR_LOG_WAR("auto", "failed to record command buffer");
    }
}

static void _inner_create_sync_objects_(PCR_VKSTRUCT pInner)
{
    pInner->data.available_semaphores = (VkSemaphore*)CRAlloc(NULL, sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    pInner->data.finished_semaphores = (VkSemaphore*)CRAlloc(NULL, sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    pInner->data.in_flight_fences = (VkFence*)CRAlloc(NULL, sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
    pInner->data.image_in_flight = (VkFence*)CRAlloc(NULL, sizeof(VkFence) * pInner->swapchain.image_count);
    if (!pInner->data.available_semaphores || !pInner->data.finished_semaphores || !pInner->data.in_flight_fences || !pInner->data.image_in_flight)
        CR_LOG_ERR("auto", "bad alloc");
    VkSemaphoreCreateInfo semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    for (CRUINT32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (
            vkCreateSemaphore(pInner->device, &semaphore_info, NULL, &(pInner->data.available_semaphores[i])) != VK_SUCCESS ||
            vkCreateSemaphore(pInner->device, &semaphore_info, NULL, &(pInner->data.finished_semaphores[i])) != VK_SUCCESS ||
            vkCreateFence(pInner->device, &fence_info, NULL, &(pInner->data.in_flight_fences[i])) != VK_SUCCESS
        )
        CR_LOG_WAR("auto", "failed to create sync objects");
    }
    for (CRUINT32 i = 0; i < pInner->swapchain.image_count; i++)
        pInner->data.image_in_flight[i] = VK_NULL_HANDLE;
}

static void _inner_recreate_swap_chain_(PCR_VKSTRUCT pInner)
{
    vkDeviceWaitIdle(pInner->device);
    //destroy
    vkDestroyCommandPool(pInner->device, pInner->data.command_pool, NULL);
    for (CRUINT32 i = 0; i < pInner->swapchain.image_count; i++)
        vkDestroyFramebuffer(pInner->device, pInner->data.framebuffers[i], NULL);
    for (CRUINT32 i = 0; i < pInner->swapchain.image_count; i++)
        vkDestroyImageView(pInner->device, pInner->swapchain.swapchain_image_views[i], NULL);
    //recreate
    _inner_create_swapchain_(pInner);
    _inner_create_frame_bffers_(pInner);
    _inner_create_command_pool_(pInner);
    _inner_create_command_buffers_(pInner);
}

/**
 * create device -
 * create swapchain -
 * get queues -
 * create render pass -
 * create graphics pipline -
 * create frame buffers -
 * create command pool -
 * create command buffers -
 * create sync objects -
 */
#ifdef CR_WINDOWS
cr_vk _inner_create_vk_(HWND hWnd, CRUINT32 w, CRUINT32 h)
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
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.hinstance = GetModuleHandle(NULL);
    createInfo.hwnd = pInner->hWnd;
    if (vkCreateWin32SurfaceKHR(CR_VKINIT.instance, &createInfo, NULL, &(pInner->surface)) != VK_SUCCESS)
    {
        CR_LOG_WAR("auto", "failed to create surface");
        CRAlloc(pInner, 0);
        return NULL;
    }
    //
    //检查窗口表面支持
    VkBool32 present_support = 0;
    for (int i = 0; i < CR_VKINIT.queue_family_count; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(CR_VKINIT.physical_device, i, pInner->surface, &present_support);
        if (present_support)
        {
            pInner->present_family_index = i;
            goto Success;
        }
    }
Failed:
    CR_LOG_WAR("auto", "failed to found present family index");
Success:
    //创建逻辑设备
    _inner_create_logical_device_(pInner);
    //创建交换链
    _inner_init_data_(pInner, w, h);
    _inner_create_swapchain_(pInner);
    //获取队列
    _inner_get_queues_(pInner);
    //创建图形管线
    _inner_create_render_pass_(pInner);
    _inner_create_pipeline_(pInner);
    //创建帧缓冲
    _inner_create_frame_bffers_(pInner);
    //创建命令池
    _inner_create_command_pool_(pInner);
    _inner_create_command_buffers_(pInner);
    //创建同步实例
    _inner_create_sync_objects_(pInner);
    //
    return pInner;
}
#elif defined CR_LINUX
cr_vk _inner_create_vk_(Display* dpy, Window win, CRUINT32 w, CRUINT32 h)
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
    //
    //检查窗口表面支持
    VkBool32 present_support = 0;
    for (int i = 0; i < CR_VKINIT.queue_family_count; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(CR_VKINIT.physical_device, i, pInner->surface, &present_support);
        if (present_support)
        {
            pInner->present_family_index = i;
            goto CRSuccess;
        }
    }
Failed:
    CR_LOG_WAR("auto", "failed to found present family index");
CRSuccess:
    //创建逻辑设备
    _inner_create_logical_device_(pInner);
    //创建交换链
    _inner_init_data_(pInner, w, h);
    _inner_create_swapchain_(pInner);
    //获取队列
    _inner_get_queues_(pInner);
    //创建图形管线
    _inner_create_render_pass_(pInner);
    _inner_create_pipeline_(pInner);
    //创建命令池
    _inner_create_command_pool_(pInner);
    _inner_create_command_buffers_(pInner);
    //创建同步实例
    _inner_create_sync_objects_(pInner);
    //
    return pInner;
}
#endif

void _inner_release_vk_(cr_vk vk)
{
    PCR_VKSTRUCT pInner = (PCR_VKSTRUCT)vk;
    //释放同步实例
    for (CRUINT32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(pInner->device, pInner->data.finished_semaphores[i], NULL);
        vkDestroySemaphore(pInner->device, pInner->data.available_semaphores[i], NULL);
        vkDestroyFence(pInner->device, pInner->data.in_flight_fences[i], NULL);
    }
    CRAlloc(pInner->data.finished_semaphores, 0);
    CRAlloc(pInner->data.available_semaphores, 0);
    CRAlloc(pInner->data.in_flight_fences, 0);
    CRAlloc(pInner->data.image_in_flight, 0);
    //释放指令缓冲
    vkDestroyCommandPool(pInner->device, pInner->data.command_pool, NULL);
    CRAlloc(pInner->data.command_buffers, 0);
    //释放帧缓冲
    for (CRUINT32 i = 0; i < pInner->swapchain.image_count; i++)
        vkDestroyFramebuffer(pInner->device, pInner->data.framebuffers[i], NULL);
    CRAlloc(pInner->data.framebuffers, 0);
    //释放管线
    vkDestroyPipeline(pInner->device, pInner->data.graphics_pipeline, NULL);
    vkDestroyPipelineLayout(pInner->device, pInner->data.pipeline_layout, NULL);
    vkDestroyRenderPass(pInner->device, pInner->data.render_pass, NULL);
    //释放交换链
    for (int i = 0; i < pInner->swapchain.image_count; i++)
        vkDestroyImageView(pInner->device, pInner->swapchain.swapchain_image_views[i], NULL);
    CRAlloc(pInner->swapchain.swapchain_image_views, 0);
    CRAlloc(pInner->swapchain.swapchain_images, 0);
    vkDestroySwapchainKHR(pInner->device, pInner->swapchain.swapchain, NULL);
    //释放设备
    vkDestroyDevice(pInner->device, NULL);
    vkDestroySurfaceKHR(CR_VKINIT.instance, pInner->surface, NULL);
}

void _inner_paint_ui_(cr_vk vk)
{
    // PCR_VKSTRUCT pInner = (PCR_VKSTRUCT)vk;
    // vkWaitForFences(pInner->device, 1, pInner->data.in_flight_fences[pInner->data.current_frame], VK_TRUE, UINT64_MAX);
    // CRUINT32 image_index = 0;
    // VkResult result = vkAcquireNextImageKHR(pInner->device, pInner->swapchain.swapchain, UINT64_MAX, pInner->data.available_semaphores[pInner->data.current_frame], VK_NULL_HANDLE, &image_index);
    // if (result == VK_ERROR_OUT_OF_DATE_KHR)
    // {
    //     _inner_recreate_swap_chain_(pInner);
    //     return;
    // }
    // else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    // {
    //     CR_LOG_ERR("auto", "failed to acquire swapchain image!");
    //     return;
    // }
    // if (pInner->data.image_in_flight[image_index] != VK_NULL_HANDLE)
    //     vkWaitForFences(pInner->device, 1, &(pInner->data.image_in_flight[image_index]), VK_TRUE, UINT64_MAX);
    // pInner->data.image_in_flight[image_index] = pInner->data.in_flight_fences[pInner->data.current_frame];
    
}