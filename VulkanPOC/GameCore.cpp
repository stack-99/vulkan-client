#include "GameCore.hpp"

#include <set>
#include <cstdint> // Necessary for UINT32_MAX
#include <algorithm> // Necessary for std::min/std::max
#include <filesystem>
namespace fs = std::filesystem;

#include <fstream>
static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

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
		//extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		//extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
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

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

		this->populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	// Finally create the instance
	VkResult res = vkCreateInstance(&createInfo, nullptr, &this->instance);

	if (res != VK_SUCCESS)
		throw std::runtime_error("Unable to create vulkan instance");
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

VkShaderModule 
GameCore::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if(vkCreateShaderModule(this->device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module!");

	return shaderModule;
}

void
GameCore::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = this->swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	/*The loadOp and storeOp determine what to do with the data in the attachment before rendering and after rendering. We have the following choices for loadOp:

VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
VK_ATTACHMENT_LOAD_OP_CLEAR: Clear the values to a constant at the start
VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined; we don't care about them
In our case we're going to use the clear operation to clear the framebuffer to black before drawing a new frame. There are only two possibilities for the storeOp:

VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later
VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents of the framebuffer will be undefined after the rendering operation
We're interested in seeing the rendered triangle on the screen, so we're going with the store operation here.*/
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	/*The loadOp and storeOp apply to color and depth data, and stencilLoadOp / stencilStoreOp apply to stencil data.
	Our application won't do anything with the stencil buffer, so the results of loading and storing are irrelevant.*/
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/*Textures and framebuffers in Vulkan are represented by VkImage objects with a certain pixel format, however the layout of the pixels in memory can change based on what you're trying to do with an image.

Some of the most common layouts are:

VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain
VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Images to be used as destination for a memory copy operation
We'll discuss this topic in more depth in the texturing chapter, but what's important to know right now is that images need to be
transitioned to specific layouts that are suitable for the operation that they're going to be involved in next.

The initialLayout specifies which layout the image will have before the render pass begins. The finalLayout specifies the
layout to automatically transition to when the render pass finishes. Using VK_IMAGE_LAYOUT_UNDEFINED for initialLayout
means that we don't care what previous layout the image was in. The caveat of this special value is that the contents of the
image are not guaranteed to be preserved, but that doesn't matter since we're going to clear it anyway. We want the image to be ready for presentation
using the swap chain after rendering, which is why we use VK_IMAGE_LAYOUT_PRESENT_SRC_KHR as finalLayout.*/
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


	// Subpasses and attachment references
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/*The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!

The following other types of attachments can be referenced by a subpass:

pInputAttachments: Attachments that are read from a shader
pResolveAttachments: Attachments used for multisampling color attachments
pDepthStencilAttachment: Attachment for depth and stencil data
pPreserveAttachments: Attachments that are not used by this subpass, but for which the data must be preserved*/
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colorAttachment;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(this->device, &createInfo, nullptr, &this->renderPass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");
}

void
GameCore::createGraphicsPipeline()
{
	auto vertShaderCode = readFile("shaders/vert.spv");
	auto fragShaderCode = readFile("shaders/frag.spv");

	VkShaderModule vertShaderModule = this->createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = this->createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStags[] = { vertShaderStageInfo, fragShaderStageInfo };

	/*The VkPipelineVertexInputStateCreateInfo structure describes the format of the vertex data that will be passed to the vertex shader. 
	It describes this in roughly two ways:
		Bindings: spacing between data and whether the data is per-vertex or per-instance (see instancing)
		Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset*/
	// In real-time computer graphics, geometry instancing is the practice of rendering multiple copies of the same mesh in a scene at once. 
	/*Because we're hard coding the vertex data directly in the vertex shader, we'll fill in this structure 
	to specify that there is no vertex data to load for now. We'll get back to it in the vertex buffer chapter.*/
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	// Input Assembly
	/*The VkPipelineInputAssemblyStateCreateInfo struct describes two things: what kind of geometry will be drawn from the vertices and
	if primitive restart should be enabled. The former is specified in the topology member and can have values like:

VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
VK_PRIMITIVE_TOPOLOGY_LINE_LIST: line from every 2 vertices without reuse
VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: the end vertex of every line is used as start vertex for the next line
VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every 3 vertices without reuse
VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: the second and third vertex of every triangle are used as first two vertices of the next triangle
Normally, the vertices are loaded from the vertex buffer by index in sequential order, but with an element buffer you can specify 
the indices to use yourself. This allows you to perform optimizations like reusing vertices. If you set the primitiveRestartEnable member to
VK_TRUE, then it's possible to break up lines and triangles in the _STRIP topology modes by using a special index of 0xFFFF or 0xFFFFFFFF.

We intend to draw triangles throughout this tutorial, so we'll stick to the following data for the structure:*/
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;


	/*It is possible to use multiple viewports and scissor rectangles on some graphics cards, 
	so its members reference an array of them. Using multiple requires enabling a GPU feature (see logical device creation).*/
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	//viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	//viewportState.pScissors = &scissor;

	// Rasterizer
	/*The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments to be 
	colored by the fragment shader. It also performs depth testing, face culling and the scissor test, and it can be configured to 
	output fragments that fill entire polygons or 
	just the edges (wireframe rendering). All this is configured using the VkPipelineRasterizationStateCreateInfo structure.*/

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE; 
	/*If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are clamped to them as opposed to discarding them. 
	This is useful in some special cases like shadow maps. Using this requires enabling a GPU feature.*/

	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	/*If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage. 
	This basically disables any output to the framebuffer.*/

	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	/*The polygonMode determines how fragments are generated for geometry. The following modes are available:

VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
VK_POLYGON_MODE_LINE: polygon edges are drawn as lines
VK_POLYGON_MODE_POINT: polygon vertices are drawn as points
Using any mode other than fill requires enabling a GPU feature.*/

	rasterizer.lineWidth = 1.0f;
	/*The maximum line width that is supported depends on the hardware and any line thicker than 1.0f requires you to enable the wideLines GPU feature.*/

	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// Multi sampling
	/*The VkPipelineMultisampleStateCreateInfo struct configures multisampling, which is one of the ways to perform anti-aliasing. 
	It works by combining the fragment shader results of multiple polygons that rasterize to the same pixel. This mainly occurs along edges,
	which is also where the most noticeable aliasing artifacts occur. Because it doesn't need to run the fragment shader multiple times if only 
	one polygon maps to a pixel, it is significantly less 
	expensive than simply rendering to a higher resolution and then downscaling. Enabling it requires enabling a GPU feature.*/
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// Depth and stencil testing

	// Color blending
	/*After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer. 
	This transformation is known as color blending and there are two ways to do it:

	Mix the oldand new value to produce a final color
		Combine the oldand new value using a bitwise operation
		There are two types of structs to configure color blending.The first struct, VkPipelineColorBlendAttachmentState contains the 
		configuration per attached framebufferand the second struct, 
		VkPipelineColorBlendStateCreateInfo contains the global color blending settings.In our case we only have one framebuffer :*/
	// PER ATTACHED FRAMEBUFFER, AND THE GLOBAL COLOR BLENDING IS THE COLORBLENDSTATE
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	// Dynamic state
	// Without recreating the pipeline you can spficy the size of the vbiewport, line width and blend cosntants, to do so use DYNAMIC STATE
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// Pipeline layout
	/*You can use uniform values in shaders, which are globals similar to dynamic state variables that can be changed at drawing time 
	to alter the behavior of your shaders without having to recreate them. They are commonly used to pass the transformation matrix to
	the vertex shader, or to create texture samplers in the fragment shader.

These uniform values need to be specified during pipeline creation by creating a VkPipelineLayout object. Even though we won't be usin
g them until a future chapter, we are still required to create an empty pipeline layout.
*/
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if(vkCreatePipelineLayout(this->device, &pipelineLayoutInfo, nullptr, &this->pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout!");


	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStags;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void
GameCore::createFrameBuffers()
{
	for (const auto& imageView : this->swapChainImageViews)
	{
		VkFramebufferCreateInfo frameBufferInfo{};
		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.renderPass = this->renderPass;
		frameBufferInfo.attachmentCount = 1;
		frameBufferInfo.pAttachments = nullptr;
		frameBufferInfo.pAttachments = &imageView;
		frameBufferInfo.width = this->swapChainExtent.width;
		frameBufferInfo.height = this->swapChainExtent.height;
		frameBufferInfo.layers = 1;
		frameBufferInfo.pNext = nullptr;

		VkFramebuffer frameBuffer{};
		if(vkCreateFramebuffer(this->device, &frameBufferInfo, nullptr, &frameBuffer) != VK_SUCCESS) 
			throw std::runtime_error("failed to create framebuffer!");

		this->swapChainFramebuffers.push_back(frameBuffer);
	}
}

void 
GameCore::createCommandPool()
{
	auto queueFamilyIndices = this->findQueueFamilies(this->physicalDevice);

	/*Command buffers are executed by submitting them on one of the device queues, like the graphics and presentation queues we retrieved. 
	Each command pool can only allocate command buffers that are submitted on a single type of queue. We're going to record commands for drawing,
	which is why we've chosen the graphics queue family.

There are two possible flags for command pools:

VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often (may change memory allocation behavior)
VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without 
this flag they all have to be reset together
We will only record the command buffers at the beginning of the program and then execute them many times in the main loop, 
so we're not going to use either of these flags.
*/
	VkCommandPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if(vkCreateCommandPool(this->device, &createInfo, nullptr, &this->commandPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool!");
}

void 
GameCore::createCommandBuffer() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(this->device, &allocInfo, &this->commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void 
GameCore::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = this->renderPass;
	renderPassInfo.framebuffer = this->swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = this->swapChainExtent;

	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(this->swapChainExtent.width);
	viewport.height = static_cast<float>(this->swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = this->swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void 
GameCore::createSyncObjects() {
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &this->imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &this->renderFinishedSemaphore) != VK_SUCCESS ||
		vkCreateFence(this->device, &fenceInfo, nullptr, &this->inFlightFence) != VK_SUCCESS) {
		throw std::runtime_error("failed to create synchronization objects for a frame!");
	}

}

void 
GameCore::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

void 
GameCore::setupDebugMessenger()
{
	if (!this->enableValidationLayers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	this->populateDebugMessengerCreateInfo(createInfo);

	if (this->CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
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
	this->createRenderPass();
	this->createGraphicsPipeline();
	this->createFrameBuffers();
	this->createCommandPool();

	this->createCommandBuffer();
	this->createSyncObjects();
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

	std::set<std::string> requiredExtensions(this->deviceExtensions.begin(), this->deviceExtensions.end());

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
GameCore::drawFrame() {
	vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &inFlightFence);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
	recordCommandBuffer(commandBuffer, imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(presentQueue, &presentInfo);
}

void 
GameCore::mainLoop()
{
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(device);
}

void 
GameCore::cleanup()
{
	if (this->enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(this->instance, this->debugMessenger, nullptr);
	}

	vkDestroyCommandPool(this->device, this->commandPool, nullptr);
	for (auto frameBuffer : this->swapChainFramebuffers)
	{
		vkDestroyFramebuffer(this->device, frameBuffer, nullptr);
	}

	vkDestroyPipeline(this->device, this->graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(this->device, this->pipelineLayout, nullptr);
	vkDestroyRenderPass(this->device, this->renderPass, nullptr);

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
