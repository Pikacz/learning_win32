#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>

int getLatestModificationTime(const char *directory, FILETIME *latestTime) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;

    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\*", directory);

    hFind = FindFirstFile(path, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Error opening directory %s: %lu\n", directory, GetLastError());
        return EXIT_FAILURE;
    }

    do {
        if (strcmp(findFileData.cFileName, ".") != 0 &&
            strcmp(findFileData.cFileName, "..") != 0) {
            snprintf(path, sizeof(path), "%s\\%s", directory, findFileData.cFileName);

            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                getLatestModificationTime(path, latestTime);
            } else {
                if (CompareFileTime(&findFileData.ftLastWriteTime, latestTime) > 0) {
                    *latestTime = findFileData.ftLastWriteTime;
                }
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
    return EXIT_SUCCESS;
}

void printFileTime(FILETIME ft) {
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    printf("%02d/%02d/%04d %02d:%02d:%02d\n", 
           st.wDay, st.wMonth, st.wYear, 
           st.wHour, st.wMinute, st.wSecond);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <directory_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILETIME latestTime = { 0 };
    
    int result = getLatestModificationTime(argv[1], &latestTime);
    printFileTime(latestTime);

    return result;
}