#include <core.h>

HWND InitWindow(const HINSTANCE hInstance, const LPCTSTR windowName, const LPCTSTR windowTitle, const WNDPROC WndProc, const int width, const int height, const bool fullscreen, int showWnd)
{
    HWND window;
    int monitor_width = width;
    int monitor_height = height;

/*
    if( fullscreen )
    {
        HMONITOR hMon = MonitorFromWindow( window, MONITOR_DEFAULTTONEAREST );
        MONITORINFO mi = { sizeof( mi ) };

        GetMonitorInfo( hMon, &mi );

        monitor_width = mi.rcMonitor.right - mi.rcMonitor.left;
        monitor_height = mi.rcMonitor.bottom - mi.rcMonitor.top;
    }
*/

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
    UpdateWindow( window );

    return window;
}

void core::Mainloop()
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

    vkDeviceWaitIdle( m_device );
}


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

VkInstance core::GetInstance()
{
    return m_instance;
}

VkDevice core::GetDevice()
{
    return m_device;
}

VkPhysicalDevice core::GetPhysicalDevice()
{
    return m_physicalDevice;
}

VkCommandBuffer core::GetCommandBuffer()
{
    return m_commandBuffer;
}

void core::EnableValidationLayers()
{
    enableValidationLayers = true;
}

std::string  core::ApplicationName()
{
    return applicationName;
}

bool core::checkValidationLayerSupport()
{
    uint32_t layersCount = 0;
    vkEnumerateInstanceLayerProperties( &layersCount, nullptr );

    std::vector<VkLayerProperties> availableLayers( layersCount );
    vkEnumerateInstanceLayerProperties( &layersCount, availableLayers.data() );

    std::set<std::string> requiredLayers( m_validationLayers.begin(), m_validationLayers.end() );

    for( const auto& layer : availableLayers )
    {
        requiredLayers.erase( layer.layerName );
    }

    return requiredLayers.empty();
}

bool core::checkInstanceExtensionSupport()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, nullptr );

    std::vector<VkExtensionProperties> availableExtensions( extensionCount );
    vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, availableExtensions.data() );

    std::set<std::string> requiredExtension( m_instanceExtensions.begin(), m_instanceExtensions.end() );

    for( const auto& extension : availableExtensions )
    {
        requiredExtension.erase( extension.extensionName );
    }

    return requiredExtension.empty();
}

void core::createInstance()
{
    if( !checkInstanceExtensionSupport() )
    {
        throw std::runtime_error( "Required Instance extensions not supported!" );
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = applicationName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.pEngineName = "Noob Engine";
    appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast< uint32_t >( m_instanceExtensions.size() );
    createInfo.ppEnabledExtensionNames = m_instanceExtensions.data();

    if( enableValidationLayers )
    {
        if( !checkValidationLayerSupport() )
        {
            throw std::runtime_error( "Validation Layer requested but not supported" );
        }

        createInfo.enabledLayerCount = static_cast< uint32_t >( m_validationLayers.size() );
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance( &createInfo, nullptr, &m_instance );
    if( result != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to create Instance!" );
    }
}

void core::createSurface( HINSTANCE hInstance, HWND window )
{
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = hInstance;
    createInfo.hwnd = window;

    if( vkCreateWin32SurfaceKHR( m_instance, &createInfo, nullptr, &m_surface ) )
    {
        throw std::runtime_error( "Error creating Win32 surface" );
    }
}

core::QueueFamilyIndices core::findQueueFamilies( VkPhysicalDevice m_device )
{
    core::QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;

    vkGetPhysicalDeviceQueueFamilyProperties( m_device, &queueFamilyCount, nullptr );

    std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( m_device, &queueFamilyCount, queueFamilies.data() );

    int i = 0;

    for( const auto& queueFamily : queueFamilies )
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR( m_device, i, m_surface, &presentSupport );

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

core::SwapchainSupportDetails core::querySwapchainSupport( VkPhysicalDevice m_device )
{
    core::SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_device, m_surface, &details.capabilities );

    uint32_t formatsCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR( m_device, m_surface, &formatsCount, nullptr );

    if( formatsCount )
    {
        details.formats.resize( formatsCount );
        vkGetPhysicalDeviceSurfaceFormatsKHR( m_device, m_surface, &formatsCount, details.formats.data() );
    }

    uint32_t presentModesCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR( m_device, m_surface, &presentModesCount, nullptr );

    if( presentModesCount )
    {
        details.presentModes.resize( presentModesCount );
        vkGetPhysicalDeviceSurfacePresentModesKHR( m_device, m_surface, &presentModesCount, details.presentModes.data() );
    }

    return details;
}

bool core::checkDeviceExtensionSupport( VkPhysicalDevice m_device )
{
    uint32_t extensionsCount = 0;
    vkEnumerateDeviceExtensionProperties( m_device, nullptr, &extensionsCount, nullptr );

    std::vector<VkExtensionProperties> availableExtensions( extensionsCount );
    vkEnumerateDeviceExtensionProperties( m_device, nullptr, &extensionsCount, availableExtensions.data() );

    std::set<std::string> requiredExtensions( m_deviceExtensions.begin(), m_deviceExtensions.end() );

    for( const auto& extension : availableExtensions )
    {
        requiredExtensions.erase( extension.extensionName );
    }

    return requiredExtensions.empty();
}


bool core::isDeviceSuitable( VkPhysicalDevice m_device )
{
    return findQueueFamilies( m_device ).isComplete() && checkDeviceExtensionSupport( m_device ) && querySwapchainSupport( m_device ).isAdequate();
}

void core::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices( m_instance, &deviceCount, nullptr );

    if( deviceCount == 0 )
    {
        throw std::runtime_error( "Could not find m_device with Vulkan support!" );
    }

    std::vector<VkPhysicalDevice> devices( deviceCount );
    vkEnumeratePhysicalDevices( m_instance, &deviceCount, devices.data() );

    for( const auto& m_device : devices )
    {
        if( isDeviceSuitable( m_device ) )
        {
            m_physicalDevice = m_device;

            // stop and use first available physical m_device
            break;
        }
    }

    if( m_physicalDevice == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "Could not find compatible m_device!" );
    }
}

void core::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies( m_physicalDevice );
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
    createInfo.queueCreateInfoCount = static_cast< uint32_t >( queueCreateInfos.size() );
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast< uint32_t >( m_deviceExtensions.size() );
    createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

    if( enableValidationLayers )
    {
        createInfo.enabledLayerCount = static_cast< uint32_t >( m_validationLayers.size() );
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if( vkCreateDevice( m_physicalDevice, &createInfo, nullptr, &m_device ) != VK_SUCCESS )
    {
        throw std::runtime_error( "Could not create Vulkan Logical Device!" );
    }

    vkGetDeviceQueue( m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue );
    vkGetDeviceQueue( m_device, indices.presentFamily.value(), 0, &m_presentQueue );
}

VkSurfaceFormatKHR core::chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR> availableFormats )
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

VkPresentModeKHR core::chooseSwapPresentMode( const std::vector<VkPresentModeKHR> availablePresentModes )
{
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D core::chooseSwapExtent( HWND window, VkSurfaceCapabilitiesKHR& capabilities )
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

void core::createSwapchain( HWND window )
{
    SwapchainSupportDetails swapchainSupport = querySwapchainSupport( m_physicalDevice );

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat( swapchainSupport.formats );
    VkPresentModeKHR presentMode = chooseSwapPresentMode( swapchainSupport.presentModes );
    VkExtent2D extent = chooseSwapExtent( window, swapchainSupport.capabilities );
    uint32_t imageCount = std::clamp(
        swapchainSupport.capabilities.minImageCount + 1,
        swapchainSupport.capabilities.minImageCount,
        swapchainSupport.capabilities.maxImageCount );

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
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

    QueueFamilyIndices indices = findQueueFamilies( m_physicalDevice );

    if( indices.graphicsFamily != indices.presentFamily )
    {
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

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

    if( vkCreateSwapchainKHR( m_device, &createInfo, nullptr, &m_swapchain ) != VK_SUCCESS )
    {
        throw std::runtime_error( "Could not create SwapChain!" );
    }

    imageCount = 0;
    vkGetSwapchainImagesKHR( m_device, m_swapchain, &imageCount, nullptr );

    m_swapchainImages.resize( imageCount );
    vkGetSwapchainImagesKHR( m_device, m_swapchain, &imageCount, m_swapchainImages.data() );

    m_swapchainExtent = extent;
    m_swapchainFormat = surfaceFormat.format;
}

void core::createImageViews()
{
    m_swapchainImageViews.resize( m_swapchainImages.size() );

    for( int i = 0; i < m_swapchainImages.size(); i++ )
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapchainImages[i];
        createInfo.format = m_swapchainFormat;
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

        if( vkCreateImageView( m_device, &createInfo, nullptr, &m_swapchainImageViews[i] ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Could not create ImageView!" );
        }
    }
}

void core::createRenderPass()
{
    VkAttachmentDescription attachment{};
    attachment.format = m_swapchainFormat;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
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

    if( vkCreateRenderPass( m_device, &renderPassInfo, nullptr, &m_renderPass ) != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to create Render pass!" );
    }
}

std::vector<char> core::readFile( const std::string& fileName )
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

VkShaderModule core::createShaderModule( const std::vector<char> code )
{
    VkShaderModule shader;
    VkShaderModuleCreateInfo Info{};
    Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    Info.codeSize = code.size();
    Info.pCode = reinterpret_cast< const uint32_t* >( code.data() );

    if( vkCreateShaderModule( m_device, &Info, nullptr, &shader ) != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to create Shader Module" );
    }

    return shader;
}

void core::createGraphicsPipeline( std::string vertSpv, std::string fragSpv )
{
    createGraphicsPipeline( vertSpv, fragSpv, 0, nullptr, 0, nullptr );
}

void core::createGraphicsPipeline( std::string vertSpv, std::string fragSpv, uint32_t numVertexInputBindings, VkVertexInputBindingDescription* vertexInputBindings, uint32_t numVertexInputAttributes, VkVertexInputAttributeDescription* vertexInputAttributes )
{
    auto vertShaderCode = readFile( vertSpv );
    auto fragShaderCode = readFile( fragSpv );

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
    vertexInputInfo.vertexBindingDescriptionCount = numVertexInputBindings;
    vertexInputInfo.pVertexBindingDescriptions = vertexInputBindings;
    vertexInputInfo.vertexAttributeDescriptionCount = numVertexInputAttributes;
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributes;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = ( float )m_swapchainExtent.width;
    viewport.height = ( float )m_swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = m_swapchainExtent;

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

    if( vkCreatePipelineLayout( m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout ) != VK_SUCCESS )
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
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if( vkCreateGraphicsPipelines( m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline ) != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to create graphics m_pipeline!" );
    }

    vkDestroyShaderModule( m_device, vertShaderModule, nullptr );
    vkDestroyShaderModule( m_device, fragShaderModule, nullptr );
}

void core::createFramebuffers()
{
    const size_t size = m_swapchainImageViews.size();
    m_swapchainFramebuffers.resize( size );
    for( size_t i = 0; i < size; i++ )
    {
        VkImageView attachment[] = { m_swapchainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachment;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.width = m_swapchainExtent.width;
        framebufferInfo.height = m_swapchainExtent.height;
        framebufferInfo.layers = 1;

        if( vkCreateFramebuffer( m_device, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i] ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create Framebuffer!" );
        }
    }
}

void core::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies( m_physicalDevice );

    VkCommandPoolCreateInfo commandPoolInfo{};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if( vkCreateCommandPool( m_device, &commandPoolInfo, nullptr, &m_commandPool ) != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to create command pool!" );
    }
}

void core::createCommandBuffer()
{
    VkCommandBufferAllocateInfo commandBufferInfo{};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferInfo.commandBufferCount = 1;
    commandBufferInfo.commandPool = m_commandPool;
    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    if( vkAllocateCommandBuffers( m_device, &commandBufferInfo, &m_commandBuffer ) != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to create command buffers!" );
    }
}

void core::recordCommandBufferProlog( uint32_t imageIndex )
{
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast< float >( m_swapchainExtent.width );
    viewport.height = static_cast< float >( m_swapchainExtent.height );
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = m_swapchainExtent;

    VkCommandBufferBeginInfo commandBufferBegin{};
    commandBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBegin.flags = 0;
    commandBufferBegin.pInheritanceInfo = nullptr;

    VkClearValue clearColor = { {{0.1f, 0.2f, 0.4f, 1.0f}} };

    VkRenderPassBeginInfo renderpassBegin{};
    renderpassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpassBegin.renderPass = m_renderPass;
    renderpassBegin.framebuffer = m_swapchainFramebuffers[imageIndex];
    renderpassBegin.renderArea.offset = { 0, 0 };
    renderpassBegin.renderArea.extent = m_swapchainExtent;
    renderpassBegin.clearValueCount = 1;
    renderpassBegin.pClearValues = &clearColor;

    if( vkBeginCommandBuffer( m_commandBuffer, &commandBufferBegin ) != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to begin command buffer" );
    }

    vkCmdBeginRenderPass( m_commandBuffer, &renderpassBegin, VK_SUBPASS_CONTENTS_INLINE );
    vkCmdBindPipeline( m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline );
    vkCmdSetViewport( m_commandBuffer, 0, 1, &viewport );
    vkCmdSetScissor( m_commandBuffer, 0, 1, &scissor );

}
void core::recordCommandBufferEpilog()
{
    vkCmdEndRenderPass( m_commandBuffer );
    if( vkEndCommandBuffer( m_commandBuffer ) != VK_SUCCESS )
    {
        throw std::runtime_error( " Failed to end command buffer" );
    }
}

void core::createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if( vkCreateSemaphore( m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore ) != VK_SUCCESS ||
        vkCreateSemaphore( m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore ) != VK_SUCCESS ||
        vkCreateFence( m_device, &fenceInfo, nullptr, &m_inflightFence ) != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to Create Synchronization objects" );
    }
}

uint32_t core::drawFrameProlog()
{
    uint32_t imageIndex;

    vkWaitForFences( m_device, 1, &m_inflightFence, 1, UINT64_MAX );
    vkResetFences( m_device, 1, &m_inflightFence );

    vkAcquireNextImageKHR( m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex );

    vkResetCommandBuffer( m_commandBuffer, 0 );

    return imageIndex;
}

void core::drawFrameEpilog(uint32_t imageIndex)
{
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore };
    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if( vkQueueSubmit( m_graphicsQueue, 1, &submitInfo, m_inflightFence ) != VK_SUCCESS )
    {
        std::runtime_error( "Failed to submit to Graphics Queue." );
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = { m_swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    vkQueuePresentKHR( m_presentQueue, &presentInfo );
}

void core::drawFrame()
{
    uint32_t imageIndex;

    imageIndex = drawFrameProlog();

    recordCommandBufferProlog( imageIndex );
    vkCmdDraw( m_commandBuffer, 3, 1, 0, 0 );
    recordCommandBufferEpilog( );

    VkSemaphore signalSemaphores[1];

    drawFrameEpilog( imageIndex );
}

void core::cleanup()
{
    vkDestroySemaphore( m_device, m_imageAvailableSemaphore, nullptr );
    vkDestroySemaphore( m_device, m_renderFinishedSemaphore, nullptr );
    vkDestroyFence( m_device, m_inflightFence, nullptr );
    vkDestroyCommandPool( m_device, m_commandPool, nullptr );
    for( auto framebuffer : m_swapchainFramebuffers )
    {
        vkDestroyFramebuffer( m_device, framebuffer, nullptr );
    }
    vkDestroyRenderPass( m_device, m_renderPass, nullptr );
    vkDestroyPipeline( m_device, m_pipeline, nullptr );
    vkDestroyPipelineLayout( m_device, m_pipelineLayout, nullptr );
    for( auto imageView : m_swapchainImageViews )
    {
        vkDestroyImageView( m_device, imageView, nullptr );
    }
    vkDestroySwapchainKHR( m_device, m_swapchain, nullptr );
    vkDestroyDevice( m_device, nullptr );
    vkDestroyInstance( m_instance, nullptr );
}