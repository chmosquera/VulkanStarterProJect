#define GLFW_include_vulkan
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <optional>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifndef NDEBUG
    const bool enableValidationLayers = false;
    #define DEBUG_PRINT() std::cout << "Not debug macro" << '\n';
#else
    const bool enableValidationLayers = true;
    #define DEBUG_PRINT() std::cout << "Debug macro" << '\n';
#endif

bool checkValidationLayerSupport() {
    DEBUG_PRINT()
    
    // Get number of layers before getting the layer data by passing in nullptr.
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    // Now get the available layers
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
    for (const char* layerName : validationLayers) {
        bool layerFound = false;
        
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        
        if (layerFound == false) {
            return false;
        }
    }
    
    return true;
}

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    
    GLFWwindow * window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    
    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        
        window = glfwCreateWindow(WIDTH, HEIGHT, "Welcome to Vulkan", nullptr, nullptr);
    }
    
    void initVulkan() {
        createInstance();
        pickPhysicalDevice();
    }
    
    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        
        // Specify the queues to create.
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;
        
        float queuePriority = 1.0f; // Queues must have priotity set, even if only one queue.
        queueCreateInfo.pQueuePriorities = &queuePriority;
        
        // Specify set of device features to use.
        // Currently, we don't need anything special, so default to false.
        VkPhysicalDeviceFeatures deviceFeatures{};
        
        // Create the logical device.
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        
        createInfo.pEnabledFeatures = &deviceFeatures;
        
        createInfo.enabledExtensionCount = 0;
        
        // Similar to when creating a vk instance, set up validation layers.
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }
        
        // Create the device
        VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Logical device was not created.");
        }
        
        // Presume that queue index is '0' because we're only creating 1 queue.
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    }
    
    // Get the first device and assign it as our Vulkan Physical Device
    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        
        if (deviceCount == 0) {
            throw std::runtime_error("GPUs with Vulkan support were not found.");
        }
        
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        
        // Iterate through list of devices and check if suitable.
        // Use the first suitable device as our Vulkan Physical Device.
        for (const auto& device : devices) {
            if (isDeviceSuitable(device) == true) {
                physicalDevice = device;
                break;
            }
        }
        
        // Throw an error if no physical device was found.
        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("A physical device was not found.");
        }
        
    }
    
    // Checks if the physical device is suitable to run this application.
    // Currently, we don't need any specific GPU properties or features, so
    // any device is suitable.
    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);
        return indices.isComplete();
    }
    
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        
        bool isComplete() {
            return graphicsFamily.has_value();
        }
        
    };
    
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        
        std::vector<VkQueueFamilyProperties> properties(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());
        
        // Iterate through properties and check for graphics bit
        for (uint32_t i = 0; i < count; i++) {
            if (indices.isComplete() == true) {
                break;
            }
            if ((properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
                indices.graphicsFamily = i;
            }
        }
        
        return indices;
    }

    void mainLoop() {
        while (glfwWindowShouldClose(window) == false) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    
    void createInstance() {
        
        if (enableValidationLayers && checkValidationLayerSupport() == false) {
            throw std::runtime_error("The validation layers you requested are not available.");
        }
        
        // Optional. Provides data to the driver to optimize our application.
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 1);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;
        
        // Required info about the instance.
        VkInstanceCreateInfo createInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        
        // Extensions are necessary to interface the hardware API to window system
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
                
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        
        // Required for MacOS to avoid VK_ERROR_INCOMPATIBLE_DRIVER
        std::vector<const char*> requiredExtensions;
        for (uint32_t i = 0; i < glfwExtensionCount; i++) {
            requiredExtensions.emplace_back(glfwExtensions[i]);
        }
        requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        createInfo.enabledExtensionCount = (uint32_t) requiredExtensions.size();
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();
        // ---
        
        // Provide data about extensions to user
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        std::cout << "Available extensions:\n";
        for (const auto& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }
        
        createInfo.enabledLayerCount = 0;
        
        // Create Vulkan instance
        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        if ( result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create instance.");
        }
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
