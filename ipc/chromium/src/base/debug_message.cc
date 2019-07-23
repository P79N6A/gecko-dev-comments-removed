



#include <windows.h>






int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow) {
  LPWSTR cmdline = GetCommandLineW();
  MessageBox(NULL, cmdline, L"Kr\x00d8m", MB_TOPMOST);
  return 0;
}
