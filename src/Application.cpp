#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include "log.h"

const int WIDTH = 800;
const int HEIGHT = 600;

class HelloTriangleApplication
{
private:
	GLFWwindow* m_window;
	VkInstance m_vkInstance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	bool cleanupStarted;
public:
	HelloTriangleApplication()
		:
		m_window(nullptr),		
		cleanupStarted(true)
	{}

	bool isCleanupStarted() { return cleanupStarted; }
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
	}

	void initVulkan()
	{
		createInstance();
		setupDebugMessanger();
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		cleanupStarted = true;
#if _DEBUG
		DestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);
#endif // _DEBUG
		vkDestroyInstance(m_vkInstance, nullptr);

		glfwDestroyWindow(m_window);

		glfwTerminate();
	}

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
#if _DEBUG
		// Enable Validation Layers
		const std::vector<const char*> validationLayers =
		{
			"VK_LAYER_KHRONOS_validation"
		};
		if (!checkValidationLayerSupport(validationLayers))
		{
			throw std::runtime_error("validation layers requested, but not available!");
		}
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
#else
		createInfo.enabledLayerCount = 0;
#endif // _DEBUG
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// GLFW Instance extensions request setup
		std::vector<const char*> extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		// Instance Creation
		if (vkCreateInstance(&createInfo, nullptr, &m_vkInstance) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create instance!");
		}
#if _DEBUG
		FLog(NULL,"VK_INSTANCE created!");
#endif
	}
	void setupDebugMessanger()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
#if _DEBUG
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
									 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;		
#else
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
#endif // _DEBUG
		VkResult result = CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_debugMessenger);
		if ( result != VK_SUCCESS) 
		{			
			throw FRuntimeErr("Failed to set up debug messanger! ERROR_C: {:d}", result);
		}
		cleanupStarted = false;
		
#if _DEBUG
		FLog(0, "VK_DebugMessenger callback binded!");
#endif // _DEBUG
	}
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
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
				printf("Layer %s not found!\n", layerName);
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
			case  VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
#if _DEBUG
				|| VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
#endif // _DEBUG
				:
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
		FLog(errortype, "{}", pCallbackData->pMessage);
		return VK_FALSE;
	}
};

int main() 
{
	HelloTriangleApplication app;

	try 
	{
		app.run();
	}
	catch (const std::exception & e) 
	{
		if (!app.isCleanupStarted())
		{
			app.emergencyCleanup();
		}
		FLog(3, "{}", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}