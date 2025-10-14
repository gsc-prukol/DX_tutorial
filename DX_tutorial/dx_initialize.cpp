#include "dx_initialize.h"

int Width = 800;
int Height = 600;

bool FullScreen = false;
// we will exit the program when this becomes false
bool Running = true;

// direct3d stuff

const int FrameBufferCount = 3;
ID3D12Device* device;
IDXGISwapChain3* swapChain;
ID3D12CommandQueue* commandQueue;
ID3D12DescriptorHeap* rtvDescriptorHeap;
ID3D12Resource* renderTargets[FrameBufferCount];
ID3D12CommandAllocator* commandAllocator[FrameBufferCount];
ID3D12GraphicsCommandList* commandList;
ID3D12Fence* fence[FrameBufferCount];

HANDLE fenceEvent;
UINT64 fenceValue[FrameBufferCount];
int frameIndex;
int rtvDescriptorSize;

bool InitD3D(HWND hwnd)
{
	HRESULT hr;

	IDXGIFactory4* dxgiFactory;
	hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	if (FAILED(hr)) 
	{
		return false;
	}

	IDXGIAdapter1* adapter;

	int adapterIndex = 0;
	bool adapterFound = false;

	while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND) 
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			++adapterIndex;
			continue;
		}

		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);

		if (SUCCEEDED(hr)) 
		{
			adapterFound = true;
			break;
		}

		++adapterIndex;
	}

	if (!adapterFound) 
	{
		return false;
	}

	// create the device
	hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));

	// create the command queue

	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	hr = device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue));
	
	if (FAILED(hr)) 
	{
		return false;
	}

	DXGI_MODE_DESC backBufferDesc = {};
	backBufferDesc.Width = Width;
	backBufferDesc.Height = Height;
	backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = FrameBufferCount;
	swapChainDesc.BufferDesc = backBufferDesc;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.SampleDesc = sampleDesc;
	swapChainDesc.Windowed = !FullScreen;

	IDXGISwapChain* tempSwapChain;
	dxgiFactory->CreateSwapChain(commandQueue, &swapChainDesc, &tempSwapChain);
	
	swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);
	
	frameIndex = swapChain->GetCurrentBackBufferIndex();

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FrameBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));

	if (FAILED(hr)) 
	{
		return false;
	}

	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < FrameBufferCount; ++i) 
	{
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));

		if (FAILED(hr)) 
		{
			return false;
		}

		device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
		rtvHandle.Offset(1, rtvDescriptorSize);
	}

	for (int i = 0; i < FrameBufferCount; ++i)
	{
		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[i]));

		if (FAILED(hr))
		{
			return false;
		}
	}

	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[0], NULL, IID_PPV_ARGS(&commandList));

	if (FAILED(hr))
	{
		return false;
	}

	commandList->Close();

	for (int i = 0; i < FrameBufferCount; ++i)
	{
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence[i]));

		if (FAILED(hr))
		{
			return false;
		}

		fenceValue[i] = 0;
	}

	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	if (fenceEvent == nullptr)
	{
		return false;
	}

	return true;
}

void Update()
{
	// update app logic
}

void UpdatePipeline()
{
	HRESULT hr;

	WaitForPreviousFrame();

	hr = commandAllocator[frameIndex]->Reset();

	if (FAILED(hr))
	{
		Running = false;
	}

	hr = commandList->Reset(commandAllocator[frameIndex], NULL);

	if (FAILED(hr))
	{
		Running = false;
	}

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], 
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		frameIndex, rtvDescriptorSize);

	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	const float clearColor[] = { 1.0f, 0.2f, 0.4f, 1.f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &barrier);

	hr = commandList->Close();

	if (FAILED(hr))
	{
		Running = false;
	}
}

void Render()
{
	HRESULT hr;

	UpdatePipeline();

	ID3D12CommandList* ppCommandLists[] = { commandList };

	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);

	if (FAILED(hr)) 
	{
		Running = false;
	}
}

void Cleanup()
{
	for (int i = 0; i < FrameBufferCount; ++i)
	{
		frameIndex = i;
		WaitForPreviousFrame();
	}

	BOOL fs = FALSE;

	if (swapChain->GetFullscreenState(&fs, nullptr))
	{
		swapChain->SetFullscreenState(false, nullptr);
	}

	SAFE_RELEASE(device);
	SAFE_RELEASE(swapChain);
	SAFE_RELEASE(commandQueue);
	SAFE_RELEASE(rtvDescriptorHeap);
	SAFE_RELEASE(commandList);

	for (int i = 0; i < FrameBufferCount; ++i) 
	{
		SAFE_RELEASE(renderTargets[i]);
		SAFE_RELEASE(commandAllocator[i]);
		SAFE_RELEASE(fence[i]);
	}
}

void WaitForPreviousFrame()
{
	HRESULT hr;

	frameIndex = swapChain->GetCurrentBackBufferIndex();

	if (fence[frameIndex]->GetCompletedValue() < fenceValue[frameIndex])
	{
		hr = fence[frameIndex]->SetEventOnCompletion(fenceValue[frameIndex], fenceEvent);

		if (FAILED(hr))
		{
			Running = false;
		}

		WaitForSingleObject(fenceEvent, INFINITE);
	}

	fenceValue[frameIndex++];
}
