#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <Windows.h>
#include <vector>
#include <optional>

class ClearApp
{
public:
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

    GLFWwindow* window;
    
    const int WIDTH  = 800;
    const int HEIGHT = 600;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;

        bool isComplete()
        {
            return graphicsFamily.has_value();
        }
    };

    void initWindow()
    {
        glfwInit();
        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

        window = glfwCreateWindow( WIDTH, HEIGHT, "Clear Application", nullptr, nullptr );
    }

    bool checkValidationLayerSupport()
    {
        uint32_t layerCount = 0;

        vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

        std::vector<VkLayerProperties> layers( layerCount );

        vkEnumerateInstanceLayerProperties( &layerCount, layers.data() );

        for( const char* layerName : validationLayers )
        {
            bool layerFound = false;

            for( const auto& layerProperties : layers )
            {
                if( !strcmp( layerName, layerProperties.layerName ) )
                {
                    layerFound = true;
                    break;
                }

            }
            if( !layerFound )
            {
                return false;
            }
        }

        return true;
    }
    void createInstance()
    {
        uint32_t glfwExtnCnt = 0;
        const char** glfwExtns = glfwGetRequiredInstanceExtensions( &glfwExtnCnt );

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
        createInfo.enabledExtensionCount = glfwExtnCnt;
        createInfo.ppEnabledExtensionNames = glfwExtns;

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

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, nullptr );

        std::vector<VkExtensionProperties> extensions( extensionCount );
        vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, extensions.data() );

        std::cout << "Available " << extensionCount << " extensions:" << std::endl;
        for( const auto extension : extensions )
        {
            std::cout << extension.extensionName << std::endl;
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
            if( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
            {
                indices.graphicsFamily = i;
                break;
            }

            i++;
        }

        return indices;
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
        return findQueueFamilies( device ).isComplete();
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

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;

        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = 0;

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
    }

    void initVulkan()
    {
        createInstance();
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void mainloop()
    {
        while( !glfwWindowShouldClose( window ) )
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        vkDestroyDevice( device, nullptr );
        vkDestroyInstance( instance, nullptr );

        glfwDestroyWindow( window );

        glfwTerminate();
    }
};

//int CALLBACK WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd )
int main()
{
    ClearApp app;
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
