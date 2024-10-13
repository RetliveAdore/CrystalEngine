#include "vk.h"

// #define CR_DEBUG

#ifndef CR_DEBUG
static CRBOOL enableValidationLayers = CRFALSE;
#else
static CRBOOL enableValidationLayers = CRTRUE;
#endif
const char* validation_layer_name[] = {"VK_LAYER_KHRONOS_validation"};
const char* device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

//用于输出错误信息的回调
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
)
{
    CR_LOG_WAR("auto", "validation layer: %s", pCallbackData->pMessage);
    return VK_FALSE;
}

void _inner_create_vk_instance_(cr_vk_inner *pInner)
{
    //
    VkApplicationInfo appInfo = { 0 };
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Crystal Application";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Crystal Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    uint32_t glfwExtensionCount = 2;
    #ifdef CR_WINDOWS
    const char* glfwExtensions[2] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };
    #elif defined CR_LINUX
    const char* glfwExtensions[2] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
    };
    #endif

    if (enableValidationLayers)
    {
        pInner->extension_count = glfwExtensionCount + 1;
        pInner->extensions[pInner->extension_count - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }
    else
    {
        pInner->extension_count = glfwExtensionCount;
    }
    for (int i = 0; i < glfwExtensionCount; i++)
        pInner->extensions[i] = glfwExtensions[i];

    VkInstanceCreateInfo instance_create_info = {0};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &appInfo;
    instance_create_info.enabledExtensionCount = pInner->extension_count;
    instance_create_info.ppEnabledExtensionNames = pInner->extensions;

    if (enableValidationLayers)
    {
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = debugCallback;
        instance_create_info.enabledLayerCount = 1;
        instance_create_info.ppEnabledLayerNames = validation_layer_name;
        instance_create_info.pNext = &debug_create_info;
    }
    else
    {
        instance_create_info.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&instance_create_info, NULL, &pInner->instance) != VK_SUCCESS)
        CR_LOG_ERR("auto", "failed to create instance!");
    else CR_LOG_DBG("auto", "create instance succeed");
}
void _inner_destroy_vk_instance_(cr_vk_inner *pInner)
{
    //
    vkDestroyInstance(pInner->instance, NULL);
}

static CRBOOL _inner_check_gpu_suitable_(CRUINT32 count, VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties prop;
    vkGetPhysicalDeviceProperties(device, &prop);
    VkPhysicalDeviceFeatures feat;
    vkGetPhysicalDeviceFeatures(device, &feat);
    CRPrint(CR_TC_GREEN, "gpu %d: %s\n", count, prop.deviceName);
    return prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && feat.geometryShader;
}
void _inner_search_gpu_(cr_vk_inner *pInner)
{
    CRUINT32 gpu_count = 0;
    vkEnumeratePhysicalDevices(pInner->instance, &gpu_count, NULL);
    if (gpu_count <= 0)
    {
        CR_LOG_ERR("auto", "no gpu found! check your driver!");
        return;
    }
    VkPhysicalDevice *pDevices = CRAlloc(NULL, sizeof(VkPhysicalDevice) * gpu_count);
    if (!pDevices) CR_LOG_ERR("auto", "bad alloc");
    vkEnumeratePhysicalDevices(pInner->instance, &gpu_count, pDevices);
    for (int i = 0; i < gpu_count; i++)
    {
        if (_inner_check_gpu_suitable_(i, pDevices[i]))
        {
            pInner->gpu = pDevices[i];
            break;
        }
    }
    //
    if (pDevices) CRAlloc(pDevices, 0);
}

void _inner_create_logical_device_(cr_vk_inner *pInner)
{
    //
    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = 0;
    queueCreateInfo.queueCount = 1;
    //
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    //
    VkPhysicalDeviceFeatures deviceFeatures = {0};
    VkDeviceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = device_extensions;
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = validation_layer_name;
    }
    else
        createInfo.enabledLayerCount = 0;
    if (vkCreateDevice(pInner->gpu, &createInfo, NULL, &pInner->device) != VK_SUCCESS)
        CR_LOG_ERR("auto", "failed to create logical device");
    else CR_LOG_DBG("auto", "create logical device succeed");
}
void _inner_destroy_logical_device_(cr_vk_inner *pInner)
{
    //
    vkDestroyDevice(pInner->device, NULL);
}

void _inner_create_swapchain_(
    cr_vk_inner *pInner,
#ifdef CR_WINDOWS
    HWND window,
#elif defined CR_LINUX
    Display *dpy,
    Window win,
#endif
    CRUINT32 w, CRUINT32 h
)
{
    //首先创建surface
#ifdef CR_WINDOWS
    VkWin32SurfaceCreateInfoKHR surfaceInfo = {0};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hinstance = GetModuleHandle(NULL);
    surfaceInfo.hwnd = window;
    if (vkCreateWin32SurfaceKHR(pInner->instance, &surfaceInfo, NULL, &pInner->surface) != VK_SUCCESS)
    {
        CR_LOG_ERR("auto", "failed to create win32 surface!");
        return;
    }
#elif defined CR_LINUX
#endif
    CR_LOG_DBG("auto", "create surface succeed");
    //然后枚举显卡队列族特性
    CRUINT32 queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(pInner->gpu, &queue_family_count, NULL);
    VkQueueFamilyProperties *properties = CRAlloc(NULL, sizeof(VkQueueFamilyProperties) * queue_family_count);
    if (!properties) CR_LOG_ERR("auto", "bad alloc");
    vkGetPhysicalDeviceQueueFamilyProperties(pInner->gpu, &queue_family_count, properties);
    for (int i = 0; i < queue_family_count; i++)
    {
        if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            pInner->graphics_queue_node_index = i;
            goto Succeed_1;
        }
    }
    if (properties) CRAlloc(properties, 0);
    CR_LOG_ERR("auto", "failed to find graphics family index!");
    return;
Succeed_1:
    for (int i = 0; i < queue_family_count; i++)
    {
        VkBool32 present_support;
        vkGetPhysicalDeviceSurfaceSupportKHR(pInner->gpu, i, pInner->surface, &present_support);
        if (present_support && i == pInner->graphics_queue_node_index)
            goto Succeed_2;
    }
    CR_LOG_ERR("auto", "failed to find present family index!");
    return;
Succeed_2:
    //
    pInner->buffers = CRAlloc(NULL, sizeof(SwapchainBuffers));
    if (!pInner->buffers)
    {
        if (properties) CRAlloc(properties, 0);
        CR_LOG_ERR("auto", "bad alloc");
        return;
    }
    //
    VkSwapchainCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = pInner->surface;
    //
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pInner->gpu, pInner->surface, &capabilities);
    //
    pInner->format = VK_FORMAT_B8G8R8A8_SRGB;
    createInfo.minImageCount = capabilities.minImageCount;
    createInfo.imageFormat = pInner->format;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = capabilities.currentExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    //
    CRUINT32 queueFamilyIndicies[] = { pInner->graphics_queue_node_index, pInner->graphics_queue_node_index };
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = NULL;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(pInner->device, &createInfo, NULL, &pInner->swapchain) != VK_SUCCESS)
        CR_LOG_ERR("auto", "failed to create swapchain");
    else CR_LOG_DBG("auto", "create swapchain succeed");
    //确定图像视图的数量
    pInner->swapchainImageCount = capabilities.minImageCount + 1;
    if (pInner->swapchainImageCount > capabilities.maxImageCount)
        pInner->swapchainImageCount = capabilities.maxImageCount;
    CR_LOG_DBG("auto", "swapchain image count: %d", pInner->swapchainImageCount);
    //获取图像队列
    vkGetDeviceQueue(pInner->device, pInner->graphics_queue_node_index, 0, &pInner->queue);
    //
    if (properties) CRAlloc(properties, 0);
}
void _inner_destroy_swapchain_(cr_vk_inner *pInner)
{
    //
    vkDestroySwapchainKHR(pInner->device, pInner->swapchain, NULL);
    //
    if (pInner->buffers) CRAlloc(pInner->buffers, 0);
    //
    vkDestroySurfaceKHR(pInner->instance, pInner->surface, NULL);
}

void _inner_create_image_view_(cr_vk_inner *pInner)
{
    //
    pInner->buffers->views = CRAlloc(NULL, sizeof(VkImageView) * pInner->swapchainImageCount);
    if (!pInner->buffers->views)
    {
        CR_LOG_ERR("auto", "bad alloc");
        return;
    }
    pInner->buffers->images = CRAlloc(NULL, sizeof(VkImage) * pInner->swapchainImageCount);
    if (!pInner->buffers->views)
    {
        if (pInner->buffers->views) CRAlloc(pInner->buffers->views, 0);
        CR_LOG_ERR("auto", "bad alloc");
        return;
    }
    vkGetSwapchainImagesKHR(pInner->device, pInner->swapchain, &pInner->swapchainImageCount, pInner->buffers->images);
    for (int i = 0; i < pInner->swapchainImageCount; i++)
    {
        VkImageViewCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = pInner->buffers->images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = pInner->format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(pInner->device, &createInfo, NULL, &pInner->buffers->views[i]) != VK_SUCCESS)
            CR_LOG_ERR("auto", "failed to create image view");
        else CR_LOG_DBG("auto", "create image view %d succeed", i);
    }
}
void _inner_destroy_image_view_(cr_vk_inner *pInner)
{
    //
    for (int i = 0; i < pInner->swapchainImageCount; i++)
        vkDestroyImageView(pInner->device, pInner->buffers->views[i], NULL);
    //
    if (pInner->buffers->images) CRAlloc(pInner->buffers->images, 0);
    if (pInner->buffers->views) CRAlloc(pInner->buffers->views, 0);
}

void _inner_create_render_pass_(cr_vk_inner *pInner)
{
    //
    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format = pInner->format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    //
    VkAttachmentReference colorAttachmentRef = {0};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    //
    VkRenderPassCreateInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    //
    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    if (vkCreateRenderPass(pInner->device, &renderPassInfo, NULL, &pInner->render_pass) != VK_SUCCESS)
        CR_LOG_ERR("auto", "failed to create render pass");
    else CR_LOG_DBG("auto", "create render pass succeed");
}
void _inner_destroy_render_pass_(cr_vk_inner *pInner)
{
    //
    vkDestroyRenderPass(pInner->device, pInner->render_pass, NULL);
}

void _inner_create_frame_buffer_(cr_vk_inner *pInner)
{
    //
    pInner->frameBuffers = CRAlloc(NULL, sizeof(VkFramebuffer) * pInner->swapchainImageCount);
    if (!pInner->frameBuffers)
    {
        CR_LOG_ERR("auto", "bad alloc");
        return;
    }
    for (int i = 0; i < pInner->swapchainImageCount; i++)
    {
        VkImageView attachments[] = {
            pInner->buffers->views[i]
        };
 
        VkFramebufferCreateInfo framebufferInfo = {0};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = pInner->render_pass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = pInner->width;
        framebufferInfo.height = pInner->height;
        framebufferInfo.layers = 1;
 
        if (vkCreateFramebuffer(pInner->device, &framebufferInfo, NULL, &pInner->frameBuffers[i]) != VK_SUCCESS)
            CR_LOG_ERR("auto", "failed to create framebuffer");
        else CR_LOG_DBG("auto", "create framebuffer %d succeed", i);
    }
}
void _inner_destroy_frame_buffer_(cr_vk_inner *pInner)
{
    //
    for (int i = 0; i < pInner->swapchainImageCount; i++)
        vkDestroyFramebuffer(pInner->device, pInner->frameBuffers[i], NULL);
    if (pInner->frameBuffers) CRAlloc(pInner->frameBuffers, 0);
}

void _inner_create_command_pool_buffer_(cr_vk_inner *pInner)
{
    //先创建命令池
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = 0;
    if (vkCreateCommandPool(pInner->device, &poolInfo, NULL, &pInner->cmd_pool) != VK_SUCCESS)
    {
        CR_LOG_ERR("auto", "failed to create command pool!");
        return;
    }
    else CR_LOG_DBG("auto", "create command pool succeed");
    //再创建命令缓冲
    pInner->cmd_buffer = CRAlloc(NULL, sizeof(VkCommandBuffer) * pInner->swapchainImageCount);
    if (!pInner->cmd_buffer)
    {
        CR_LOG_ERR("auto", "bad alloc");
        return;
    }
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pInner->cmd_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = pInner->swapchainImageCount;
    if (vkAllocateCommandBuffers(pInner->device, &allocInfo, pInner->cmd_buffer) != VK_SUCCESS)
    {   //申请的空间再command pool销毁时会自动释放，故无需编写清理相关逻辑
        CR_LOG_ERR("auto", "failed to allocate command buffer!");
        return;
    }
    else CR_LOG_DBG("auto", "allocate command buffer succeed");
    //申请完毕后可以开始填充命令了
    for (int i = 0; i < pInner->swapchainImageCount; i++)
    {
        VkCommandBufferBeginInfo bgInfo = {0};
        bgInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(pInner->cmd_buffer[i], &bgInfo) != VK_SUCCESS)
        {
            CR_LOG_ERR("auto", "failed to begin command buffer %d", i);
            return;
        }
        VkClearValue clearColor = {0.3f, 0.7f, 0.2f, 1.0f};
        VkRenderPassBeginInfo renderPassInfo = {0};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = pInner->render_pass;
        renderPassInfo.framebuffer = pInner->frameBuffers[i];
        renderPassInfo.renderArea.offset.x = 0;
        renderPassInfo.renderArea.offset.y = 0;
        renderPassInfo.renderArea.extent.width = pInner->width;
        renderPassInfo.renderArea.extent.height = pInner->height;
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        vkCmdBeginRenderPass(pInner->cmd_buffer[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(pInner->cmd_buffer[i]);

        if (vkEndCommandBuffer(pInner->cmd_buffer[i]) != VK_SUCCESS)
        {
            CR_LOG_ERR("auto", "failed to end command buffer %d", i);
            return;
        }
    }
}
void _inner_destroy_command_pool_buffer_(cr_vk_inner *pInner)
{
    //
    if (pInner->cmd_buffer) CRAlloc(pInner->cmd_buffer, 0);
    //
    vkDestroyCommandPool(pInner->device, pInner->cmd_pool, NULL);
}

void _inner_create_sync_object_(cr_vk_inner* pInner)
{
    //
    pInner->available_semaphores = CRAlloc(NULL, sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    pInner->finished_semaphores = CRAlloc(NULL, sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    pInner->in_flight_fences = CRAlloc(NULL, sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
    pInner->image_in_flight = CRAlloc(NULL, sizeof(VkFence) * pInner->swapchainImageCount);
    if (!(pInner->available_semaphores && pInner->finished_semaphores && pInner->in_flight_fences && pInner->image_in_flight))
    {
        CR_LOG_ERR("auto", "bad alloc");
        if (pInner->available_semaphores) CRAlloc(pInner->available_semaphores, 0);
        if (pInner->finished_semaphores) CRAlloc(pInner->finished_semaphores, 0);
        if (pInner->in_flight_fences) CRAlloc(pInner->in_flight_fences, 0);
        if (pInner->image_in_flight) CRAlloc(pInner->image_in_flight, 0);
        return;
    }
    VkSemaphoreCreateInfo semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    //
    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    //
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (
            vkCreateSemaphore(pInner->device, &semaphore_info, NULL, &pInner->available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(pInner->device, &semaphore_info, NULL, &pInner->finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(pInner->device, &fence_info, NULL, &pInner->in_flight_fences[i]) != VK_SUCCESS
        )
        {
            CR_LOG_ERR("auto", "failed to create sync objects, index: %d", i);
            return;
        }
    }
    for (int i = 0; i < pInner->swapchainImageCount; i++)
        pInner->image_in_flight[i] = VK_NULL_HANDLE;
    CR_LOG_DBG("auto", "create sync objects succeed");
}
void _inner_destroy_sync_object_(cr_vk_inner* pInner)
{
    //
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(pInner->device, pInner->available_semaphores[i], NULL);
        vkDestroySemaphore(pInner->device, pInner->finished_semaphores[i], NULL);
        vkDestroyFence(pInner->device, pInner->in_flight_fences[i], NULL);
    }
}

void _crvk_draw_frame_(cr_vk vk)
{
    cr_vk_inner* pInner = (cr_vk_inner*)vk;
    if (!pInner)
    {
        CR_LOG_ERR("auto", "nullptr error");
        return;
    }
    vkWaitForFences(pInner->device, 1, &pInner->in_flight_fences[pInner->curFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(pInner->device, 1, &pInner->in_flight_fences[pInner->curFrame]);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(pInner->device, pInner->swapchain, UINT64_MAX, pInner->available_semaphores[pInner->curFrame], VK_NULL_HANDLE, &imageIndex);

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { pInner->available_semaphores[pInner->curFrame] };
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &pInner->cmd_buffer[imageIndex];

    VkSemaphore signalSemaphores[] = { pInner->finished_semaphores[pInner->curFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
 
    if (vkQueueSubmit(pInner->queue, 1, &submitInfo, pInner->in_flight_fences[pInner->curFrame]) != VK_SUCCESS)
    {
        CR_LOG_ERR("auto", "failed to subbmit queue");
        return;
    }

    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { pInner->swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    //
    vkQueuePresentKHR(pInner->queue, &presentInfo);
    pInner->curFrame = (pInner->curFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}