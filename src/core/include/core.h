#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include <Windows.h>

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>

HWND InitWindow(const HINSTANCE hInstance, const LPCTSTR windowName, const LPCTSTR windowTitle, const WNDPROC WndProc, const int width, const int height, const bool fullscreen, int showWnd);
LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

class core
{
public:
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

    core( std::string appName ) :
        applicationName( appName )
    {

    }
protected:
    VkInstance GetInstance();
    VkDevice GetDevice();
    VkPhysicalDevice GetPhysicalDevice();
    VkCommandBuffer GetCommandBuffer();
    void EnableValidationLayers();
    std::string ApplicationName();

    void createInstance();
    void createSurface(HINSTANCE hInstance, HWND window);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain(HWND window);
    void createImageViews();
    void createGraphicsPipeline(std::string vertSpv, std::string fragSpv);
    void createGraphicsPipeline( std::string vertSpv, std::string fragSpv, uint32_t numVertexInputBindings, VkVertexInputBindingDescription* vertexInputBindings, uint32_t numVertexInputAttributes, VkVertexInputAttributeDescription* vertexInputAttributes );
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffer();
    uint32_t drawFrameProlog();
    void recordCommandBufferProlog( uint32_t imageIndex );
    void recordCommandBufferEpilog();
    void drawFrameEpilog( uint32_t imageIndex);
    virtual void drawFrame();
    void recordCommandBufferEpilog( VkCommandBuffer commandBuffer, uint32_t imageIndex );
    void createSyncObjects();
    void Mainloop();
    void cleanup();

private:

    VkInstance m_instance;
    VkSurfaceKHR m_surface;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_presentQueue;
    VkQueue m_graphicsQueue;
    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_swapchainImages;
    VkExtent2D m_swapchainExtent;
    VkFormat m_swapchainFormat;
    std::vector<VkImageView> m_swapchainImageViews;
    VkRenderPass m_renderPass;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline;
    std::vector<VkFramebuffer> m_swapchainFramebuffers;
    VkCommandPool m_commandPool;
    VkCommandBuffer m_commandBuffer;
    VkSemaphore m_imageAvailableSemaphore;
    VkSemaphore m_renderFinishedSemaphore;
    VkFence m_inflightFence;



    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> m_instanceExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };

    const std::vector<const char*> m_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    bool enableValidationLayers = false;

    std::string applicationName;

    bool checkValidationLayerSupport();
    bool checkInstanceExtensionSupport();
    QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device );
    SwapchainSupportDetails querySwapchainSupport( VkPhysicalDevice device );
    bool isDeviceSuitable( VkPhysicalDevice device );
    bool checkDeviceExtensionSupport( VkPhysicalDevice device );
    VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR> availableFormats );
    VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR> availablePresentModes );
    VkExtent2D chooseSwapExtent( HWND window, VkSurfaceCapabilitiesKHR& capabilities );
    std::vector<char> readFile( const std::string& fileName );
    VkShaderModule createShaderModule( const std::vector<char> code );

};