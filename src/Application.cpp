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

const int WIDTH = 800;
const int HEIGHT = 600;

class HelloTriangleApplication
{
private:
	GLFWwindow* m_window;
	VkInstance m_vkInstance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_logicalDevice = VK_NULL_HANDLE;
	VkQueue m_graphicsQueue; 
	VkQueue m_presentQueue; // presentation queue
	VkSurfaceKHR m_surface;
public:
	HelloTriangleApplication()
		:
		m_window(nullptr)
	{}

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
		CLog(0, "initWindow: success.");

	}

	void initVulkan()
	{
		createInstance();
		setupDebugMessanger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		CLog(0, "initVulkan: success.");
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
		vkDestroyDevice(m_logicalDevice, nullptr);
#if _DEBUG
		DestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);
#endif // _DEBUG
		vkDestroySurfaceKHR(m_vkInstance, m_surface, nullptr);
		vkDestroyInstance(m_vkInstance, nullptr);

		glfwDestroyWindow(m_window);

		glfwTerminate();
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
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());

		std::multimap<uint32_t, VkPhysicalDevice> candidates;
		
		for (const auto& device : devices)
		{
			candidates.emplace(rateDeviceSuitability(device), device);
		}
		CVerifyCrash(candidates.size() != 0, "No valid Physical Devices found!");
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

		createInfo.enabledExtensionCount = 0;

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
		CVerifyCrash(indices.isComplete(), "QueueFamilies doesnt suport desired queue functionality!");
		return indices;
	}
	bool isDeviceSuitable(VkPhysicalDevice _physicalDevice)
	{
		QueueFamilyIndices indicies = findQueueFamilies(_physicalDevice);
		return indicies.isComplete();
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
	/*
	bool isDeviceSuitable(VkPhysicalDevice& _device)
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(_device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(_device, &deviceFeatures);
		
		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			   deviceFeatures.geometryShader;

	}
	*/
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