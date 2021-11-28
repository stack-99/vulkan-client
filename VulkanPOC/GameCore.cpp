#include "GameCore.hpp"

#include <set>
#include <cstdint> // Necessary for UINT32_MAX
#include <algorithm> // Necessary for std::min/std::max

void 
GameCore::initWindow()
{
	glfwInit();

	// Options
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	this->window = glfwCreateWindow(this->width, this->height, "Vulkan", nullptr, nullptr);

	if (!this->window)
		throw std::runtime_error("Window could not be initialized");
}

bool 
GameCore::areValidationLayersAvailable() {
	uint32_t count;
	vkEnumerateInstanceLayerProperties(&count, nullptr);

	std::vector<VkLayerProperties> props(count);

	vkEnumerateInstanceLayerProperties(&count, props.data());

	for (const auto& layer : this->validationLayers)
	{
		bool layerExists = false;


		for (const auto& prop : props)
		{
			if (strcmp(prop.layerName, layer) == 0)
			{
				layerExists = true;
				break;
			}
		}


		if (!layerExists)
			return false;
	}

	return true;
}

void 
GameCore::showAvaialbleExtensions()
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);

	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	std::cout << "Available extensions";

	for (const auto& ext : extensions)
	{
		std::cout << '\t' << ext.extensionName << '\n';
	}
}

std::vector<const char*> 
GameCore::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);


	if (this->enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

void 
GameCore::createVulkanInstance()
{
	if (this->enableValidationLayers && !this->areValidationLayersAvailable()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	showAvaialbleExtensions();

	VkApplicationInfo appInfo{};

	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
	appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.pApplicationName = "Triangle";
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;


	VkInstanceCreateInfo createInfo{};

	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;


	// Unsure about this (what are extensions)?


	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (this->enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	// Finally create the instance
	VkResult res = vkCreateInstance(&createInfo, nullptr, &this->instance);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Unable to create vulkan instance");
	}
}

void
GameCore::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);

	if (deviceCount == 0)
		throw std::runtime_error("failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(deviceCount);

	vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data());

	std::multimap<int, VkPhysicalDevice> orderedDevices;

	for (const auto& device : devices)
	{
		if (isDeviceSuitable(device))
		{
			orderedDevices.insert(std::make_pair(this->rateDeviceSuitability(device), device));
		}
	}

	if (orderedDevices.size() == 0)
	{
		throw std::runtime_error("No devices suitable");
	}

	if (orderedDevices.rbegin()->first <= 0)
	{
		throw std::runtime_error("No devices suitable for application");
	}


	this->physicalDevice = orderedDevices.rbegin()->second;
}

void 
GameCore::createLogicalDevice()
{
	QueueFamilyIndices indices = this->findQueueFamilies(this->physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	//VkDeviceQueueCreateInfo queueCreateInfo{};
	//queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	//queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
	//queueCreateInfo.queueCount = 1;
	//float queuePriority = 1.0f;
	//queueCreateInfo.pQueuePriorities = &queuePriority;

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = this->deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = this->deviceExtensions.data();

	if (this->enableValidationLayers)
	{
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else 
	{
		createInfo.enabledLayerCount = 0;
	}

	if(vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->device) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device!");
		
	// Since we only have 1 queue from that family queue we get at indx 0
	vkGetDeviceQueue(this->device, indices.graphicsFamily.value(), 0, &graphicsQueue);

	vkGetDeviceQueue(this->device, indices.graphicsFamily.value(), 0, &presentQueue);
}

SwapChainSupportDetails 
GameCore::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, this->surface, &details.capabilities);

	uint32_t count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &count, nullptr);

	if (count > 0) 
	{
		details.formats.resize(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &count, details.formats.data());
	}

	uint32_t presentCount;

	vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->surface, &presentCount, nullptr);

	if (presentCount > 0)
	{
		details.presentModes.resize(presentCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->surface, &presentCount, details.presentModes.data());
	}

	return details;
}

void 
GameCore::createSurface()
{
	//VkWin32SurfaceCreateInfoKHR createInfo{};
	//createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	//createInfo.hwnd = glfwGetWin32Window(this->window);
	//createInfo.hinstance = GetModuleHandle(nullptr);

	//if(vkCreateWin32SurfaceKHR(this->instance, &createInfo, nullptr, &this->surface) != VK_SUCCESS)
	//	throw std::runtime_error("failed to create window surface!");

	if(glfwCreateWindowSurface(this->instance, this->window, nullptr, &this->surface) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface!");
}

VkSurfaceFormatKHR
GameCore::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	// Each VkSurfaceFormatKHR entry contains a format and a colorSpace member
	for (const auto& format : availableFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR
GameCore::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	// (commonly found presentation) This is most similar to vertical sync as found in modern games. The moment that the display is refreshed is known as "vertical blank".
	for (const auto& present : availablePresentModes)
	{
		// Uses a lot of energy not suitable for mobiles but most performant
		if (present == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return present;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
GameCore::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != UINT32_MAX)
		return capabilities.currentExtent;

	int width, height = 0;

	glfwGetFramebufferSize(this->window, &width, &height);

	VkExtent2D actualExtent = {
		   static_cast<uint32_t>(width),
		   static_cast<uint32_t>(height)
	};

	actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualExtent;
}


void
GameCore::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = this->querySwapChainSupport(this->physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = this->chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = this->chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = this->chooseSwapExtent(swapChainSupport.capabilities);

	// How many images in the swap chain, recommende dto request at least one more image 
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = this->surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;  //This is always 1 unless you are developing a stereoscopic 3D application
	/*n. The imageUsage bit field specifies what kind of operations we'll use the images in the swap chain for. 
	In this tutorial we're going to render directly to them, which means that they're used as color attachment.
	It is also possible that you'll render images to a separate image first to perform operations like post-processing. In that case you may use a value like 
	VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use a memory operation
	to transfer the rendered image to a swap chain image.*/
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


	QueueFamilyIndices indices = this->findQueueFamilies(this->physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily)
	{
		/*We'll be drawing on the images in the swap chain from the graphics queue and then submitting them on the presentation queue.
		There are two ways to handle images that are accessed from multiple queues:*/
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		/*VK_SHARING_MODE_EXCLUSIVE: An image is owned by one queue family at a time and ownership 
		must be explicitly transferred before using it in another queue family. This option offers the best performance.*/
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	// Most hardware use the same family queus for graphiccs and presentation but if not advanced topics are required

	/// <summary>
	/// / IF YOU DONT WANT ANY TRANSFORMATION SPOECIFY THE CURRENT TRANSFORMATION
	/// </summary>
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

	// Specifies the alpha channel, you ALMOST ALWAY WANT TO SIMPLY IGNOER THE ALPHA CHANNEL therefoer use thiso ption..
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE; // best performance
	createInfo.oldSwapchain = VK_NULL_HANDLE; // FOR NOW WE WIL LASSSUME ONLY 1 SWAPCHAINM WILL BE CREATED
	// AS IF THE WINDOW IS REZIED THE SWAP CHAIN NEEDS TO BE RECREATED FEROM SCRATCH, AND A REFERENCE TO TH EOLD ONE MUST BE SPECIFIED.

	if(vkCreateSwapchainKHR(this->device, &createInfo, nullptr, &this->swapChain) != VK_SUCCESS) 
		throw std::runtime_error("failed to create swap chain!");

	uint32_t swapChainCount = 0;
	vkGetSwapchainImagesKHR(this->device, this->swapChain, &swapChainCount, nullptr);
	this->swapChainImages.resize(swapChainCount);
	vkGetSwapchainImagesKHR(this->device, this->swapChain, &swapChainCount, this->swapChainImages.data());

	this->swapChainImageFormat = surfaceFormat.format;
	this->swapChainExtent = extent;

}

void
GameCore::createImageViews()
{
	this->swapChainImageViews.resize(this->swapChainImages.size());

	for (const auto& image : this->swapChainImages)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = image;

		// 2D OR 32RD
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = this->swapChainImageFormat;

		// COLOR CHANNELS
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		//
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		/*An image view is sufficient to start using an image as a texture, but it's not quite ready to be used as a render target just yet. 
		That requires one more step of indirection, known as a framebuffer. But first we'll have to set up the graphics pipeline.*/
		if(vkCreateImageView(this->device, &createInfo, nullptr, &imageView) != VK_SUCCESS)
			throw std::runtime_error("failed to create image views!");

		this->swapChainImageViews.push_back(imageView);
	}
}

void 
GameCore::setupDebugMessenger()
{
	if (!this->enableValidationLayers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr; // Optional

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void
GameCore::initVulkan()
{
	this->createVulkanInstance();
	this->setupDebugMessenger();

	this->createSurface();
	
	this->pickPhysicalDevice();
	this->createLogicalDevice();
	this->createSwapChain();
	this->createImageViews();
}

int 
GameCore::rateDeviceSuitability(VkPhysicalDevice device) {

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	int score = 0;

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	// Application can't function without geometry shaders
	if (!deviceFeatures.geometryShader) {
		return 0;
	}

	return score;
}

QueueFamilyIndices  
GameCore::findQueueFamilies(VkPhysicalDevice device) {

	uint32_t queueFamilyCount = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	QueueFamilyIndices indices;

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		// you could add logic to explicitly prefer a physical device that supports drawing and presentation in the same queue for improved performance.
		if (presentSupport)
		{
			indices.presentFamily = i;
		}

		i++;
	}


	// Logic to find queue family indices to populate struct with
	return indices;

}

bool
GameCore::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExts(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExts.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExts) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

bool 
GameCore::isDeviceSuitable(VkPhysicalDevice device)
{
	bool success = findQueueFamilies(device).isComplete();
	bool extensionsSupported = checkDeviceExtensionSupport(device);
	
	bool swapChainGood = false;

	if (extensionsSupported)
	{
		SwapChainSupportDetails details = this->querySwapChainSupport(device);
		swapChainGood = !details.formats.empty() && !details.presentModes.empty();
	}

	return success && extensionsSupported && swapChainGood;
}


void 
GameCore::mainLoop()
{
	while (!glfwWindowShouldClose(this->window))
	{
		glfwPollEvents();
	}
}

void 
GameCore::cleanup()
{
	if (this->enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(this->instance, this->debugMessenger, nullptr);
	}

	for (auto imageView : this->swapChainImageViews)
	{
		vkDestroyImageView(this->device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(this->device, this->swapChain, nullptr);
	vkDestroyDevice(this->device, nullptr);
	vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
	vkDestroyInstance(this->instance, nullptr);
	glfwDestroyWindow(this->window);

	glfwTerminate();
}

void 
GameCore::Initialize() 
{
	this->initWindow();
	this->initVulkan();
}

void
GameCore::Run()
{
	this->mainLoop();
	this->cleanup();
}
