#include "vk.h"

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
    CRAlloc(pDevices, 0);
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
