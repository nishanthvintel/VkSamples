#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include <Windows.h>

#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>

//#define THROW_ON_FAIL  x(...) do { if( x(...) != VK_SUCCESS ) throw std::runtime_error( #x "Failed" ); } while( 0 );

LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

class ClearApp
{
public:
    ClearApp( HINSTANCE instance, int sWnd, bool fscreen ) :
        hInstance( instance ),
        showWnd( sWnd ),
        fullscreen( fscreen )
    {
    }

    void run()
    {
        initWindow();
        initVulkan();
        mainloop();
        cleanup();
    }

private:
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkSurfaceKHR surface;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> swapchainFramebuffers;
    VkExtent2D swapchainExtent;
    VkFormat swapchainFormat;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inflightFence;

    HWND window;
    HINSTANCE hInstance;
    int showWnd;
    bool fullscreen;
    
    const int WIDTH  = 800;
    const int HEIGHT = 600;

    const LPCTSTR windowName = "ClearApplication";
    const LPCTSTR windowTitle = "Clear Application";

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> InstanceExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;

        bool isAdequate()
        {
            return !formats.empty() && !presentModes.empty();
        }
    };

    void createSurface()
    {
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hinstance = hInstance;
        createInfo.hwnd = window;

        if( vkCreateWin32SurfaceKHR( instance, &createInfo, nullptr, &surface ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Could not create Win32 Surface!" );
        }
    }

    void initWindow()
    {
        int width = WIDTH;
        int height = HEIGHT;

        if( fullscreen )
        {
            HMONITOR hMon = MonitorFromWindow( window, MONITOR_DEFAULTTONEAREST );
            MONITORINFO mi = { sizeof( mi ) };

            GetMonitorInfo( hMon, &mi );

            width = mi.rcMonitor.right - mi.rcMonitor.left;
            height = mi.rcMonitor.bottom - mi.rcMonitor.top;
        }

        WNDCLASSEX wc;

        wc.cbSize = sizeof( WNDCLASSEX );
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.cbClsExtra = NULL;
        wc.cbWndExtra = NULL;
        wc.hInstance = hInstance;
        wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
        wc.hIconSm = LoadIcon( NULL, IDI_APPLICATION );
        wc.hCursor = LoadCursor( NULL, IDC_ARROW );
        wc.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 2 );
        wc.lpszMenuName = NULL;
        wc.lpszClassName = windowName;

        if( !RegisterClassEx( &wc ) )
        {
            throw std::runtime_error( "Error registering window class" );
        }

        window = CreateWindowEx( NULL,
            windowName,
            windowTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            width, height,
            NULL, NULL,
            hInstance,
            NULL );

        if( !window )
        {
            throw std::runtime_error( "Error creating window" );
        }

        if( fullscreen )
        {
            SetWindowLong( window, GWL_STYLE, 0 );
        }

        ShowWindow( window, showWnd );
        UpdateWindow( window);
    }

    bool checkValidationLayerSupport()
    {
        uint32_t layersCount = 0;
        vkEnumerateInstanceLayerProperties( &layersCount, nullptr );

        std::vector<VkLayerProperties> availableLayers( layersCount );
        vkEnumerateInstanceLayerProperties( &layersCount, availableLayers.data() );

        std::set<std::string> requiredLayers( validationLayers.begin(), validationLayers.end() );

        for( const auto& layer : availableLayers )
        {
            requiredLayers.erase( layer.layerName );
        }

        return requiredLayers.empty();
    }

    bool checkInstanceExtensionSupport()
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, nullptr );

        std::vector<VkExtensionProperties> availableExtensions( extensionCount );
        vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, availableExtensions.data() );

        std::set<std::string> requiredExtension( InstanceExtensions.begin(), InstanceExtensions.end() );
        
        for( const auto& extension : availableExtensions )
        {
            requiredExtension.erase( extension.extensionName );
        }

        return requiredExtension.empty();
    }

    void createInstance()
    {
        if( !checkInstanceExtensionSupport() )
        {
            throw std::runtime_error( "Required Instance extensions not supported!" );
        }

        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Clear Application";
        appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
        appInfo.pEngineName = "Noob Engine";
        appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>( InstanceExtensions.size() );
        createInfo.ppEnabledExtensionNames = InstanceExtensions.data();

        if( enableValidationLayers )
        {
            if( !checkValidationLayerSupport() )
            {
                throw std::runtime_error( "Validation Layer requested but not supported" );
            }

            createInfo.enabledLayerCount = static_cast<uint32_t>( validationLayers.size() );
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        VkResult result = vkCreateInstance( &createInfo, nullptr, &instance );
        if( result != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create Instance!" );
        }
    }

    QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device )
    {
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;

        vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

        std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
        vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

        int i = 0;

        for( const auto& queueFamily : queueFamilies )
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport );

            if( presentSupport )
            {
                indices.presentFamily = i;
            }

            if( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
            {
                indices.graphicsFamily = i;
                break;
            }

            i++;
        }

        return indices;
    }

    SwapchainSupportDetails querySwapchainSupport( VkPhysicalDevice device )
    {
        SwapchainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &details.capabilities );

        uint32_t formatsCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatsCount, nullptr );

        if( formatsCount )
        {
            details.formats.resize( formatsCount );
            vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatsCount, details.formats.data() );
        }

        uint32_t presentModesCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModesCount, nullptr );

        if( presentModesCount )
        {
            details.presentModes.resize( presentModesCount );
            vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModesCount, details.presentModes.data() );
        }

        return details;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR> availableFormats )
    {
        for( const auto format : availableFormats )
        {
            if( format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR )
            {
                return format;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR> availablePresentModes )
    {
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities )
    {
        if( capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() )
        {
            return capabilities.currentExtent;
        }
        else
        {
            RECT rect;
            GetWindowRect( window, &rect );

            VkExtent2D extent = {
                static_cast< uint32_t >( rect.right - rect.left ),
                static_cast< uint32_t >( rect.bottom - rect.top )
            };

            extent.width = std::clamp( extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width );
            extent.height = std::clamp( extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height );

            return extent;
        }
    }

    bool checkDeviceExtensionSupport( VkPhysicalDevice device )
    {
        uint32_t extensionsCount = 0;
        vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionsCount, nullptr );

        std::vector<VkExtensionProperties> availableExtensions( extensionsCount );
        vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionsCount, availableExtensions.data() );

        std::set<std::string> requiredExtensions( deviceExtensions.begin(), deviceExtensions.end() );

        for( const auto& extension : availableExtensions )
        {
            requiredExtensions.erase( extension.extensionName );
        }

        return requiredExtensions.empty();
    }

    bool isDeviceSuitable( VkPhysicalDevice device )
    {
#if 0
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceFeatures( device, &features );
        vkGetPhysicalDeviceProperties( device, &properties );

        return ( properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && features.geometryShader );
#else

        return findQueueFamilies( device ).isComplete() && checkDeviceExtensionSupport(device) && querySwapchainSupport( device ).isAdequate();
#endif
    }

    void pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices( instance, &deviceCount, nullptr );

        if( deviceCount == 0 )
        {
            throw std::runtime_error( "Could not find device with Vulkan support!" );
        }

        std::vector<VkPhysicalDevice> devices( deviceCount );
        vkEnumeratePhysicalDevices( instance, &deviceCount, devices.data() );

        for( const auto& device : devices )
        {
            if( isDeviceSuitable( device ) )
            {
                physicalDevice = device;

                // stop and use first available physical device
                break;
            }
        }

        if( physicalDevice == VK_NULL_HANDLE )
        {
            throw std::runtime_error( "Could not find compatible device!" );
        }
    }

    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies( physicalDevice );
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
        float queuePriority = 1.0f;

        for( const uint32_t queueFamily : uniqueQueueFamilies )
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;

            queueCreateInfos.push_back( queueCreateInfo );
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>( queueCreateInfos.size() );
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>( deviceExtensions.size() );
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if( enableValidationLayers )
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>( validationLayers.size() );
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        if( vkCreateDevice( physicalDevice, &createInfo, nullptr, &device ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Could not create Vulkan Logical Device!" );
        }

        vkGetDeviceQueue( device, indices.graphicsFamily.value(), 0, &graphicsQueue );
        vkGetDeviceQueue( device, indices.presentFamily.value(), 0, &presentQueue );
    }

    void createSwapchain()
    {
        SwapchainSupportDetails swapchainSupport = querySwapchainSupport( physicalDevice );

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat( swapchainSupport.formats );
        VkPresentModeKHR presentMode = chooseSwapPresentMode( swapchainSupport.presentModes );
        VkExtent2D extent = chooseSwapExtent( swapchainSupport.capabilities );
        uint32_t imageCount = std::clamp( 
            swapchainSupport.capabilities.minImageCount + 1,
            swapchainSupport.capabilities.minImageCount,
            swapchainSupport.capabilities.maxImageCount );

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.presentMode = presentMode;
        createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;
        createInfo.minImageCount = imageCount;

        QueueFamilyIndices indices = findQueueFamilies( physicalDevice );

        if( indices.graphicsFamily != indices.presentFamily )
        {
            uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        if( vkCreateSwapchainKHR( device, &createInfo, nullptr, &swapchain ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Could not create SwapChain!" );
        }

        imageCount = 0;
        vkGetSwapchainImagesKHR( device, swapchain, &imageCount, nullptr );

        swapchainImages.resize( imageCount );
        vkGetSwapchainImagesKHR( device, swapchain, &imageCount, swapchainImages.data() );

        swapchainExtent = extent;
        swapchainFormat = surfaceFormat.format;
    }

    void createImageViews()
    {
        swapchainImageViews.resize( swapchainImages.size() );

        for( int i = 0; i < swapchainImages.size(); i++ )
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapchainImages[i];
            createInfo.format = swapchainFormat;
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if( vkCreateImageView( device, &createInfo, nullptr, &swapchainImageViews[i] ) != VK_SUCCESS )
            {
                throw std::runtime_error( "Could not create ImageView!" );
            }
        }
    }

    void createRenderPass()
    {
        VkAttachmentDescription attachment{};
        attachment.format = swapchainFormat;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference attachmentRef{};
        attachmentRef.attachment = 0;
        attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &attachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &attachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if( vkCreateRenderPass( device, &renderPassInfo, nullptr, &renderPass ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create Render pass!" );
        }
    }

/*
#ifndef SPIRV_DIR
#define SPIRV_DIR "."
#endif
*/

    void createGraphicsPipeline()
    {
        std::string dirname = SPIRV_DIR;
        std::string vertSpv = dirname + "/shader.frag.spv";
        std::replace( vertSpv.begin(), vertSpv.end(), '/', '\\');

//        auto vertShaderCode = readFile( "D:\\Workspace\\source\\repos\\VkSamples\\out\\bin\\Debug\\shaders\\shader.vert.spv" );
        auto vertShaderCode = readFile( vertSpv );
        auto fragShaderCode = readFile( "D:\\Workspace\\source\\repos\\VkSamples\\out\\bin\\Debug\\shaders\\shader.frag.spv" );

        VkShaderModule vertShaderModule = createShaderModule( vertShaderCode );
        VkShaderModule fragShaderModule = createShaderModule( fragShaderCode );

        VkPipelineShaderStageCreateInfo vertStageInfo{};
        vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStageInfo.module = vertShaderModule;
        vertStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragStageInfo{};
        fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStageInfo.module = fragShaderModule;
        fragStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = ( float )swapchainExtent.width;
        viewport.height = ( float )swapchainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor;
        scissor.offset = { 0, 0 };
        scissor.extent = swapchainExtent;

		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = static_cast< uint32_t >( dynamicStates.size() );
		dynamicStateInfo.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisample{};
        multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.sampleShadingEnable = VK_FALSE;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisample.minSampleShading = 1.0f;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

        VkPipelineColorBlendStateCreateInfo colorBlendState{};
        colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendState.logicOpEnable = VK_FALSE;
        colorBlendState.logicOp = VK_LOGIC_OP_COPY;
        colorBlendState.attachmentCount = 1;
        colorBlendState.pAttachments = &colorBlendAttachment;
        colorBlendState.blendConstants[0] = 0.0f;
        colorBlendState.blendConstants[1] = 0.0f;
        colorBlendState.blendConstants[2] = 0.0f;
        colorBlendState.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        if( vkCreatePipelineLayout( device, &pipelineLayoutInfo, nullptr, &pipelineLayout ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed creating Pipeline layout" );
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisample;
        pipelineInfo.pDepthStencilState = nullptr;
        pipelineInfo.pColorBlendState = &colorBlendState;
        pipelineInfo.pDynamicState = &dynamicStateInfo;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        if( vkCreateGraphicsPipelines( device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create graphics pipeline!" );
        }

        vkDestroyShaderModule( device, vertShaderModule, nullptr );
        vkDestroyShaderModule( device, fragShaderModule, nullptr );
    }

    std::vector<char> readFile( const std::string& fileName )
    {
        std::ifstream file( fileName, std::ios::ate | std::ios::binary );

        if( !file.is_open() )
        {
            throw std::runtime_error( "Error while opening file" );
        }

        size_t fileSize = file.tellg();
        std::vector<char> buffer( fileSize );

        file.seekg( 0 );
        file.read( buffer.data(), fileSize );

        file.close();
        
        return buffer;
    }

    VkShaderModule createShaderModule( const std::vector<char> code )
    {
        VkShaderModule shader;
        VkShaderModuleCreateInfo Info{};
        Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        Info.codeSize = code.size();
        Info.pCode = reinterpret_cast< const uint32_t* >( code.data() );

        if( vkCreateShaderModule( device, &Info, nullptr, &shader ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create Shader Module" );
        }

        return shader;
    }

    void createFramebuffers()
    {
        const size_t size =  swapchainImageViews.size();
        swapchainFramebuffers.resize( size );
        for( size_t i = 0; i < size; i++ )
        {
            VkImageView attachment[] = { swapchainImageViews[i] };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachment;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.width = swapchainExtent.width;
            framebufferInfo.height = swapchainExtent.height;
            framebufferInfo.layers = 1;

            if( vkCreateFramebuffer( device, &framebufferInfo, nullptr, &swapchainFramebuffers[i] ) != VK_SUCCESS )
            {
                throw std::runtime_error( "Failed to create Framebuffer!" );
            }
        }
    }

    void createCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies( physicalDevice );

        VkCommandPoolCreateInfo commandPoolInfo{};
        commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if( vkCreateCommandPool( device, &commandPoolInfo, nullptr, &commandPool ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create command pool!" );
        }
    }

    void createCommandBuffer()
    {
        VkCommandBufferAllocateInfo commandBufferInfo{};
        commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferInfo.commandBufferCount = 1;
        commandBufferInfo.commandPool = commandPool;
        commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        if( vkAllocateCommandBuffers( device, &commandBufferInfo, &commandBuffer ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create command buffers!" );
        }
    }

    void recordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t imageIndex )
    {
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast< float >( swapchainExtent.width );
		viewport.height = static_cast< float >( swapchainExtent.height );
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapchainExtent;

        VkCommandBufferBeginInfo commandBufferBegin{};
        commandBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBegin.flags = 0;
        commandBufferBegin.pInheritanceInfo = nullptr;

		VkRenderPassBeginInfo renderpassBegin{};
		renderpassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderpassBegin.renderPass = renderPass;
		renderpassBegin.framebuffer = swapchainFramebuffers[imageIndex];
		renderpassBegin.renderArea.offset = { 0, 0 };
		renderpassBegin.renderArea.extent = swapchainExtent;

        if( vkBeginCommandBuffer( commandBuffer, &commandBufferBegin ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to begin command buffer" );
        }

        vkCmdBeginRenderPass( commandBuffer, &renderpassBegin, VK_SUBPASS_CONTENTS_INLINE );
        vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
        vkCmdSetViewport( commandBuffer, 0, 1, &viewport );
        vkCmdSetScissor( commandBuffer, 0, 1, &scissor );
        vkCmdDraw( commandBuffer, 6, 1, 0, 0 );
        
        vkCmdEndRenderPass( commandBuffer );
        if( vkEndCommandBuffer( commandBuffer ) != VK_SUCCESS )
        {
            throw std::runtime_error( " Failed to end command buffer" );
        }
    }

    void createSyncObjects()
    {
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if( vkCreateSemaphore( device, &semaphoreInfo, nullptr, &imageAvailableSemaphore ) != VK_SUCCESS ||
            vkCreateSemaphore( device, &semaphoreInfo, nullptr, &renderFinishedSemaphore ) != VK_SUCCESS ||
            vkCreateFence( device, &fenceInfo, nullptr, &inflightFence ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to Create Synchronization objects" );
        }
    }

    void initVulkan()
    {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapchain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffer();
        createSyncObjects();
    }

    void drawFrame()
    {
        uint32_t imageIndex;

        vkWaitForFences( device, 1, &inflightFence, 1, UINT64_MAX );
        vkResetFences( device, 1, &inflightFence );

        vkAcquireNextImageKHR( device, swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex );

        vkResetCommandBuffer( commandBuffer, 0 );

        recordCommandBuffer( commandBuffer, imageIndex );

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if( vkQueueSubmit( graphicsQueue, 1, &submitInfo, inflightFence ) != VK_SUCCESS )
        {
            std::runtime_error( "Failed to submit to Graphics Queue." );
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains[] = { swapchain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        vkQueuePresentKHR( presentQueue, &presentInfo );
    }

    void mainloop()
    {
        MSG msg;

        ZeroMemory( &msg, sizeof( msg ) );

        while( true )
        {
            if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
            {
                if( msg.message == WM_QUIT )
                {
                    break;
                }

                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
            
            drawFrame();
        }

        vkDeviceWaitIdle( device );
    }

    void cleanup()
    {
        vkDestroySemaphore( device, imageAvailableSemaphore, nullptr );
        vkDestroySemaphore( device, renderFinishedSemaphore, nullptr );
        vkDestroyFence( device, inflightFence, nullptr );
        vkDestroyCommandPool( device, commandPool, nullptr );
        for( auto framebuffer : swapchainFramebuffers )
        {
            vkDestroyFramebuffer( device, framebuffer, nullptr );
        }
        vkDestroyRenderPass( device, renderPass, nullptr );
        vkDestroyPipeline( device, pipeline, nullptr );
        vkDestroyPipelineLayout( device, pipelineLayout, nullptr );
        for( auto imageView : swapchainImageViews )
        {
            vkDestroyImageView( device, imageView, nullptr );
        }
        vkDestroySwapchainKHR( device, swapchain, nullptr );
        vkDestroyDevice( device, nullptr );
        vkDestroyInstance( instance, nullptr );
    }
};

LRESULT CALLBACK WndProc( HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam )
{
    switch( msg )
    {

    case WM_KEYDOWN:
        if( wParam == VK_ESCAPE )
        {
            DestroyWindow( hwnd );
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        return 0;
    }
    return DefWindowProc( hwnd,
        msg,
        wParam,
        lParam );
}



int CALLBACK WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd )
{
    ClearApp app( hInstance, nShowCmd, true );
    try
    {
        app.run();
    }
    catch( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
