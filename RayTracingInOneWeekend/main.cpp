#include "MathUtils.hpp"

#include "Camera.hpp"
#include "Color.hpp"
#include "HittableList.hpp"
#include "Material.hpp"
#include "Sphere3.hpp"
#include "ProfileLogScope.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>

#include <d3d11_4.h>
#include <dxgi1_6.h>

// DEBUG STUFF
#include <D3Dcommon.h>
#include <d3d11sdklayers.h>
#include <dxgidebug.h>

// LIBS
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

void RunMessagePump();
bool UnRegister();

bool isQuitting = false;

#define GetHInstance() ::GetModuleHandleA(nullptr)

int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int) {

    //Create Window 1600x900 client area
    if (auto hwnd = Create(); hwnd) {
        if(InitializeDX11(hwnd)) {
            while (!isQuitting) {
                RunMessagePump();
            }
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
        RECT rect{ 0, 0, 1600, 900 };
        auto windowStyle = WS_OVERLAPPEDWINDOW;
        if (::AdjustWindowRect(&rect, windowStyle, false)) {
            const auto width = rect.right - rect.left;
            const auto height = rect.bottom - rect.top;
            if (auto hwnd = ::CreateWindowA("SimpleWindowClass", "Raytracing in One Weekend", windowStyle, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, GetHInstance(), nullptr); hwnd) {
                hdc = ::GetDCEx(hwnd, nullptr, 0);
                ::ShowWindow(hwnd, SW_SHOW);
                return hwnd;
            }
        }
    }
    return 0;
}

bool InitializeDX11(HWND hwnd) {

	//Create DX11 Device
	ID3D11Device* tempDevice{};
	ID3D11DeviceContext* tempDeviceContext{};
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};
	if (auto hresult = ::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevels, 2, D3D11_SDK_VERSION, &tempDevice, nullptr, &tempDeviceContext); FAILED(hresult)) {
		return false;
	}

	ID3D11Device5* device{};
	if (auto hresult = tempDevice->QueryInterface(__uuidof(ID3D11Device5), reinterpret_cast<void**>(&device)); FAILED(hresult)) {
        tempDeviceContext->Release();
        tempDeviceContext = nullptr;
		tempDevice->Release();
		tempDevice = nullptr;
		return false;
	}
	else {
		tempDevice->Release();
		tempDevice = nullptr;
	}

	ID3D11DeviceContext4* deviceContext{};
	if (auto hresult = tempDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext4), reinterpret_cast<void**>(&deviceContext)); FAILED(hresult)) {
		tempDeviceContext->Release();
		tempDeviceContext = nullptr;
		tempDevice->Release();
		tempDevice = nullptr;
		return false;
	}
	else {
		tempDeviceContext->Release();
		tempDeviceContext = nullptr;
	}

	//Create DXGI interfaces

    //Get IDXGI Device

    IDXGIDevice4* dxgiDevice{};
    if (auto hresult = device->QueryInterface(__uuidof(IDXGIDevice4), reinterpret_cast<void**>(&dxgiDevice)); FAILED(hresult)) {
    	device->Release();
		device = nullptr;
        deviceContext->Release();
        deviceContext = nullptr;
        return false;
    }

    //Get Adapter
    IDXGIAdapter4* adapter{};
    if (auto hresult = dxgiDevice->GetParent(__uuidof(IDXGIAdapter4), reinterpret_cast<void**>(&adapter)); FAILED(hresult)) {
		dxgiDevice->Release();
		dxgiDevice = nullptr;
		device->Release();
		device = nullptr;
		deviceContext->Release();
		deviceContext = nullptr;
		return false;

    }

	IDXGIFactory4* tempFactory{};
    if (auto hresult = adapter->GetParent(__uuidof(IDXGIFactory4), reinterpret_cast<void**>(&tempFactory)); FAILED(hresult)) {
		dxgiDevice->Release();
		dxgiDevice = nullptr;
		device->Release();
		device = nullptr;
		deviceContext->Release();
		deviceContext = nullptr;
    }

    IDXGIFactory7* factory{};
    if(auto hresult = tempFactory->QueryInterface(__uuidof(IDXGIFactory7), reinterpret_cast<void**>(&factory)); FAILED(hresult)) {
        tempFactory->Release();
        tempFactory = nullptr;
		dxgiDevice->Release();
		dxgiDevice = nullptr;
		device->Release();
		device = nullptr;
		deviceContext->Release();
		deviceContext = nullptr;
        return false;
    }
	tempFactory->Release();
	tempFactory = nullptr;


	std::vector<IDXGIAdapter4*> adapters{};
	{
		IDXGIAdapter4* cur_adapter{};
		for (unsigned int i = 0u;
			SUCCEEDED(factory->EnumAdapterByGpuPreference(
				i,
				DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
				__uuidof(IDXGIAdapter4),
				reinterpret_cast<void**>(&cur_adapter)));
			++i) {
			adapters.push_back(cur_adapter);
		}

		if (adapters.empty()) {
			cur_adapter->Release();
			cur_adapter = nullptr;
			return false;
		}
	}

    auto* chosen_adapter = adapters[0];

	std::vector<IDXGIOutput*> outputs;
	{
		IDXGIOutput* output;
		for (int i = 0; DXGI_ERROR_NOT_FOUND != chosen_adapter->EnumOutputs(i, &output); ++i) {
			outputs.push_back(output);
		}
	}

	//Create Swapchain
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
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.Flags = 0;

	IDXGISwapChain1* swapchain1{};
    if (auto hresult = factory->CreateSwapChainForHwnd(device, hwnd, &desc, nullptr, nullptr, &swapchain1); FAILED(hresult)) {
        factory->Release();
        factory = nullptr;
		dxgiDevice->Release();
		dxgiDevice = nullptr;
		device->Release();
		device = nullptr;
		deviceContext->Release();
		deviceContext = nullptr;
        return false;
    }

	IDXGISwapChain4* swapchain4{};
    if (auto hresult = swapchain1->QueryInterface(__uuidof(IDXGISwapChain4), reinterpret_cast<void**>(&swapchain4)); FAILED(hresult)) {
		swapchain1->Release();
		swapchain1 = nullptr;
		factory->Release();
		factory = nullptr;
		dxgiDevice->Release();
		dxgiDevice = nullptr;
		device->Release();
		device = nullptr;
		deviceContext->Release();
		deviceContext = nullptr;
    }
	swapchain1->Release();
	swapchain1 = nullptr;

    //TODO (casey): Start Here
    //swapchain4->GetBuffer(/* Fill In BackBuffer UUID */);

	//Create Shader

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
        isQuitting = true;
        break;
    default:
        return ::DefWindowProc(hWnd, Msg, wParam, lParam);
    }
    return ::DefWindowProc(hWnd, Msg, wParam, lParam);
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
