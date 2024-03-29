



#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

#include "resource.h"

#define MAX_LOADSTRING 100

TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];

int APIENTRY _tWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow) {
  
  int count = 0;
  LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
  if (szTitle[0] != 0) ++count;
  LoadString(hInstance, IDC_HELLO, szWindowClass, MAX_LOADSTRING);
  if (szWindowClass[0] != 0) ++count;
  if (LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL)) != NULL) ++count;
  if (LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HELLO)) != NULL) ++count;
  return count;
}
