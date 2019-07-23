











































#include <windows.h>
#include <stdio.h>

static const char *kMozHeapDumpMessageString = "MOZ_HeapDump";
static const char *kMozAppClassName = "MozillaWindowClass";

BOOL IsMozilla(const char *title)
{
    if (!title || !*title)
        return FALSE;
    
    if (strstr(title, " - Mozilla"))
      return TRUE;

    
    
    if (strstr(title, "Mozilla {"))
      return TRUE;

    return FALSE;
}

BOOL IsNetscape(const char *title)
{
    if (!title || !*title)
        return FALSE;
    
    return strstr(title, " - Netscape 6") ? TRUE : FALSE;
}





BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    HWND * pwnd = (HWND *)lParam;
    char buf[1024];

    
    GetClassName(hwnd, buf, sizeof(buf)-1);
    if (strcmp(buf, kMozAppClassName))
        return TRUE;

    GetWindowText(hwnd, buf, sizeof(buf)-1);

    if (IsMozilla(buf)) {
        
        *pwnd = hwnd;
        return FALSE;
    }
    
    
    
    if (IsNetscape(buf))
        *pwnd = hwnd;
    
    return TRUE;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    UINT msgHandle = RegisterWindowMessage(kMozHeapDumpMessageString);

    
    
    
    
    
    HWND mozwindow = 0;
    EnumWindows(EnumWindowsProc, (LPARAM) &mozwindow);
    if (mozwindow == 0) {
        printf("Cannot find Mozilla or Netscape 6 window. Exit\n");
        exit(-1);
    }

    
    char buf[1024];
    GetWindowText(mozwindow, buf, sizeof(buf)-1);
    if (IsMozilla(buf))
        printf("Found Mozilla window with title : %s\n", buf); 
    else if (IsNetscape(buf))
        printf("Found Netscape window with title : %s\n", buf); 

    SendMessage(mozwindow, msgHandle, 0, 0);

    printf("Sending HeapDump Message done.  Heapdump available in c:\\heapdump.txt\n");
    return 0;
}

