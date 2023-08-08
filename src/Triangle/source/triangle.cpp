#include <core.h>
#include <glm/glm.hpp>


class Triangle : core
{
public:
    Triangle( HINSTANCE hInst, int sWnd, bool fscreen ) :
        core( "Triangle Application" ),
        hInstance( hInst ),
        showWnd(sWnd),
        fullscreen( fscreen )
    {
      
    }

    void run()
    {
        initWindow();
        initVulkan();
        Mainloop();
        cleanup();
    }

    void drawFrame()
    {
        uint32_t imageIndex = drawFrameProlog();

        recordCommandBufferProlog( imageIndex );

        recordCmds();

        recordCommandBufferEpilog();

        drawFrameEpilog( imageIndex );
    }

private:
    HINSTANCE hInstance;
    HWND hWindow;

    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;

    bool fullscreen;
    int showWnd;

    bool enableValidationLayers = true;

    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 color;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDesc = {};
            bindingDesc.binding = 0;
            bindingDesc.stride = sizeof( Vertex );
            bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDesc;
        }

        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescription()
        {
            std::array<VkVertexInputAttributeDescription, 2> attrDesc = {};
            attrDesc[0].binding = 0;
            attrDesc[0].location = 0;
            attrDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
            attrDesc[0].offset = offsetof( Vertex, pos );

            attrDesc[1].binding = 0;
            attrDesc[1].location = 1;
            attrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attrDesc[1].offset = offsetof( Vertex, color );

            return attrDesc;
        }
    };

    const std::vector<Vertex> vertices = {
    { {0.0f, -0.5f}, {1.0, 0.0, 0.0} },
    { {0.5f, 0.5f}, {0.0, 1.0, 0.0} },
    { {-0.5f, 0.5f}, {0.0, 0.0, 1.0} } };

    void initWindow()
    {
        hWindow = InitWindow( hInstance, "TriangleWindow", ApplicationName().c_str(), WndProc, 800, 600, fullscreen, showWnd);
    }

    uint32_t findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties )
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties( GetPhysicalDevice(), &memProperties );

        for( int i = 0; i < memProperties.memoryTypeCount; i++ )
        {
            if( ( typeFilter & ( 1 << i ) ) &&
                ( memProperties.memoryTypes[i].propertyFlags & properties ) == properties )
            {
                return i;
            }
        }

        throw std::runtime_error( "Error while finding suitable memory type" );
    }

    void createVertexBuffers()
    {
        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size = sizeof( vertices[0] ) * vertices.size();
        createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if( vkCreateBuffer( GetDevice(), &createInfo, NULL, &m_vertexBuffer ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Error while creating vertex buffer" );
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements( GetDevice(), m_vertexBuffer, &memoryRequirements );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memoryRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType( memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

        if( vkAllocateMemory( GetDevice(), &allocInfo, nullptr, &m_vertexBufferMemory ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Error while allocating memory for vertex buffer" );
        }

        vkBindBufferMemory( GetDevice(), m_vertexBuffer, m_vertexBufferMemory, 0 );

        void* data = nullptr;

        vkMapMemory( GetDevice(), m_vertexBufferMemory, 0, createInfo.size, 0, &data );
        memcpy( data, vertices.data(), createInfo.size );
        vkUnmapMemory( GetDevice(), m_vertexBufferMemory );
    }

    void initVulkan()
    {
        std::string vertSpv = "D:\\Workspace\\source\\repos\\VkSamples\\out\\bin\\Debug\\shaders\\triangle.vert.spv";
        std::string fragSpv = "D:\\Workspace\\source\\repos\\VkSamples\\out\\bin\\Debug\\shaders\\triangle.frag.spv";
        auto bindings = Vertex::getBindingDescription();
        auto attributes = Vertex::getAttributeDescription();

        createInstance();
        createSurface( hInstance, hWindow );
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapchain( hWindow );
        createImageViews();
        createRenderPass();
        createGraphicsPipeline(vertSpv, fragSpv, 1, &bindings, attributes.size(), attributes.data());
        createFramebuffers();
        createCommandPool();
        createVertexBuffers();
        createCommandBuffer();
        createSyncObjects();
    }

    void recordCmds()
    {
        VkBuffer vertexBuffers[] = { m_vertexBuffer };
        VkDeviceSize offsets[] = { 0 };

        vkCmdBindVertexBuffers( GetCommandBuffer(), 0, 1, vertexBuffers, offsets );

        vkCmdDraw( GetCommandBuffer(), 3, 1, 0, 0 );
    }

    void cleanup()
    {
        vkDestroyBuffer( GetDevice(), m_vertexBuffer, nullptr );
        vkFreeMemory( GetDevice(), m_vertexBufferMemory, nullptr );
        core::cleanup();
    }
};

int CALLBACK WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd )
{
    Triangle triangle( hInstance, nShowCmd, false );
    try
    {
        triangle.run();
    }
    catch( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
