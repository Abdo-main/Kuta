#include <vulkan/vulkan_core.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>


#include "main.h"
#include "textures.h"
#include "vertex_data.h"
#include "utils.h"
#include "descriptors.h"

void create_graphics_pipeline(State *state){
    size_t vert_size;
    const uint32_t *vert_shader_src = read_file("./shaders/vert.spv", &vert_size);
    EXPECT(!vert_shader_src, "emtpy sprv file");

    size_t frag_size;
    const uint32_t *frag_shader_src = read_file("./shaders/frag.spv", &frag_size);
    EXPECT(!frag_shader_src, "emtpy sprv file");

    VkShaderModule vertex_shader_module, fragment_shader_module;
    
    EXPECT(vkCreateShaderModule(state->device, &(VkShaderModuleCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pCode = vert_shader_src,
        .codeSize = vert_size,
    }, state->allocator, &vertex_shader_module), "Failed to create shader modules")

    EXPECT(vkCreateShaderModule(state->device, &(VkShaderModuleCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pCode = frag_shader_src,
        .codeSize = frag_size,
    }, state->allocator, &fragment_shader_module), "Failed to create shader modules")


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
            .width = state->renderer.image_extent.width,
            .height = state->renderer.image_extent.height,
            .maxDepth = 1.0f,
        }
    };

    VkRect2D scissors[] = {
        {
            .extent = state->renderer.image_extent,
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

    EXPECT(vkCreatePipelineLayout(state->device, &(VkPipelineLayoutCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &state->renderer.descriptor_set_layout,
    }, state->allocator, &state->renderer.pipeline_layout), "Faailed to create pipeline layout")

    VkVertexInputBindingDescription binding_description = get_binding_description();
    AttributeDescriptions attribute_descriptions = get_attribute_descriptions();

    EXPECT(vkCreateGraphicsPipelines(state->device, NULL, 1, &(VkGraphicsPipelineCreateInfo){
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
        .layout = state->renderer.pipeline_layout,
        .renderPass = state->renderer.render_pass,
    }, state->allocator, &state->renderer.graphics_pipeline), "Failed to Create Graphics Pipeline")

    vkDestroyShaderModule(state->device, vertex_shader_module, state->allocator);
    vkDestroyShaderModule(state->device, fragment_shader_module, state->allocator);

    free((void*)vert_shader_src);
    free((void*)frag_shader_src);
}

void destroy_graphics_pipeline(State *state){
    vkDestroyPipeline(state->device, state->renderer.graphics_pipeline, state->allocator);
    vkDestroyPipelineLayout(state->device, state->renderer.pipeline_layout, state->allocator);
}

void create_render_pass(State *state){

    VkFormat image_format = state->image_format;

    VkAttachmentReference color_attachment_refrences[] = {
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        }
    };

    VkSubpassDescription subpass_descriptions[] = {
        {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = color_attachment_refrences,
        }
    };

    VkAttachmentDescription attachment_descriptions[] = {
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
    };

    EXPECT(vkCreateRenderPass(state->device, &(VkRenderPassCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .subpassCount = sizeof(subpass_descriptions)/sizeof(*subpass_descriptions),
        .pSubpasses = (const VkSubpassDescription *)subpass_descriptions,
        .attachmentCount = sizeof(attachment_descriptions)/sizeof(*attachment_descriptions),
        .pAttachments = attachment_descriptions,
    }, state->allocator, &state->renderer.render_pass), "Failed to create a render pass")
}

void destroy_render_pass(State *state){
    vkDestroyRenderPass(state->device, state->renderer.render_pass, state->allocator);
}

void create_frame_buffers(State *state) {
    uint32_t frame_buffer_count = state->swap_chain_image_count;
    state->renderer.frame_buffers = malloc(frame_buffer_count * sizeof(VkFramebuffer));
    EXPECT(state->renderer.frame_buffers == NULL, "Couldn't allocate memory for framebuffers array")
    VkExtent2D frame_buffers_extent = state->renderer.image_extent;

    for (int framebufferIndex = 0; framebufferIndex < frame_buffer_count; ++framebufferIndex) {
        EXPECT(vkCreateFramebuffer(state->device, &(VkFramebufferCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .layers = 1,
            .renderPass = state->renderer.render_pass,
            .width = frame_buffers_extent.width,
            .height = frame_buffers_extent.height,
            .attachmentCount = 1,
            .pAttachments = &state->swap_chain_image_views[framebufferIndex],
        }, state->allocator, &state->renderer.frame_buffers[framebufferIndex]), "Couldn't create framebuffer %i", framebufferIndex)
    }
}

void destroy_frame_buffers(State *state) {
    uint32_t framebuffer_count = state->swap_chain_image_count;

    for (int framebuffer_index = 0; framebuffer_index < framebuffer_count; ++framebuffer_index) {
        vkDestroyFramebuffer(state->device, state->renderer.frame_buffers[framebuffer_index], state->allocator);
    }

    free(state->renderer.frame_buffers);
}

void create_command_pool(State *state) {
    EXPECT(vkCreateCommandPool(state->device, &(VkCommandPoolCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = state->queue_family,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    }, state->allocator, &state->renderer.command_pool), "Failed to create command pool")
}

void destroy_coommand_pool(State *state) {
    vkDestroyCommandPool(state->device, state->renderer.command_pool, state->allocator);
}



void allocate_command_buffer(State *state) {
    uint32_t count = state->swap_chain_image_count;
    state->renderer.command_buffers = malloc(count * sizeof(VkCommandBuffer));
    EXPECT(!state->renderer.command_buffers, "Failed to allocate command buffers array");

    EXPECT(vkAllocateCommandBuffers(state->device, &(VkCommandBufferAllocateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = state->renderer.command_pool,
        .commandBufferCount = count,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    }, state->renderer.command_buffers), "Failed to allocate command buffers");
}

void create_sync_objects(State *state) {
    Renderer* renderer = &state->renderer;
    uint32_t image_count = state->swap_chain_image_count; // or renderer->image_count

    renderer->acquired_image_semaphore = malloc(image_count * sizeof(VkSemaphore));
    renderer->finished_render_semaphore = malloc(image_count * sizeof(VkSemaphore));
    renderer->in_flight_fence = malloc(image_count * sizeof(VkFence));

    for (uint32_t i = 0; i < image_count; ++i) {
        EXPECT(
            vkCreateSemaphore(state->device, &(VkSemaphoreCreateInfo){ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }, 
                              state->allocator, &renderer->acquired_image_semaphore[i]),
            "Couldn't create acquired image semaphore %u", i
        );
        EXPECT(
            vkCreateSemaphore(state->device, &(VkSemaphoreCreateInfo){ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }, 
                              state->allocator, &renderer->finished_render_semaphore[i]),
            "Couldn't create finished render semaphore %u", i
        );
        EXPECT(
            vkCreateFence(state->device, &(VkFenceCreateInfo){ 
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, 
                .flags = VK_FENCE_CREATE_SIGNALED_BIT 
            }, 
            state->allocator, &renderer->in_flight_fence[i]),
            "Couldn't create in-flight fence %u", i
        );
    }
}


void destroy_sync_objects(State *state) {
    Renderer* renderer = &state->renderer;

    for (uint32_t i = 0; i < state->swap_chain_image_count; ++i) {
        vkDestroyFence(state->device, renderer->in_flight_fence[i], state->allocator);
        vkDestroySemaphore(state->device, renderer->acquired_image_semaphore[i], state->allocator);
        vkDestroySemaphore(state->device, renderer->finished_render_semaphore[i], state->allocator);
    }
    free(renderer->in_flight_fence);
    free(renderer->acquired_image_semaphore);
    free(renderer->finished_render_semaphore);
}

void record_command_buffer(State *state) {
    VkCommandBuffer command_buffer = state->renderer.command_buffers[state->current_frame];
    
    // Reset the command buffer
    vkResetCommandBuffer(command_buffer, 0);

    EXPECT(vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    }), "Couldn't begin command buffer for frame");

    VkClearValue clear_values[] = { state->background_color };
    uint32_t image_index = state->acquired_image_index;
    
    vkCmdBeginRenderPass(command_buffer, &(VkRenderPassBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = state->renderer.render_pass,
        .framebuffer = state->renderer.frame_buffers[image_index],
        .renderArea = (VkRect2D) { .extent = state->renderer.image_extent },
        .clearValueCount = 1,
        .pClearValues = clear_values,
    }, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->renderer.graphics_pipeline);

    VkBuffer vertex_buffers = {state->vertex_buffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, state->index_buffer, 0, VK_INDEX_TYPE_UINT16);

    VkViewport viewport = {
        .x = 0.0f, .y = 0.0f,
        .width = state->renderer.image_extent.width,
        .height = state->renderer.image_extent.height,
        .minDepth = 0.0f, .maxDepth = 1.0f
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = state->renderer.image_extent,
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->renderer.pipeline_layout, 0, 1, &state->renderer.descriptor_sets[state->current_frame], 0, NULL);
    uint32_t index_count = sizeof(indices) / sizeof(indices[0]); 
    vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
    vkCmdEndRenderPass(command_buffer);

    EXPECT(vkEndCommandBuffer(command_buffer), "Couldn't end command buffer");
}

void submit_command_buffer(State *state) {
    uint32_t frame = state->current_frame;
    uint32_t image_index = state->acquired_image_index;
    VkCommandBuffer command_buffer = state->renderer.command_buffers[frame];

    update_uniform_buffer(state, state->current_frame);
    
    EXPECT(vkQueueSubmit(state->queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &state->renderer.acquired_image_semaphore[frame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &state->renderer.finished_render_semaphore[image_index],
        .pWaitDstStageMask = (VkPipelineStageFlags[]) {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        }
    }, state->renderer.in_flight_fence[frame]), "Couldn't submit command buffer");
}

void create_renderer(State *state){
    create_render_pass(state);
    create_descriptor_set_layout(state);
    create_graphics_pipeline(state);
    create_frame_buffers(state);
    create_command_pool(state);
    create_texture_image(state, "./textures/twitch.jpg");
    create_texture_image_view(state);
    create_texture_sampler(state);
    create_vertex_buffer(state);
    create_index_buffer(state);
    create_descriptor_pool(state);
    create_uniform_buffers(state);
    create_descriptor_sets(state);
    allocate_command_buffer(state);
    create_sync_objects(state);
}

void destroy_renderer(State *state, Renderer *renderer){
    vkQueueWaitIdle(state->queue);

    if (state->renderer.command_buffers) {
        vkFreeCommandBuffers(state->device, state->renderer.command_pool, 
                           state->swap_chain_image_count, state->renderer.command_buffers);
        free(state->renderer.command_buffers);
    }

    destroy_sync_objects(state);
    destroy_coommand_pool(state);
    destroy_frame_buffers(state);
    destroy_graphics_pipeline(state);
    destroy_render_pass(state);
}
