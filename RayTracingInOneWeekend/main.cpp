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

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")

#include <d3d11shader.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")


Color ray_color(const Ray3& r, const Hittable& world, int depth);
float hit_sphere(const Point3& center, float radius, const Ray3& r);

HittableList random_scene();

#ifdef _WIN32

int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int);

HWND Create();
bool InitializeDX11(HWND hwnd);
void ReleaseGlobalDXResources();
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
bool CreateBlendState();
bool CreateSamplerState();
bool CreateRasterState();

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
        if(InitializeDX11(g_hWnd)) {
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
            ReleaseGlobalDXResources();
        }
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
                aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
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

IDXGIFactory7* factory{};
std::vector<IDXGIAdapter4*> adapters{};
IDXGIAdapter4* adapter{};
std::vector<IDXGIOutput*> outputs{};
ID3D11Device5* device{};
ID3D11DeviceContext4* deviceContext{};
IDXGIDevice4* dxgiDevice{};
IDXGISwapChain4* swapchain4{};
ID3D11RenderTargetView* backbuffer{};
ID3D11VertexShader* vs{};
ID3D11PixelShader* ps{};
D3D_FEATURE_LEVEL highest_feature_level;

#define SAFE_RELEASE(x) { \
    if(x) { \
        x->Release(); \
        x = nullptr; \
    } \
} \

void ReleaseGlobalDXResources() {
    SAFE_RELEASE(ps);
    SAFE_RELEASE(vs);
    SAFE_RELEASE(backbuffer);
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

}

bool InitializeDX11(HWND hwnd) {
    {
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
    }

    //Get Adapter
    {
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

        if (adapters.empty()) {
            ReleaseGlobalDXResources();
            return false;
        }
    }

    adapter = adapters[0];
    {
        IDXGIOutput* output{};
        for (int i = 0; DXGI_ERROR_NOT_FOUND != adapter->EnumOutputs(i, &output); ++i) {
            outputs.push_back(output);
        }
    }

    //Create DX11 Device
    {
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
            unsigned int flags{0u};
#ifdef _DEBUG
            flags |= D3D11_CREATE_DEVICE_DEBUG;
            //flags |= D3D11_CREATE_DEVICE_DEBUGGABLE;
#else
            flags |= D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY;
#endif
            flags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;
            {
                DXGI_ADAPTER_DESC3 adapterDesc{};
                adapter->GetDesc3(&adapterDesc);
                if(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {
                    ::OutputDebugStringA("D3D11 Device creation failed. Software Devices are not supported.");
                    ReleaseGlobalDXResources();
                    return false;
                }
            }
            if (auto hresult = ::D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, featureLevels.data(), static_cast<unsigned int>(featureLevels.size()), D3D11_SDK_VERSION, &tempDevice, &highest_feature_level, &tempDeviceContext); FAILED(hresult)) {
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
    }
    //Create DXGI interfaces

    //Get IDXGI Device

    if (auto hresult = device->QueryInterface(__uuidof(IDXGIDevice4), reinterpret_cast<void**>(&dxgiDevice)); FAILED(hresult)) {
        ReleaseGlobalDXResources();
        return false;
    }

    //Create Swapchain
    {
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
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        IDXGISwapChain1* swapchain1{};
        if (auto hresult = factory->CreateSwapChainForHwnd(device, hwnd, &desc, nullptr, nullptr, &swapchain1); FAILED(hresult)) {
            ReleaseGlobalDXResources();
            return false;
        }

        if (auto hresult = swapchain1->QueryInterface(__uuidof(IDXGISwapChain4), reinterpret_cast<void**>(&swapchain4)); FAILED(hresult)) {
            SAFE_RELEASE(swapchain1);
            ReleaseGlobalDXResources();
        }
        SAFE_RELEASE(swapchain1);
    }

    // Create Backbuffer
    ID3D11Texture2D* tBackbuffer{};
    if (FAILED(swapchain4->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tBackbuffer)))) {
        ReleaseGlobalDXResources();
        return false;
    }

    if (FAILED(device->CreateRenderTargetView(tBackbuffer, nullptr, &backbuffer))) {
        SAFE_RELEASE(tBackbuffer);
        ReleaseGlobalDXResources();
        return false;
    }
    SAFE_RELEASE(tBackbuffer);

    deviceContext->OMSetRenderTargets(1, &backbuffer, nullptr);

    if (!CreateBlendState()) {
        return false;
    }
    if (!CreateSamplerState()) {
        return false;
    }
    if (!CreateRasterState()) {
        return false;
    }

    if (!CreateShaders()) {
        return false;
    }

    return true;
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
    return true;
}

bool CreateRasterState() {
    ID3D11RasterizerState2* state{};
    D3D11_RASTERIZER_DESC2 desc{};
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_BACK;
    //TODO (casey): Check FrontCounterClockwise
    desc.FrontCounterClockwise = false;
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
    return true;
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
            return false;
        }
    } else {
        std::error_code hlsl_ec{};
        if (std::filesystem::path hlsl_path_canon = std::filesystem::canonical(hlsl_path, hlsl_ec); hlsl_ec) {
            OutputError(hlsl_path.string() + " could not be accessed. Reason: " + hlsl_ec.message() + '\n');
            return false;
        } else {
            hlsl_path_canon.make_preferred();
            if (CompileVertexShaderFromFile(vs_bytecode, hlsl_path_canon) && !WriteShaderCacheToFile(hlsl_path_canon, vs_bytecode)) {
                return false;
            }
        }
    }
    if (!CreateVS(vs_bytecode)) {
        return false;
    }
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
            return false;
        }
    }
    else {
        std::error_code hlsl_ec{};
        if (std::filesystem::path hlsl_path_canon = std::filesystem::canonical(hlsl_path, hlsl_ec); hlsl_ec) {
            OutputError(hlsl_path.string() + " could not be accessed. Reason: " + hlsl_ec.message() + '\n');
            return false;
        } else {
            hlsl_path_canon.make_preferred();
            if (CompilePixelShaderFromFile(ps_bytecode, hlsl_path_canon) && !WriteShaderCacheToFile(hlsl_path_canon, ps_bytecode)) {
                return false;
            }
        }
    }
    if (!CreatePS(ps_bytecode)) {
        return false;
    }
    return true;
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
        return false;
    } else {
        const std::array desc = {
            D3D11_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"COLOR"   , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        ID3D11InputLayout* il{};
        if (auto hresult_input = device->CreateInputLayout(desc.data(), static_cast<unsigned int>(desc.size()), vs_bytecode->GetBufferPointer(), vs_bytecode->GetBufferSize(), &il); FAILED(hresult_input)) {
            OutputError("FAILED TO CREATE INPUT LAYOUT\n");
            return false;
        }
        else {
            deviceContext->IASetInputLayout(il);
            //TODO (casey): Create Vertex Buffer? Look in to VertexID since the vertex buffer doesn't do much here.
            ID3D11Buffer* vBuffer{ nullptr };
            unsigned int stride{sizeof(Vector3)};
            unsigned int offset{0u};
            deviceContext->IASetVertexBuffers(0u, 1u, &vBuffer, &stride, &offset);
            deviceContext->VSSetShader(vs, nullptr, 0u);
        }
    }
    return true;
}

bool CreatePS(ID3DBlob* ps_bytecode) {
    if (auto hresult_create = device->CreatePixelShader(ps_bytecode->GetBufferPointer(), ps_bytecode->GetBufferSize(), nullptr, &ps); FAILED(hresult_create)) {
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
    default:
        return ::DefWindowProc(hWnd, Msg, wParam, lParam);
    }
    return ::DefWindowProc(hWnd, Msg, wParam, lParam);
}

void BeginFrame() {
}

void Update(float /*deltaSeconds*/) {
}

void Render() {
    float color[] = {0.0f, 0.0f, 0.0f, 1.0f};
    deviceContext->ClearRenderTargetView(backbuffer, color);
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

