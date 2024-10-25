#pragma once

#include <wchar.h>
#include <windows.h>
#include <stdio.h>




#ifdef DEBUG


#ifdef TERMINAL_RUN
void SetupDiagnostics()
{
    SetConsoleOutputCP(CP_UTF8);
}

#define LOG(fmt, ...) printf("%s:%lu ", __FILE__, __LINE__); printf(fmt, ##__VA_ARGS__)

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
    int sizeNeeded = _snprintf(NULL, 0, format, file, line) + 1;
    char* buffer = (char*) malloc(sizeNeeded * sizeof(char));
    _snprintf(buffer, sizeNeeded, format, file, line);
    OutputDebugStringA(buffer);
    free(buffer);
}

#define LOG(format, ...) \
    { \
        _logFile( __FILE__, __LINE__); \
        int sizeNeeded = _snprintf(NULL, 0, format, ##__VA_ARGS__) + 1; \
        char* buffer = (char*) malloc(sizeNeeded * sizeof(char)); \
        _snprintf(buffer, sizeNeeded, format, ##__VA_ARGS__); \
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

#define LOG(format, ...)

#endif