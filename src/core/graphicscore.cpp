#include "graphicscore.h"
#include "headers.h"
#include "descriptorheapallocator.h"
#include "resources/depthbuffer.h"
#include "resources/colorbuffer.h"


namespace GRAPHICS_CORE
{
	TextureManager g_textureManager;
	CommandManager g_commandManager;
	ContextManager g_contextManager;
	MaterialManager g_materialManager;

	//the following two descriptor heaps will be binded into related context 
	StaticDescriptorHeap g_texturesDescriptorHeap;
	StaticDescriptorHeap g_samplersDescriptorHeap;
	ModelManager g_staticModelsManager;

	ID3D12Device* g_device = nullptr;
	bool g_tearingSupport;

	std::string g_texturePath = "resource/textures/";
	std::string g_pbrmaterialTextureName[5] = {
		"alebdo", "normal", "roughness", "metalness", "ao"
	};


	SamplerDesc g_samplerLinearWrapDesc;
	D3D12_CPU_DESCRIPTOR_HANDLE g_samplerLinearWrap;
	SamplerDesc g_samplerAnisoWrapDesc;
	D3D12_CPU_DESCRIPTOR_HANDLE g_samplerAnisoWrap;
	SamplerDesc g_samplerPointClampDesc;
	D3D12_CPU_DESCRIPTOR_HANDLE g_samplerPointClamp;
	SamplerDesc g_samplerLinearBorderDesc;
	D3D12_CPU_DESCRIPTOR_HANDLE g_samplerLinearBorder;
	SamplerDesc g_samplerPointBorderDesc;
	D3D12_CPU_DESCRIPTOR_HANDLE g_samplerPointBorder;

	DescriptorAllocator g_descriptorHeapAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	};

	UINT32 GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type)
	{
		return GRAPHICS_CORE::g_device->GetDescriptorHandleIncrementSize(type);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count)
	{
		return g_descriptorHeapAllocator[type].Allocator(count);
	}

	//Get the adapter from current computer
	ComPtr<IDXGIAdapter4> GainAdapter(bool bUseWarp) {
		ComPtr<IDXGIFactory4> dxgiFactory;
		UINT createFactoryFlags = 0;

#if defined(_DEBUG)
		createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		// Create DXGI Factory
		ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

		ComPtr<IDXGIAdapter1> dxgiAdapater1;
		ComPtr<IDXGIAdapter4> dxgiAdapater4;

		if (bUseWarp)
		{
			ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapater1)));
			ThrowIfFailed(dxgiAdapater1.As(&dxgiAdapater4));
		}
		else
		{
			SIZE_T maxDedicatedVideoMeory = 0;
			for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapater1) != DXGI_ERROR_NOT_FOUND; i++) {
				DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
				dxgiAdapater1->GetDesc1(&dxgiAdapterDesc1);

				if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
					SUCCEEDED(D3D12CreateDevice(dxgiAdapater1.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)) &&
					dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMeory) {
					maxDedicatedVideoMeory = dxgiAdapterDesc1.DedicatedVideoMemory;
					ThrowIfFailed(dxgiAdapater1.As(&dxgiAdapater4));
				}
			}
		}
		return dxgiAdapater4;
	}

	//Create Device
	ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter) {
		ComPtr<ID3D12Device2> device;
		ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));


#if defined(_DEBUG)
		ComPtr<ID3D12InfoQueue> pInfoQueue;
		if (SUCCEEDED(device.As(&pInfoQueue))) {
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

			D3D12_MESSAGE_SEVERITY severities[] = {
				D3D12_MESSAGE_SEVERITY_INFO
			};

			D3D12_MESSAGE_ID denyIds[] = {
				D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
				D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
				D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
			};

			D3D12_INFO_QUEUE_FILTER newFilter = {};
			newFilter.DenyList.NumSeverities = _countof(severities);
			newFilter.DenyList.pSeverityList = severities;
			newFilter.DenyList.NumIDs = _countof(denyIds);
			newFilter.DenyList.pIDList = denyIds;
			ThrowIfFailed(pInfoQueue->PushStorageFilter(&newFilter));
		}
#endif

		return device;
	}

	bool CheckTearingSupport() {
		BOOL allowTearing = FALSE;

		ComPtr<IDXGIFactory4> factory4;
		if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4)))) {
			ComPtr<IDXGIFactory5> factory5;
			if (SUCCEEDED(factory4.As(&factory5))) {
				if (FAILED(factory5->CheckFeatureSupport(
					DXGI_FEATURE_PRESENT_ALLOW_TEARING,
					&allowTearing, sizeof(allowTearing)))) {
					allowTearing = FALSE;
				}
			}
		}
		return allowTearing == TRUE;
	}

	void EnableDX12DebugLayer() {
#if defined(_DEBUG)
		ComPtr<ID3D12Debug> debugInterface;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
		debugInterface->EnableDebugLayer();
#endif
	}

	void SamplersInitialize() {
		g_samplerLinearWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		g_samplerLinearWrap = g_samplerLinearWrapDesc.CreateSamplerDescHandle();

		g_samplerAnisoWrapDesc.Filter = D3D12_FILTER_ANISOTROPIC;
		g_samplerAnisoWrapDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		g_samplerAnisoWrapDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		g_samplerAnisoWrapDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		g_samplerAnisoWrapDesc.MaxAnisotropy = 16;
		g_samplerAnisoWrap = g_samplerAnisoWrapDesc.CreateSamplerDescHandle();
	
		//point clamp sampler
		g_samplerPointClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		g_samplerPointClampDesc.SetAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		g_samplerPointClamp = g_samplerPointClampDesc.CreateSamplerDescHandle();

		//linear border sampler
		g_samplerLinearBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		g_samplerLinearBorderDesc.SetAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
		g_samplerLinearBorder = g_samplerLinearBorderDesc.CreateSamplerDescHandle();

		//point border sampler
		g_samplerPointBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		g_samplerPointBorderDesc.SetAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
		g_samplerPointBorder = g_samplerPointBorderDesc.CreateSamplerDescHandle();

	
	}

	void GraphicsCoreInitialize()
	{
		EnableDX12DebugLayer();

		ComPtr<IDXGIAdapter4> dxgiAdapter = GainAdapter(false);

		if (dxgiAdapter) {
			ComPtr<ID3D12Device2> device = CreateDevice(dxgiAdapter);
			GRAPHICS_CORE::g_device = device.Detach();
		}
		if (GRAPHICS_CORE::g_device) {
			//Update essential d3d12 device
			GRAPHICS_CORE::g_commandManager.Initialize(GRAPHICS_CORE::g_device);
			GRAPHICS_CORE::g_textureManager.Initialize(g_texturePath);
			SamplersInitialize();

			//Create the descriptor heap for textures and samplers
			GRAPHICS_CORE::g_texturesDescriptorHeap.Initialize(L"TextureDescriptorHeap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);
			GRAPHICS_CORE::g_samplersDescriptorHeap.Initialize(L"SamplerDescriptorHeap", D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048);
			
			//Initialize the static model loading
			GRAPHICS_CORE::g_staticModelsManager.Initialize();

			//Initialize the static material
			GRAPHICS_CORE::g_materialManager.Initialize();

			g_tearingSupport = CheckTearingSupport();
		}
	}

	void GraphicsCoreRelease() {
		if (GRAPHICS_CORE::g_device != nullptr) {
			GRAPHICS_CORE::g_device->Release();
			GRAPHICS_CORE::g_device = nullptr;
		}

		GRAPHICS_CORE::g_commandManager.Release();
	}


	UINT GetDXGIFormatSize(DXGI_FORMAT format) {
		switch (format) {
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 16; // 4 components, each 4 bytes (32 bits)

		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 12; // 3 components, each 4 bytes (32 bits)

		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return 8; // 4 components, each 2 bytes (16 bits)

		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
			return 8; // 2 components, each 4 bytes (32 bits)

		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
			return 4; // 4 bytes (32 bits)

		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
			return 4; // 4 components, each 1 byte (8 bits)

		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
			return 4; // 2 components, each 2 bytes (16 bits)

		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
			return 4; // 4 bytes (32 bits)

		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
			return 2; // 2 components, each 1 byte (8 bits)

		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
			return 2; // 2 bytes (16 bits)

		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
			return 1; // 1 byte (8 bits)

		case DXGI_FORMAT_R1_UNORM:
			return 1; // 1 bit per pixel, but stored in 1 byte (8 bits)

			// Add more cases as needed for other DXGI_FORMAT values

		default:
			throw std::runtime_error("Unsupported DXGI_FORMAT");
		}
	}
}