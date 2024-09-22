#include "vk.h"

CRBOOL _inner_memory_type_from_properties_(cr_vk_inner *pInner, CRUINT32 typeBits, VkFlags requirements_mask, uint32_t *typeIndex)
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

void _inner_flush_init_cmd_(cr_vk_inner *pInner)
{
    VkResult err;

    if (pInner->cmd == VK_NULL_HANDLE)
        return;

    err = vkEndCommandBuffer(pInner->cmd);
    if (err) CR_LOG_ERR("auto", "failed to end command buffer");

    const VkCommandBuffer cmd_bufs[] = {pInner->cmd};
    VkFence nullFence = VK_NULL_HANDLE;
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = NULL,
        .pWaitDstStageMask = NULL,
        .commandBufferCount = 1,
        .pCommandBuffers = cmd_bufs,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = NULL
    };

    err = vkQueueSubmit(pInner->queue, 1, &submit_info, nullFence);
    if (err) CR_LOG_ERR("auto", "failed to submmit vulkan queue!");

    err = vkQueueWaitIdle(pInner->queue);
    if (err) CR_LOG_ERR("auto", "wait idle error");

    vkFreeCommandBuffers(pInner->device, pInner->cmd_pool, 1, cmd_bufs);
    pInner->cmd = VK_NULL_HANDLE;
}

CRBOOL _inner_check_layers_(uint32_t check_count, char **check_names, uint32_t layer_count, VkLayerProperties *layers)
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

void _inner_create_device_(cr_vk_inner *pInner)
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
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = pInner->enabled_extension_count,
        .ppEnabledExtensionNames = (const char *const *)pInner->extension_names,
        .pEnabledFeatures = NULL
    };
    err = vkCreateDevice(pInner->gpu, &device, NULL, &pInner->device);
    if (err) CR_LOG_ERR("auto", "failed to create logic device!");
}
void _inner_destroy_device_(cr_vk_inner *pInner)
{
    vkDestroyDevice(pInner->device, NULL);
}

