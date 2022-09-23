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
    VkExtent2D swapchainExtent;
    VkFormat swapchainFormat;

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

    void initVulkan()
    {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapchain();
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
        }
    }

    void cleanup()
    {
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
    ClearApp app( hInstance, nShowCmd, false );
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
