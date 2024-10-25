#include <windows.h>
#include <cstdio>
#include "diagnostics.h"

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    case WM_CLOSE:
    {
        DestroyWindow(window);
        return 0;
    }
    default:
        break;
    }

    return DefWindowProc(window, message, wParam, lParam);
}


#ifdef TERMINAL_RUN
int main(int argc, char* argv[])
{

    HINSTANCE hInstance = GetModuleHandle(NULL);
#else
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
#endif
    SetupDiagnostics();

    WNDCLASSEX windowClass = { 0 };
    
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = hInstance;
    windowClass.hIcon = NULL;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground;
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = TEXT("a");
    windowClass.hIconSm = NULL;

    ATOM windowClassAtom = RegisterClassEx(&windowClass);

    HWND window = CreateWindowEx(
        0,                                         // dwExStyle,
        (const WCHAR *) windowClassAtom,           // lpClassName,
        TEXT("Okno zi\x0105""bel"),                // lpWindowName,
        WS_OVERLAPPEDWINDOW,                       // dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,    // X, Y, width, height
        NULL,                                      // hWndParent
        NULL,                                      // hMenu
        hInstance,                                 // hInstance
        NULL                                       // lpParam
    );
    
    ShowWindow(window, SW_SHOW);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}