#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <set>
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
    VkSurfaceKHR surface;
    VkQueue graphicsQueue;
    VkQueue presentationQueue;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    
    
    void initWindow() {
        
        int result = glfwInit();
        if (result == GLFW_FALSE) {
            throw std::runtime_error("GLFW could not initialize.");
        }
        
        result = glfwVulkanSupported();
        if (result == GLFW_FALSE) {
            throw std::runtime_error("Vulkan is not available. Run this application on a machine that supports Vulkan.");
        }
        
        int major, minor, revision;
        glfwGetVersion(&major, &minor, &revision);
        std::cout << "GLFW Version " << major << "." << minor << "." << revision << '\n';
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        
        window = glfwCreateWindow(WIDTH, HEIGHT, "Welcome to Vulkan", nullptr, nullptr);
    }
    
    void initVulkan() {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
    }
    
    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        
        // Iterate over all swap chain images
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            
            VkResult result = vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]);
            if (result != VK_SUCCESS) {
                throw std::runtime_error("Image view was not created.");
            }
        }
    }
    
    void createSwapChain() {
        SwapChainSupportDetails details = querySwapChainSupport(physicalDevice);
        
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(details.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(details.presentModes);
        VkExtent2D extent = chooseSwapExtent(details.capabilities);
        
        uint32_t imageCount = details.capabilities.minImageCount + 1;
        if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
            imageCount = details.capabilities.maxImageCount;
        }
        
        VkSwapchainCreateInfoKHR createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.presentMode = presentMode;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentationFamily.value()};
        
        // `imageSharingMode` describes how an image is shared across multiple queue families. If the sharing
        // mode is VK_SHARING_MODE_CONCURRENT, then we must also define the number of queue families sharing
        // the image (queueFamilyIndexCount) and the list of queue families (pQueueFamilyIndices).
        if (indices.graphicsFamily != indices.presentationFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;       // Optional
            createInfo.pQueueFamilyIndices = nullptr;   // Optional
        }
        
        createInfo.preTransform = details.capabilities.currentTransform; // Don't change transform
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Ignore alpha channel
        createInfo.clipped = VK_TRUE; // Clip pixels that are obscured from view
        createInfo.oldSwapchain = VK_NULL_HANDLE;
        
        VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Swap chain was not created. Error code " + std::to_string(result));
        }
        
        // Get handles to swap chain images
        uint32_t swapChainImageCount;
        vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, nullptr);
        swapChainImages.resize(swapChainImageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, swapChainImages.data());
        
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
        
    }
    
    void createSurface() {
        int result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Vulkan surface was not created. Error code " + std::to_string(result));
        }
    }
    
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    
    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        
        // Holds creation info for each queue family required by the device.
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        
        // Unique queue family indices required by this program.
        std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(),
            indices.presentationFamily.value(),
        };
        
        float queuePriority = 1.0f; // Queues must have priotity set, even if only one queue.
        for (const auto& queueFamily : uniqueQueueFamilies) {
            // Specify the queues to create for this queue family.
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
                
        // Create the logical device.
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        
        // Specify the device features to use with this queue family.
        // Currently, we don't need anything special, so default to false.
        VkPhysicalDeviceFeatures deviceFeatures{};
        createInfo.pEnabledFeatures = &deviceFeatures;
        
        // Enables device extensions
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        
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
        
        // Presume that queue index is '0' because we're only creating 1 queue for each queue family.
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentationFamily.value(), 0, &presentationQueue);
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
    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);
        
        bool extensionSupported = checkDeviceExtensionSupport(device);
                
        bool swapChainAdequate = false;
        if (extensionSupported) {
            SwapChainSupportDetails swapchain = querySwapChainSupport(device);
            swapChainAdequate = !swapchain.formats.empty() && !swapchain.presentModes.empty();
        }
        
        
        return indices.isComplete() && extensionSupported && swapChainAdequate;
    }
    
    // Checks if the device has the required extensiosn.
    // Enumerates the device's extensions, then checks if required extensions are among the list.
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
        
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
        
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }
        
        bool result = requiredExtensions.empty();
        
        return result;
    }
    
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentationFamily;
        
        bool isComplete() {
            return graphicsFamily.has_value() && presentationFamily.has_value();
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
            
            VkBool32 presentationSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
            if (presentationSupport == true) {
                indices.presentationFamily = i;
            }
        }
        
        return indices;
    }
    
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        
        SwapChainSupportDetails details = {};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
        
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount > 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }
        
        uint32_t modeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, nullptr);
        if (modeCount > 0) {
            details.presentModes.resize(modeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, details.presentModes.data());
        }
        
        return details;
    }
    
    
    /// Searches for a surface with the preferred properties: `VK_FORMAT_B8G8R8A8_SRGB` format and `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR` color space.
    /// If format and color space is not available, then the first available surface format in the list is chosen.
    /// - Parameter availableFormats: The list of swap chain surface formats available in the device. Passing in as a constant prevents modification.
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& format : availableFormats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
        return availableFormats[0];
    }
    
    
    /// Searches for a swap chain with `VK_PRESENT_MODE_MAILBOX_KH` presentation mode. If the preferred presentation mode is not available,
    /// `VK_PRESENT_MODE_FIFO_KHR` is chosen by default because it's guaranteed to be available.
    /// - Parameter availablePresentModes: The list of presentation modes available in the device. Passing in as a constant prevents
    ///  modification of the list.
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
        
        for (const auto& presentMode : availablePresentModes) {
            if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return presentMode;
            }
        }
        
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    
    /// Specifies the swap chain's image resolution, or "extent". If the `currentExtent` is set to a "special value", which is the maximum value
    /// for an unsigned 32-bit integer, then the actual resolution must be calculated. Otherwise, `currentExtent` is already the optimal resolution
    /// that the window manager specified. To calculate the actual resolution, the GLFW's frame buffer's size is clamped between the swap chain's
    /// min/max image extents.
    /// - Parameter capabilities: The swap chain's surface capabilities.
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            
            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height),
            };
        
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            
            return actualExtent;
        }
    }
    

    void mainLoop() {
        while (glfwWindowShouldClose(window) == false) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
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
