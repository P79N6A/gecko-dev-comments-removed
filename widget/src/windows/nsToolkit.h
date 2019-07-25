





































#ifndef nsToolkit_h__
#define nsToolkit_h__

#include "nsdefs.h"

#include "nsITimer.h"
#include "nsCOMPtr.h"

#include <imm.h>


#ifndef GET_X_LPARAM
#define GET_X_LPARAM(pt) (short(LOWORD(pt)))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(pt) (short(HIWORD(pt)))
#endif







#define FILE_BUFFER_SIZE 4096 







 

class MouseTrailer 
{
public:
    HWND                  GetMouseTrailerWindow() { return mMouseTrailerWindow; }
    HWND                  GetCaptureWindow() { return mCaptureWindow; }

    void                  SetMouseTrailerWindow(HWND aWnd);
    void                  SetCaptureWindow(HWND aWnd);
    void                  Disable() { mEnabled = false; DestroyTimer(); }
    void                  Enable() { mEnabled = true; CreateTimer(); }
    void                  DestroyTimer();

                          MouseTrailer();
                          ~MouseTrailer();
private:

    nsresult              CreateTimer();

    static void           TimerProc(nsITimer* aTimer, void* aClosure);

    
    HWND                  mMouseTrailerWindow;
    HWND                  mCaptureWindow;
    bool                  mIsInCaptureMode;
    bool                  mEnabled;
    nsCOMPtr<nsITimer>    mTimer;
};





 

class nsToolkit
{
public:
    nsToolkit();

private:
    ~nsToolkit();

public:
    static nsToolkit* GetToolkit();

    static HINSTANCE mDllInstance;
    static MouseTrailer *gMouseTrailer;

    static void Startup(HMODULE hModule);
    static void Shutdown();
    static void StartAllowingD3D9();

protected:
    static nsToolkit* gToolkit;

    nsCOMPtr<nsITimer> mD3D9Timer;
    MouseTrailer mMouseTrailer;
};

#endif  
