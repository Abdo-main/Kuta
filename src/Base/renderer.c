#include <vulkan/vulkan_core.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>


#include "main.h"
#include "models.h"
#include "textures.h"
#include "vertex_data.h"
#include "utils.h"
#include "descriptors.h"

void create_graphics_pipeline(VkCore *vk_core, SwapchainData *swp_ch, Renderer *renderer){
    size_t vert_size;
    const uint32_t *vert_shader_src = read_file("./shaders/vert.spv", &vert_size);
    EXPECT(!vert_shader_src, "emtpy sprv file");

    size_t frag_size;
    const uint32_t *frag_shader_src = read_file("./shaders/frag.spv", &frag_size);
    EXPECT(!frag_shader_src, "emtpy sprv file");

    VkShaderModule vertex_shader_module, fragment_shader_module;
    
    EXPECT(vkCreateShaderModule(vk_core->device, &(VkShaderModuleCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pCode = vert_shader_src,
        .codeSize = vert_size,
    }, vk_core->allocator, &vertex_shader_module), "Failed to create shader modules")

    EXPECT(vkCreateShaderModule(vk_core->device, &(VkShaderModuleCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pCode = frag_shader_src,
        .codeSize = frag_size,
    }, vk_core->allocator, &fragment_shader_module), "Failed to create shader modules")


    VkPipelineShaderStageCreateInfo shader_stages[] = {
        (VkPipelineShaderStageCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,   
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader_module,
            .pName = "main",
        },
        (VkPipelineShaderStageCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,   
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_shader_module,
            .pName = "main",           
        },
    };

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    
    VkViewport viewports[] = {
        { 
            .width = swp_ch->extent.width,
            .height = swp_ch->extent.height,
            .maxDepth = 1.0f,
        }
    };

    VkRect2D scissors[] = {
        {
            .extent = swp_ch->extent
        }
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment_states[] = {
        {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable = VK_FALSE, // Add this
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Add proper blending
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
        }
    };

    EXPECT(vkCreatePipelineLayout(vk_core->device, &(VkPipelineLayoutCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &renderer->descriptor_set_layout,
    }, vk_core->allocator, &renderer->pipeline_layout), "Faailed to create pipeline layout")

    VkVertexInputBindingDescription binding_description = get_binding_description();
    AttributeDescriptions attribute_descriptions = get_attribute_descriptions();

    EXPECT(vkCreateGraphicsPipelines(vk_core->device, NULL, 1, &(VkGraphicsPipelineCreateInfo){
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pStages = shader_stages,
        .stageCount = sizeof(shader_stages)/sizeof(*shader_stages),
        .pDynamicState = &(VkPipelineDynamicStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = sizeof(dynamic_states)/sizeof(*dynamic_states),
            .pDynamicStates = dynamic_states,
        },
        .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &binding_description,
            .vertexAttributeDescriptionCount = attribute_descriptions.count,
            .pVertexAttributeDescriptions = attribute_descriptions.items,
        },
        .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        },
        .pViewportState = &(VkPipelineViewportStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = sizeof(viewports)/sizeof(*viewports),
            .pViewports = viewports,
            .scissorCount = sizeof(scissors)/sizeof(*scissors),
            .pScissors = scissors,
        },
        .pDepthStencilState = &(VkPipelineDepthStencilStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE, 
        },
        .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .lineWidth = 1.0,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .polygonMode = VK_POLYGON_MODE_FILL,
        },
        .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        },
        .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = sizeof(color_blend_attachment_states)/sizeof(*color_blend_attachment_states),
            .pAttachments = color_blend_attachment_states,
        },
        .layout = renderer->pipeline_layout,
        .renderPass = renderer->render_pass,
    }, vk_core->allocator, &renderer->graphics_pipeline), "Failed to Create Graphics Pipeline")

    vkDestroyShaderModule(vk_core->device, vertex_shader_module, vk_core->allocator);
    vkDestroyShaderModule(vk_core->device, fragment_shader_module, vk_core->allocator);

    free((void*)vert_shader_src);
    free((void*)frag_shader_src);
}

void destroy_graphics_pipeline(VkCore *vk_core, Renderer *renderer){
    vkDestroyPipeline(vk_core->device, renderer->graphics_pipeline, vk_core->allocator);
    vkDestroyPipelineLayout(vk_core->device, renderer->pipeline_layout, vk_core->allocator);
}

void create_render_pass(VkCore *vk_core, SwapchainData *swp_ch, Renderer *renderer){
    VkFormat image_format = swp_ch->image_format;

    VkAttachmentReference color_attachment_refrences[] = {
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        }
    };

    VkAttachmentDescription depth_attachment = {
        .format = find_depth_format(vk_core),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depth_attachment_ref = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };


    VkAttachmentDescription attachment_descriptions[2] = {
        /* color */
        {
            .format = image_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        },
        /* depth */
        {
            .format = find_depth_format(vk_core),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        } 
    };
    VkSubpassDescription subpass_descriptions[] = {
        {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = color_attachment_refrences,
            .pDepthStencilAttachment = &depth_attachment_ref,
        }
    };

    VkSubpassDependency dependency = {
        /* common robust setup from the tutorial */
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    EXPECT(vkCreateRenderPass(vk_core->device, &(VkRenderPassCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .subpassCount = 1,
        .pSubpasses = subpass_descriptions,
        .attachmentCount = 2,
        .pAttachments = attachment_descriptions,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    }, vk_core->allocator, &renderer->render_pass), "Failed to create a render pass")
}

void destroy_render_pass(VkCore *vk_core, Renderer *renderer){
    vkDestroyRenderPass(vk_core->device, renderer->render_pass, vk_core->allocator);
}

void create_frame_buffers(VkCore *vk_core, SwapchainData *swp_ch, TextureData *texture_data, Renderer *renderer) {
    uint32_t frame_buffer_count = swp_ch->image_count;
    renderer->frame_buffers = malloc(frame_buffer_count * sizeof(VkFramebuffer));
    EXPECT(renderer->frame_buffers == NULL, "Couldn't allocate memory for framebuffers array")
    VkExtent2D frame_buffers_extent = swp_ch->extent;

    

    for (int framebufferIndex = 0; framebufferIndex < frame_buffer_count; ++framebufferIndex) {
        VkImageView attachments[2] = {
            swp_ch->image_views[framebufferIndex],
            texture_data->depth_image_view,
        };
        EXPECT(vkCreateFramebuffer(vk_core->device, &(VkFramebufferCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .layers = 1,
            .renderPass = renderer->render_pass,
            .width = frame_buffers_extent.width,
            .height = frame_buffers_extent.height,
            .attachmentCount = sizeof(attachments)/sizeof(attachments[0]),
            .pAttachments = attachments,
        }, vk_core->allocator, &renderer->frame_buffers[framebufferIndex]), "Couldn't create framebuffer %i", framebufferIndex)
    }
}

void destroy_frame_buffers(VkCore *vk_core, SwapchainData *swp_ch, Renderer *renderer) {
    uint32_t framebuffer_count = swp_ch->image_count;

    for (int framebuffer_index = 0; framebuffer_index < framebuffer_count; ++framebuffer_index) {
        vkDestroyFramebuffer(vk_core->device, renderer->frame_buffers[framebuffer_index], vk_core->allocator);
    }

    free(renderer->frame_buffers);
}

void create_command_pool(VkCore *vk_core, Renderer *renderer) {
    EXPECT(vkCreateCommandPool(vk_core->device, &(VkCommandPoolCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = vk_core->graphics_queue_family,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    }, vk_core->allocator, &renderer->command_pool), "Failed to create command pool")
}

void destroy_coommand_pool(VkCore *vk_core, Renderer *renderer) {
    vkDestroyCommandPool(vk_core->device, renderer->command_pool, vk_core->allocator);
}



void allocate_command_buffer(VkCore *vk_core, SwapchainData *swp_ch, Renderer *renderer) {
    uint32_t count = swp_ch->image_count;
    renderer->command_buffers = malloc(count * sizeof(VkCommandBuffer));
    EXPECT(!renderer->command_buffers, "Failed to allocate command buffers array");

    EXPECT(vkAllocateCommandBuffers(vk_core->device, &(VkCommandBufferAllocateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = renderer->command_pool,
        .commandBufferCount = count,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    }, renderer->command_buffers), "Failed to allocate command buffers");
}

void create_sync_objects(VkCore *vk_core, SwapchainData *swp_ch, Renderer *renderer) {
    uint32_t image_count = swp_ch->image_count; // or renderer->image_count

    renderer->acquired_image_semaphore = malloc(image_count * sizeof(VkSemaphore));
    renderer->finished_render_semaphore = malloc(image_count * sizeof(VkSemaphore));
    renderer->in_flight_fence = malloc(image_count * sizeof(VkFence));

    for (uint32_t i = 0; i < image_count; ++i) {
        EXPECT(
            vkCreateSemaphore(vk_core->device, &(VkSemaphoreCreateInfo){ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }, 
                              vk_core->allocator, &renderer->acquired_image_semaphore[i]),
            "Couldn't create acquired image semaphore %u", i
        );
        EXPECT(
            vkCreateSemaphore(vk_core->device, &(VkSemaphoreCreateInfo){ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }, 
                              vk_core->allocator, &renderer->finished_render_semaphore[i]),
            "Couldn't create finished render semaphore %u", i
        );
        EXPECT(
            vkCreateFence(vk_core->device, &(VkFenceCreateInfo){ 
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, 
                .flags = VK_FENCE_CREATE_SIGNALED_BIT 
            }, 
            vk_core->allocator, &renderer->in_flight_fence[i]),
            "Couldn't create in-flight fence %u", i
        );
    }
}


void destroy_sync_objects(VkCore *vk_core, SwapchainData *swp_ch, Renderer *renderer) {

    for (uint32_t i = 0; i < swp_ch->image_count; ++i) {
        vkDestroyFence(vk_core->device, renderer->in_flight_fence[i], vk_core->allocator);
        vkDestroySemaphore(vk_core->device, renderer->acquired_image_semaphore[i], vk_core->allocator);
        vkDestroySemaphore(vk_core->device, renderer->finished_render_semaphore[i], vk_core->allocator);
    }
    free(renderer->in_flight_fence);
    free(renderer->acquired_image_semaphore);
    free(renderer->finished_render_semaphore);
}

void record_command_buffer(SwapchainData *swp_ch, BufferData *buffer_data, Config *config, Renderer *renderer, GeometryData *geometry_data) {
    VkCommandBuffer command_buffer = renderer->command_buffers[renderer->current_frame];
    
    // Reset the command buffer
    vkResetCommandBuffer(command_buffer, 0);


    EXPECT(vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    }), "Couldn't begin command buffer for frame");

    VkClearValue clear_values[2] = { 
        {
            .color = config->background_color,
        },
        {
            .depthStencil = {1.0f, 0},
        }
        
    };
    uint32_t image_index = swp_ch->acquired_image_index;
    
    vkCmdBeginRenderPass(command_buffer, &(VkRenderPassBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderer->render_pass,
        .framebuffer = renderer->frame_buffers[image_index],
        .renderArea = (VkRect2D) { .extent = swp_ch->extent},
        .clearValueCount = (uint32_t)(sizeof(clear_values)/sizeof(clear_values[0])),
        .pClearValues = clear_values,
    }, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->graphics_pipeline);

    VkBuffer vertex_buffers = {buffer_data->vertex_buffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, buffer_data->index_buffer, 0, VK_INDEX_TYPE_UINT32);

    VkViewport viewport = {
        .x = 0.0f, .y = 0.0f,
        .width = swp_ch->extent.width,
        .height = swp_ch->extent.height,
        .minDepth = 0.0f, .maxDepth = 1.0f
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = swp_ch->extent
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,renderer->pipeline_layout, 0, 1, &renderer->descriptor_sets[renderer->current_frame], 0, NULL);
    uint32_t index_count = geometry_data->index_count; 
    vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
    vkCmdEndRenderPass(command_buffer);

    EXPECT(vkEndCommandBuffer(command_buffer), "Couldn't end command buffer");
}

void submit_command_buffer(VkCore *vk_core, SwapchainData *swp_ch, BufferData *buffer_data, Renderer *renderer) {
    uint32_t frame = renderer->current_frame;
    uint32_t image_index = swp_ch->acquired_image_index;
    VkCommandBuffer command_buffer = renderer->command_buffers[frame];

    update_uniform_buffer(renderer->current_frame, swp_ch, buffer_data);
    
    EXPECT(vkQueueSubmit(vk_core->graphics_queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderer->acquired_image_semaphore[frame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderer->finished_render_semaphore[image_index],
        .pWaitDstStageMask = (VkPipelineStageFlags[]) {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        }
    }, renderer->in_flight_fence[frame]), "Couldn't submit command buffer");
}

void create_renderer(VkCore *vk_core, SwapchainData *swp_ch, BufferData *buffer_data, TextureData *texture_data, Renderer *renderer, GeometryData *geometry_data){
    create_render_pass(vk_core, swp_ch, renderer);
    create_descriptor_set_layout(vk_core, renderer);
    create_graphics_pipeline(vk_core, swp_ch, renderer);
    create_command_pool(vk_core, renderer);
    create_depth_resources(vk_core, swp_ch, texture_data, renderer);
    create_frame_buffers(vk_core, swp_ch, texture_data, renderer);
    create_texture_image("./textures/pasted__twitch.png", vk_core, texture_data, renderer);
    create_texture_image_view(vk_core, texture_data);
    create_texture_sampler(vk_core, texture_data);
    load_model("./models/twitch.glb", geometry_data);
    create_vertex_buffer(vk_core, buffer_data, geometry_data, renderer);
    create_index_buffer(vk_core, buffer_data, geometry_data, renderer);
    create_descriptor_pool(vk_core, renderer);
    create_uniform_buffers(vk_core, buffer_data);
    create_descriptor_sets(vk_core, buffer_data, texture_data, renderer);
    allocate_command_buffer(vk_core, swp_ch, renderer);
    create_sync_objects(vk_core, swp_ch, renderer);
}

void destroy_renderer(VkCore *vk_core, SwapchainData *swp_ch, Renderer *renderer){
    vkQueueWaitIdle(vk_core->graphics_queue);

    if (renderer->command_buffers) {
        vkFreeCommandBuffers(vk_core->device, renderer->command_pool, 
                           swp_ch->image_count, renderer->command_buffers);
        free(renderer->command_buffers);
    }

    destroy_sync_objects(vk_core, swp_ch, renderer);
    destroy_coommand_pool(vk_core, renderer);
    destroy_frame_buffers(vk_core, swp_ch, renderer);
    destroy_graphics_pipeline(vk_core, renderer);
    destroy_render_pass(vk_core, renderer);
}
