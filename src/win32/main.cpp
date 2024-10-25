#include <windows.h>
#include <cstdio>
#include "diagnostics.h"



#ifdef TERMINAL_RUN
int main(int argc, char* argv[])
{

    HINSTANCE hInstance = GetModuleHandle(NULL);
#else
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
#endif
    SetupDiagnostics();
  
    LOG("elo elo ziąbel\n");
    LOG("elo elo ziąbel2 %d %s\n", 1, "dwa i trzy");
    LOG("dwa i trzy\n");
    
    return 0;
}