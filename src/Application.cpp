#include "Core.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan_win32.h>

#include <iostream>
#include <cstdlib>
#include <vector>
#include <map>
#include <optional>
#include <set>
#include <cstdint> // Necessary for UINT32_MAX

const int WIDTH = 800;
const int HEIGHT = 600;

class HelloTriangleApplication
{
private:
	GLFWwindow* m_window; // the window "handle"
	VkInstance m_vkInstance; // Vulkan works of instances
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_logicalDevice = VK_NULL_HANDLE;
	VkQueue m_graphicsQueue;	// graphics queue
	VkQueue m_presentQueue;		// presentation queue
	VkSurfaceKHR m_surface;		// rendering window view

	std::vector<VkImage> m_swapChainImages;			// each individual image
	VkSwapchainKHR m_swapChain;						// queues of images that are awaiting to be drawn on screen, synchronize with refresh rate of the screen.
	VkFormat m_swapChainImageFormat;				// color format and colorSpace (linear or non-linear gamma)
	VkExtent2D m_swapChainExtent;					// "extents" of the buffer. (width and height of the surface
	std::vector<VkImageView> m_swapChainImageViews; // schematic on how to access a single image on the swap chain

	const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME }; // Add desired extensions here


public:
	HelloTriangleApplication()
		: m_window(nullptr)	{}

	void emergencyCleanup()
	{
		cleanup();
	}
	void run()
	{
		initWindow();
		initVulkan();
		
		mainLoop();
		cleanup();
	}
private:

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_window = glfwCreateWindow(WIDTH, HEIGHT, "3D_Sandbox", nullptr, nullptr);
		CVerifyCrash(m_window != nullptr, "GLFW window not succesfully created!");
	}

	void initVulkan()
	{
		createInstance();
		setupDebugMessanger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		CLog(0, "initVulkan: Success.");
	}
	void createImageViews()
	{
		m_swapChainImageViews.resize(m_swapChainImages.size());
		for (size_t i = 0; i < m_swapChainImages.size(); i++)
		{
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_swapChainImages[i];

			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_swapChainImageFormat;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			VkResult result = vkCreateImageView(m_logicalDevice, &createInfo, nullptr, &m_swapChainImageViews[i]);
			CVerifyCrash(result == VK_SUCCESS, "Failed to create Image view for index: {}. Result: {}", i, result);
		}

	}

	void createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		m_swapChainImageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		m_swapChainExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily)
		{
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
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;//VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // default = identity matrix
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = presentMode;

		createInfo.clipped = VK_TRUE; createInfo.oldSwapchain = VK_NULL_HANDLE; // change this when window gets resized
		VkResult result = vkCreateSwapchainKHR(m_logicalDevice, &createInfo, nullptr, &m_swapChain);
		CVerifyCrash(result == VK_SUCCESS, "Swapchain failed to create! Result: {0:d}", result);


		vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, nullptr);
		m_swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());

		CDebugLog(0, "Create Swapchain: Success.");

	}

	void mainLoop()
	{
		CLog(0, "mainloop: Start.");
		while (!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		for (auto it : m_swapChainImageViews)
		{
			vkDestroyImageView(m_logicalDevice, it, nullptr);
		}

		vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);
		vkDestroyDevice(m_logicalDevice, nullptr);
#if _DEBUG
		DestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);
#endif // _DEBUG
		vkDestroySurfaceKHR(m_vkInstance, m_surface, nullptr);
		vkDestroyInstance(m_vkInstance, nullptr);

		glfwDestroyWindow(m_window);

		glfwTerminate();
	}

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice _physicalDevice)
	{
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, m_surface, &formatCount, nullptr);
		SwapChainSupportDetails details;

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, m_surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, m_surface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, m_surface, &presentModeCount, details.presentModes.data());
		}

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, m_surface, &details.capabilities);

		return details;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats)
	{
		for (const auto& it : _availableFormats)
		{
			if (it.format == VK_FORMAT_B8G8R8A8_SRGB && it.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return it;
			}
		}
		CLog(1, "B8G8R8A8_SRGB_NONLINEAR swapchain format not found! Defaulting to first format.");
		return _availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availablePresentModes)
	{
		for (const auto& it : _availablePresentModes)
		{
			if (it == VK_PRESENT_MODE_MAILBOX_KHR) // triple buffering vsync
				return it;
		}
		CLog(1, "Presentation mode set to default: TRIPLE_BUFFER_VSYNC");
		return VK_PRESENT_MODE_FIFO_KHR; // guaranteed presentation mode. apparently. vsync
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR _capabilities)
	{
		if (_capabilities.currentExtent.width != UINT32_MAX)
		{
			return _capabilities.currentExtent;
		}
		else
		{
			VkExtent2D actualExtent{ WIDTH, HEIGHT };

			actualExtent.width = std::max(_capabilities.minImageExtent.width, std::min(_capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(_capabilities.minImageExtent.height, std::min(_capabilities.maxImageExtent.height, actualExtent.height));
			return actualExtent;
		}
	}














	void createSurface()
	{
		VkResult result = glfwCreateWindowSurface(m_vkInstance, m_window, nullptr, &m_surface);
		CVerifyCrash(result == VK_SUCCESS, "failed to create VK_Surface! {:d}", result);
	}
	void pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount,nullptr);
		CVerifyCrash(deviceCount != 0, "Failed to obtain physical devices.");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());

		std::multimap<uint32_t, VkPhysicalDevice> candidates;
		
		for (const auto& device : devices)
		{
			if (isDeviceSuitable(device))
			{
				candidates.emplace(rateDeviceSuitability(device), device);
			}
		}
		CVerifyCrash(candidates.size() != 0, "No suitable Physical Devices found!");
		m_physicalDevice = candidates.rbegin()->second;
#if _DEBUG
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);
		CLog(0,"Selected: {}", deviceProperties.deviceName);
#endif		
	}
	void createLogicalDevice()
	{
		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };


		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	  // Device specific validation layers are deprecated and the instance created layers are used instead, the following set up is for backwards compatibility. 
#if _DEBUG
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
#else // Backwards compatibility end
		createInfo.enabledLayerCount = 0;
#endif

		VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice);
		CVerifyCrash(result == VK_SUCCESS, "failed to create VK_LogicalDevice! {:d}", result);

		// Retrieve queue handles
		vkGetDeviceQueue(m_logicalDevice, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_logicalDevice, indices.presentFamily.value(), 0, &m_presentQueue);

		CDebugLog(0, "VK_Device created!");
	}
	struct QueueFamilyIndices
	{
		// the uint32_t m_variables are associated with the queue that supports that call type
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
		bool isComplete() // All required device queues are accounted for
		{
			return this->graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	// Looks like a long winded way of getting a valid queueFamily, but it's necessary for a chapter in presentation.
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice _physicalDevice)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilyVec(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, queueFamilyVec.data());

		for (uint32_t i = 0; i< queueFamilyVec.size(); i++)
		{
			if (queueFamilyVec[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) // Does the queue support Graphics Bit?
			{
				indices.graphicsFamily = i;
			}			
			VkBool32 presentSupport;
			vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, i, m_surface, &presentSupport);
			if (presentSupport) // Does the queue support presentation queue?
			{
				indices.presentFamily = i;
			}
			if (indices.isComplete()) // Break if queues are accounted for
			{
				break;
			}
			i++;
		}
		CVerifyCrash(indices.isComplete(), "QueueFamilies doesnt support desired queue functionality!");
		return indices;
	}
	bool isDeviceSuitable(VkPhysicalDevice _physicalDevice)
	{
		QueueFamilyIndices indicies = findQueueFamilies(_physicalDevice);

		bool swapChainAdequate = false;
		if (checkDeviceExtentionSupport(_physicalDevice))
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_physicalDevice);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}
		
		return indicies.isComplete() && swapChainAdequate /* implicit: && extentionSupported as it's included with swapchainAdequate*/;
	}
	struct CheckExtentionHelper
	{
	public:
		CheckExtentionHelper(const std::vector<const char*>& _vec)
			: deviceExtensions(_vec), bfoundExtents(std::vector<bool>(_vec.size(), false))
		{}
		void changeBTrue(uint32_t i)
		{
			bfoundExtents[i] = true;
		}
		void setIfAllFound()
		{
			for (auto bfextent : bfoundExtents)
			{
				if (bfextent == false)
				{
					m_isAllFound = false;
					return;
				}
			}
			m_isAllFound = true;
		}
		bool isAllFound()
		{
			setIfAllFound();
			return m_isAllFound;
		}

		const std::vector<const char*>& deviceExtensions;
	private:
		bool m_isAllFound;
		std::vector<bool> bfoundExtents;
	};
	bool checkDeviceExtentionSupport(VkPhysicalDevice _physicalDevice)
	{
		CheckExtentionHelper extentHelp(deviceExtensions);
		

		uint32_t extentionCount;
		vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extentionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtentions(extentionCount);
		vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extentionCount, availableExtentions.data());

		
		for (auto avExt : availableExtentions)
		{
			for (uint32_t i = 0; i < extentHelp.deviceExtensions.size(); i++)
			{
				if (strcmp(avExt.extensionName, deviceExtensions[i]) == 0)
				{
					extentHelp.changeBTrue(i);
				}
			}
			if (extentHelp.isAllFound())
			{
				return true;
			}
		}	

		return false;		
	}

	uint32_t rateDeviceSuitability(const VkPhysicalDevice& _device)
	{

		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(_device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(_device, &deviceFeatures);

		// Must include geometry shader
		if (!deviceFeatures.geometryShader)
		{
			return 0;
		}

		uint32_t score = 0;
		score += deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 1000 : 0;
		score += deviceProperties.limits.maxImageArrayLayers;

		return score;
	}

	// Enable Validation Layers
	const std::vector<const char*> validationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};
	void createInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

#if _DEBUG
		
		CVerifyCrash(checkValidationLayerSupport(validationLayers), "Validation layers requested, but not available!");
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();	

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else
		createInfo.enabledLayerCount = 0;
#endif // _DEBUG		

		// GLFW Instance extensions request setup
		std::vector<const char*> extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		// Instance Creation
		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
		CVerifyCrash(result== VK_SUCCESS, "failed to create VK_Instance! {:d}", result);
		
#if _DEBUG
		CLog(0,"VK_Instance created!");
#endif
	}
	void setupDebugMessanger()
	{
#if !_DEBUG
		return;
#endif // _DEBUG
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);
		
		VkResult result = CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_debugMessenger);
		CVerifyCrash((result == VK_SUCCESS), "Failed to set up debug messanger! Error: {:d}", result);
		CLog(0, "VK_DebugMessenger callback binded!");

	}
	// used for the debug messenger of vkInstance creation/destruction
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
	{
		createInfo = {}; // initialize the struct...
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.flags = (VkFlags)0;
#if _DEBUG
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
									 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
									 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT   ;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
								 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
#else
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
#endif // _DEBUG
	}

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) 
		{
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else 
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) 
		{
			func(instance, debugMessenger, pAllocator);
			CLog(0,"VK_Debug_Utils: Destroyed!");
		}
	}
	

	bool checkValidationLayerSupport(const std::vector<const char*>& _validationLayers)
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : _validationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) 
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}
			if (!layerFound) 
			{
				CLog(3, "Validation layer {:s} not found!", layerName);
				return false;
			}
		}
		return true;
	}
	std::vector<const char*> getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#if _DEBUG
		
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		
#endif // _DEBUG
		return extensions;
	}
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) 
	{
		int errortype = -1;
		switch (messageSeverity)
		{
		case  VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
#if _DEBUG
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
#endif // _DEBUG
				errortype = 0;
					break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				errortype = 1;
				break;			
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				errortype = 2;
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
				errortype = 3;
				break;
		}
		CLog(errortype, "{}", pCallbackData->pMessage);
		return VK_FALSE;
	}
};

int main() 
{	
	HelloTriangleApplication app;

	app.run();
	
	return EXIT_SUCCESS;
}