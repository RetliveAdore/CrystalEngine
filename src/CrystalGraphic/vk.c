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

//此函数没有创建设备或者申请内存，故不需要释放
static void _inner_draw_build_cmd_(cr_vk_inner *pInner, VkCommandBuffer cmd_buf)
{
    VkCommandBufferInheritanceInfo cmd_buf_hinfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        .pNext = NULL,
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
        .framebuffer = VK_NULL_HANDLE,
        .occlusionQueryEnable = VK_FALSE,
        .queryFlags = 0,
        .pipelineStatistics = 0,
    };
    const VkCommandBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = &cmd_buf_hinfo,
    };
    const VkClearValue clear_values[2] = {
            [0] = {.color.float32 = {0.5f, 0.4f, 0.7f, 1.0f}},
            [1] = {.depthStencil = {1.0f, 0}},
    };
    const VkRenderPassBeginInfo rp_begin = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = pInner->render_pass,
        .framebuffer = pInner->frameBuffers[pInner->current_buffer],
        .renderArea.offset.x = 0,
        .renderArea.offset.y = 0,
        .renderArea.extent.width = pInner->width,
        .renderArea.extent.height = pInner->height,
        .clearValueCount = 2,
        .pClearValues = clear_values,
    };
    VkResult err;
    err = vkBeginCommandBuffer(cmd_buf, &cmd_buf_info);
    if (err) CR_LOG_ERR("auto", "failed to begin command buffer!");
    //
    vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    //
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pInner->pipeline);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pInner->pipeline_layout, 0, 1, &pInner->desc_set, 0, NULL);

    VkViewport viewport;
    memset(&viewport, 0, sizeof(viewport));
    viewport.height = (float)pInner->height;
    viewport.width = (float)pInner->width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    VkRect2D scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent.width = pInner->width;
    scissor.extent.height = pInner->height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    vkCmdDraw(cmd_buf, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmd_buf);

    VkImageMemoryBarrier prePresentBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    prePresentBarrier.image = pInner->buffers[pInner->current_buffer].image;
    VkImageMemoryBarrier *pmemory_barrier = &prePresentBarrier;
    vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, pmemory_barrier);

    err = vkEndCommandBuffer(cmd_buf);
    if (err) CR_LOG_ERR("auto", "failed to end command buffer!");
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
    //
    _inner_prepare_descriptor_layout_(pInner);
    _inner_prepare_render_pass_(pInner);
    _inner_prepare_pipeline_(pInner);
    //
    for (CRUINT32 i = 0; i < pInner->swapchainImageCount; i++)
    {
        err = vkAllocateCommandBuffers(pInner->device, &cmd, &pInner->buffers[i].cmd);
        if (err) CR_LOG_ERR("auto", "failed to allocate command buffers!");
    }
    //创建描述符池，该池中保存有纹理对象之类的描述符
    _inner_prepare_descriptor_pool_(pInner);
    _inner_prepare_framebuffers_(pInner);
    //
    for (CRUINT32 i = 0; i < pInner->swapchainImageCount; i++)
    {
        pInner->current_buffer = i;
        _inner_draw_build_cmd_(pInner, pInner->buffers[i].cmd);
    }
    _inner_flush_init_cmd_(pInner);
    //
    pInner->current_buffer = 0;
}

static void _inner_destroy_pipeline_(cr_vk_inner *pInner)
{
    _inner_deprepare_framebuffers_(pInner);
    _inner_deprepare_descriptor_pool_(pInner);
    //
    for (CRUINT32 i = 0; i < pInner->swapchainImageCount; i++)
        vkFreeCommandBuffers(pInner->device, pInner->cmd_pool, 1, &pInner->buffers[i].cmd);
    //
    _inner_deprepare_pipeline_(pInner);
    _inner_deprepare_render_pass_(pInner);
    _inner_deprepare_descriptor_layout_(pInner);
    //
    _inner_deprepare_textures_(pInner);
    _inner_deprepare_depth_(pInner);
    _inner_deprepare_buffers_(pInner);
    vkDestroyCommandPool(pInner->device, pInner->cmd_pool, NULL);
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
    _inner_create_instance_(pInner);
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

static void _inner_resize_(cr_vk_inner *pInner)
{
    _inner_destroy_pipeline_(pInner);
    _inner_create_pipeline_(pInner);
}

void _inner_destroy_vk_(cr_vk vk)
{
    cr_vk_inner *pInner = (cr_vk_inner*)vk;
    //
    _inner_destroy_pipeline_(pInner);
    //
    _inner_uninit_swapchain_(pInner);
    //
    _inner_destroy_instance_(pInner);
    //
    CRAlloc(pInner, 0);
}

static void _inner_draw_(cr_vk_inner *pInner) 
{
    VkResult err;
    VkSemaphore presentCompleteSemaphore;
    VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };
    VkFence nullFence = VK_NULL_HANDLE;

    err = vkCreateSemaphore(pInner->device, &presentCompleteSemaphoreCreateInfo, NULL, &presentCompleteSemaphore);
    if (err) CR_LOG_ERR("auto", "failed to create semaphore");

    // Get the index of the next available swapchain image:
    err = pInner->fpAcquireNextImageKHR(pInner->device, pInner->swapchain, UINT64_MAX, presentCompleteSemaphore, (VkFence)0, &pInner->current_buffer);
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // pInner->swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        _inner_resize_(pInner);
        _inner_draw_(pInner);
        vkDestroySemaphore(pInner->device, presentCompleteSemaphore, NULL);
        return;
    }
    else if (err == VK_SUBOPTIMAL_KHR)
    {
        // pInner->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    }
    else
        if (err) CR_LOG_ERR("auto", "failed to acquire next image!");

    // Assume the command buffer has been run on current_buffer before so
    // we need to set the image layout back to COLOR_ATTACHMENT_OPTIMAL
    _inner_set_image_layout_(pInner, pInner->buffers[pInner->current_buffer].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    _inner_flush_init_cmd_(pInner);

    // Wait for the present complete semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.

    // FIXME/TODO: DEAL WITH VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &presentCompleteSemaphore,
        .pWaitDstStageMask = &pipe_stage_flags,
        .commandBufferCount = 1,
        .pCommandBuffers = &pInner->buffers[pInner->current_buffer].cmd,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = NULL
    };

    err = vkQueueSubmit(pInner->queue, 1, &submit_info, nullFence);
    if (err) CR_LOG_ERR("auto", "failed to submmit queue");

    VkPresentInfoKHR present = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .swapchainCount = 1,
        .pSwapchains = &pInner->swapchain,
        .pImageIndices = &pInner->current_buffer,
    };

    // TBD/TODO: SHOULD THE "present" PARAMETER BE "const" IN THE HEADER?
    err = pInner->fpQueuePresentKHR(pInner->queue, &present);
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // pInner->swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        _inner_resize_(pInner);
    }
    else if (err == VK_SUBOPTIMAL_KHR)
    {
        // pInner->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    }
    else
        if (err) CR_LOG_ERR("auto", "queue present error");

    err = vkQueueWaitIdle(pInner->queue);

    vkDestroySemaphore(pInner->device, presentCompleteSemaphore, NULL);
}

void _inner_paint_ui_(cr_vk vk)
{
    cr_vk_inner *pInner = (cr_vk_inner*)vk;
    // Wait for work to finish before updating MVP.
    vkDeviceWaitIdle(pInner->device);

    _inner_draw_(pInner);

    // Wait for work to finish before updating MVP.
    vkDeviceWaitIdle(pInner->device);

    pInner->curFrame++;
}