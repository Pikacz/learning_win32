// DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP


#include <Windows.h>

#include <DirectXMath.h>

#include "diagnostics.h"
#include "Dx12Renderer.h"

#include <tuple>



LRESULT CALLBACK WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool sInSizeMove = false;   
    switch (message)
    {
    case WM_CREATE:
    {
        if (lParam)
        {
            LPCREATESTRUCTW params = reinterpret_cast<LPCREATESTRUCTW>(lParam);
            SetWindowLongPtr(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(params->lpCreateParams));
        }
        break;
    }
    case WM_PAINT:
    {
        Dx12Renderer* renderer = reinterpret_cast<Dx12Renderer*>(GetWindowLongPtr(windowHandle, GWLP_USERDATA));
        if (sInSizeMove && renderer)
        {
            renderer->Render();
        } else {
            PAINTSTRUCT ps;
            std::ignore = BeginPaint(windowHandle, &ps);
            EndPaint(windowHandle, &ps);
        }
        break;
    }
    case WM_SIZE:
    {
        Dx12Renderer* renderer = reinterpret_cast<Dx12Renderer*>(GetWindowLongPtr(windowHandle, GWLP_USERDATA));
        if (!sInSizeMove && renderer)
        {
            renderer->Resize(LOWORD(lParam), HIWORD(lParam));
        }
        break;
    }
    case WM_ENTERSIZEMOVE:
    {
        sInSizeMove = true;
        break;
    }
    case WM_EXITSIZEMOVE:
    {
        sInSizeMove = false;
        Dx12Renderer* renderer = reinterpret_cast<Dx12Renderer*>(GetWindowLongPtr(windowHandle, GWLP_USERDATA));
        if (renderer)
        {
            RECT clientRect;
            GetClientRect(windowHandle, &clientRect);
            renderer->Resize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
        }
        break;
    }
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = 320;
        info->ptMinTrackSize.y = 200;
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    case WM_CLOSE:
    {
        DestroyWindow(windowHandle);
        return 0;
    }
    default:
        break;
    }

    return DefWindowProc(windowHandle, message, wParam, lParam);
}



#ifdef TERMINAL_RUN
int main(int argc, char* argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    HINSTANCE appInstance = GetModuleHandle(NULL);
    int cmdShow = SW_SHOW;
#else
int CALLBACK wWinMain(HINSTANCE appInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int cmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
#endif
    SetupDiagnostics();

    if (!DirectX::XMVerifyCPUSupport())
    {
        LOG("No support for directX math!\n");
        return 1;
    }
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    Dx12Renderer dx12Renderer;

    {
        int width = 800; int height = 600;
    
        WNDCLASSEXW windowClass = {};
        windowClass.cbSize = sizeof(WNDCLASSEXW);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.cbClsExtra = 0;
        windowClass.cbWndExtra = 0;
        windowClass.hInstance = appInstance;
        windowClass.hIcon = NULL;
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        windowClass.lpszMenuName = NULL;
        windowClass.lpszClassName = L"a";
        windowClass.hIconSm = NULL;

        ATOM windowClassAtom = RegisterClassEx(&windowClass);
        if (!windowClassAtom)
        {
            LOG("Unable to register window\n");
            return 1;
        }

        RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        HWND windowHandle = CreateWindowExW(
            0,                                         // dwExStyle,
            (const WCHAR *) windowClassAtom,           // lpClassName,
            L"Okno zi\x0105""bel",                     // lpWindowName,
            WS_OVERLAPPEDWINDOW,                       // dwStyle,
            CW_USEDEFAULT, CW_USEDEFAULT,              // X, Y
            windowRect.right - windowRect.left,        // width
            windowRect.bottom - windowRect.top,        // height
            NULL,                                      // hWndParent
            NULL,                                      // hMenu
            appInstance,                               // hInstance
            &dx12Renderer                              // lpParam
        );
        if (!windowHandle)
        {
            LOG("Unable to create window\n");
            return 1;
        }

        GetClientRect(windowHandle, &windowRect);
        dx12Renderer.Initialize(windowHandle, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
        ShowWindow(windowHandle, cmdShow);
    }
    
    MSG msg = {};

    LARGE_INTEGER previousTimestamp, currentTimestamp;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    QueryPerformanceCounter(&previousTimestamp);
    while (WM_QUIT != msg.message)
    {
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            QueryPerformanceCounter(&currentTimestamp);
            LONGLONG numberOfTicks = (currentTimestamp.QuadPart - previousTimestamp.QuadPart) * 60 / (frequency.QuadPart);
            if (numberOfTicks)
            {
                std::swap(previousTimestamp, currentTimestamp);
            }
            dx12Renderer.Tick(static_cast<uint64_t>(numberOfTicks));
            dx12Renderer.Render();
        }
    }
    
    return static_cast<int>(msg.wParam);
}