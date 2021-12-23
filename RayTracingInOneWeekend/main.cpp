#include "MathUtils.hpp"

#include "Camera.hpp"
#include "Color.hpp"
#include "HittableList.hpp"
#include "Material.hpp"
#include "Sphere3.hpp"
#include "ProfileLogScope.hpp"

#include <array>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>
#include <memory>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>

#include <shellapi.h>

#pragma comment(lib, "Shell32.lib")

#include <d3d11_4.h>
#include <dxgi1_6.h>

#include <D3Dcommon.h>

#include <d3d11sdklayers.h>
#include <dxgidebug.h>

#include <DirectXMath.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")

#include <d3d11shader.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

struct Vertex3D {
    Vector3 position{};
    Color color{};
    DirectX::XMFLOAT2 uv{};
};

Color ray_color(const Ray3& r, const Hittable& world, int depth);
float hit_sphere(const Point3& center, float radius, const Ray3& r);

HittableList random_scene();

#ifdef _WIN32

int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int);

HWND Create();
bool InitializeDX11();
bool InitializeDebuggerInstance();
bool CreateFactory();
void ReportLiveObjects();
bool InitializeWorld();
void ReleaseGlobalDXResources();
bool GetAdapter();
void GetOutputs();
bool CreateDx11DeviceAndContext();
bool GetDxgiDevice();
bool CreateSwapchain();
bool CreateDepthStencil();
bool CreateBackbuffer();
bool CreateVertexBuffer(const std::vector<Vertex3D>& initialData, std::size_t size);
bool CreateIndexBuffer(const std::vector<unsigned int>& initialData, std::size_t size);
bool CreateShaders();
void OutputError(const std::string& msg);
void OutputShaderCompileErrors(ID3DBlob* errors, const char* msg);
bool FillBlobFromCache(std::filesystem::path cso_path_canon, ID3DBlob*& blob);
bool WriteShaderCacheToFile(const std::filesystem::path& path, ID3DBlob* blob);
bool CompileVertexShaderFromFile(ID3DBlob*& vs_bytecode, const std::filesystem::path& path);
bool CreateVertexShader();
bool CreateVS(ID3DBlob*& vs_bytecode);
bool CreatePixelShader();
bool CompilePixelShaderFromFile(ID3DBlob*& ps_bytecode, const std::filesystem::path& path);
bool CreatePS(ID3DBlob* ps_bytecode);
void CreateViewport();
bool CreateBlendState();
bool CreateSamplerState();
bool CreateRasterState();
bool CreateConstantBuffers();
void UpdateConstantBuffer(int index, void* buffer, unsigned int size);
void SetConstantBuffer(int index);

bool CreateRandomUVTexture();
void UpdateRandomUVTexture();
bool CreateRandomDiskTexture();
void UpdateRandomDiskTexture();

void RunMessagePump();
bool UnRegister();

void BeginFrame();
std::chrono::duration<float> GetCurrentTimeElapsed();
void Update(float deltaSeconds);
void Render();
void EndFrame();

bool g_isQuitting = false;

#define GetHInstance() ::GetModuleHandleA(nullptr)

std::chrono::duration<float> GetCurrentTimeElapsed() {
    static auto initial_now = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return (now - initial_now);
}

HWND g_hWnd{};

int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int) {

    if (g_hWnd = Create(); g_hWnd) {
        if(InitializeDX11()) {
            if (InitializeWorld()) {
                while (!g_isQuitting) {
                    RunMessagePump();
                    BeginFrame();

                    static auto previousFrameTime = GetCurrentTimeElapsed().count();
                    auto currentFrameTime = GetCurrentTimeElapsed().count();
                    auto deltaSeconds = (currentFrameTime - previousFrameTime);
                    previousFrameTime = currentFrameTime;

                    Update(deltaSeconds);
                    Render();
                    EndFrame();
                }
            }
        }
        ReleaseGlobalDXResources();
        UnRegister();
    }
}

constexpr const std::size_t max_name_string = 256u;

HDC hdc{};
WNDCLASSEX wndcls{};

LRESULT CALLBACK WindowProcedure(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam);

bool Register() {
    wndcls.cbSize = sizeof(WNDCLASSEX);
    auto style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wndcls.style = style;
    wndcls.lpfnWndProc = WindowProcedure;
    wndcls.cbClsExtra = 0;
    wndcls.cbWndExtra = 0;
    wndcls.hInstance = GetHInstance();
    wndcls.hIcon = ::LoadIcon(0, IDI_APPLICATION);
    wndcls.hCursor = ::LoadCursorA(GetHInstance(), IDC_ARROW);
    wndcls.hbrBackground = (HBRUSH)::GetStockObject(NULL_BRUSH);
    wndcls.lpszMenuName = nullptr;
    wndcls.lpszClassName = "SimpleWindowClass";
    wndcls.hIconSm = ::LoadIcon(0, IDI_APPLICATION);
    return 0 != ::RegisterClassEx(&wndcls);
}

float viewportWidth = 1600.0f;
float viewportHeight = 900.0f;
float screenWidth = 1600.0f;
float screenHeight = 900.0f;

HWND Create() {
    if (Register()) {
        int argc{ 0 };
        RECT rect{};
        float aspect_ratio = 16.0f / 9.0f;
        if(auto* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc); argc <= 1 || argv == nullptr) {
            rect = RECT{0, 0, 1600, 900};
            ::LocalFree(argv);
        }
        else {
            const auto width = argc > 1 ? static_cast<int>(std::stoll(argv[1])) : 1600;
            const auto height = argc > 2 ? static_cast<int>(std::stoll(argv[2])) : static_cast<int>(std::floor(static_cast<float>(width) / aspect_ratio));
            if (argc > 2) {
                const auto width_f = static_cast<float>(width);
                const auto height_f = static_cast<float>(height);
                viewportWidth = width_f;
                viewportHeight = height_f;
                screenWidth = width_f;
                screenHeight = height_f;
                aspect_ratio = width_f / height_f;
            }
            rect = RECT{0, 0, width, height};
            ::LocalFree(argv);
        }
        auto windowStyle = WS_OVERLAPPEDWINDOW;
        if (::AdjustWindowRect(&rect, windowStyle, false)) {
            const auto width = rect.right - rect.left;
            const auto height = rect.bottom - rect.top;
            if (auto hwnd = ::CreateWindowA("SimpleWindowClass", "Raytracing in One Weekend - Pixel Shader", windowStyle, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, GetHInstance(), nullptr); hwnd) {
                hdc = ::GetDCEx(hwnd, nullptr, 0);
                ::ShowWindow(hwnd, SW_SHOW);
                return hwnd;
            }
        }
    }
    return 0;
}

IDXGIDebug* debuggerInstance{};
IDXGIFactory7* factory{};
std::vector<IDXGIAdapter4*> adapters{};
IDXGIAdapter4* adapter{};
std::vector<IDXGIOutput*> outputs{};
ID3D11Device5* device{};
ID3D11DeviceContext4* deviceContext{};
IDXGIDevice4* dxgiDevice{};
IDXGISwapChain4* swapchain4{};
ID3D11Texture2D* backbuffer_t{};
ID3D11RenderTargetView* backbuffer_rtv{};
ID3D11ShaderResourceView* backbuffer_srv{};
ID3D11Texture2D* randomDiskResult_t{};
ID3D11ShaderResourceView* randomDiskResult_srv{};
ID3D11Texture1D* randomUVResult_t{};
ID3D11ShaderResourceView* randomUVResult_srv{};
ID3D11DepthStencilView* depthStencil{};
ID3D11DepthStencilState* depthStencilState{};
ID3D11VertexShader* vs{};
ID3D11PixelShader* ps{};
ID3D11InputLayout* il{};
D3D_FEATURE_LEVEL highest_feature_level;
ID3D11Buffer* vertexBuffer{};
std::size_t vertexBufferSize{ 0u };
ID3D11Buffer* indexBuffer{};
std::size_t indexBufferSize{ 0u };

const unsigned int world_cbuffer_index = 0u;
const unsigned int frame_cbuffer_index = 1u;
const unsigned int object_cbuffer_index = 2u;
ID3D11Buffer* world_cb{};
ID3D11Buffer* frame_cb{};
ID3D11Buffer* object_cb{};

float gSamplesPerPixel = 400.0f;
float gMaxDepth = 50.0f;

struct world_data_t {
    float screenWidth;
    float screenHeight;
    float padding[2];
};
world_data_t world_data;

struct frame_data_t {
    DirectX::XMMATRIX viewProjectionMatrix;
    DirectX::XMMATRIX viewMatrix;
    DirectX::XMMATRIX projectionMatrix;
    float gameTime;
    float gameFrameTime;
    float samplesPerPixel;
    float maxDepth;
    Vector3 eyePosition;
    Vector3 lookAt;
    float vFovRadians;
    float aspectRatio;
    float aperture;
    float lensRadius;
    float focusDistance;
    float framePadding;
};
frame_data_t frame_data;

struct object_data_t {
    DirectX::XMMATRIX modelMatrix;
    float gMaterialColor[4];
    Vector3 gMaterialAttenuation;
    float gSphereRadius;
    float gMaterialType;
    float gMaterialRoughness;
    float gMaterialMetallic;
    float gMaterialRefractionIndex;
};
object_data_t object_data;

#define SAFE_RELEASE(x) { \
    if((x)) { \
        (x)->Release(); \
        (x) = nullptr; \
    } \
} \

void ReleaseGlobalDXResources() {

    deviceContext->IASetInputLayout(nullptr);
    deviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0u);
    deviceContext->IASetVertexBuffers(0u, 0u, nullptr, nullptr, nullptr);

    {
        std::array<ID3D11Buffer*, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> buffers{};
        deviceContext->VSSetConstantBuffers(0, static_cast<unsigned int>(buffers.size()), buffers.data());
    }
    {
        std::array<ID3D11SamplerState*, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT> buffers{};
        deviceContext->VSSetSamplers(0, static_cast<unsigned int>(buffers.size()), buffers.data());
    }
    deviceContext->VSSetShader(nullptr, nullptr, 0u);


    {
        std::array<ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> buffers{};
        deviceContext->VSSetShaderResources(0, static_cast<unsigned int>(buffers.size()), buffers.data());
    }

    {
        std::array<ID3D11Buffer*, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> buffers{};
        deviceContext->PSSetConstantBuffers(0, static_cast<unsigned int>(buffers.size()), buffers.data());
    }
    {
        std::array<ID3D11SamplerState*, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT> buffers{};
        deviceContext->PSSetSamplers(0, static_cast<unsigned int>(buffers.size()), buffers.data());
    }
    deviceContext->PSSetShader(nullptr, nullptr, 0u);
    {
        std::array<ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> buffers{};
        deviceContext->PSSetShaderResources(0, static_cast<unsigned int>(buffers.size()), buffers.data());
    }


    deviceContext->ClearState();

    SAFE_RELEASE(il);
    SAFE_RELEASE(vertexBuffer);
    SAFE_RELEASE(indexBuffer);
    SAFE_RELEASE(world_cb);
    SAFE_RELEASE(frame_cb);
    SAFE_RELEASE(object_cb);
    SAFE_RELEASE(ps);
    SAFE_RELEASE(vs);
    SAFE_RELEASE(depthStencil);
    SAFE_RELEASE(depthStencilState);
    SAFE_RELEASE(backbuffer_t);
    SAFE_RELEASE(backbuffer_rtv);
    SAFE_RELEASE(backbuffer_srv);
    SAFE_RELEASE(randomDiskResult_t);
    SAFE_RELEASE(randomDiskResult_srv);
    SAFE_RELEASE(randomUVResult_t);
    SAFE_RELEASE(randomUVResult_srv);
    SAFE_RELEASE(swapchain4);
    SAFE_RELEASE(deviceContext);
    SAFE_RELEASE(device);
    SAFE_RELEASE(dxgiDevice);

    for (auto& o : outputs) {
        SAFE_RELEASE(o);
    }
    outputs.clear();
    outputs.shrink_to_fit();

    for (auto& a : adapters) {
        SAFE_RELEASE(a);
    }
    adapter = nullptr;
    adapters.clear();
    adapters.shrink_to_fit();

    SAFE_RELEASE(factory);

    ReportLiveObjects();
    SAFE_RELEASE(debuggerInstance);
}

bool CreateDx11DeviceAndContext() {
    ID3D11Device* tempDevice{};
    ID3D11DeviceContext* tempDeviceContext{};

    const auto ReleaseTemporaryDXResources = [&]() {
        SAFE_RELEASE(tempDeviceContext);
        SAFE_RELEASE(tempDevice);
    };

    std::array featureLevels = {
        //D3D_FEATURE_LEVEL_12_2,
        //D3D_FEATURE_LEVEL_12_1,
        //D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };
    {
        unsigned int flags{ 0u };
#ifdef _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
        //flags |= D3D11_CREATE_DEVICE_DEBUGGABLE;
#else
        flags |= D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY;
#endif
        flags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        if(adapter) {
            DXGI_ADAPTER_DESC3 adapterDesc{};
            adapter->GetDesc3(&adapterDesc);
            if (adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {
                ::OutputDebugStringA("D3D11 Device creation failed. Software Devices are not supported.");
                ReleaseGlobalDXResources();
                return false;
            }
        }
        if (auto hresult = ::D3D11CreateDevice(adapter ? adapter : nullptr, adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, featureLevels.data(), static_cast<unsigned int>(featureLevels.size()), D3D11_SDK_VERSION, &tempDevice, &highest_feature_level, &tempDeviceContext); FAILED(hresult)) {
            ::OutputDebugStringA("D3D11 Device creation failed. Minimum feature set is not supported.");
            ReleaseGlobalDXResources();
            return false;
        }
        if (!(highest_feature_level >= D3D_FEATURE_LEVEL_11_0)) {
            ::OutputDebugStringA("Your graphics card does not support at least DirectX 11.0. Please update your drivers or hardware.");
            ReleaseGlobalDXResources();
            return false;
        }
    }

    if (auto hresult = tempDevice->QueryInterface(__uuidof(ID3D11Device5), reinterpret_cast<void**>(&device)); FAILED(hresult)) {
        ReleaseTemporaryDXResources();
        return false;
    } else {
        SAFE_RELEASE(tempDevice);
    }

    if (auto hresult = tempDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext4), reinterpret_cast<void**>(&deviceContext)); FAILED(hresult)) {
        ReleaseTemporaryDXResources();
        return false;
    } else {
        SAFE_RELEASE(tempDeviceContext);
    }
    return true;
}

void ReportLiveObjects() {
#ifdef _DEBUG
    if (debuggerInstance) {
        ::OutputDebugStringA("\n-------------------------------");
        if (auto hr = debuggerInstance->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL)); FAILED(hr)) {
            ::OutputDebugStringA("\nThere was an error reporting live objects");
            ::OutputDebugStringA("\n-------------------------------\n");
            return;
        }
        ::OutputDebugStringA("\n-------------------------------\n");
    }
#endif
}

bool InitializeDX11() {

    if (!InitializeDebuggerInstance()) {
        return false;
    }

    if (!CreateFactory()) {
        return false;
    }

    if (!GetAdapter()) {
        return false;
    }

    GetOutputs();

    if (!CreateDx11DeviceAndContext()) {
        return false;
    }

    if (!GetDxgiDevice()) {
        return false;
    }

    if (!CreateSwapchain()) {
        return false;
    }

    if (!CreateDepthStencil()) {
        return false;
    }

    if (!CreateBackbuffer()) {
        return false;
    }
    
    deviceContext->OMSetRenderTargets(1, &backbuffer_rtv, depthStencil);

    CreateViewport();

    if (!CreateBlendState()) {
        return false;
    }
    if (!CreateSamplerState()) {
        return false;
    }
    if (!CreateRasterState()) {
        return false;
    }

    CreateConstantBuffers();

    if (!CreateRandomUVTexture()) {
        return false;
    }

    if (!CreateRandomDiskTexture()) {
        return false;
    }

    if (!CreateShaders()) {
        return false;
    }

    return true;
}

bool InitializeDebuggerInstance() {
#ifdef _DEBUG
    HMODULE debug_module = nullptr;

    debug_module = ::LoadLibraryA("Dxgidebug.dll");
    if (debug_module) {
        using GetDebugModuleCB = HRESULT(WINAPI*)(REFIID, void**);
        GetDebugModuleCB cb = (GetDebugModuleCB)::GetProcAddress(debug_module, "DXGIGetDebugInterface");
        if (auto hr = cb(__uuidof(IDXGIDebug), reinterpret_cast<void**>(&debuggerInstance)); FAILED(hr)) {
            OutputError("DXGIDebugger failed to initialize.");
            return false;
        }
        ReportLiveObjects();
    }
#endif
    return true;
}

bool CreateFactory() {
    IDXGIFactory2* tempFactory{};
#ifdef _DEBUG
    if (auto hresult = ::CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, __uuidof(IDXGIFactory2), reinterpret_cast<void**>(&tempFactory)); FAILED(hresult)) {
#else
    if (auto hresult = ::CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), reinterpret_cast<void**>(&tempFactory)); FAILED(hresult)) {
#endif
        return false;
    }
    if (auto hresult = tempFactory->QueryInterface(__uuidof(IDXGIFactory7), reinterpret_cast<void**>(&factory)); FAILED(hresult)) {
        SAFE_RELEASE(tempFactory);
        return false;
    }
    SAFE_RELEASE(tempFactory);
    return true;
}

bool GetAdapter() {
    IDXGIAdapter4* cur_adapter{};
    for (unsigned int i = 0u;
        SUCCEEDED(factory->EnumAdapterByGpuPreference(
            i,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            __uuidof(IDXGIAdapter4),
            reinterpret_cast<void**>(&cur_adapter)));
        ++i)
    {
        adapters.push_back(cur_adapter);
    }
    SAFE_RELEASE(cur_adapter);
    if (adapters.empty()) {
        ReleaseGlobalDXResources();
        return false;
    }
    adapter = adapters[0];
    return true;
}

void GetOutputs() {
    IDXGIOutput* output{};
    for (int i = 0; DXGI_ERROR_NOT_FOUND != adapter->EnumOutputs(i, &output); ++i) {
        outputs.push_back(output);
    }
}

bool GetDxgiDevice() {
    if (auto hresult = device->QueryInterface(__uuidof(IDXGIDevice4), reinterpret_cast<void**>(&dxgiDevice)); FAILED(hresult)) {
        ReleaseGlobalDXResources();
        return false;
    }
    return true;
}

bool CreateSwapchain() {
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = 0;
    desc.Height = 0;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Stereo = FALSE;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    IDXGISwapChain1* swapchain1{};
    if (auto hresult = factory->CreateSwapChainForHwnd(device, g_hWnd, &desc, nullptr, nullptr, &swapchain1); FAILED(hresult)) {
        ReleaseGlobalDXResources();
        return false;
    }

    if (auto hresult = swapchain1->QueryInterface(__uuidof(IDXGISwapChain4), reinterpret_cast<void**>(&swapchain4)); FAILED(hresult)) {
        SAFE_RELEASE(swapchain1);
        ReleaseGlobalDXResources();
        return false;
    }
    SAFE_RELEASE(swapchain1);
    return true;
}

bool CreateDepthStencil() {
    ID3D11Texture2D* tDepthStencil{};
    {

        D3D11_TEXTURE2D_DESC tDesc{};
        std::memset(&tDesc, 0, sizeof(tDesc));
        tDesc.Width = static_cast<unsigned int>(screenWidth);
        tDesc.Height = static_cast<unsigned int>(screenHeight);
        tDesc.MipLevels = 1;
        tDesc.ArraySize = 1;
        tDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        tDesc.SampleDesc.Count = 1;
        tDesc.SampleDesc.Quality = 0;
        tDesc.Usage = D3D11_USAGE_DEFAULT;
        tDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        tDesc.CPUAccessFlags = 0;
        tDesc.MiscFlags = 0;

        if (auto hr = device->CreateTexture2D(&tDesc, nullptr, &tDepthStencil); FAILED(hr)) {
            SAFE_RELEASE(tDepthStencil);
            ReleaseGlobalDXResources();
            return false;
        }
    }

    {
        D3D11_DEPTH_STENCIL_DESC dsDesc{};
        std::memset(&dsDesc, 0, sizeof(dsDesc));
        dsDesc.DepthEnable = true;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

        dsDesc.StencilEnable = true;
        dsDesc.StencilReadMask = 0xFF;
        dsDesc.StencilWriteMask = 0xFF;

        dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
        dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        if (FAILED(device->CreateDepthStencilState(&dsDesc, &depthStencilState))) {
            ReleaseGlobalDXResources();
            return false;
        }
    }

    deviceContext->OMSetDepthStencilState(depthStencilState, 1);

    {
        D3D11_DEPTH_STENCIL_VIEW_DESC desc{};
        std::memset(&desc, 0, sizeof(desc));
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = 0;

        if (FAILED(device->CreateDepthStencilView(tDepthStencil, &desc, &depthStencil))) {
            SAFE_RELEASE(tDepthStencil);
            ReleaseGlobalDXResources();
            return false;
        }
        SAFE_RELEASE(tDepthStencil);
    }

    return true;
}

bool CreateBackbuffer() {
    if (FAILED(swapchain4->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backbuffer_t)))) {
        ReleaseGlobalDXResources();
        return false;
    }

    D3D11_TEXTURE2D_DESC t_desc;
    backbuffer_t->GetDesc(&t_desc);

    bool success = true;
    if (t_desc.BindFlags & D3D11_BIND_RENDER_TARGET) {
        if(auto hr = device->CreateRenderTargetView(backbuffer_t, nullptr, &backbuffer_rtv); FAILED(hr)) {
            success &= false;
        }
    }

    if (t_desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
        if(auto hr = device->CreateShaderResourceView(backbuffer_t, nullptr, &backbuffer_srv); FAILED(hr)) {
            success &= false;
        }
    }
    if (!success) {
        ReleaseGlobalDXResources();
        return false;
    }
    return true;
}

void CreateViewport() {
    D3D11_VIEWPORT viewport{};
    viewport.Width = viewportWidth;
    viewport.Height = viewportHeight;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    deviceContext->RSSetViewports(1, &viewport);
}

bool CreateBlendState() {
    ID3D11BlendState* state{};
    D3D11_BLEND_DESC desc{};
    desc.AlphaToCoverageEnable = false;
    desc.IndependentBlendEnable = false;
    auto& brt = desc.RenderTarget[0];
    brt.BlendEnable = true;
    brt.SrcBlend = D3D11_BLEND_SRC_ALPHA;
    brt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    brt.BlendOp = D3D11_BLEND_OP_ADD;
    brt.SrcBlendAlpha = D3D11_BLEND_ZERO;
    brt.DestBlendAlpha = D3D11_BLEND_ZERO;
    brt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    brt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (auto hresult = device->CreateBlendState(&desc, &state); FAILED(hresult)) {
        return false;
    }
    float factor[] = { 1.0f, 1.0f, 1.0f, 1.f };
    deviceContext->OMSetBlendState(state, factor, 0xffffffffu);
    SAFE_RELEASE(state);
    return true;
}

bool CreateRasterState() {
    ID3D11RasterizerState2* state{};
    D3D11_RASTERIZER_DESC2 desc{};
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_BACK;
    desc.FrontCounterClockwise = true;
    desc.DepthBias = 0;
    desc.DepthBiasClamp = 0.0f;
    desc.SlopeScaledDepthBias = 0.0f;
    desc.DepthClipEnable = true;
    desc.ScissorEnable = false;
    desc.MultisampleEnable = false;
    desc.AntialiasedLineEnable = false;
    desc.ForcedSampleCount = 0;
    desc.ConservativeRaster = D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    if (FAILED(device->CreateRasterizerState2(&desc, &state))) {
        return false;
    }
    deviceContext->RSSetState(state);
    SAFE_RELEASE(state);
    return true;
}

bool CreateSamplerState() {
    ID3D11SamplerState* state{};
    D3D11_SAMPLER_DESC desc{};
    desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.BorderColor[0] = 1.0f;
    desc.BorderColor[1] = 1.0f;
    desc.BorderColor[2] = 1.0f;
    desc.BorderColor[3] = 1.0f;
    desc.MinLOD = 0.0f;
    desc.MaxLOD = 1.0f;
    desc.MipLODBias = 0.0f;
    desc.ComparisonFunc = D3D11_COMPARISON_LESS;
    desc.MaxAnisotropy = 0u;
    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    if (auto hresult = device->CreateSamplerState(&desc, &state); FAILED(hresult)) {
        return false;
    }
    deviceContext->PSSetSamplers(0, 1, &state);
    deviceContext->VSSetSamplers(0, 1, &state);
    SAFE_RELEASE(state);
    return true;
}

bool CreateConstantBuffers() {
    {
        D3D11_BUFFER_DESC desc{};
        std::memset(&desc, 0, sizeof(desc));
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.ByteWidth = sizeof(world_data);

        D3D11_SUBRESOURCE_DATA data{};
        std::memset(&data, 0, sizeof(data));
        data.pSysMem = &world_data;

        if (auto hr = device->CreateBuffer(&desc, &data, &world_cb); FAILED(hr)) {
            return false;
        }
    }
    {
        D3D11_BUFFER_DESC desc{};
        std::memset(&desc, 0, sizeof(desc));
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.ByteWidth = sizeof(frame_data);

        D3D11_SUBRESOURCE_DATA data{};
        std::memset(&data, 0, sizeof(data));
        data.pSysMem = &frame_data;

        if (auto hr = device->CreateBuffer(&desc, &data, &frame_cb); FAILED(hr)) {
            return false;
        }
    }
    {
        D3D11_BUFFER_DESC desc{};
        std::memset(&desc, 0, sizeof(desc));
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.ByteWidth = sizeof(object_data);

        D3D11_SUBRESOURCE_DATA data{};
        std::memset(&data, 0, sizeof(data));
        data.pSysMem = &object_data;

        if (auto hr = device->CreateBuffer(&desc, &data, &object_cb); FAILED(hr)) {
            return false;
        }
    }
    return true;
}

void UpdateConstantBuffer(int index, void* buffer, unsigned int size) {
    D3D11_MAPPED_SUBRESOURCE resource{};
    HRESULT hr{};
    switch (index) {
    case world_cbuffer_index:
        hr = deviceContext->Map(world_cb, 0, D3D11_MAP_WRITE_DISCARD, 0U, &resource);
        break;
    case frame_cbuffer_index:
        hr = deviceContext->Map(frame_cb, 0, D3D11_MAP_WRITE_DISCARD, 0U, &resource);
        break;
    case object_cbuffer_index:
        hr = deviceContext->Map(object_cb, 0, D3D11_MAP_WRITE_DISCARD, 0U, &resource);
        break;
    default:
        return;
    }
    if(bool succeeded = SUCCEEDED(hr); succeeded) {
        std::memcpy(resource.pData, buffer, size);
    }
    switch (index) {
    case world_cbuffer_index:
        deviceContext->Unmap(world_cb, 0);
        break;
    case frame_cbuffer_index:
        deviceContext->Unmap(frame_cb, 0);
        break;
    case object_cbuffer_index:
        deviceContext->Unmap(object_cb, 0);
        break;
    default:
        return;
    }
}

void SetConstantBuffer(int index) {
    switch (index) {
    case world_cbuffer_index:
        deviceContext->VSSetConstantBuffers(index, 1, &world_cb);
        deviceContext->PSSetConstantBuffers(index, 1, &world_cb);
        break;
    case frame_cbuffer_index:
        deviceContext->VSSetConstantBuffers(index, 1, &frame_cb);
        deviceContext->PSSetConstantBuffers(index, 1, &frame_cb);
        break;
    case object_cbuffer_index:
        deviceContext->VSSetConstantBuffers(index, 1, &object_cb);
        deviceContext->PSSetConstantBuffers(index, 1, &object_cb);
        break;
    default:
        break;
    }
}

bool CreateShaders() {
    const auto vs_created = CreateVertexShader();
    const auto ps_created = CreatePixelShader();
    if (!(vs_created && ps_created)) {
        return false;
    }
    return true;
}

bool CreateVertexShader() {
    ID3DBlob* vs_bytecode{ nullptr };
    auto cso_path = std::filesystem::path{ "vs.cso" };
    auto hlsl_path = std::filesystem::path{ "vs.hlsl" };
    std::error_code cso_ec{};
    const auto cso_exists = std::filesystem::exists(cso_path);
    const auto hlsl_exists = std::filesystem::exists(hlsl_path);
    const auto cso_needs_recompile = !cso_exists || (cso_exists && hlsl_exists && std::filesystem::last_write_time(cso_path) < std::filesystem::last_write_time(hlsl_path));
    if (std::filesystem::path cso_path_canon = std::filesystem::canonical(cso_path, cso_ec); !cso_needs_recompile && !cso_ec) {
        cso_path_canon.make_preferred();
        if (!FillBlobFromCache(cso_path_canon, vs_bytecode)) {
            SAFE_RELEASE(vs_bytecode);
            return false;
        }
    } else {
        std::error_code hlsl_ec{};
        if (std::filesystem::path hlsl_path_canon = std::filesystem::canonical(hlsl_path, hlsl_ec); hlsl_ec) {
            OutputError(hlsl_path.string() + " could not be accessed. Reason: " + hlsl_ec.message() + '\n');
            SAFE_RELEASE(vs_bytecode);
            return false;
        } else {
            hlsl_path_canon.make_preferred();
            if (CompileVertexShaderFromFile(vs_bytecode, hlsl_path_canon)) {
                if (!WriteShaderCacheToFile(hlsl_path_canon, vs_bytecode)) {
                    SAFE_RELEASE(vs_bytecode);
                    return false;
                }
            } else {
                SAFE_RELEASE(vs_bytecode);
                return false;
            }
        }
    }
    if (!CreateVS(vs_bytecode)) {
        SAFE_RELEASE(vs_bytecode);
        return false;
    }
    SAFE_RELEASE(vs_bytecode);
    return true;
}

bool CreatePixelShader() {
    ID3DBlob* ps_bytecode{ nullptr };
    auto hlsl_path = std::filesystem::path{ "ps.hlsl" };
    auto cso_path = std::filesystem::path{ "ps.cso" };
    std::error_code cso_ec{};
    const auto cso_exists = std::filesystem::exists(cso_path);
    const auto hlsl_exists = std::filesystem::exists(hlsl_path);
    const auto cso_needs_recompile = !cso_exists || (cso_exists && hlsl_exists && std::filesystem::last_write_time(cso_path) < std::filesystem::last_write_time(hlsl_path));
    if (std::filesystem::path cso_path_canon = std::filesystem::canonical(cso_path, cso_ec); !cso_needs_recompile && !cso_ec) {
        cso_path_canon.make_preferred();
        if (!FillBlobFromCache(cso_path_canon, ps_bytecode)) {
            SAFE_RELEASE(ps_bytecode);
            return false;
        }
    } else {
        std::error_code hlsl_ec{};
        if (std::filesystem::path hlsl_path_canon = std::filesystem::canonical(hlsl_path, hlsl_ec); hlsl_ec) {
            OutputError(hlsl_path.string() + " could not be accessed. Reason: " + hlsl_ec.message() + '\n');
            SAFE_RELEASE(ps_bytecode);
            return false;
        } else {
            hlsl_path_canon.make_preferred();
            if (CompilePixelShaderFromFile(ps_bytecode, hlsl_path_canon)) {
                if (!WriteShaderCacheToFile(hlsl_path_canon, ps_bytecode)) {
                    SAFE_RELEASE(ps_bytecode);
                    return false;
                }
            } else {
                SAFE_RELEASE(ps_bytecode);
                return false;
            }
        }
    }
    if (!CreatePS(ps_bytecode)) {
        SAFE_RELEASE(ps_bytecode);
        return false;
    }
    SAFE_RELEASE(ps_bytecode);
    return true;
}

bool CreateVertexBuffer(const std::vector<Vertex3D>& initialData, std::size_t size) {
    D3D11_BUFFER_DESC desc{};
    std::memset(&desc, 0, sizeof(desc));
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_VERTEX_BUFFER;
    desc.ByteWidth = sizeof(std::remove_cvref_t<decltype(initialData)>::value_type) * static_cast<unsigned int>(size);
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = initialData.data();

    if (auto hr = device->CreateBuffer(&desc, &data, &vertexBuffer); FAILED(hr)) {
        return false;
    }
    return true;
}

bool CreateIndexBuffer(const std::vector<unsigned int>& initialData, std::size_t size) {
    D3D11_BUFFER_DESC desc{};
    std::memset(&desc, 0, sizeof(desc));
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_INDEX_BUFFER;
    desc.ByteWidth = sizeof(std::remove_cvref_t<decltype(initialData)>::value_type) * static_cast<unsigned int>(size);
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = initialData.data();

    if (auto hr = device->CreateBuffer(&desc, &data, &indexBuffer); FAILED(hr)) {
        return false;
    }
    return true;
}


bool CreateRandomUVTexture() {
    D3D11_TEXTURE1D_DESC desc{};
    std::memset(&desc, 0, sizeof(desc));

    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    const auto samplesAsUInt = static_cast<unsigned int>(gSamplesPerPixel);
    desc.Width = samplesAsUInt;
    desc.ArraySize = 1;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.MipLevels = 1;

    std::vector<std::array<float, 4>> initialData(samplesAsUInt);

    for (unsigned int x = 0; x < samplesAsUInt; ++x) {
        initialData[x][0] = random_float();
        initialData[x][1] = random_float();
        initialData[x][2] = random_float();
        initialData[x][3] = random_float();
    }

    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = initialData.data();
    data.SysMemPitch = desc.Width * (sizeof(float) * 4);

    if (auto hr = device->CreateTexture1D(&desc, &data, &randomUVResult_t); FAILED(hr)) {
        return false;
    }

    if (auto hr = device->CreateShaderResourceView(randomUVResult_t, nullptr, &randomUVResult_srv); FAILED(hr)) {
        return false;
    }
    return true;
}

void UpdateRandomUVTexture() {

    D3D11_TEXTURE1D_DESC desc{};
    std::memset(&desc, 0, sizeof(desc));

    randomUVResult_t->GetDesc(&desc);

    const auto width = desc.Width;
    std::vector<std::array<float, 4>> updatedData(width);

    for (unsigned int x = 0; x < width; ++x) {
        updatedData[x][0] = random_float();
        updatedData[x][1] = random_float();
        updatedData[x][2] = random_float();
        updatedData[x][3] = random_float();
    }

    D3D11_MAPPED_SUBRESOURCE resource{};
    std::memset(&resource, 0u, sizeof(resource));
    if (auto hr = deviceContext->Map(randomUVResult_t, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &resource); SUCCEEDED(hr)) {
        const auto rowPitch = resource.RowPitch;
        std::memcpy(resource.pData, updatedData.data(), rowPitch);
        deviceContext->Unmap(randomUVResult_t, 0u);
    }
}

bool CreateRandomDiskTexture() {
    D3D11_TEXTURE2D_DESC desc{};
    std::memset(&desc, 0, sizeof(desc));

    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Width = static_cast<unsigned int>(screenWidth);
    desc.Height = static_cast<unsigned int>(screenHeight);
    desc.ArraySize = 1;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    const auto size = std::size_t{static_cast<unsigned int>(screenWidth) * static_cast<unsigned int>(screenHeight)};
    std::vector<std::array<float, 4>> initialData(size);

    for (unsigned int x = 0; x < size; ++x) {
        const auto r = random_in_unit_disk();
        initialData[x][0] = (1.0f + r.x()) / 2.0f;
        initialData[x][1] = (1.0f + r.y()) / 2.0f;
        initialData[x][2] = 0.0f;
        initialData[x][3] = 1.0f;
    }

    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = initialData.data();
    data.SysMemPitch = desc.Width * (sizeof(float) * 4);
    data.SysMemSlicePitch = desc.Width * desc.Height * (sizeof(float) * 4);

    if (auto hr = device->CreateTexture2D(&desc, &data, &randomDiskResult_t); FAILED(hr)) {
        return false;
    }

    if (auto hr = device->CreateShaderResourceView(randomDiskResult_t, nullptr, &randomDiskResult_srv); FAILED(hr)) {
        return false;
    }
    return true;
}

void UpdateRandomDiskTexture() {


    D3D11_TEXTURE2D_DESC desc{};
    std::memset(&desc, 0, sizeof(desc));

    randomDiskResult_t->GetDesc(&desc);

    const auto twidth = desc.Width;
    const auto theight = desc.Height;
    const auto tsize = twidth * theight;
    std::vector<std::array<float, 4>> updatedData(static_cast<std::size_t>(tsize));

    for (unsigned int x = 0; x < tsize; ++x) {
        const auto r = random_in_unit_disk();
        updatedData[x][0] = (1.0f + r.x()) / 2.0f;
        updatedData[x][1] = (1.0f + r.y()) / 2.0f;
        updatedData[x][2] = 0.0f;
        updatedData[x][3] = 1.0f;
    }

    D3D11_MAPPED_SUBRESOURCE resource{};
    std::memset(&resource, 0u, sizeof(resource));
    if (auto hr = deviceContext->Map(randomUVResult_t, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &resource); SUCCEEDED(hr)) {
        const auto rwidth = resource.RowPitch;
        const auto rheight = resource.DepthPitch;
        if (rwidth != twidth) {
            auto* src = updatedData.data();
            auto* dest = reinterpret_cast<unsigned int*>(resource.pData);
            for (unsigned int row = 0; row < rheight; ++row) {
                std::memcpy(dest, src, rwidth);
                dest += rwidth >> 2;
                src += twidth;
            }
        }
        else {
            std::memcpy(resource.pData, updatedData.data(), updatedData.size());
        }
        deviceContext->Unmap(randomUVResult_t, 0u);
    }
}

void OutputError(const std::string& msg) {
#ifdef _MSC_VER
    ::OutputDebugStringA(msg.c_str());
#else
    std::cout << err_str;
#endif
}

void OutputShaderCompileErrors(ID3DBlob* errors, const char* msg) {
    ::MessageBoxA(nullptr, msg, "D3DCompile Failed", MB_OK);
    if (errors) {
        OutputError(reinterpret_cast<char*>(errors->GetBufferPointer()));
        errors->Release();
        errors = nullptr;
    }
}

bool FillBlobFromCache(std::filesystem::path cso_path_canon, ID3DBlob*& blob) {
    const auto size = std::filesystem::file_size(cso_path_canon);
    std::vector<uint8_t> buffer{};
    buffer.resize(size);
    if (std::ifstream ifs{ cso_path_canon, std::ios_base::binary }; ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size())) {
        if (auto hresult_blob = ::D3DCreateBlob(size, &blob); FAILED(hresult_blob)) {
            OutputError("Blob creation failed.\n");
            return false;
        } else {
            std::memcpy(blob->GetBufferPointer(), buffer.data(), buffer.size());
        }
    }
    return true;
}

bool CompileVertexShaderFromFile(ID3DBlob*& vs_bytecode, const std::filesystem::path& path) {
    ID3DBlob* errors{ nullptr };

    unsigned int hlsl_flags{ 0u };
#ifdef _DEBUG
    hlsl_flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    hlsl_flags |= D3DCOMPILE_SKIP_VALIDATION;
    hlsl_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    hlsl_flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
    hlsl_flags |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
    hlsl_flags |= D3DCOMPILE_ALL_RESOURCES_BOUND;
    unsigned int fx_flags{ 0u };

    if (auto hresult_compile = ::D3DCompileFromFile(path.wstring().c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", hlsl_flags, fx_flags, &vs_bytecode, &errors); FAILED(hresult_compile)) {
        OutputShaderCompileErrors(errors, "Vertex Shader failed to compile. See Output Window for details.");
        SAFE_RELEASE(errors);
        SAFE_RELEASE(vs_bytecode);
        return false;
    }
    return true;
}

bool CompilePixelShaderFromFile(ID3DBlob*& ps_bytecode, const std::filesystem::path& path) {
    ID3DBlob* errors{ nullptr };
    unsigned int hlsl_flags{ 0u };
#ifdef _DEBUG
    hlsl_flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    hlsl_flags |= D3DCOMPILE_SKIP_VALIDATION;
    hlsl_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    hlsl_flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
    hlsl_flags |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
    hlsl_flags |= D3DCOMPILE_ALL_RESOURCES_BOUND;
    unsigned int fx_flags{ 0u };

    if (auto hresult_compile = ::D3DCompileFromFile(path.wstring().c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", hlsl_flags, fx_flags, &ps_bytecode, &errors); FAILED(hresult_compile)) {
        OutputShaderCompileErrors(errors, "Pixel Shader failed to compile. See Output Window for details.");
        SAFE_RELEASE(errors);
        SAFE_RELEASE(ps_bytecode);
        return false;
    }
    return true;
}

bool WriteShaderCacheToFile(const std::filesystem::path& path, ID3DBlob* blob) {
    auto cso_path = path;
    cso_path.replace_extension(".cso");
    cso_path.make_preferred();
    if (std::ofstream ofs{ cso_path, std::ios_base::binary }; !ofs.write(reinterpret_cast<const char*>(blob->GetBufferPointer()), blob->GetBufferSize())) {
        OutputError(std::string{ "Could not write shader cache to " } + cso_path.string() + '\n');
        return false;
    }
    return true;
}

bool CreateVS(ID3DBlob*& vs_bytecode) {
    if (auto hresult_create = device->CreateVertexShader(vs_bytecode->GetBufferPointer(), vs_bytecode->GetBufferSize(), nullptr, &vs); FAILED(hresult_create)) {
        OutputError("FAILED TO CREATE VERTEX SHADER\n");
        SAFE_RELEASE(vs_bytecode);
        return false;
    } else {
        const std::array desc = {
            D3D11_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"COLOR"   , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        if (auto hresult_input = device->CreateInputLayout(desc.data(), static_cast<unsigned int>(desc.size()), vs_bytecode->GetBufferPointer(), vs_bytecode->GetBufferSize(), &il); FAILED(hresult_input)) {
            OutputError("FAILED TO CREATE INPUT LAYOUT\n");
            SAFE_RELEASE(vs_bytecode);
            return false;
        }
        else {
            deviceContext->IASetInputLayout(il);
            deviceContext->VSSetShader(vs, nullptr, 0u);
        }
    }
    return true;
}

bool CreatePS(ID3DBlob* ps_bytecode) {
    if (auto hresult_create = device->CreatePixelShader(ps_bytecode->GetBufferPointer(), ps_bytecode->GetBufferSize(), nullptr, &ps); FAILED(hresult_create)) {
        SAFE_RELEASE(ps_bytecode);
        return false;
    } else {
        deviceContext->PSSetShader(ps, nullptr, 0u);
    }
    return true;
}

void RunMessagePump() {

    MSG msg{};
    for (;;) {
        const BOOL hasMsg = ::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
        if (!hasMsg) {
            break;
        }
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
}

bool UnRegister() {
    return 0 != ::UnregisterClass(wndcls.lpszClassName, nullptr);
}

LRESULT CALLBACK WindowProcedure(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
    switch (Msg) {
    case WM_CLOSE:
        ::DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        g_isQuitting = true;
        break;
    case WM_KEYDOWN: {
        switch ((unsigned int)wParam) {
        case VK_ESCAPE:
            g_isQuitting = true;
            return 0;
        default: /* DO NOTHING */
            break;;
        }
        break;
    }
    case WM_KEYUP:
        break;
    default:
        return ::DefWindowProc(hWnd, Msg, wParam, lParam);
    }
    return ::DefWindowProc(hWnd, Msg, wParam, lParam);
}

std::unique_ptr<Camera> camera{};

bool InitializeWorld() {

    world_data.screenWidth = screenWidth;
    world_data.screenHeight = screenHeight;

    UpdateConstantBuffer(world_cbuffer_index, &world_data, sizeof(world_data));

    deviceContext->PSSetConstantBuffers(frame_cbuffer_index, 1, &frame_cb);
    deviceContext->PSSetConstantBuffers(world_cbuffer_index, 1, &world_cb);
    deviceContext->PSSetConstantBuffers(object_cbuffer_index, 1, &object_cb);
    deviceContext->VSSetConstantBuffers(frame_cbuffer_index, 1, &frame_cb);
    deviceContext->VSSetConstantBuffers(world_cbuffer_index, 1, &world_cb);
    deviceContext->VSSetConstantBuffers(object_cbuffer_index, 1, &object_cb);

    const auto lookFrom = Point3{ 13.0f, 2.0f, 3.0f };
    const auto lookAt = Point3{ 0.0f, 0.0f, 0.0f };
    const auto vUp = Vector3{ 0.0f, 1.0f, 0.0f };
    const auto distance_to_focus = 10.0f;
    const auto aperture = 0.1f;
    const auto fovDegrees = 20.0f;
    const auto aspectRatio = 3.0f / 2.0f;

    camera = std::make_unique<Camera>(lookFrom, lookAt, vUp, fovDegrees, aspectRatio, aperture, distance_to_focus);

    return true;
}

void BeginFrame() {
    {
        std::array<ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> buffers{};
        deviceContext->PSSetShaderResources(0, static_cast<unsigned int>(buffers.size()), buffers.data());
    }
}

void Update(float deltaSeconds) {

    camera->Update(deltaSeconds);

    object_data.modelMatrix = DirectX::XMMatrixIdentity();

    UpdateConstantBuffer(object_cbuffer_index, &object_data, sizeof(object_data));

    frame_data.gameFrameTime = deltaSeconds;
    frame_data.gameTime += deltaSeconds;
    frame_data.samplesPerPixel = gSamplesPerPixel;
    frame_data.maxDepth = gMaxDepth;
    frame_data.projectionMatrix = camera->GetProjectionMatrix();
    frame_data.viewMatrix = camera->GetViewMatrix();
    frame_data.viewProjectionMatrix = camera->GetViewProjectionMatrix();
    frame_data.vFovRadians = camera->GetFovRadians();

    UpdateRandomUVTexture();
    UpdateRandomDiskTexture();

    UpdateConstantBuffer(frame_cbuffer_index, &frame_data, sizeof(frame_data));

    deviceContext->PSSetConstantBuffers(frame_cbuffer_index, 1, &frame_cb);
    deviceContext->PSSetConstantBuffers(world_cbuffer_index, 1, &world_cb);
    deviceContext->PSSetConstantBuffers(object_cbuffer_index, 1, &object_cb);

}

void Render() {

    deviceContext->OMSetRenderTargets(1, &backbuffer_rtv, depthStencil);

    float color[] = {0.0f, 0.0f, 0.0f, 1.0f};
    deviceContext->ClearRenderTargetView(backbuffer_rtv, color);
    deviceContext->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

    CreateViewport();

    std::vector vbuffer{
        Vertex3D{Vector3{-1.0f, 1.0f, 0.0f}, Color{0.0f, 0.0f, 0.0f}, DirectX::XMFLOAT2{0.0f, 0.0f}}
        ,Vertex3D{Vector3{-1.0f, -1.0f, 0.0f}, Color{0.0f, 1.0f, 0.0f}, DirectX::XMFLOAT2{0.0f, 1.0f}}
        ,Vertex3D{Vector3{1.0f, -1.0f, 0.0f}, Color{1.0f, 1.0f, 0.0f}, DirectX::XMFLOAT2{1.0f, 1.0f}}
        ,Vertex3D{Vector3{1.0f, 1.0f, 0.0f}, Color{1.0f, 0.0f, 0.0f}, DirectX::XMFLOAT2{1.0f, 0.0f}}
    };
    if (vbuffer.size() > vertexBufferSize) {
        vertexBufferSize = vbuffer.size() * 2u;
        SAFE_RELEASE(vertexBuffer);
        CreateVertexBuffer(vbuffer, vertexBufferSize);
    }
    {
        D3D11_MAPPED_SUBRESOURCE resource{};
        if(SUCCEEDED(deviceContext->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0U, &resource))) {
            std::memcpy(resource.pData, vbuffer.data(), sizeof(Vertex3D) * vbuffer.size());
            deviceContext->Unmap(vertexBuffer, 0);
        }
    }

    std::vector ibuffer{
        0u, 1u, 2u, 0u, 2u, 3u
    };
    if (ibuffer.size() > indexBufferSize) {
        indexBufferSize = ibuffer.size() * 2u;
        SAFE_RELEASE(indexBuffer);
        CreateIndexBuffer(ibuffer, indexBufferSize);
    }
    {
        D3D11_MAPPED_SUBRESOURCE resource{};
        if(SUCCEEDED(deviceContext->Map(indexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0U, &resource))) {
            std::memcpy(resource.pData, ibuffer.data(), sizeof(unsigned int) * ibuffer.size());
            deviceContext->Unmap(indexBuffer, 0);
        }
    }
    unsigned int stride{ sizeof(Vertex3D) };
    unsigned int offset{ 0u };
    deviceContext->IASetVertexBuffers(0u, 1u, &vertexBuffer, &stride, &offset);
    deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0u);
    deviceContext->PSSetShaderResources(0, 1, &backbuffer_srv);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->DrawIndexed(static_cast<unsigned int>(ibuffer.size()), 0, 0);
}

void EndFrame() {
    DXGI_PRESENT_PARAMETERS params{};
    params.DirtyRectsCount = 0;
    params.pDirtyRects = nullptr;
    params.pScrollOffset = nullptr;
    params.pScrollRect = nullptr;
    const auto should_tear = true;
    const auto is_vsync_off = true;
    const auto use_no_sync_interval = should_tear && is_vsync_off;
    const auto sync_interval = use_no_sync_interval ? 0u : 1u;
    const auto present_flags = use_no_sync_interval ? DXGI_PRESENT_ALLOW_TEARING : 0ul;
    if (const auto hresult = swapchain4->Present1(sync_interval, present_flags, &params); FAILED(hresult)) {
        ::ShowWindow(g_hWnd, SW_HIDE);
        switch (hresult) {
        case DXGI_ERROR_DEVICE_REMOVED: /* FALLTHROUGH */
        case DXGI_ERROR_DEVICE_RESET: {
            ReleaseGlobalDXResources();
            ::MessageBoxA(g_hWnd, "Your GPU Device has been lost. Please restart the application.", "Graphics Device Lost", MB_OK);
            g_isQuitting = true;
            break;
        }
        default:
            ReleaseGlobalDXResources();
            ::MessageBoxA(g_hWnd, "Present Call Failed.", "Unknown Present Failure", MB_OK);
            g_isQuitting = true;
        }
    }
}


#else

int main(int argc, char** argv) {

    //Image
    float aspect_ratio = 3.0f / 2.0f;
    const int image_width = [argc, argv]() -> const int {
        return argc > 1 ? static_cast<int>(std::stoll(argv[1])) : 400;
    }();
    const int image_height = [argc, argv, image_width, &aspect_ratio]() -> const int {
        if(argc > 2) {
            const int h = static_cast<int>(std::stoll(argv[2]));
            aspect_ratio = image_width / static_cast<float>(h);
            return h;
        } else {
            return static_cast<int>(image_width / aspect_ratio);
        }
    }();
    const int samples_per_pixel = [argc, argv]() {
        return argc > 3 ? static_cast<int>(std::stoll(argv[3])) : 100;
    }();
    const int max_depth = [argc, argv]() {
        return argc > 4 ? static_cast<int>(std::stoll(argv[4])) : 50;
    }();


    //World
    HittableList world = random_scene();

    //Camera
    const auto lookFrom = Point3{13.0f, 2.0f, 3.0f};
    const auto lookAt = Point3{0.0f, 0.0f, 0.0f};
    const auto vUp = Vector3{0.0f, 1.0f, 0.0f};
    const auto distance_to_focus = 10.0f;
    const auto aperture = 0.1f;

    Camera camera{lookFrom, lookAt, vUp, 20, aspect_ratio, aperture, distance_to_focus};

    //Render
    const int max_pixel_value = 255;

    std::ofstream bin_file("image_binary.ppm", std::ios_base::binary);
    bin_file << "P6\n" << image_width << ' ' << image_height << '\n' << max_pixel_value << '\n';
    
    {
        PROFILE_LOG_SCOPE("Image Generation");
        for(int y = image_height - 1; y >= 0; --y) {
            std::cerr << "\rScanlines remaining: " << y << ' ' << std::flush;
            for(int x = 0; x < image_width; ++x) {
                Color pixel_color{ 0.0f, 0.0f, 0.0f };
                for(int sample = 0; sample < samples_per_pixel; ++sample) {
                    const auto u = (x + random_float()) / (image_width - 1);
                    const auto v = (y + random_float()) / (image_height - 1);
                    const auto r = camera.get_ray(u, v);
                    pixel_color += ray_color(r, world, max_depth);
                }
                write_color_binary(bin_file, pixel_color, samples_per_pixel);
            }
        }
        std::cerr << "\nDone.\n";
    }
    bin_file.close();
    return 0;
}

#endif

Color ray_color(const Ray3& r, const Hittable& world, int depth) {
    hit_record rec{};

    //If we've exceeded the ray bounce limit, no more light is gathered.
    if(depth <= 0) {
        return Color{0.0f, 0.0f, 0.0f};
    }
    if(world.hit(r, 0.001f, infinity, rec)) {
        Ray3 scattered{};
        if(rec.material.scatter(r, rec, scattered)) {
            return rec.material.color * ray_color(scattered, world, depth - 1);
        }
        return Color{0.0f, 0.0f, 0.0f};
    }

    Vector3 direction = unit_vector(r.direction());
    auto t = 0.5f * (direction.y() + 1.0f);
    return (1.0f - t) * Color(1.0f, 1.0f, 1.0f) + t * Color(0.5f, 0.7f, 1.0f);
}

float hit_sphere(const Point3& center, float radius, const Ray3& r) {
    Vector3 oc = r.origin() - center;
    const auto a = r.direction().length_squared();
    const auto half_b = dot(oc, r.direction());
    const auto c = oc.length_squared() - radius * radius;
    const auto discriminant = half_b * half_b - a * c;
    if(discriminant < 0.0f) {
        return -1.0f;
    } else {
        return (-half_b - std::sqrt(discriminant)) / a;
    }
}

HittableList random_scene() {
    HittableList world{};

    const auto ground_material = make_lambertian(MaterialDesc{ Color{0.5f, 0.5f, 0.5f} });
    world.add(std::make_shared<Sphere3>(Point3{0.0f, -1000.0f, 0.0f}, 1000.0f, ground_material));

    for(int a = -11; a < 11; ++a) {
        for(int b = -11; b < 11; ++b) {
            const auto choose_mat = random_float();
            const auto center = Point3{a + 0.9f * random_float(), 0.2f, b + 0.9f * random_float()};

            if((center - Point3{4.0f, 0.2f, 0.0f}).length() > 0.9f) {
                Material material{};
                MaterialDesc desc{};
                if(choose_mat < 0.8f) {
                    desc.color = Color::random() * Color::random();
                    material = make_lambertian(desc);
                } else if(choose_mat < 0.95f) {
                    desc.color = Color::random(0.5f, 1.0f);
                    desc.roughness = random_float(0.0f, 0.5f);
                    desc.metallic = 1.0f;
                    material = make_metal(desc);
                } else {
                    desc.refractionIndex = 1.5f;
                    desc.color = Color{1.0f, 1.0f, 1.0f};
                    desc.roughness = 0.0f;
                    material = make_dielectric(desc);
                }
                world.add(std::make_shared<Sphere3>(center, 0.2f, material));
            }
        }
    }

    const auto glass = make_dielectric(MaterialDesc{ Color{1.0f, 1.0f, 1.0f}, 0.0f, 0.0f, 1.5f });
    const auto lambertian = make_lambertian(MaterialDesc{ Color{0.4f, 0.2f, 0.1f}});
    const auto metal = make_metal(MaterialDesc{ Color{0.7f, 0.6f, 0.5f}, 0.0f, 1.0f});

    world.add(std::make_shared<Sphere3>(Point3{0.0f, 1.0f, 0.0f}, 1.0f, glass));
    world.add(std::make_shared<Sphere3>(Point3{-4.0f, 1.0f, 0.0f}, 1.0f, lambertian));
    world.add(std::make_shared<Sphere3>(Point3{4.0f, 1.0f, 0.0f}, 1.0f, metal));

    return world;
}

