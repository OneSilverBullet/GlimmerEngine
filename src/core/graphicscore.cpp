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
	StaticDescriptorHeap g_texturesDescriptorHeap;
	StaticDescriptorHeap g_samplersDescriptorHeap;
	ID3D12Device* g_device = nullptr;
	bool g_tearingSupport;

	SamplerDesc g_samplerLinearWrapDesc;
	D3D12_CPU_DESCRIPTOR_HANDLE g_samplerLinearWrap;
	SamplerDesc g_samplerAnisoWrapDesc;
	D3D12_CPU_DESCRIPTOR_HANDLE g_samplerAnisoWrap;



	DescriptorAllocator g_descriptorHeapAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	};

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

		g_samplerAnisoWrapDesc.MaxAnisotropy = 4;
		g_samplerAnisoWrap = g_samplerAnisoWrapDesc.CreateSamplerDescHandle();
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
			GRAPHICS_CORE::g_textureManager.Initialize("");
			SamplersInitialize();

			//Create the descriptor heap for textures and samplers
			GRAPHICS_CORE::g_texturesDescriptorHeap.Initialize(L"TextureDescriptorHeap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);
			GRAPHICS_CORE::g_samplersDescriptorHeap.Initialize(L"SamplerDescriptorHeap", D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048);


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



}