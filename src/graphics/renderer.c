#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

#include "buffer_data.h"
#include "internal_types.h"
#include "kuta_internal.h"
#include "utils.h"

void create_graphics_pipeline(State *state) {
  size_t vert_size;
  const uint32_t *vert_shader_src = read_file("./shaders/vert.spv", &vert_size);
  EXPECT(!vert_shader_src, "emtpy sprv file");

  size_t frag_size;
  const uint32_t *frag_shader_src = read_file("./shaders/frag.spv", &frag_size);
  EXPECT(!frag_shader_src, "emtpy sprv file");

  VkShaderModule vertex_shader_module, fragment_shader_module;

  EXPECT(vkCreateShaderModule(
             state->vk_core.device,
             &(VkShaderModuleCreateInfo){
                 .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                 .pCode = vert_shader_src,
                 .codeSize = vert_size,
             },
             state->vk_core.allocator, &vertex_shader_module),
         "Failed to create shader modules")

  EXPECT(vkCreateShaderModule(
             state->vk_core.device,
             &(VkShaderModuleCreateInfo){
                 .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                 .pCode = frag_shader_src,
                 .codeSize = frag_size,
             },
             state->vk_core.allocator, &fragment_shader_module),
         "Failed to create shader modules")

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

  VkViewport viewports[] = {{
      .width = state->swp_ch.extent.width,
      .height = state->swp_ch.extent.height,
      .maxDepth = 1.0f,
  }};

  VkRect2D scissors[] = {{.extent = state->swp_ch.extent}};

  VkPipelineColorBlendAttachmentState color_blend_attachment_states[] = {{
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      .blendEnable = VK_FALSE,                    // Add this
      .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Add proper blending
      .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp = VK_BLEND_OP_ADD,
  }};

  VkPushConstantRange push_constant_range = {
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .offset = 0,
      .size = sizeof(mat4) // Size of your model matrix
  };

  EXPECT(vkCreatePipelineLayout(
             state->vk_core.device,
             &(VkPipelineLayoutCreateInfo){
                 .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                 .setLayoutCount = 1,
                 .pSetLayouts = &state->renderer.descriptor_set_layout,
                 .pushConstantRangeCount = 1,
                 .pPushConstantRanges = &push_constant_range,
             },
             state->vk_core.allocator, &state->renderer.pipeline_layout),
         "Failed to create pipeline layout")

  VkVertexInputBindingDescription binding_description =
      get_binding_description();
  AttributeDescriptions attribute_descriptions = get_attribute_descriptions();

  EXPECT(
      vkCreateGraphicsPipelines(
          state->vk_core.device, NULL, 1,
          &(VkGraphicsPipelineCreateInfo){
              .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
              .pStages = shader_stages,
              .stageCount = sizeof(shader_stages) / sizeof(*shader_stages),
              .pDynamicState =
                  &(VkPipelineDynamicStateCreateInfo){
                      .sType =
                          VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                      .dynamicStateCount =
                          sizeof(dynamic_states) / sizeof(*dynamic_states),
                      .pDynamicStates = dynamic_states,
                  },
              .pVertexInputState =
                  &(VkPipelineVertexInputStateCreateInfo){
                      .sType =
                          VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                      .vertexBindingDescriptionCount = 1,
                      .pVertexBindingDescriptions = &binding_description,
                      .vertexAttributeDescriptionCount =
                          attribute_descriptions.count,
                      .pVertexAttributeDescriptions =
                          attribute_descriptions.items,
                  },
              .pInputAssemblyState =
                  &(VkPipelineInputAssemblyStateCreateInfo){
                      .sType =
                          VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                  },
              .pViewportState =
                  &(VkPipelineViewportStateCreateInfo){
                      .sType =
                          VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                      .viewportCount = sizeof(viewports) / sizeof(*viewports),
                      .pViewports = viewports,
                      .scissorCount = sizeof(scissors) / sizeof(*scissors),
                      .pScissors = scissors,
                  },
              .pDepthStencilState =
                  &(VkPipelineDepthStencilStateCreateInfo){
                      .sType =
                          VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                      .depthTestEnable = VK_TRUE,
                      .depthWriteEnable = VK_TRUE,
                      .depthCompareOp = VK_COMPARE_OP_LESS,
                      .depthBoundsTestEnable = VK_FALSE,
                      .stencilTestEnable = VK_FALSE,
                  },
              .pRasterizationState =
                  &(VkPipelineRasterizationStateCreateInfo){
                      .sType =
                          VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                      .lineWidth = 1.0,
                      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                      .cullMode = VK_CULL_MODE_NONE,
                      .polygonMode = VK_POLYGON_MODE_FILL,
                  },
              .pMultisampleState =
                  &(VkPipelineMultisampleStateCreateInfo){
                      .sType =
                          VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                  },
              .pColorBlendState =
                  &(VkPipelineColorBlendStateCreateInfo){
                      .sType =
                          VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                      .attachmentCount = sizeof(color_blend_attachment_states) /
                                         sizeof(*color_blend_attachment_states),
                      .pAttachments = color_blend_attachment_states,
                  },
              .layout = state->renderer.pipeline_layout,
              .renderPass = state->renderer.render_pass,
          },
          state->vk_core.allocator, &state->renderer.graphics_pipeline),
      "Failed to Create Graphics Pipeline")

  vkDestroyShaderModule(state->vk_core.device, vertex_shader_module,
                        state->vk_core.allocator);
  vkDestroyShaderModule(state->vk_core.device, fragment_shader_module,
                        state->vk_core.allocator);

  free((void *)vert_shader_src);
  free((void *)frag_shader_src);
}

void destroy_graphics_pipeline(State *state) {
  vkDestroyPipeline(state->vk_core.device, state->renderer.graphics_pipeline,
                    state->vk_core.allocator);
  vkDestroyPipelineLayout(state->vk_core.device,
                          state->renderer.pipeline_layout,
                          state->vk_core.allocator);
}

void create_render_pass(State *state) {
  VkFormat image_format = state->swp_ch.image_format;

  VkAttachmentReference color_attachment_refrences[] = {{
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  }};

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
          .format = find_depth_format(state),
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      }};
  VkSubpassDescription subpass_descriptions[] = {{
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = color_attachment_refrences,
      .pDepthStencilAttachment = &depth_attachment_ref,
  }};

  VkSubpassDependency dependency = {
      /* common robust setup from the tutorial */
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .srcAccessMask = 0,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
  };

  EXPECT(vkCreateRenderPass(
             state->vk_core.device,
             &(VkRenderPassCreateInfo){
                 .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                 .subpassCount = 1,
                 .pSubpasses = subpass_descriptions,
                 .attachmentCount = 2,
                 .pAttachments = attachment_descriptions,
                 .dependencyCount = 1,
                 .pDependencies = &dependency,
             },
             state->vk_core.allocator, &state->renderer.render_pass),
         "Failed to create a render pass")
}

void destroy_render_pass(State *state) {
  vkDestroyRenderPass(state->vk_core.device, state->renderer.render_pass,
                      state->vk_core.allocator);
}

void create_frame_buffers(State *state) {
  uint32_t frame_buffer_count = state->swp_ch.image_count;
  state->renderer.frame_buffers =
      malloc(frame_buffer_count * sizeof(VkFramebuffer));
  EXPECT(state->renderer.frame_buffers == NULL,
         "Couldn't allocate memory for framebuffers array")
  VkExtent2D frame_buffers_extent = state->swp_ch.extent;

  for (uint32_t framebufferIndex = 0; framebufferIndex < frame_buffer_count;
       ++framebufferIndex) {
    VkImageView attachments[2] = {
        state->swp_ch.image_views[framebufferIndex],
        state->renderer.depth_image_view,
    };
    EXPECT(
        vkCreateFramebuffer(
            state->vk_core.device,
            &(VkFramebufferCreateInfo){
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .layers = 1,
                .renderPass = state->renderer.render_pass,
                .width = frame_buffers_extent.width,
                .height = frame_buffers_extent.height,
                .attachmentCount = sizeof(attachments) / sizeof(attachments[0]),
                .pAttachments = attachments,
            },
            state->vk_core.allocator,
            &state->renderer.frame_buffers[framebufferIndex]),
        "Couldn't create framebuffer %i", framebufferIndex)
  }
}

void destroy_frame_buffers(State *state) {
  uint32_t framebuffer_count = state->swp_ch.image_count;

  for (uint32_t framebuffer_index = 0; framebuffer_index < framebuffer_count;
       ++framebuffer_index) {
    vkDestroyFramebuffer(state->vk_core.device,
                         state->renderer.frame_buffers[framebuffer_index],
                         state->vk_core.allocator);
  }

  free(state->renderer.frame_buffers);
}

void create_command_pool(State *state) {
  EXPECT(vkCreateCommandPool(
             state->vk_core.device,
             &(VkCommandPoolCreateInfo){
                 .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                 .queueFamilyIndex = state->vk_core.graphics_queue_family,
                 .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
             },
             state->vk_core.allocator, &state->renderer.command_pool),
         "Failed to create command pool")
}

void destroy_coommand_pool(State *state) {
  vkDestroyCommandPool(state->vk_core.device, state->renderer.command_pool,
                       state->vk_core.allocator);
}

void allocate_command_buffer(State *state) {
  uint32_t count = state->swp_ch.image_count;
  state->renderer.command_buffers = malloc(count * sizeof(VkCommandBuffer));
  EXPECT(!state->renderer.command_buffers,
         "Failed to allocate command buffers array");

  EXPECT(vkAllocateCommandBuffers(
             state->vk_core.device,
             &(VkCommandBufferAllocateInfo){
                 .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                 .commandPool = state->renderer.command_pool,
                 .commandBufferCount = count,
                 .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
             },
             state->renderer.command_buffers),
         "Failed to allocate command buffers");
}

void create_sync_objects(State *state) {
  uint32_t image_count = state->swp_ch.image_count;

  state->renderer.acquired_image_semaphore =
      malloc(image_count * sizeof(VkSemaphore));
  state->renderer.finished_render_semaphore =
      malloc(image_count * sizeof(VkSemaphore));
  state->renderer.in_flight_fence = malloc(image_count * sizeof(VkFence));

  for (uint32_t i = 0; i < image_count; ++i) {
    EXPECT(
        vkCreateSemaphore(state->vk_core.device,
                          &(VkSemaphoreCreateInfo){
                              .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO},
                          state->vk_core.allocator,
                          &state->renderer.acquired_image_semaphore[i]),
        "Couldn't create acquired image semaphore %u", i);
    EXPECT(
        vkCreateSemaphore(state->vk_core.device,
                          &(VkSemaphoreCreateInfo){
                              .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO},
                          state->vk_core.allocator,
                          &state->renderer.finished_render_semaphore[i]),
        "Couldn't create finished render semaphore %u", i);
    EXPECT(
        vkCreateFence(
            state->vk_core.device,
            &(VkFenceCreateInfo){.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                 .flags = VK_FENCE_CREATE_SIGNALED_BIT},
            state->vk_core.allocator, &state->renderer.in_flight_fence[i]),
        "Couldn't create in-flight fence %u", i);
  }
}

void destroy_sync_objects(State *state) {

  for (uint32_t i = 0; i < state->swp_ch.image_count; ++i) {
    vkDestroyFence(state->vk_core.device, state->renderer.in_flight_fence[i],
                   state->vk_core.allocator);
    vkDestroySemaphore(state->vk_core.device,
                       state->renderer.acquired_image_semaphore[i],
                       state->vk_core.allocator);
    vkDestroySemaphore(state->vk_core.device,
                       state->renderer.finished_render_semaphore[i],
                       state->vk_core.allocator);
  }
  free(state->renderer.in_flight_fence);
  free(state->renderer.acquired_image_semaphore);
  free(state->renderer.finished_render_semaphore);
}

void record_command_buffer(BufferData *buffer_data, Settings *settings,
                           State *state, World *world) {
  VkCommandBuffer command_buffer =
      state->renderer.command_buffers[state->renderer.current_frame];

  vkResetCommandBuffer(command_buffer, 0);

  EXPECT(vkBeginCommandBuffer(
             command_buffer,
             &(VkCommandBufferBeginInfo){
                 .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
             }),
         "Couldn't begin command buffer for frame");

  VkClearValue clear_values[2] = {{
                                      .color = settings->background_color,
                                  },
                                  {
                                      .depthStencil = {1.0f, 0},
                                  }

  };
  uint32_t image_index = state->swp_ch.acquired_image_index;

  vkCmdBeginRenderPass(
      command_buffer,
      &(VkRenderPassBeginInfo){
          .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
          .renderPass = state->renderer.render_pass,
          .framebuffer = state->renderer.frame_buffers[image_index],
          .renderArea = (VkRect2D){.extent = state->swp_ch.extent},
          .clearValueCount =
              (uint32_t)(sizeof(clear_values) / sizeof(clear_values[0])),
          .pClearValues = clear_values,
      },
      VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    state->renderer.graphics_pipeline);

  VkViewport viewport = {.x = 0.0f,
                         .y = 0.0f,
                         .width = state->swp_ch.extent.width,
                         .height = state->swp_ch.extent.height,
                         .minDepth = 0.0f,
                         .maxDepth = 1.0f};
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);

  VkRect2D scissor = {.offset = {0, 0}, .extent = state->swp_ch.extent};
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);

  render_system_draw(world, command_buffer);

  vkCmdEndRenderPass(command_buffer);

  EXPECT(vkEndCommandBuffer(command_buffer), "Couldn't end command buffer");
}

void submit_command_buffer(BufferData *buffer_data, State *state, World *world) {
  uint32_t frame = state->renderer.current_frame;
  uint32_t image_index = state->swp_ch.acquired_image_index;
  VkCommandBuffer command_buffer = state->renderer.command_buffers[frame];

  update_camera_uniform_buffer(world, buffer_data, state, frame);
  update_lighting_uniform_buffer(world, state, frame);

  EXPECT(vkQueueSubmit(
             state->vk_core.graphics_queue, 1,
             &(VkSubmitInfo){
                 .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                 .commandBufferCount = 1,
                 .pCommandBuffers = &command_buffer,
                 .waitSemaphoreCount = 1,
                 .pWaitSemaphores =
                     &state->renderer.acquired_image_semaphore[frame],
                 .signalSemaphoreCount = 1,
                 .pSignalSemaphores =
                     &state->renderer.finished_render_semaphore[image_index],
                 .pWaitDstStageMask =
                     (VkPipelineStageFlags[]){
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     }},
             state->renderer.in_flight_fence[frame]),
         "Couldn't submit command buffer");
}

void destroy_renderer(State *state) {
  vkQueueWaitIdle(state->vk_core.graphics_queue);

  if (state->renderer.command_buffers) {
    vkFreeCommandBuffers(state->vk_core.device, state->renderer.command_pool,
                         state->swp_ch.image_count,
                         state->renderer.command_buffers);
    free(state->renderer.command_buffers);
  }

  destroy_sync_objects(state);
  destroy_coommand_pool(state);
  destroy_frame_buffers(state);
  destroy_graphics_pipeline(state);
  destroy_render_pass(state);
}
