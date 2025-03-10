#pragma once
#include "task.hpp"
#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <d3d11.h>
#include <tchar.h>

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

HWND FindRobloxWindow(DWORD process_id) {
    HWND hwnd = nullptr;
    while ((hwnd = FindWindowEx(nullptr, hwnd, nullptr, nullptr))) {
        DWORD window_process_id;
        GetWindowThreadProcessId(hwnd, &window_process_id);
        if (window_process_id == process_id) {
            wchar_t class_name[256];
            wchar_t window_title[256];
            GetClassName(hwnd, class_name, sizeof(class_name) / sizeof(wchar_t));
            GetWindowText(hwnd, window_title, sizeof(window_title) / sizeof(wchar_t));
            printf("[DEBUG] Found window with class name: %ls, title: %ls\n", class_name, window_title);
            if (wcsstr(class_name, L"WINDOWSCLIENT") != nullptr) {
                return hwnd;
            }
        }
    }
    return nullptr;
}

std::string dksm330a(size_t length) {
    const std::string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

    std::string random_string;
    for (size_t i = 0; i < length; ++i) {
        random_string += CHARACTERS[distribution(generator)];
    }
    return random_string;
}

bool isAppResponding() {
    DWORD dwExitCode;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, task_scheduler::get_roblox_process_id());
    if (hProcess == NULL) {
        return false;
    }
    if (GetExitCodeProcess(hProcess, &dwExitCode)) {
        if (dwExitCode == STILL_ACTIVE) {
            CloseHandle(hProcess);
            return true;
        }
    }
    CloseHandle(hProcess);
    return false;
}

int task() {
    DWORD process_id = task_scheduler::get_roblox_process_id();
    if (process_id == 0) {
        printf("[ERROR] Failed to find Roblox process.\n");
        return 1;
    }
    system("title NVIDIA app");
    system("clear");
    printf("[LOG] Roblox Process ID: %lu\n", process_id);

    HWND roblox_hwnd = FindRobloxWindow(process_id);
    if (!roblox_hwnd) {
        printf("[ERROR] Failed to find Roblox window.\n");
        return 1;
    }
    printf("[LOG] Roblox Window Handle: 0x%p\n", roblox_hwnd);

    HANDLE process_handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, process_id);
    if (process_handle == NULL) {
        printf("[ERROR] Failed to open process handle. Error Code: %lu\n", GetLastError());
        return 1;
    }

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, _T("ImGui Overlay"), nullptr };
    ::RegisterClassEx(&wc);
    HWND overlay_hwnd = ::CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST, wc.lpszClassName, _T("NVIDIA overlay"), WS_POPUP, 0, 0, 1920, 1080, nullptr, nullptr, wc.hInstance, nullptr
    );
    SetWindowLong(overlay_hwnd, GWL_EXSTYLE, GetWindowLong(overlay_hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(overlay_hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    SetWindowLongPtr(overlay_hwnd, GWLP_HWNDPARENT, (LONG_PTR)roblox_hwnd);
    SetLayeredWindowAttributes(overlay_hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    if (!CreateDeviceD3D(overlay_hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(overlay_hwnd, SW_SHOW);
    ::UpdateWindow(overlay_hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(overlay_hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    system("cls");
    std::cout << " ____   __    ___  _  _ \n"
        << "(_  _) /__\\  / __)( )/ )\n"
        << "  )(  /(__)\\ \\__ \\ )  ( \n"
        << " (__)(__)(__)(___/(_)\\_)\n@8snd, taskscheduler.\n"
        << std::endl;

    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        if (!isAppResponding()) {
            printf("[ERROR] Roblox is no longer responding. Exiting...\n");
            break;
        }

        RECT roblox_rect;
        GetWindowRect(roblox_hwnd, &roblox_rect);
        SetWindowPos(overlay_hwnd, nullptr, roblox_rect.left, roblox_rect.top,
            roblox_rect.right - roblox_rect.left, roblox_rect.bottom - roblox_rect.top, SWP_NOZORDER | SWP_NOACTIVATE);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)(roblox_rect.right - roblox_rect.left), (float)(roblox_rect.bottom - roblox_rect.top)));
        ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
        ImGui::Text("\n\nRoblox task scheduler. @8snd");

        uintptr_t base_address = task_scheduler::get_base_address(process_id);
        if (base_address == 0) {
            printf("[ERROR] Failed to get base address.\n");
            CloseHandle(process_handle);
            return 1;
        }

        uintptr_t task_scheduler = 0;
        while (task_scheduler == 0) {
            task_scheduler = task_scheduler::get_scheduler(process_handle, base_address);
            if (task_scheduler == 0) {
                printf("[ERROR] Failed to retrieve TaskScheduler. Retrying...\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }

        uintptr_t datamodel = 0;
        while (datamodel == 0) {
            datamodel = task_scheduler::get_datamodel(process_handle, task_scheduler);
            if (datamodel == 0) {
                printf("[ERROR] Failed to retrieve DataModel. Retrying...\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }

        uintptr_t render_job = 0;
        while (render_job == 0) {
            render_job = task_scheduler::get_renderview(process_handle, task_scheduler);
            if (render_job == 0) {
                printf("[ERROR] RenderJob (renderview) not found. Retrying...\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }

        int total_jobs = task_scheduler::get_total_jobs(process_handle, task_scheduler);

        ImGui::Text("Process Handle: 0x%p", process_handle);
        ImGui::Text("Base Address: 0x%llx", base_address);
        ImGui::Text("TaskScheduler: 0x%llx", task_scheduler);
        ImGui::Text("DataModel: 0x%llx", datamodel);
        ImGui::Text("RenderJob: 0x%llx", render_job);
        ImGui::TextColored(ImVec4(0, 0, 1, 1), "Total Jobs: %d", total_jobs);
        std::string datamodel_name = task_scheduler::get_datamodel_name(process_handle, datamodel);
        if (datamodel_name.empty()) {
            printf("[ERROR] Failed to get DataModel name.\n");
        }

        if (datamodel_name == "Ugc") {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "In game, DataModel is Ugc.");
        }
        else if (datamodel_name == "LuaApp") {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "In homepage, DataModel is LuaApp.");
        }
        else {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "DataModel is neither Ugc nor LuaApp.");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        ImGui::End();

        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 0.01f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(overlay_hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);
    CloseHandle(process_handle);

    return 0;
}

int main() {
    return task();
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pd3dDeviceContext) g_pd3dDeviceContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) g_mainRenderTargetView->Release();
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
