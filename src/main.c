#define WIN32_LEAN_AND_MEAN
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR

#include <windows.h>
#include <vulkan/vulkan.h>

// one window
// minimal error handling
// designated initializers
// one struct and one file

typedef struct Vec2 {
    float x, y;
} Vec2;

constexpr size_t MAX_SWAPCHAIN_IMAGES = 8;
constexpr size_t IN_FLIGHT_FRAMES = 1;

typedef struct {
    wchar_t const *window_title;

    HANDLE process_heap;
    HINSTANCE hinstance;
    HWND window;

    HMODULE vulkan_library;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
    VkInstance instance;
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue queue;

    VkPipelineLayout pipeline_layout;
    VkShaderModule shader_module;
    void *shader_module_bytes;

    VkSurfaceCapabilitiesKHR surface_capabilities;

    VkCommandPool command_pool;

    size_t current_frame;
    VkCommandBuffer command_buffers[IN_FLIGHT_FRAMES];
    VkSemaphore image_available_semaphores[IN_FLIGHT_FRAMES];
    VkSemaphore render_finished_semaphores[IN_FLIGHT_FRAMES];
    VkFence in_flight_fences[IN_FLIGHT_FRAMES];

    VkSwapchainKHR swapchain;
    bool is_swapchain_dirty;
    uint32_t swapchain_image_count;
    VkImage swapchain_images[MAX_SWAPCHAIN_IMAGES];
    VkImageView swapchain_image_views[MAX_SWAPCHAIN_IMAGES];
    VkFramebuffer framebuffers[MAX_SWAPCHAIN_IMAGES];

    VkRenderPass renderpass;
    VkPipeline pipeline;

    PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
    PFN_vkQueuePresentKHR vkQueuePresentKHR;
    PFN_vkWaitForFences vkWaitForFences;
    PFN_vkResetFences vkResetFences;
    PFN_vkResetCommandBuffer vkResetCommandBuffer;
    PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
    PFN_vkEndCommandBuffer vkEndCommandBuffer;
    PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
    PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
    PFN_vkCmdBindPipeline vkCmdBindPipeline;
    PFN_vkCmdSetViewport vkCmdSetViewport;
    PFN_vkCmdSetScissor vkCmdSetScissor;
    PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
    PFN_vkQueueSubmit vkQueueSubmit;
    PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
    PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;

    VkBuffer vertex_buffer;
    VkBuffer index_buffer;
} App;

void show_window(App const *app, int const nCmdShow) { ShowWindow(app->window, nCmdShow); }

WPARAM main_loop() {
    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return message.wParam;
}

void load_vulkan_library(App *const app) {
    app->vulkan_library = LoadLibraryW(L"vulkan-1.dll");
    if (!app->vulkan_library) {
        MessageBoxW(nullptr, L"Cannot find vulkan, please make sure to update your driver!", app->window_title,
                    MB_OK);
        ExitProcess(1);
    }
    app->vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(
        app->vulkan_library, "vkGetInstanceProcAddr");
}

void create_instance(App *const app) {
    auto const vkCreateInstance = (PFN_vkCreateInstance)app->vkGetInstanceProcAddr(nullptr, "vkCreateInstance");

    vkCreateInstance(&(VkInstanceCreateInfo){
                         .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                         .pApplicationInfo = &(VkApplicationInfo){
                             .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                             .apiVersion = VK_API_VERSION_1_3,
                         },
                         .enabledExtensionCount = 2,
                         .ppEnabledExtensionNames = (const char*[]){"VK_KHR_surface", "VK_KHR_win32_surface"},
                     }, nullptr, &app->instance);

    app->vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)app->
        vkGetInstanceProcAddr(app->instance, "vkGetDeviceProcAddr");
}

void pick_physical_device(App *const app) {
    auto const vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)app->vkGetInstanceProcAddr(
        app->instance, "vkEnumeratePhysicalDevices");
    vkEnumeratePhysicalDevices(app->instance, &(uint32_t){1}, &app->physical_device);
}

void create_device(App *const app) {
    auto const vkCreateDevice = (PFN_vkCreateDevice)app->vkGetInstanceProcAddr(app->instance, "vkCreateDevice");
    auto const vkGetDeviceQueue = (PFN_vkGetDeviceQueue)app->vkGetDeviceProcAddr(app->device, "vkGetDeviceQueue");

    vkCreateDevice(app->physical_device, &(VkDeviceCreateInfo){
                       .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                       .enabledExtensionCount = 1,
                       .ppEnabledExtensionNames = (const char*[]){VK_KHR_SWAPCHAIN_EXTENSION_NAME},
                       .queueCreateInfoCount = 1,
                       .pQueueCreateInfos = &(VkDeviceQueueCreateInfo){
                           .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                           .queueCount = 1,
                           .pQueuePriorities = (float[]){1.0f},
                       },
                       .pNext = &(VkPhysicalDeviceFeatures2){
                           .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                           .pNext = &(VkPhysicalDeviceVulkan12Features){
                               .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
                           },
                       },
                   },
                   nullptr, &app->device);
    vkGetDeviceQueue(app->device, 0, 0, &app->queue);
}

void create_surface(App *const app) {
    auto const vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)app->vkGetInstanceProcAddr(
        app->instance, "vkCreateWin32SurfaceKHR");
    vkCreateWin32SurfaceKHR(app->instance, &(VkWin32SurfaceCreateInfoKHR){
                                .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                                .hinstance = app->hinstance,
                                .hwnd = app->window,
                            },
                            nullptr, &app->surface);
}

void configure_swapchain(App *const app) {
    auto const vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)app->vkGetInstanceProcAddr(
        app->instance, "vkCreateSwapchainKHR");
    auto const vkGetImagesKHR = (PFN_vkGetSwapchainImagesKHR)app->vkGetInstanceProcAddr(
        app->instance, "vkGetSwapchainImagesKHR");
    auto const vkCreateImageView = (PFN_vkCreateImageView)app->vkGetDeviceProcAddr(app->device, "vkCreateImageView");

    auto const old_swapchain = app->swapchain;
    vkCreateSwapchainKHR(app->device, &(VkSwapchainCreateInfoKHR){
                             .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                             .surface = app->surface,
                             .minImageCount = app->surface_capabilities.minImageCount + 1,
                             .imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
                             .imageExtent = app->surface_capabilities.currentExtent,
                             .imageArrayLayers = 1,
                             .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                             .preTransform = app->surface_capabilities.currentTransform,
                             .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                             .presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
                             .clipped = true,
                             .oldSwapchain = old_swapchain,
                         },
                         nullptr, &app->swapchain);
    if (old_swapchain) {
        auto const vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)app->vkGetDeviceProcAddr(
            app->device, "vkDestroySwapchainKHR");
        auto const vkDestroyImageView = (PFN_vkDestroyImageView)app->vkGetDeviceProcAddr(
            app->device, "vkDestroyImageView");

        for (uint32_t i = 0; i < app->swapchain_image_count; ++i)
            vkDestroyImageView(app->device, app->swapchain_image_views[i], nullptr);
        vkDestroySwapchainKHR(app->device, old_swapchain, nullptr);
    }

    vkGetImagesKHR(app->device, app->swapchain, &app->swapchain_image_count, nullptr);
    vkGetImagesKHR(app->device, app->swapchain, &app->swapchain_image_count, app->swapchain_images);

    for (uint32_t i = 0; i < app->swapchain_image_count; ++i)
        vkCreateImageView(app->device, &(VkImageViewCreateInfo){
                              .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                              .image = app->swapchain_images[i],
                              .viewType = VK_IMAGE_VIEW_TYPE_2D,
                              .format = VK_FORMAT_B8G8R8A8_SRGB,
                              .subresourceRange = {
                                  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .levelCount = 1,
                                  .layerCount = 1,
                              },
                          },
                          nullptr, &app->swapchain_image_views[i]);
}

void update_surface_capabilities(App *const app) {
    auto const vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)app->
        vkGetInstanceProcAddr(app->instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app->physical_device, app->surface, &app->surface_capabilities);
}

void destroy_framebuffers(App const *app) {
    auto const vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)app->vkGetDeviceProcAddr(
        app->device, "vkDestroyFramebuffer");
    for (uint32_t i = 0; i < app->swapchain_image_count; ++i)
        vkDestroyFramebuffer(app->device, app->framebuffers[i], nullptr);
}

void configure_framebuffers(App *app) {
    auto const vkCreateFramebuffer = (PFN_vkCreateFramebuffer)app->vkGetDeviceProcAddr(
        app->device, "vkCreateFramebuffer");

    auto const extent = app->surface_capabilities.currentExtent;
    for (uint32_t i = 0; i < app->swapchain_image_count; ++i)
        vkCreateFramebuffer(app->device, &(VkFramebufferCreateInfo){
                                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                .renderPass = app->renderpass,
                                .attachmentCount = 1,
                                .pAttachments = &app->swapchain_image_views[i],
                                .width = extent.width,
                                .height = extent.height,
                                .layers = 1,
                            },
                            nullptr, &app->framebuffers[i]);
}

void setup_swapchain_dependent_resources(App *const app) {
    update_surface_capabilities(app);
    auto const extent = app->surface_capabilities.currentExtent;
    if (extent.width == 0 || extent.height == 0) return;
    auto const vkQueueWaitIdle = (PFN_vkQueueWaitIdle)app->vkGetDeviceProcAddr(app->device, "vkQueueWaitIdle");
    vkQueueWaitIdle(app->queue);
    destroy_framebuffers(app);
    configure_swapchain(app);
    configure_framebuffers(app);
}

typedef struct {
    VkDeviceAddress vertex_buffer_device_address;
} PushConstants;

void render(App *const app) {
    app->vkWaitForFences(app->device, 1, &app->in_flight_fences[app->current_frame], true, UINT64_MAX);

    if (app->is_swapchain_dirty) {
        app->is_swapchain_dirty = false;
        setup_swapchain_dependent_resources(app);
    }

    if (app->surface_capabilities.currentExtent.width == 0 || app->surface_capabilities.currentExtent.height == 0)
        return;

    app->vkResetFences(app->device, 1, &app->in_flight_fences[app->current_frame]);

    uint32_t image_index;
    app->vkAcquireNextImageKHR(app->device, app->swapchain, UINT64_MAX,
                               app->image_available_semaphores[app->current_frame], nullptr,
                               &image_index);

    auto const command_buffer = app->command_buffers[app->current_frame];
    app->vkResetCommandBuffer(command_buffer, 0);
    app->vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo){
                                  .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                              });
    app->vkCmdBeginRenderPass(command_buffer, &(VkRenderPassBeginInfo){
                                  .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                  .renderPass = app->renderpass,
                                  .framebuffer = app->framebuffers[image_index],
                                  .renderArea = {
                                      .extent = app->surface_capabilities.currentExtent,
                                  },
                                  .clearValueCount = 1,
                                  .pClearValues = &(VkClearValue){
                                      .color = {
                                          .float32 = {0.0f, 0.0f, 0.0f, 1.0f}
                                      },
                                  },
                              },
                              VK_SUBPASS_CONTENTS_INLINE);
    app->vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipeline);
    app->vkCmdSetViewport(command_buffer, 0, 1, &(VkViewport){
                              .width = (float)app->surface_capabilities.currentExtent.width,
                              .height = (float)app->surface_capabilities.currentExtent.height,
                              .maxDepth = 1.0f,
                          });
    app->vkCmdSetScissor(command_buffer, 0, 1, &(VkRect2D){
                             .extent = app->surface_capabilities.currentExtent,
                         });
    app->vkCmdBindVertexBuffers(command_buffer, 0, 1, &app->vertex_buffer, &(VkDeviceSize){0});
    app->vkCmdBindIndexBuffer(command_buffer, app->index_buffer, 0, VK_INDEX_TYPE_UINT32);
    app->vkCmdDrawIndexed(command_buffer, 3, 1, 0, 0, 0);
    app->vkCmdEndRenderPass(command_buffer);
    app->vkEndCommandBuffer(command_buffer);

    app->vkQueueSubmit(app->queue, 1, &(VkSubmitInfo){
                           .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                           .waitSemaphoreCount = 1,
                           .pWaitSemaphores = &app->image_available_semaphores[app->current_frame],
                           .pWaitDstStageMask = (VkPipelineStageFlags[]){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
                           .commandBufferCount = 1,
                           .pCommandBuffers = &command_buffer,
                           .signalSemaphoreCount = 1,
                           .pSignalSemaphores = &app->render_finished_semaphores[app->current_frame],
                       },
                       app->in_flight_fences[app->current_frame]);
    app->vkQueuePresentKHR(app->queue, &(VkPresentInfoKHR){
                               .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                               .waitSemaphoreCount = 1,
                               .pWaitSemaphores = &app->render_finished_semaphores[app->current_frame],
                               .swapchainCount = 1,
                               .pSwapchains = &app->swapchain,
                               .pImageIndices = &image_index,
                           });
    app->current_frame = (app->current_frame + 1) % IN_FLIGHT_FRAMES;
}

LRESULT handle_message(App *const app, HWND const window, unsigned int const message, WPARAM const wparam,
                       LPARAM const lparam) {
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            app->is_swapchain_dirty = true;
            return 0;
        case WM_PAINT:
            render(app);
            return 0;
        default:
            return DefWindowProcW(window, message, wparam, lparam);
    }
}

LRESULT CALLBACK window_procedure(HWND const window, unsigned int const message, WPARAM const wparam,
                                  LPARAM const lparam) {
    auto const app = (App*)GetWindowLongPtrW(window, GWLP_USERDATA);
    if (app) return handle_message(app, window, message, wparam, lparam);
    if (message == WM_CREATE)
        SetWindowLongPtrW(window, GWLP_USERDATA,
                          (LONG_PTR)((CREATESTRUCTW*)lparam)->lpCreateParams);
    return DefWindowProcW(window, message, wparam, lparam);
}

void create_window(App *const app) {
    RegisterClassExW(&(WNDCLASSEXW){
        .cbSize = sizeof(WNDCLASSEXW),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = window_procedure,
        .hInstance = app->hinstance,
        .hCursor = LoadCursorA(nullptr, IDC_ARROW),
        .lpszClassName = app->window_title,
    });
    app->window = CreateWindowExW(0, app->window_title, app->window_title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                  nullptr, nullptr, app->hinstance, app);
}

bool load_file(wchar_t const *filename, void **data, size_t *size) {
    HANDLE const file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        *data = nullptr;
        *size = 0;
        return false;
    }
    *size = GetFileSize(file, nullptr);
    *data = HeapAlloc(GetProcessHeap(), 0, *size);
    DWORD read;
    ReadFile(file, *data, *size, &read, nullptr);
    CloseHandle(file);
    return true;
}

void load_shaders(App *const app) {
    auto const vkCreateShaderModule = (PFN_vkCreateShaderModule)app->vkGetDeviceProcAddr(
        app->device, "vkCreateShaderModule");

    size_t shader_size;
    if (!load_file(RESOURCES_PATH L"shaders/shader.spv", &app->shader_module_bytes, &shader_size)) {
        MessageBoxW(nullptr, L"Cannot find shader!", app->window_title, MB_OK);
        ExitProcess(1);
    }

    vkCreateShaderModule(app->device, &(VkShaderModuleCreateInfo){
                             .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                             .codeSize = shader_size,
                             .pCode = (uint32_t*)app->shader_module_bytes,
                         },
                         nullptr, &app->shader_module);
}

void unload_shaders(App const *const app) {
    auto const vkDestroyShaderModule = (PFN_vkDestroyShaderModule)app->vkGetDeviceProcAddr(
        app->device, "vkDestroyShaderModule");
    vkDestroyShaderModule(app->device, app->shader_module, nullptr);
    HeapFree(app->process_heap, 0, app->shader_module_bytes);
}

void create_pipeline_layout(App *const app) {
    auto const vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)app->vkGetDeviceProcAddr(
        app->device, "vkCreatePipelineLayout");
    vkCreatePipelineLayout(app->device, &(VkPipelineLayoutCreateInfo){
                               .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                           },
                           nullptr, &app->pipeline_layout);
}

void create_pipeline(App *const app) {
    auto const vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)app->vkGetDeviceProcAddr(
        app->device, "vkCreateGraphicsPipelines");

    vkCreateGraphicsPipelines(app->device, nullptr, 1, &(VkGraphicsPipelineCreateInfo){
                                  .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                                  .stageCount = 2,
                                  .pStages = (VkPipelineShaderStageCreateInfo[]){
                                      {
                                          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                          .stage = VK_SHADER_STAGE_VERTEX_BIT,
                                          .module = app->shader_module,
                                          .pName = "main",
                                      },
                                      {
                                          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                                          .module = app->shader_module,
                                          .pName = "main",
                                      },
                                  },
                                  .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo){
                                      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                                      .vertexBindingDescriptionCount = 1,
                                      .pVertexBindingDescriptions = &(VkVertexInputBindingDescription){
                                          .stride = sizeof(Vec2),
                                      },
                                      .vertexAttributeDescriptionCount = 1,
                                      .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]){
                                          {
                                              .format = VK_FORMAT_R32G32_SFLOAT,
                                          }
                                      },

                                  },
                                  .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo){
                                      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                  },
                                  .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo){
                                      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                                      .lineWidth = 1.0f,
                                      .frontFace = VK_FRONT_FACE_CLOCKWISE,
                                  },
                                  .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo){
                                      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                                  },
                                  .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo){
                                      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                      .attachmentCount = 1,
                                      .pAttachments = &(VkPipelineColorBlendAttachmentState){
                                          .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
                                      },
                                  },
                                  .layout = app->pipeline_layout,
                                  .renderPass = app->renderpass,
                                  .pDynamicState = &(VkPipelineDynamicStateCreateInfo){
                                      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                      .dynamicStateCount = 2,
                                      .pDynamicStates = (VkDynamicState[]){
                                          VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
                                      },
                                  },
                                  .pViewportState = &(VkPipelineViewportStateCreateInfo){
                                      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                      .viewportCount = 1,
                                      .scissorCount = 1,
                                      .pViewports = &(VkViewport){},
                                      .pScissors = &(VkRect2D){},
                                  }
                              },
                              nullptr, &app->pipeline);
}

void create_renderpass(App *app) {
    auto const vkCreateRenderPass = (PFN_vkCreateRenderPass)app->vkGetDeviceProcAddr(app->device, "vkCreateRenderPass");
    vkCreateRenderPass(app->device, &(VkRenderPassCreateInfo){
                           .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                           .attachmentCount = 1,
                           .pAttachments = &(VkAttachmentDescription){
                               .format = VK_FORMAT_B8G8R8A8_SRGB,
                               .samples = VK_SAMPLE_COUNT_1_BIT,
                               .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                               .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                           },
                           .subpassCount = 1,
                           .pSubpasses = &(VkSubpassDescription){
                               .colorAttachmentCount = 1,
                               .pColorAttachments = &(VkAttachmentReference){
                                   .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                               },
                           },
                           .pDependencies = &(VkSubpassDependency){
                               .srcSubpass = VK_SUBPASS_EXTERNAL,
                               .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                               .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                               .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                           }
                       },
                       nullptr, &app->renderpass);
}

void create_command_pool(App *app) {
    auto const vkCreateCommandPool = (PFN_vkCreateCommandPool)app->vkGetDeviceProcAddr(
        app->device, "vkCreateCommandPool");
    vkCreateCommandPool(app->device, &(VkCommandPoolCreateInfo){
                            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                        },
                        nullptr, &app->command_pool);
}

void allocate_command_buffers(App *app) {
    auto const vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)app->vkGetDeviceProcAddr(
        app->device, "vkAllocateCommandBuffers");
    vkAllocateCommandBuffers(app->device, &(VkCommandBufferAllocateInfo){
                                 .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                 .commandPool = app->command_pool,
                                 .commandBufferCount = IN_FLIGHT_FRAMES,
                             },
                             app->command_buffers);
}

void create_synchronization_objects(App *app) {
    auto const vkCreateSemaphore = (PFN_vkCreateSemaphore)app->vkGetDeviceProcAddr(app->device, "vkCreateSemaphore");
    auto const vkCreateFence = (PFN_vkCreateFence)app->vkGetDeviceProcAddr(app->device, "vkCreateFence");

    for (uint32_t i = 0; i < IN_FLIGHT_FRAMES; ++i) {
        vkCreateSemaphore(app->device, &(VkSemaphoreCreateInfo){.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO},
                          nullptr,
                          &app->image_available_semaphores[i]);
        vkCreateSemaphore(app->device, &(VkSemaphoreCreateInfo){.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO},
                          nullptr,
                          &app->render_finished_semaphores[i]);
        vkCreateFence(app->device, &(VkFenceCreateInfo){
                          .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                          .flags = VK_FENCE_CREATE_SIGNALED_BIT
                      }, nullptr,
                      &app->in_flight_fences[i]);
    }
}

PFN_vkVoidFunction load_device_proc(App const *const app, char const *const name) {
    return app->vkGetDeviceProcAddr(app->device, name);
}

void load_vulkan_functions(App *app) {
    app->vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)load_device_proc(app, "vkAcquireNextImageKHR");
    app->vkQueuePresentKHR = (PFN_vkQueuePresentKHR)load_device_proc(app, "vkQueuePresentKHR");
    app->vkWaitForFences = (PFN_vkWaitForFences)load_device_proc(app, "vkWaitForFences");
    app->vkResetFences = (PFN_vkResetFences)load_device_proc(app, "vkResetFences");
    app->vkResetCommandBuffer = (PFN_vkResetCommandBuffer)load_device_proc(app, "vkResetCommandBuffer");
    app->vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)load_device_proc(app, "vkBeginCommandBuffer");
    app->vkEndCommandBuffer = (PFN_vkEndCommandBuffer)load_device_proc(app, "vkEndCommandBuffer");
    app->vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)load_device_proc(app, "vkCmdBeginRenderPass");
    app->vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)load_device_proc(app, "vkCmdEndRenderPass");
    app->vkCmdBindPipeline = (PFN_vkCmdBindPipeline)load_device_proc(app, "vkCmdBindPipeline");
    app->vkCmdSetViewport = (PFN_vkCmdSetViewport)load_device_proc(app, "vkCmdSetViewport");
    app->vkCmdSetScissor = (PFN_vkCmdSetScissor)load_device_proc(app, "vkCmdSetScissor");
    app->vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)load_device_proc(app, "vkCmdDrawIndexed");
    app->vkQueueSubmit = (PFN_vkQueueSubmit)load_device_proc(app, "vkQueueSubmit");
    app->vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)load_device_proc(app, "vkCmdBindVertexBuffers");
    app->vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)load_device_proc(app, "vkCmdBindIndexBuffer");
}

uint32_t find_memory_type(const VkPhysicalDeviceMemoryProperties *pMemoryProperties,
                          uint32_t const memoryTypeBitsRequirement,
                          VkMemoryPropertyFlags const requiredProperties) {
    const uint32_t memoryCount = pMemoryProperties->memoryTypeCount;
    for (uint32_t memoryIndex = 0; memoryIndex < memoryCount; ++memoryIndex) {
        const uint32_t memoryTypeBits = (1 << memoryIndex);
        const bool isRequiredMemoryType = memoryTypeBitsRequirement & memoryTypeBits;

        const VkMemoryPropertyFlags properties =
            pMemoryProperties->memoryTypes[memoryIndex].propertyFlags;
        const bool hasRequiredProperties =
            (properties & requiredProperties) == requiredProperties;

        if (isRequiredMemoryType && hasRequiredProperties)
            return memoryIndex;
    }
    return UINT32_MAX;
}

VkBuffer create_buffer(App const *const app, VkDeviceSize const size,
                       VkDeviceMemory *memory, VkBufferUsageFlags const usage,
                       VkMemoryPropertyFlags const required_properties) {
    auto const vkCreateBuffer = (PFN_vkCreateBuffer)app->vkGetDeviceProcAddr(app->device, "vkCreateBuffer");
    auto const vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)app->vkGetDeviceProcAddr(
        app->device, "vkGetBufferMemoryRequirements");
    auto const vkAllocateMemory = (PFN_vkAllocateMemory)app->vkGetDeviceProcAddr(app->device, "vkAllocateMemory");
    auto const vkBindBufferMemory = (PFN_vkBindBufferMemory)app->vkGetDeviceProcAddr(app->device, "vkBindBufferMemory");

    VkBuffer buffer;
    vkCreateBuffer(app->device, &(VkBufferCreateInfo){
                       .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                       .size = size,
                       .usage = usage,
                   },
                   nullptr, &buffer);

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(app->device, buffer, &memory_requirements);

    auto const vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)app->
        vkGetInstanceProcAddr(app->instance, "vkGetPhysicalDeviceMemoryProperties");

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(app->physical_device, &memory_properties);
    uint32_t const memory_type_index = find_memory_type(&memory_properties, memory_requirements.memoryTypeBits,
                                                        required_properties);

    vkAllocateMemory(app->device, &(VkMemoryAllocateInfo){
                         .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                         .allocationSize = memory_requirements.size,
                         .memoryTypeIndex = memory_type_index,
                     },
                     nullptr, memory);

    vkBindBufferMemory(app->device, buffer, *memory, 0);
    return buffer;
}

void create_buffers(App *app) {
    auto const vkMapMemory = (PFN_vkMapMemory)app->vkGetDeviceProcAddr(app->device, "vkMapMemory");
    auto const vkUnmapMemory = (PFN_vkUnmapMemory)app->vkGetDeviceProcAddr(app->device, "vkUnmapMemory");

    Vec2 const vertex_data[] = {
        {.x = 0.0f, .y = -0.5f},
        {.x = 0.5f, .y = 0.5f},
        {.x = -0.5f, .y = 0.5f},
    };
    constexpr uint32_t index_data[] = {0, 1, 2};

    VkDeviceMemory vertex_buffer_memory, index_buffer_memory;

    app->vertex_buffer = create_buffer(app, sizeof(vertex_data), &vertex_buffer_memory,
                                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    app->index_buffer = create_buffer(app, sizeof(index_data), &index_buffer_memory,
                                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *buffer_vertex_data, *buffer_index_data;
    vkMapMemory(app->device, vertex_buffer_memory, 0, VK_WHOLE_SIZE, 0, &buffer_vertex_data);
    vkMapMemory(app->device, index_buffer_memory, 0, VK_WHOLE_SIZE, 0, &buffer_index_data);

    memcpy(buffer_vertex_data, vertex_data, sizeof(vertex_data));
    memcpy(buffer_index_data, index_data, sizeof(index_data));

    vkUnmapMemory(app->device, vertex_buffer_memory);
    vkUnmapMemory(app->device, index_buffer_memory);
}

int WINAPI wWinMain(HINSTANCE const hInstance, HINSTANCE const, PWSTR const, int const nShowCmd) {
    App app = {
        .window_title = L"Minimal Window",

        .process_heap = GetProcessHeap(),
        .hinstance = hInstance,
    };

    load_vulkan_library(&app);
    create_instance(&app);
    create_window(&app);
    create_surface(&app);
    pick_physical_device(&app);
    create_device(&app);

    create_buffers(&app);
    create_renderpass(&app);
    create_pipeline_layout(&app);
    load_shaders(&app);
    create_pipeline(&app);
    unload_shaders(&app);

    create_command_pool(&app);
    allocate_command_buffers(&app);
    create_synchronization_objects(&app);

    show_window(&app, nShowCmd);
    load_vulkan_functions(&app);
    return main_loop();
}
