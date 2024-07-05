#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <unordered_set>
#include <map>
#include <algorithm>
#include <limits>
#include <filesystem>
#include <fstream>
#include <chrono>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const char* WINDOW_TITLE = "Vulkan Test";
const char* APP_NAME = "Hello Triangle";
const char* ENGINE_NAME = "No Engine";
const int MAX_FRAMES_IN_FLIGHT = 2;

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 tex_coord;
    
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription binding_desc{};
        binding_desc.binding = 0u;
        binding_desc.stride = sizeof(Vertex);
        binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        return binding_desc;
    }
    
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescritpions() {
        std::array<VkVertexInputAttributeDescription, 3> attribute_desc{};
        attribute_desc[0].binding = 0;
        attribute_desc[0].location = 0;
        attribute_desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_desc[0].offset = offsetof(Vertex, pos);
        
        attribute_desc[1].binding = 0;
        attribute_desc[1].location = 1;
        attribute_desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_desc[1].offset = offsetof(Vertex, color);
        
        attribute_desc[2].binding = 0;
        attribute_desc[2].location = 2;
        attribute_desc[2].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_desc[2].offset = offsetof(Vertex, tex_coord);
        
        return attribute_desc;
    }
};

const std::vector<Vertex> g_vertices = {
    {{-0.5f, -0.5f,  0.0f}, { 1.0f,  0.0f,  0.0f}, { 0.0f,  0.0f}},
    {{ 0.5f, -0.5f,  0.0f}, { 0.0f,  1.0f,  0.0f}, { 1.0f,  0.0f}},
    {{ 0.5f,  0.5f,  0.0f}, { 0.0f,  0.0f,  1.0f}, { 1.0f,  1.0f}},
    {{-0.5f,  0.5f,  0.0f}, { 1.0f,  1.0f,  1.0f}, { 0.0f,  1.0f}},

    {{-0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, { 0.0f,  0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, { 1.0f,  0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f,  1.0f}, { 1.0f,  1.0f}},
    {{-0.5f,  0.5f, -0.5f}, { 1.0f,  1.0f,  1.0f}, { 0.0f,  1.0f}}
};

const std::vector<uint16_t> g_indices = {
    0, 1, 2,
    2, 3, 0,
    4, 5, 6,
    6, 7, 4
};

class InputFileStramGuard final {
public:
    InputFileStramGuard(std::ifstream&& stream) : m_stream(std::move(stream)) {}
    ~InputFileStramGuard() {
        m_stream.close();
    }
    InputFileStramGuard(const InputFileStramGuard&) = delete;
    InputFileStramGuard& operator=(const InputFileStramGuard&) = delete;
    
    std::ifstream& Get() {
        return m_stream;
    }
private:
    std::ifstream m_stream;
};

static std::vector<char> readFile(const std::string& file_name) {
    InputFileStramGuard stream_guard(std::ifstream(file_name, std::ios::ate | std::ios::binary));
    std::ifstream& file = stream_guard.Get(); 
    if(!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    
    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);
    
    file.seekg(0u);
    file.read(buffer.data(), file_size);
    
    return buffer;
}

const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool ENABLE_VALIDATION_LAYERS = false;
#else
const bool ENABLE_VALIDATION_LAYERS = true;
#endif

class HelloTriangleApplication;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    
    const HelloTriangleApplication* app = (const HelloTriangleApplication*)pUserData;
    std::cout << "validation layer: " << pCallbackData->pMessage << std::endl;
    
    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
    const VkAllocationCallbacks* p_allocator,
    VkDebugUtilsMessengerEXT* p_debug_messenger) {
    
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debug_messenger,
    const VkAllocationCallbacks* p_allocator) {
    
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debug_messenger, p_allocator);
	}
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;
    std::optional<uint32_t> transfer_family;
    
    bool isComplete() {
        return graphics_family.has_value() && present_family.has_value() && transfer_family.has_value();
    }
    
    VkSharingMode getBufferSharingMode() {
        if (graphics_family.has_value() && transfer_family.has_value() && graphics_family.value() != transfer_family.value()) {
            return VK_SHARING_MODE_CONCURRENT;
        }
        return VK_SHARING_MODE_EXCLUSIVE;
    }
    
    std::unordered_set<uint32_t> getFamilies() {
        std::unordered_set<uint32_t> family_indices;
        if(graphics_family.has_value()) {
            family_indices.insert(graphics_family.value());
        }
        if(present_family.has_value()) {
            family_indices.insert(present_family.value());
        }
        if(transfer_family.has_value()) {
            family_indices.insert(transfer_family.value());
        }
        return family_indices;
    }
    
    std::vector<uint32_t> getIndices() {
        const auto& families = getFamilies();
        std::vector<uint32_t> result(families.cbegin(), families.cend());
        return result;
    }
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

struct SwapchainParams {
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;
    VkExtent2D extent;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class HelloTriangleApplication {
public:
    
    void run() {
        initMainWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* m_window = nullptr;
    VkInstance m_vk_instance = VK_NULL_HANDLE;;
    std::unordered_set<std::string> m_available_instance_ext;
    std::unordered_set<std::string> m_available_device_ext;
    std::unordered_set<std::string> m_validation_layers;
    VkDebugUtilsMessengerEXT m_debug_messenger = VK_NULL_HANDLE;;
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphics_queue = VK_NULL_HANDLE;
    VkQueue m_transfer_queue = VK_NULL_HANDLE;
    VkQueue m_present_queue = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_swapchain_images;
    std::vector<VkImageView> m_swapchain_views;
    std::vector<VkFramebuffer> m_swapchain_framebuffers;
    SwapchainParams m_swapchain_params;
    SwapchainSupportDetails m_swapchain_support_details;
    VkDescriptorSetLayout m_desc_set_layout = VK_NULL_HANDLE;
    VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
    VkRenderPass m_render_pass = VK_NULL_HANDLE;
    VkShaderModule m_vert_shader_modeule = VK_NULL_HANDLE;
    VkShaderModule m_frag_shader_modeule = VK_NULL_HANDLE;
    VkPipeline m_graphics_pipeline = VK_NULL_HANDLE;
    VkCommandPool m_grapics_cmd_pool = VK_NULL_HANDLE;
    VkCommandPool m_transfer_cmd_pool = VK_NULL_HANDLE;
    VkDescriptorPool m_desc_pool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_desc_sets;
    std::vector<VkCommandBuffer> m_command_buffers;
    std::vector<VkSemaphore> m_image_available; // signaled when the presentation engine is finished using the image.
    std::vector<VkSemaphore> m_render_finished;
    std::vector<VkFence> m_in_flight_frame; // will be signaled when the command buffers finish
    uint32_t m_current_frame = 0u;
    bool m_framebuffer_resized = false;
    VkBuffer m_vertex_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertex_memory = VK_NULL_HANDLE;
    VkBuffer m_index_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_index_memory = VK_NULL_HANDLE;
    std::vector<VkBuffer> m_uniform_buffers;
    std::vector<VkDeviceMemory> m_uniform_memory;
    std::vector<void*> m_uniform_mapped;
    VkImage m_texture_image = VK_NULL_HANDLE;
    VkDeviceMemory m_texture_memory = VK_NULL_HANDLE;
    VkImageView m_texture_view = VK_NULL_HANDLE;
    VkSampler m_texture_sampler = VK_NULL_HANDLE;
    uint32_t m_mip_levels = 1u;
    VkImage m_depth_image = VK_NULL_HANDLE;
    VkDeviceMemory m_depth_memory = VK_NULL_HANDLE;
    VkImageView m_depth_view = VK_NULL_HANDLE;
    VkSampleCountFlagBits m_msaa_samples = VK_SAMPLE_COUNT_1_BIT;
    VkImage m_color_image;
    VkDeviceMemory m_color_image_memory;
    VkImageView m_color_image_view;
    
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory) {
        QueueFamilyIndices queue_family_indices = findQueueFamilies(m_physical_device, m_surface);
        
        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = queue_family_indices.getBufferSharingMode();
        
        VkResult result = vkCreateBuffer(m_device, &buffer_info, nullptr, &buffer);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }
        
        VkMemoryRequirements mem_req;
        vkGetBufferMemoryRequirements(m_device, buffer, &mem_req);
        uint32_t mem_type_idx = findMemoryType(mem_req.memoryTypeBits, properties);
        
        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_req.size;
        alloc_info.memoryTypeIndex = mem_type_idx;
        
        result = vkAllocateMemory(m_device, &alloc_info, nullptr, &memory);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }
        
        vkBindBufferMemory(m_device, buffer, memory, 0u);
    }
    
    
    
    static void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->m_framebuffer_resized = true;
    }
        
    void initMainWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_window = glfwCreateWindow(WIDTH, HEIGHT, WINDOW_TITLE, nullptr, nullptr);
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, framebuffer_resize_callback);
    }
    
    uint32_t getVkApiVersion() {
        uint32_t api_version = VK_API_VERSION_1_0;
        VkResult result = vkEnumerateInstanceVersion(&api_version);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to get Vulkan API version!");
        }
        return api_version;
    }
    
    std::vector<const char*> getRequiredInstanceExtensions() {
		uint32_t glfw_extension_count = 0;
		const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
		std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
		if (ENABLE_VALIDATION_LAYERS) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
#ifdef __APPLE__
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
		return extensions;
	}
 
    std::vector<const char*> getRequiredValidationLayers() {
		std::vector<const char*> layers;
		if (ENABLE_VALIDATION_LAYERS) {
			layers.insert(layers.begin(), VALIDATION_LAYERS.cbegin(), VALIDATION_LAYERS.cend());
		}
		return layers;
	}
 
    std::unordered_set<std::string> getAvailableInstanceExtensions() {
        uint32_t ext_count = 0u;
        VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to read instance properties!");
        }
        std::vector<VkExtensionProperties> ext_props(ext_count);
        result = vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, ext_props.data());
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to read instance properties!");
        }
        std::unordered_set<std::string> available_ext;
        available_ext.reserve(ext_count);
        for(const VkExtensionProperties& prop : ext_props) {
            available_ext.insert(std::string(prop.extensionName));
        }
        
        return available_ext;
    }
    
    std::vector<const char*> getRequiredDeviceExtensions() {
        std::vector<const char*> extensions {"VK_KHR_portability_subset"};
        extensions.insert(extensions.begin(), DEVICE_EXTENSIONS.cbegin(), DEVICE_EXTENSIONS.cend());
        return extensions;
    }
    
    std::unordered_set<std::string> getAvailableValidationLayers() {
        uint32_t layers_count = 0u;
        VkResult result = vkEnumerateInstanceLayerProperties(&layers_count, nullptr);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to read validation layer properties!");
        }
        std::vector<VkLayerProperties> layer_props(layers_count);
        result = vkEnumerateInstanceLayerProperties(&layers_count, layer_props.data());
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to read validation layer properties!");
        }
        std::unordered_set<std::string> validation_layers; 
        validation_layers.reserve(layers_count);
        for(const VkLayerProperties& prop : layer_props) {
            validation_layers.insert(std::string(prop.layerName));
        }
        
        return validation_layers;
    }
    
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& messanger_info) {
        messanger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        messanger_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        messanger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        messanger_info.pfnUserCallback = debugCallback;
        messanger_info.pUserData = (void*)this;
    }
    
     VkDebugUtilsMessengerEXT setupDebugMessanger() {
        if(!ENABLE_VALIDATION_LAYERS) return VK_NULL_HANDLE;
        
        VkDebugUtilsMessengerCreateInfoEXT messanger_info{};
        populateDebugMessengerCreateInfo(messanger_info);
        
        VkDebugUtilsMessengerEXT debug_messanger = VK_NULL_HANDLE;
        VkResult result = CreateDebugUtilsMessengerEXT(m_vk_instance, &messanger_info, nullptr, &debug_messanger);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
        
        return debug_messanger;
    }
    
    template<typename Container>
    void printInfo(std::string_view header, Container container) {
        std::cout << header << ": " << std::endl;
        for(const auto& ext : container) {
            std::cout << "\t - " << ext << std::endl;
        }
    }
    
    bool checkNamesSupported(const std::unordered_set<std::string>& available_ext, const std::vector<const char*>& req_layer_names) {
        bool result = false;
        
        result = std::all_of(
            req_layer_names.cbegin(),
            req_layer_names.cend(),
            [&available_ext](const char* req_name){
                return available_ext.count(std::string(req_name));
            }
        );
        
        return result;
    }
    
    std::vector<VkImage> getSwapchainImages(VkDevice device, VkSwapchainKHR swapchain) {
        uint32_t swapchain_image_count = 0u;
        VkResult result = vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchain_image_count, nullptr);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to get swapchain images!");
        }
        std::vector<VkImage> swapchain_images(swapchain_image_count);
        result = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data());
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to get swapchain images!");
        }
        
        return swapchain_images; 
    }
    
    std::vector<VkImageView> getImageViews(VkDevice device, const std::vector<VkImage>& images, VkSurfaceFormatKHR surface_format) {
        uint32_t sz = static_cast<uint32_t>(images.size());
        std::vector<VkImageView> image_views(sz);
        for(uint32_t i = 0u; i < sz; ++i) {
            image_views[i] = createImageView(images[i], surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1u);
        }
        
        return image_views;
    }
    
    VkInstance createInstance() {
        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = APP_NAME;
        app_info.applicationVersion= VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = ENGINE_NAME;
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = getVkApiVersion();
        
        VkInstanceCreateInfo instance_info{};
        instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_info.pApplicationInfo = &app_info;
        
        m_available_instance_ext = getAvailableInstanceExtensions();
#ifndef NDEBUG
        printInfo("Supported extensions", m_available_instance_ext);
#endif
        std::vector<const char*> req_instance_extensions = getRequiredInstanceExtensions();
        bool is_all_ext_supported = checkNamesSupported(m_available_instance_ext, req_instance_extensions);
        if(!is_all_ext_supported) {
            throw std::runtime_error("instance not support some extensions!");
        }
        instance_info.enabledExtensionCount = static_cast<uint32_t>(req_instance_extensions.size());
        instance_info.ppEnabledExtensionNames = req_instance_extensions.data();
        
#ifndef NDEBUG
        m_validation_layers = getAvailableValidationLayers();
        printInfo("Supported validation layers", m_validation_layers);
        std::vector<const char*> req_validation_layers = getRequiredValidationLayers();
        bool is_all_layers_supported = checkNamesSupported(m_validation_layers, req_validation_layers);
        if(!is_all_layers_supported) {
            throw std::runtime_error("instance not support some layers!");
        }
        instance_info.enabledLayerCount = static_cast<uint32_t>(req_validation_layers.size());
        instance_info.ppEnabledLayerNames = req_validation_layers.data();
        
        VkDebugUtilsMessengerCreateInfoEXT messenger_info{};
        populateDebugMessengerCreateInfo(messenger_info);
        instance_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &messenger_info;
#else
        instance_info.enabledLayerCount = 0u;
#endif
        
#ifdef __APPLE__
        instance_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
        
        VkInstance instance = VK_NULL_HANDLE;
        VkResult result = vkCreateInstance(&instance_info, nullptr, &instance);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
        
        return instance;
    }
    
    std::vector<VkFramebuffer> createFramebuffers(const std::vector<VkImageView>& views, VkExtent2D extent, VkRenderPass render_pass) {
        size_t ct = views.size();
        std::vector<VkFramebuffer> result_framebuffers(ct);
        for(size_t i = 0u; i < ct; ++i) {
            std::array<VkImageView, 3> attachments = {m_color_image_view, m_depth_view, views[i]};
            //std::array<VkImageView, 3> attachments = {views[i], m_depth_view, m_color_image_view};
            VkFramebufferCreateInfo framebuffer_info{};
            framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.renderPass = render_pass;
            framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebuffer_info.pAttachments = attachments.data();
            framebuffer_info.width = extent.width;
            framebuffer_info.height = extent.height;
            framebuffer_info.layers = 1u;
            
            VkResult result = vkCreateFramebuffer(
                m_device,
                &framebuffer_info,
                nullptr,
                &result_framebuffers[i]
            );
            if(result != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
        
        return result_framebuffers;
    }
    
    void createDescSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_desc_set_layout);
    
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = m_desc_pool;
        alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        alloc_info.pSetLayouts = layouts.data();
        
        m_desc_sets.resize(MAX_FRAMES_IN_FLIGHT);
        VkResult result = vkAllocateDescriptorSets(m_device, &alloc_info, m_desc_sets.data());
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
        
        for(size_t i = 0u; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            VkDescriptorBufferInfo buffer_info{};
            buffer_info.buffer = m_uniform_buffers[i];
            buffer_info.offset = 0u;
            buffer_info.range = sizeof(UniformBufferObject);
            
            VkDescriptorImageInfo image_info{};
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView = m_texture_view;
            image_info.sampler = m_texture_sampler;
            
            std::array<VkWriteDescriptorSet, 2u> desc_writes{};
            desc_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            desc_writes[0].dstSet = m_desc_sets[i];
            desc_writes[0].dstBinding = 0u;
            desc_writes[0].dstArrayElement = 0u;
            desc_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            desc_writes[0].descriptorCount = 1u;
            desc_writes[0].pBufferInfo = &buffer_info;
            desc_writes[0].pImageInfo = nullptr;
            desc_writes[0].pTexelBufferView = nullptr;
            
            desc_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            desc_writes[1].dstSet = m_desc_sets[i];
            desc_writes[1].dstBinding = 1u;
            desc_writes[1].dstArrayElement = 0u;
            desc_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            desc_writes[1].descriptorCount = 1u;
            desc_writes[1].pImageInfo = &image_info;
            desc_writes[1].pBufferInfo = nullptr;
            desc_writes[1].pTexelBufferView = nullptr;
            
            vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(desc_writes.size()), desc_writes.data(), 0u, nullptr);
            
        }
    }
    
    VkDescriptorPool createDescPool() {
        std::array<VkDescriptorPoolSize, 2u> pool_sizes{};
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        
        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();
        pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        pool_info.flags = 0u;
     
        VkDescriptorPool desc_pool;
        VkResult result = vkCreateDescriptorPool(m_device, &pool_info, nullptr, &desc_pool);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
        return desc_pool;
    }
    
    void createCommandPools() {
        QueueFamilyIndices queue_family_indices = findQueueFamilies(m_physical_device, m_surface);
        VkCommandPoolCreateInfo gfx_cmd_pool_info{};
        gfx_cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        gfx_cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        gfx_cmd_pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
        
        VkResult result = vkCreateCommandPool(
            m_device,
            &gfx_cmd_pool_info,
            nullptr,
            &m_grapics_cmd_pool
        );
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
        
        VkCommandPoolCreateInfo transfer_cmd_pool_info{};
        transfer_cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        transfer_cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        transfer_cmd_pool_info.queueFamilyIndex = queue_family_indices.transfer_family.value();
        
        result = vkCreateCommandPool(
            m_device,
            &transfer_cmd_pool_info,
            nullptr,
            &m_transfer_cmd_pool
        );
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }
    
    void createCommandBuffers() {
        m_command_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    
        VkCommandBufferAllocateInfo command_alloc_info{};
        command_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_alloc_info.commandPool = m_grapics_cmd_pool;
        command_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_alloc_info.commandBufferCount = static_cast<uint32_t>(m_command_buffers.size());
        
        VkResult result = vkAllocateCommandBuffers(
            m_device,
            &command_alloc_info,
            m_command_buffers.data()
        );
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }
    
    void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index) {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0u;
        begin_info.pInheritanceInfo = nullptr;
        
        VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording commandbuffer!");
        }
        
        VkRenderPassBeginInfo renderpass_info{};
        renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderpass_info.renderPass = m_render_pass;
        renderpass_info.framebuffer = m_swapchain_framebuffers[image_index];
        renderpass_info.renderArea.offset = {0, 0};
        renderpass_info.renderArea.extent = m_swapchain_params.extent;
        
        std::array<VkClearValue, 2> clear_values{};
        clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clear_values[1].depthStencil = {1.0f, 0};
        renderpass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        renderpass_info.pClearValues = clear_values.data();
        
        vkCmdBeginRenderPass(command_buffer, &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);
        
        VkBuffer vertex_buffers[] = {m_vertex_buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(command_buffer, m_index_buffer, 0u, VK_INDEX_TYPE_UINT16);
        
        VkViewport view_port{};
        view_port.x = 0.0f;
        view_port.y = 0.0f;
        view_port.width = static_cast<float>(m_swapchain_params.extent.width);
        view_port.height = static_cast<float>(m_swapchain_params.extent.height);
        view_port.minDepth = 0.0f;
        view_port.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffer, 0u, 1u, &view_port);
        
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_swapchain_params.extent;
        vkCmdSetScissor(command_buffer, 0u, 1u, &scissor);
        
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_desc_sets[m_current_frame], 0, nullptr);
        
        vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(g_indices.size()), 1u, 0u, 0u, 0u);
        vkCmdEndRenderPass(command_buffer);
        
        result = vkEndCommandBuffer(command_buffer);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
    
    void createSyncObjects(){
        m_image_available.resize(MAX_FRAMES_IN_FLIGHT);
        m_render_finished.resize(MAX_FRAMES_IN_FLIGHT);
        m_in_flight_frame.resize(MAX_FRAMES_IN_FLIGHT);
    
        VkSemaphoreCreateInfo image_available_sema_info{};
        image_available_sema_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        
        VkSemaphoreCreateInfo render_finished_sem_info{};
        render_finished_sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        
        VkFenceCreateInfo in_flight_fen_info{};
        in_flight_fen_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        in_flight_fen_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        
        for(size_t i = 0u; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            VkResult result = vkCreateSemaphore(m_device, &image_available_sema_info, nullptr, &m_image_available[i]);
            if(result != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphore!");
            }
            
            result = vkCreateSemaphore(m_device, &render_finished_sem_info, nullptr, &m_render_finished[i]);
            if(result != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphore!");
            }
            
            result = vkCreateFence(m_device, &in_flight_fen_info, nullptr, &m_in_flight_frame[i]);
            if(result != VK_SUCCESS) {
                throw std::runtime_error("failed to create fence!");
            }
        }
    }
    
    void copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) {
//        VkCommandBufferAllocateInfo alloc_info{};
//        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//        alloc_info.commandPool = m_transfer_cmd_pool;
//        alloc_info.commandBufferCount = 1u;
//        
//        VkCommandBuffer command_buffer;
//        vkAllocateCommandBuffers(m_device, &alloc_info, &command_buffer);
//        
//        VkCommandBufferBeginInfo begin_info{};
//        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
//        vkBeginCommandBuffer(command_buffer, &begin_info);

        VkCommandBuffer command_buffer = beginSingleTimeCommands(m_transfer_cmd_pool);
        
        VkBufferCopy copy_regions{};
        copy_regions.srcOffset = 0u;
        copy_regions.dstOffset = 0u;
        copy_regions.size = size;
        vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_regions);
        
        //vkEndCommandBuffer(command_buffer);
        
        endSingleTimeCommands(command_buffer, m_transfer_queue, m_transfer_cmd_pool);
        
//        VkSubmitInfo submit_info{};
//        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//        submit_info.commandBufferCount = 1u;
//        submit_info.pCommandBuffers = &command_buffer;
//        vkQueueSubmit(m_transfer_queue, 1u, &submit_info, VK_NULL_HANDLE);
//        vkQueueWaitIdle(m_transfer_queue);
//        vkFreeCommandBuffers(m_device, m_transfer_cmd_pool, 1, &command_buffer);
    }
    
    void createAndTransferIndexBuffer(const std::vector<uint16_t>& indices) {
        VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();
        
        VkBuffer staging_buffer;
        VkDeviceMemory staging_memory;
        
        createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, staging_buffer, staging_memory);
        void* data;
        vkMapMemory(m_device, staging_memory, 0u, buffer_size, 0u, &data);
        memcpy(data, indices.data(), buffer_size);
        vkUnmapMemory(m_device, staging_memory);
        
        createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_index_buffer, m_index_memory);
        copyBuffer(staging_buffer, m_index_buffer, buffer_size);
        
        vkDestroyBuffer(m_device, staging_buffer, nullptr);
        vkFreeMemory(m_device, staging_memory, nullptr);
    }
    
    void createAndTransferVertexBuffer(const std::vector<Vertex>& vertices) {
        VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
        
        VkBuffer staging_buffer;
        VkDeviceMemory staging_memory;
        createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_memory);
        
        void* data;
        vkMapMemory(m_device, staging_memory, 0, buffer_size, 0u, &data);
        memcpy(data, vertices.data(), buffer_size);
        vkUnmapMemory(m_device, staging_memory);
        
        createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertex_buffer, m_vertex_memory);
        copyBuffer(staging_buffer, m_vertex_buffer, buffer_size);
        
        vkDestroyBuffer(m_device, staging_buffer, nullptr);
        vkFreeMemory(m_device, staging_memory, nullptr);
    }
    
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer command_buffer = beginSingleTimeCommands(m_grapics_cmd_pool);
        
        VkBufferImageCopy region{};
        region.bufferOffset = 0u;
        region.bufferRowLength = 0u;
        region.bufferImageHeight = 0u;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0u;
        region.imageSubresource.baseArrayLayer = 0u;
        region.imageSubresource.layerCount = 1u;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};
        vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &region);
        
        endSingleTimeCommands(command_buffer, m_graphics_queue, m_grapics_cmd_pool);
    }
    
    uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties mem_prop{};
        vkGetPhysicalDeviceMemoryProperties(m_physical_device, &mem_prop);
        for(uint32_t i = 0u; i < mem_prop.memoryTypeCount; ++i) {
            bool is_type_suit = type_filter & (1 << i);
            bool is_type_adequate = mem_prop.memoryTypes[i].propertyFlags & properties;
            if(is_type_suit && is_type_adequate) {
                return i;
            }
        }
                
        throw std::runtime_error("failed to find suitable memory type!");
    }
    
    VkDeviceMemory createMemory(VkBuffer buffer, VkMemoryPropertyFlags properties) {
        VkMemoryRequirements mem_requirements{};
        vkGetBufferMemoryRequirements(m_device, buffer, &mem_requirements);
        uint32_t mem_type_idx = findMemoryType(mem_requirements.memoryTypeBits, properties);
        
        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = mem_type_idx;
        VkDeviceMemory memory;
        VkResult result = vkAllocateMemory(m_device, &alloc_info, nullptr, &memory);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }
        return memory;
    }
    
    VkDescriptorSetLayout createDescSetLayout() {
        VkDescriptorSetLayoutBinding ubo_layout_binding{};
        ubo_layout_binding.binding = 0u;
        ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo_layout_binding.descriptorCount = 1u;
        ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        ubo_layout_binding.pImmutableSamplers = nullptr;
        
        VkDescriptorSetLayoutBinding sampler_layout_binding{};
        sampler_layout_binding.binding = 1u;
        sampler_layout_binding.descriptorCount = 1u;
        sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler_layout_binding.pImmutableSamplers = nullptr;
        sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        
        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {ubo_layout_binding, sampler_layout_binding};
     
        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();
        
        VkDescriptorSetLayout desc_set_layout = VK_NULL_HANDLE;
        VkResult result = vkCreateDescriptorSetLayout(m_device, &layout_info, nullptr, &desc_set_layout);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
        
        return desc_set_layout;
    }
    
    void createUniformBuffers() {
        VkDeviceSize buffer_size = sizeof(UniformBufferObject);
        
        m_uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_uniform_memory.resize(MAX_FRAMES_IN_FLIGHT);
        m_uniform_mapped.resize(MAX_FRAMES_IN_FLIGHT);
        
        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            createBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniform_buffers[i], m_uniform_memory[i]);
            vkMapMemory(m_device, m_uniform_memory[i], 0u, buffer_size, 0u, &m_uniform_mapped[i]);
        }
    }
    
    void createImage(const VkImageCreateInfo& image_info, VkImage& image, VkDeviceMemory& memory, VkMemoryPropertyFlags properties) {
        VkResult result = vkCreateImage(m_device, &image_info, nullptr, &image);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }
        
        VkMemoryRequirements mem_req{};
        vkGetImageMemoryRequirements(m_device, image, &mem_req);
        
        uint32_t mem_type_idx = findMemoryType(mem_req.memoryTypeBits, properties);
        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_req.size;
        alloc_info.memoryTypeIndex = mem_type_idx;
        
        result = vkAllocateMemory(m_device, &alloc_info, nullptr, &memory);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }
        vkBindImageMemory(m_device, image, memory, 0u);
    }
    
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels) {
        VkCommandBuffer command_buffer = beginSingleTimeCommands(m_grapics_cmd_pool);
        
        VkPipelineStageFlags source_stage;
        VkPipelineStageFlags destination_stage;
        
        VkImageMemoryBarrier barrier{};
        if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (hasStencilComponent(format)) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
        else {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        
        if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0u;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if(old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0u;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        //barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0u;
        barrier.subresourceRange.levelCount = mip_levels;
        barrier.subresourceRange.baseArrayLayer = 0u;
        barrier.subresourceRange.layerCount = 1u;
        vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0u, 0u, nullptr, 0u, nullptr, 1u, &barrier);
        
        endSingleTimeCommands(command_buffer, m_graphics_queue, m_grapics_cmd_pool);
    }
    
    VkCommandBuffer beginSingleTimeCommands(VkCommandPool command_pool) {
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandPool = command_pool;
        alloc_info.commandBufferCount = 1u;
        
        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(m_device, &alloc_info, &command_buffer);
        
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(command_buffer, &begin_info);
        
        return command_buffer;
    }
    
    void endSingleTimeCommands(VkCommandBuffer command_buffer, VkQueue queue, VkCommandPool command_pool) {
        vkEndCommandBuffer(command_buffer);
        
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1u;
        submit_info.pCommandBuffers = &command_buffer;
        
        vkQueueSubmit(queue, 1u, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);
        vkFreeCommandBuffers(m_device, command_pool, 1u, &command_buffer);
    }
    
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels) {
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = aspect_flags;
        view_info.subresourceRange.baseMipLevel = 0u;
        view_info.subresourceRange.levelCount = mip_levels;
        view_info.subresourceRange.baseMipLevel = 0u;
        view_info.subresourceRange.layerCount = 1u;
        
        VkImageView image_view;
        VkResult result = vkCreateImageView(m_device, &view_info, nullptr, &image_view);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }
        
        return image_view;
    }
    
    void createTextureSampler() {
        VkPhysicalDeviceFeatures supported_features{};
        vkGetPhysicalDeviceFeatures(m_physical_device, &supported_features);
    
        VkPhysicalDeviceProperties device_props{};
        vkGetPhysicalDeviceProperties(m_physical_device, &device_props);
    
        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.anisotropyEnable = supported_features.samplerAnisotropy;
        sampler_info.maxAnisotropy = device_props.limits.maxSamplerAnisotropy;
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        sampler_info.compareEnable = VK_FALSE;
        sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = static_cast<float>(m_mip_levels);;
        
        VkResult result = vkCreateSampler(m_device, &sampler_info, nullptr, &m_texture_sampler);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }
    
    void generateMipmaps(VkImage image, VkFormat image_format, int32_t tex_width, uint32_t tex_height, uint32_t mip_levels) {
        VkFormatProperties format_properties;
        vkGetPhysicalDeviceFormatProperties(m_physical_device, image_format, &format_properties);
        if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }
    
        VkCommandBuffer command_buffer = beginSingleTimeCommands(m_grapics_cmd_pool);
        
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0u;
        barrier.subresourceRange.layerCount = 1u;
        barrier.subresourceRange.levelCount = 1u;
        
        int32_t mip_width = tex_width;
        int32_t mip_height = tex_height;
        for (uint32_t i = 1u; i < mip_levels; ++i) {
            barrier.subresourceRange.baseMipLevel = i - 1u;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, nullptr, 0u, nullptr, 1u, &barrier);
            
            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mip_width, mip_height, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1u;
            blit.srcSubresource.baseArrayLayer = 0u;
            blit.srcSubresource.layerCount = 1u;
            
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0u;
            blit.dstSubresource.layerCount = 1u;
            
            vkCmdBlitImage(command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &blit, VK_FILTER_LINEAR);
            
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u, 0u, nullptr, 0u, nullptr, 1u, &barrier);
            
            if (mip_width > 1) {
                mip_width /= 2;
            }
            if (mip_height > 1) {
                mip_height /= 2;
            }
        }
        
        barrier.subresourceRange.baseMipLevel = mip_levels - 1u;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u, 0u, nullptr, 0u, nullptr, 1u, &barrier);
        
        endSingleTimeCommands(command_buffer, m_graphics_queue, m_grapics_cmd_pool);
    }
    
    void createColorResources() {
        VkFormat color_format = m_swapchain_params.surface_format.format;
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = static_cast<uint32_t>(m_swapchain_params.extent.width);
        image_info.extent.height = static_cast<uint32_t>(m_swapchain_params.extent.height);
        image_info.extent.depth = 1u;
        image_info.mipLevels = 1u;
        image_info.arrayLayers = 1u;
        image_info.format = color_format;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.samples = m_msaa_samples;
        image_info.flags = 0u;
        createImage(image_info, m_color_image, m_color_image_memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_color_image_view = createImageView(m_color_image, color_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
    
    VkImage createImage(const std::string& path_to_file) {
        int tex_width;
        int tex_height;
        int tex_channels;
        stbi_uc* pixels = stbi_load(
            path_to_file.c_str(),
            &tex_width,
            &tex_height,
            &tex_channels,
            STBI_rgb_alpha
        );
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }
        VkDeviceSize image_size = tex_width * tex_height * 4;
        
        m_mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(tex_width, tex_height)))) + 1u;
        
        VkBuffer staging_buffer;
        VkDeviceMemory staging_memory;
        createBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_memory);
        
        void* data;
        vkMapMemory(m_device, staging_memory, 0, image_size, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(image_size));
        vkUnmapMemory(m_device, staging_memory);
        
        stbi_image_free(pixels);
        
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = static_cast<uint32_t>(tex_width);
        image_info.extent.height = static_cast<uint32_t>(tex_height);
        image_info.extent.depth = 1u;
        image_info.mipLevels = m_mip_levels;
        image_info.arrayLayers = 1u;
        image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.flags = 0u;
        
        VkImage image;
        createImage(image_info, image, m_texture_memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        
        transitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mip_levels);
        copyBufferToImage(staging_buffer, image, static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height));
        //transitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_mip_levels);
        generateMipmaps(image, VK_FORMAT_R8G8B8A8_SRGB, tex_width, tex_height, m_mip_levels);
        
        vkDestroyBuffer(m_device, staging_buffer, nullptr);
        vkFreeMemory(m_device, staging_memory, nullptr);
        
        return image;
    }
    
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_physical_device, format, &props);
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        throw std::runtime_error("failed to find supported format!");
    }
    
    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }
    
    VkFormat findDepthFormat() {
        return findSupportedFormat(
            {
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT
            },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }
    
    void createDepthResources() {
        VkFormat depth_format = findDepthFormat();
        
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = static_cast<uint32_t>(m_swapchain_params.extent.width);
        image_info.extent.height = static_cast<uint32_t>(m_swapchain_params.extent.height);
        image_info.extent.depth = 1u;
        image_info.mipLevels = 1u;
        image_info.arrayLayers = 1u;
        image_info.format = depth_format;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.samples = m_msaa_samples;
        image_info.flags = 0u;
        createImage(image_info, m_depth_image, m_depth_memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_depth_view = createImageView(m_depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, 1u);
        transitionImageLayout(m_depth_image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1u);
    }
    
    VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physical_device) {
        VkPhysicalDeviceProperties physical_device_properties;
        vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
        VkSampleCountFlags counts = physical_device_properties.limits.framebufferColorSampleCounts & physical_device_properties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) {
            return VK_SAMPLE_COUNT_64_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_32_BIT) {
            return VK_SAMPLE_COUNT_32_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_16_BIT) {
            return VK_SAMPLE_COUNT_16_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_8_BIT) {
            return VK_SAMPLE_COUNT_8_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_4_BIT) {
            return VK_SAMPLE_COUNT_4_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_2_BIT) {
            return VK_SAMPLE_COUNT_2_BIT;
        }
        return VK_SAMPLE_COUNT_1_BIT;
    }
    
    void initVulkan() {
        m_vk_instance = createInstance();
        m_debug_messenger = setupDebugMessanger();
        m_surface = createSurface();
        m_physical_device = pickPhysicalDevice();
        m_msaa_samples = getMaxUsableSampleCount(m_physical_device);
        
        QueueFamilyIndices queue_family_indices = findQueueFamilies(m_physical_device, m_surface);
        m_device = createLogicalDevice(m_physical_device, queue_family_indices);
        
        vkGetDeviceQueue(
            m_device,
            queue_family_indices.graphics_family.value(),
            0,
            &m_graphics_queue
        );
        
        vkGetDeviceQueue(
            m_device,
            queue_family_indices.present_family.value(),
            0,
            &m_present_queue
        );
        
        vkGetDeviceQueue(
            m_device,
            queue_family_indices.transfer_family.value(),
            0,
            &m_transfer_queue
        );
        
        createSwapchain();
        loadShaders();
        createRenderPass();
        m_desc_set_layout = createDescSetLayout();
        createPipeline(m_vert_shader_modeule, m_frag_shader_modeule, m_render_pass);
        createCommandPools();
        createColorResources();
        createDepthResources();
        m_swapchain_framebuffers = createFramebuffers(m_swapchain_views, m_swapchain_params.extent, m_render_pass);
        
        m_texture_image = createImage("textures/texture.jpg");
        m_texture_view = createImageView(m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_mip_levels);
        createTextureSampler();
        createAndTransferVertexBuffer(g_vertices);
        createAndTransferIndexBuffer(g_indices);
        
        createUniformBuffers();
        m_desc_pool = createDescPool();
        createDescSets();
        
        createCommandBuffers();
        createSyncObjects();
    }
    
    VkShaderModule CreateShaderModule(const std::vector<char>& buffer) {
        VkShaderModuleCreateInfo shader_module_info{};
        shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_info.codeSize = buffer.size();
        shader_module_info.pCode = reinterpret_cast<const u_int32_t*>(buffer.data());
        VkShaderModule shader_module = VK_NULL_HANDLE;
        VkResult result = vkCreateShaderModule(m_device, &shader_module_info, nullptr, &shader_module);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader!");
        }
        return shader_module;
    }
    
    VkRenderPass createRenderPass(VkSubpassDependency pass_dependency) {
        VkAttachmentDescription color_attachment{};
        color_attachment.format = m_swapchain_params.surface_format.format;
        color_attachment.samples = m_msaa_samples;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        VkAttachmentReference color_attachment_ref{};
        color_attachment_ref.attachment = 0u;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        VkAttachmentDescription depth_attachment{};
        depth_attachment.format = findDepthFormat();
        depth_attachment.samples = m_msaa_samples;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        VkAttachmentReference depth_attachment_ref{};
        depth_attachment_ref.attachment = 1;
        depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        VkAttachmentDescription color_attachment_resolve{};
        color_attachment_resolve.format = m_swapchain_params.surface_format.format;
        color_attachment_resolve.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment_resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment_resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment_resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment_resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment_resolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        
        VkAttachmentReference color_attachment_resolve_ref{};
        color_attachment_resolve_ref.attachment = 2;
        color_attachment_resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        VkSubpassDescription subpass_desc{};
        subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_desc.colorAttachmentCount = 1u;
        subpass_desc.pColorAttachments = &color_attachment_ref;
        subpass_desc.pDepthStencilAttachment = &depth_attachment_ref;
        subpass_desc.pResolveAttachments = &color_attachment_resolve_ref;
        
        std::array<VkAttachmentDescription, 3> attachments = {color_attachment, depth_attachment, color_attachment_resolve};
        
        VkRenderPassCreateInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        render_pass_info.pAttachments = attachments.data();
        render_pass_info.subpassCount = 1u;
        render_pass_info.pSubpasses = &subpass_desc;
        render_pass_info.dependencyCount = 1u;
        render_pass_info.pDependencies = &pass_dependency;
        
        VkRenderPass render_pass = VK_NULL_HANDLE;
        VkResult result = vkCreateRenderPass(m_device, &render_pass_info, nullptr, &render_pass);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
        
        return render_pass;
    }
    
    void loadShaders() {
        m_frag_shader_modeule = CreateShaderModule("shaders/frag.spv");    
        m_vert_shader_modeule = CreateShaderModule("shaders/vert.spv");
    }
    
    void createRenderPass() {
        VkSubpassDependency pass_dependency{};
        pass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        pass_dependency.dstSubpass = 0;
        pass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        pass_dependency.srcAccessMask = 0u;
        pass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        pass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        m_render_pass = createRenderPass(pass_dependency);
    }
    
    void createPipeline(VkShaderModule vert_shader_modeule, VkShaderModule frag_shader_modeule, VkRenderPass render_pass) {
        VkPipelineShaderStageCreateInfo frag_shader_info{};
        frag_shader_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_shader_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_shader_info.module = m_frag_shader_modeule;
        frag_shader_info.pName = "main";
        frag_shader_info.pSpecializationInfo = nullptr; 
    
        VkPipelineShaderStageCreateInfo vertex_shader_info{};
        vertex_shader_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertex_shader_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertex_shader_info.module = m_vert_shader_modeule;
        vertex_shader_info.pName = "main";
        vertex_shader_info.pSpecializationInfo = nullptr;
        VkPipelineShaderStageCreateInfo shader_stages[] = {frag_shader_info, vertex_shader_info};
        
        std::array<VkDynamicState, 2> dynamic_states = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        
        VkPipelineDynamicStateCreateInfo dynamic_state_info{};
        dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state_info.pDynamicStates = dynamic_states.data();
        
        auto binding_desc = Vertex::getBindingDescription();
        auto attribute_desc = Vertex::getAttributeDescritpions();
        
        VkPipelineDepthStencilStateCreateInfo depth_stencil_info{};
        depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_info.depthTestEnable = VK_TRUE;
        depth_stencil_info.depthWriteEnable = VK_TRUE;
        depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
        depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_info.minDepthBounds = 0.0f;
        depth_stencil_info.maxDepthBounds = 1.0f;
        depth_stencil_info.stencilTestEnable = VK_FALSE;
        depth_stencil_info.front = {};
        depth_stencil_info.back = {};
        
        VkPipelineVertexInputStateCreateInfo vertex_input_info{};
        vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_info.vertexBindingDescriptionCount = 1u;
        vertex_input_info.pVertexBindingDescriptions = &binding_desc;
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_desc.size());
        vertex_input_info.pVertexAttributeDescriptions = attribute_desc.data();
        
        VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
        input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly_info.primitiveRestartEnable = VK_FALSE;
        
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_swapchain_params.extent;
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)m_swapchain_params.extent.width;
        viewport.height = (float)m_swapchain_params.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        
        VkPipelineViewportStateCreateInfo viewport_state_info{};
        viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_info.viewportCount = 1u;
        viewport_state_info.pViewports = &viewport;
        viewport_state_info.scissorCount = 1u;
        viewport_state_info.pScissors = &scissor;
        
        VkPipelineRasterizationStateCreateInfo rasterizer_info{};
        rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer_info.depthBiasClamp = VK_FALSE;
        rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
        rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
        //rasterizer_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer_info.depthBiasEnable = VK_FALSE;
        rasterizer_info.depthBiasConstantFactor = 0.0f;
        rasterizer_info.depthBiasClamp = 0.0f;
        rasterizer_info.depthBiasSlopeFactor = 0.0f;
        rasterizer_info.lineWidth = 1.0f;
        
        VkPipelineMultisampleStateCreateInfo multisample_info{};
        multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_info.sampleShadingEnable = VK_FALSE;
        multisample_info.rasterizationSamples = m_msaa_samples;
        //multisample_info.minSampleShading = 1.0f;
        //multisample_info.pSampleMask = nullptr;
        //multisample_info.alphaToCoverageEnable = VK_FALSE;
        //multisample_info.alphaToOneEnable = VK_FALSE;
        
        VkPipelineColorBlendAttachmentState color_blend_state{};
        color_blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_state.blendEnable = VK_FALSE;
        color_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_blend_state.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_blend_state.alphaBlendOp = VK_BLEND_OP_ADD;
        
        VkPipelineColorBlendStateCreateInfo color_blend_info{};
        color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_info.logicOpEnable = VK_FALSE;
        color_blend_info.logicOp = VK_LOGIC_OP_COPY;
        color_blend_info.attachmentCount = 1u;
        color_blend_info.pAttachments = &color_blend_state;
        color_blend_info.blendConstants[0] = 0.0f;
        color_blend_info.blendConstants[1] = 0.0f;
        color_blend_info.blendConstants[2] = 0.0f;
        color_blend_info.blendConstants[3] = 0.0f;
         
        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 1u;
        pipeline_layout_info.pSetLayouts = &m_desc_set_layout;
        pipeline_layout_info.pushConstantRangeCount = 0u;
        pipeline_layout_info.pPushConstantRanges = nullptr;
        
        VkResult result = vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_pipeline_layout);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
        
        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = 2;
        pipeline_info.pStages = shader_stages;
        pipeline_info.pVertexInputState = &vertex_input_info;
        pipeline_info.pInputAssemblyState = &input_assembly_info;
        pipeline_info.pViewportState = &viewport_state_info;
        pipeline_info.pRasterizationState = &rasterizer_info;
        pipeline_info.pMultisampleState = &multisample_info;
        pipeline_info.pDepthStencilState = &depth_stencil_info;
        pipeline_info.pColorBlendState = &color_blend_info;
        pipeline_info.pDynamicState = &dynamic_state_info;
        pipeline_info.layout = m_pipeline_layout;
        pipeline_info.renderPass = render_pass;
        pipeline_info.subpass = 0u;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;
        
        result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_graphics_pipeline);
        
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
    }
    
    void createSwapchain() {
        m_swapchain_support_details = querySwapChainSupport(m_physical_device);
        
        m_swapchain_params.surface_format = chooseSwapSurfaceFormat(m_swapchain_support_details.formats);
        m_swapchain_params.present_mode = chooseSwapPresentMode(m_swapchain_support_details.present_modes);
        m_swapchain_params.extent = chooseSwapExtent(m_swapchain_support_details.capabilities);
        
        m_swapchain = createSwapchain(m_physical_device, m_device, m_swapchain_params);
        m_swapchain_images = getSwapchainImages(m_device, m_swapchain);
        
        m_swapchain_views = getImageViews(m_device, m_swapchain_images, m_swapchain_params.surface_format);
    }
    
    VkShaderModule CreateShaderModule(const std::string& path) {
        auto shader_buff = readFile(path);
        VkShaderModule shader_modeule = CreateShaderModule(shader_buff);
        return shader_modeule;
    }
    
    uint64_t getDeviceMaxMemoryLimit(VkPhysicalDevice device) {
        uint64_t max_memory_limit = 0;
        VkPhysicalDeviceMemoryProperties phys_device_mem_prop;
        vkGetPhysicalDeviceMemoryProperties(device, &phys_device_mem_prop);
        for (uint32_t i = 0u; i < phys_device_mem_prop.memoryHeapCount; ++i) {
            VkMemoryHeap mem_heap_prop = phys_device_mem_prop.memoryHeaps[i];
            max_memory_limit += mem_heap_prop.size;
        }
        return max_memory_limit;
    }
    
    int rateDeviceSuitability(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties device_props;
        vkGetPhysicalDeviceProperties(device, &device_props);
        
        VkPhysicalDeviceFeatures device_features;
        vkGetPhysicalDeviceFeatures(device, &device_features);
        
        int score = 0;
        
        if(device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }
        score += device_props.limits.maxImageDimension2D;
        
        uint64_t max_memory = getDeviceMaxMemoryLimit(device);
        score += (int)std::log2(max_memory);
        
        if(!device_features.geometryShader) {
            score += 1000;
        }
        if(isDeviceSuitable(device, m_surface)){
            return 0;
        }
        
        return score;
    }
    
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
        QueueFamilyIndices indices;
        
        uint32_t queue_family_count = 0u;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());
        
        for (int i = 0; const auto& queue_family : queue_families) {
            if(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphics_family = i;
            }
            if(queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                indices.transfer_family = i;
            }
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
            if(present_support) {
                indices.present_family = i;
            }
            
            if (indices.isComplete()) {
                break;
            }
            ++i;
        }
        
        return indices;
    }
    
    std::unordered_set<std::string> getDeviceExtensionSupported(VkPhysicalDevice device) {
        uint32_t extensions_count = 0u;
        VkResult result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensions_count, nullptr);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to read instance properties!");
        }
        std::vector<VkExtensionProperties> available_extensions(extensions_count);
        result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensions_count, available_extensions.data());
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to read instance properties!");
        }
    
        std::unordered_set<std::string> available_ext;
        available_ext.reserve(extensions_count);
        for(const VkExtensionProperties& prop : available_extensions) {
            available_ext.insert(std::string(prop.extensionName));
        }
        
        return available_ext;
    }
    
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes){
        for (const auto& available_present_mode : available_present_modes) {
            if(available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return available_present_mode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            int width = 0;
            int height = 0;
            glfwGetFramebufferSize(m_window, &width, &height);
            
            VkExtent2D actual_extent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };
            
            actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            
            return actual_extent;
        }
    }
    
    VkSwapchainKHR createSwapchain(VkPhysicalDevice physical_device, VkDevice logical_device, SwapchainParams swapchain_params) {
        uint32_t image_count = m_swapchain_support_details.capabilities.minImageCount + 1u;
        if(   m_swapchain_support_details.capabilities.maxImageCount > 0
           && image_count > m_swapchain_support_details.capabilities.maxImageCount
        ){
            image_count = m_swapchain_support_details.capabilities.maxImageCount;
        }
        
        VkSwapchainCreateInfoKHR swapchain_create_info{};
        swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_create_info.surface = m_surface;
        swapchain_create_info.minImageCount = image_count;
        swapchain_create_info.imageFormat = m_swapchain_params.surface_format.format;
        swapchain_create_info.imageColorSpace = m_swapchain_params.surface_format.colorSpace;
        swapchain_create_info.imageExtent = m_swapchain_params.extent;
        swapchain_create_info.imageArrayLayers = 1u;
        swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        
        QueueFamilyIndices queue_family_indices = findQueueFamilies(physical_device, m_surface);
        std::vector<uint32_t> family_indices = queue_family_indices.getIndices();
        if(queue_family_indices.graphics_family != queue_family_indices.present_family) {
            swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchain_create_info.queueFamilyIndexCount = 2u;
            swapchain_create_info.pQueueFamilyIndices = family_indices.data();
        }
        else {
            swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchain_create_info.queueFamilyIndexCount = 0u;
            swapchain_create_info.pQueueFamilyIndices = nullptr;
        }
        
        swapchain_create_info.preTransform = m_swapchain_support_details.capabilities.currentTransform;
        swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_create_info.presentMode = m_swapchain_params.present_mode;
        swapchain_create_info.clipped = VK_TRUE;
        swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
        
        VkSwapchainKHR swap_chain = VK_NULL_HANDLE;
        VkResult result = vkCreateSwapchainKHR(logical_device, &swapchain_create_info, nullptr, &swap_chain);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }
        
        
        return swap_chain;
    }
     
    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
        VkPhysicalDeviceFeatures supported_features{};
        vkGetPhysicalDeviceFeatures(device, &supported_features);
    
        QueueFamilyIndices queue_family_indices = findQueueFamilies(device, surface);
        
        std::vector<const char*> req_ext = getRequiredDeviceExtensions();
        std::unordered_set<std::string> extension_supported = getDeviceExtensionSupported(device);
        
        bool all_queue_families_supported = queue_family_indices.isComplete(); 
        bool all_device_ext_supported = checkNamesSupported(extension_supported, req_ext);
        
        bool swap_chain_adequate = false;
        if(all_device_ext_supported) {
            SwapchainSupportDetails swap_chain_details = querySwapChainSupport(device);
            swap_chain_adequate = !swap_chain_details.formats.empty() && !swap_chain_details.present_modes.empty();
        }
        
        bool result = all_queue_families_supported && all_device_ext_supported && swap_chain_adequate && supported_features.samplerAnisotropy;
        return result;
    }
    
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats) {
        for(const auto& available_format : available_formats) {
            if(available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return available_format;
            }
        }
        return available_formats[0];
    }
    
    std::vector<VkPhysicalDevice> getPhysicalDevices() {
        uint32_t devices_count = 0u;
        VkResult result = vkEnumeratePhysicalDevices(m_vk_instance, &devices_count, nullptr);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to enumerate devices!");
        }
        if(!devices_count) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        
        std::vector<VkPhysicalDevice> devices(devices_count);
        result = vkEnumeratePhysicalDevices(m_vk_instance, &devices_count, devices.data());
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to enumerate devices!");
        }
        
        return devices;
    }
    
    VkPhysicalDevice pickPhysicalDevice() {        
        std::vector<VkPhysicalDevice> devices = getPhysicalDevices();
        
        std::multimap<int, VkPhysicalDevice> candidates;
        for(const auto& device : devices){
            int score = rateDeviceSuitability(device);
            candidates.insert(std::make_pair(score, device));
        }
        
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        if (candidates.size() > 0u) {
            physical_device = candidates.rbegin()->second;
        }
        else {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
        m_available_device_ext = getDeviceExtensionSupported(physical_device);
        
        return physical_device;
    }
    
    VkSurfaceKHR createSurface() {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkResult result = glfwCreateWindowSurface(m_vk_instance, m_window, nullptr, &surface);
        if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
        return surface;
    }
    
    SwapchainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapchainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);
        
        uint32_t format_count = 0u;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);
        if(format_count) {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, details.formats.data());
        }
        
        uint32_t present_mode_count = 0u;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, nullptr);
        if(present_mode_count) {
            details.present_modes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, details.present_modes.data());
        }
        
        return details;
    }
    
    VkDevice createLogicalDevice(VkPhysicalDevice physical_device, QueueFamilyIndices queue_family_indices) {
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        const auto& family_indices = queue_family_indices.getFamilies();
        float queue_priority = 1.0f;
        for(uint32_t family_index : family_indices) {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = family_index;
            queue_create_info.queueCount = 1u;
            queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }
        
        VkPhysicalDeviceFeatures device_features{};
        device_features.samplerAnisotropy = VK_TRUE;
        
        VkDeviceCreateInfo device_create_info{};
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.pQueueCreateInfos = queue_create_infos.data();
        device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
        device_create_info.pEnabledFeatures = &device_features;
        
        std::vector<const char*> device_ext = getRequiredDeviceExtensions();
        device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_ext.size());
        device_create_info.ppEnabledExtensionNames = device_ext.data();
        
        std::vector<const char*> validation_layers = getRequiredValidationLayers();
        if(ENABLE_VALIDATION_LAYERS) {
            device_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            device_create_info.ppEnabledLayerNames = validation_layers.data();
        }
        else {
            device_create_info.enabledLayerCount = 0u;
        }
        
        VkDevice device;
        VkResult result = vkCreateDevice(physical_device, &device_create_info, nullptr, &device);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }
        
        return device;
    }
    
    void cleanupSwapchain() {
        vkDestroyImageView(m_device, m_color_image_view, nullptr);
        vkDestroyImage(m_device, m_color_image, nullptr);
        vkFreeMemory(m_device, m_color_image_memory, nullptr);
    
        vkDestroyImageView(m_device, m_depth_view, nullptr);
        vkDestroyImage(m_device, m_depth_image, nullptr);
        vkFreeMemory(m_device, m_depth_memory, nullptr);
    
        size_t sz = m_swapchain_framebuffers.size();
        for(size_t i = 0u; i < sz; ++i) {
            vkDestroyFramebuffer(m_device, m_swapchain_framebuffers[i], nullptr);
        }
        for(size_t i = 0u; i < sz; ++i) {
            vkDestroyImageView(m_device, m_swapchain_views[i], nullptr);
        }
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    }
    
    void recreateSwapchain() {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_window, &width, &height);
            glfwWaitEvents();
        }
    
        vkDeviceWaitIdle(m_device);
        
        cleanupSwapchain();
        vkDestroyRenderPass(m_device, m_render_pass, nullptr);
        vkDestroyPipeline(m_device, m_graphics_pipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
        for(size_t i = 0u; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroySemaphore(m_device, m_image_available[i], nullptr);
            vkDestroySemaphore(m_device, m_render_finished[i], nullptr);
            vkDestroyFence(m_device, m_in_flight_frame[i], nullptr);
        }
        
        createSwapchain();
        createColorResources();      
        createDepthResources();
        //m_swapchain_views = getImageViews(m_device, m_swapchain_images, m_swapchain_params.surface_format);
        createRenderPass();
        createPipeline(m_vert_shader_modeule, m_frag_shader_modeule, m_render_pass);
        createFramebuffers(m_swapchain_views, m_swapchain_params.extent, m_render_pass);
        createSyncObjects();
    }
    
    void update_frame(uint32_t current_image) {
        static auto start_time = std::chrono::high_resolution_clock::now();
        
        auto current_time = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
        float angle = dt * glm::radians(90.f);
        glm::vec3 rotation_axis = glm::vec3(0.0f, 0.0f, 1.0f);
        float aspect = (float)m_swapchain_params.extent.width / (float)m_swapchain_params.extent.height;
        
        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), angle, rotation_axis);
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1.0f;
        
        memcpy(m_uniform_mapped[current_image], &ubo, sizeof(ubo));
        
        
    }
    
    void drawFrame() {
        vkWaitForFences(m_device, 1u, &m_in_flight_frame[m_current_frame], VK_TRUE, UINT64_MAX);
        
        uint32_t image_index;
        VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_image_available[m_current_frame], VK_NULL_HANDLE, &image_index);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapchain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
    
        // Only reset the fence if we are submitting work
        vkResetFences(m_device, 1u, &m_in_flight_frame[m_current_frame]);
        
        update_frame(m_current_frame);
        
        vkResetCommandBuffer(m_command_buffers[m_current_frame], 0u);
        recordCommandBuffer(m_command_buffers[m_current_frame], image_index);
        
        VkSemaphore render_end_semaphores[] = {m_render_finished[m_current_frame]};
        VkSemaphore wait_semaphores[] = {m_image_available[m_current_frame]};
        VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 1u;
        submit_info.pWaitSemaphores = wait_semaphores;
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1u;
        submit_info.pCommandBuffers = &m_command_buffers[m_current_frame];
        submit_info.signalSemaphoreCount = 1u;
        submit_info.pSignalSemaphores = render_end_semaphores;
        
        result = vkQueueSubmit(m_graphics_queue, 1u, &submit_info, m_in_flight_frame[m_current_frame]);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
        
        VkSwapchainKHR swapchains[] = {m_swapchain};
        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1u;
        present_info.pWaitSemaphores = render_end_semaphores;
        present_info.swapchainCount = 1u;
        present_info.pSwapchains = swapchains;
        present_info.pImageIndices = &image_index;
        present_info.pResults = nullptr;
        result = vkQueuePresentKHR(m_present_queue, &present_info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebuffer_resized) {
            m_framebuffer_resized = false;
            //recreateSwapchain();
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
        
        m_current_frame = (m_current_frame + 1u) % MAX_FRAMES_IN_FLIGHT;
    }

    void mainLoop() {
        while(!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
            drawFrame();
        }
        
        vkDeviceWaitIdle(m_device);
    }

    void cleanup() {
        cleanupSwapchain();
        
        vkDestroySampler(m_device, m_texture_sampler, nullptr);
        vkDestroyImageView(m_device, m_texture_view, nullptr);
        vkDestroyImage(m_device, m_texture_image, nullptr);
        vkFreeMemory(m_device, m_texture_memory, nullptr);
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(m_device, m_uniform_buffers[i], nullptr);
            vkFreeMemory(m_device, m_uniform_memory[i], nullptr);
        }
        
        vkDestroyDescriptorPool(m_device, m_desc_pool, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_desc_set_layout, nullptr);
        
        vkDestroyBuffer(m_device, m_vertex_buffer, nullptr);
        vkFreeMemory(m_device, m_vertex_memory, nullptr);
        vkDestroyBuffer(m_device, m_index_buffer, nullptr);
        vkFreeMemory(m_device, m_index_memory, nullptr);
        
        vkDestroyRenderPass(m_device, m_render_pass, nullptr);
        vkDestroyPipeline(m_device, m_graphics_pipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
        for(size_t i = 0u; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroySemaphore(m_device, m_image_available[i], nullptr);
            vkDestroySemaphore(m_device, m_render_finished[i], nullptr);
            vkDestroyFence(m_device, m_in_flight_frame[i], nullptr);
        }
        if(m_grapics_cmd_pool != m_transfer_cmd_pool) {
            vkDestroyCommandPool(m_device, m_grapics_cmd_pool, nullptr);
            vkDestroyCommandPool(m_device, m_transfer_cmd_pool, nullptr);
        }
        else {
            vkDestroyCommandPool(m_device, m_grapics_cmd_pool, nullptr);
        }
        vkDestroyShaderModule(m_device, m_frag_shader_modeule, nullptr);
        vkDestroyShaderModule(m_device, m_vert_shader_modeule, nullptr);
        vkDestroyDevice(m_device, nullptr);
        if (ENABLE_VALIDATION_LAYERS) {
            DestroyDebugUtilsMessengerEXT(m_vk_instance, m_debug_messenger, nullptr);
        }
        vkDestroySurfaceKHR(m_vk_instance, m_surface, nullptr);
        vkDestroyInstance(m_vk_instance, nullptr);
        
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
