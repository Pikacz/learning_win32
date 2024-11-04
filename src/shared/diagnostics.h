#pragma once

#ifdef WIN_32_BUILD

#include <wchar.h>
#include <windows.h>
#include <stdio.h>

#ifdef DEBUG
#define LOG(format, ...) LOG_PATH( __FILE__, __LINE__, format, ##__VA_ARGS__)
#define CanLog() true

#ifdef TERMINAL_RUN

void SetupDiagnostics()
{
    SetConsoleOutputCP(CP_UTF8);
}

#define LOG_PATH(file, line, format, ...) printf("%s:%lu ", file, line); printf(format, ##__VA_ARGS__)

#else

void SetupDiagnostics()
{
}

wchar_t* _convertToUtf16(const char * utf8)
{
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    wchar_t* utf16 = (wchar_t*) malloc(sizeNeeded * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, utf16, sizeNeeded);
    return utf16;
}

void _logFile(const char * file, unsigned int line)
{
    const char* format = "%s:%lu ";
    int sizeNeeded = _scprintf(format, file, line) + 1;
    char* buffer = (char*) malloc(sizeNeeded * sizeof(char));
    _snprintf_s(buffer, sizeNeeded, sizeNeeded - 1, format, file, line);
    OutputDebugStringA(buffer);
    free(buffer);
}

#define LOG_PATH(file, line, format, ...) \
    { \
        _logFile(file, line); \
        int sizeNeeded = _scprintf(format, ##__VA_ARGS__) + 1; \
        char* buffer = (char*) malloc(sizeNeeded * sizeof(char)); \
        _snprintf_s(buffer, sizeNeeded, sizeNeeded - 1, format, ##__VA_ARGS__); \
        wchar_t* output = _convertToUtf16(buffer); \
        OutputDebugStringW(output); \
        free(buffer); \
        free(output); \
    }

#endif
#else

void SetupDiagnostics()
{
}

#define LOG_PATH(file, line, format, ...)

bool CanLog()
{
    return false;
}

#endif

#endif