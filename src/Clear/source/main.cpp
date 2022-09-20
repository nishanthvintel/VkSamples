#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <Windows.h>

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
    GLFWwindow* window;
    
    const int WIDTH  = 800;
    const int HEIGHT = 600;

    void initWindow()
    {
        glfwInit();
        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

        window = glfwCreateWindow( WIDTH, HEIGHT, "Course01", nullptr, nullptr );

        uint32_t extensionCount = 0;

        vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, nullptr );

        printf( "Vulkan extension count: %i\n", extensionCount );

        glm::mat4x4 testMatrix( 1.0f );
        glm::vec4 testVector( 1.0f );

        auto resultMul = testMatrix * testVector;


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

        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &appInfo;
        instanceCreateInfo.enabledExtensionCount = glfwExtnCnt;
        instanceCreateInfo.ppEnabledExtensionNames = glfwExtns;
        instanceCreateInfo.enabledLayerCount = 0;

        VkResult result = vkCreateInstance( &instanceCreateInfo, nullptr, &instance );
    }

    void initVulkan()
    {
        createInstance();
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
        vkDestroyInstance( instance, nullptr );

        glfwDestroyWindow( window );

        glfwTerminate();
    }
};

int CALLBACK WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd )
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

