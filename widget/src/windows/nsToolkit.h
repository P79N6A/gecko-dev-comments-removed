





































#ifndef TOOLKIT_H      
#define TOOLKIT_H

#include "nsdefs.h"
#include "nsIToolkit.h"

#include "nsITimer.h"
#include "nsCOMPtr.h"

#include <imm.h>


#ifndef GET_X_LPARAM
#define GET_X_LPARAM(pt) (short(LOWORD(pt)))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(pt) (short(HIWORD(pt)))
#endif

class nsIEventQueue;
class MouseTrailer;







#define FILE_BUFFER_SIZE 4096 






 

class nsToolkit : public nsIToolkit
{

  public:

            NS_DECL_ISUPPORTS

                            nsToolkit();
            NS_IMETHOD      Init(PRThread *aThread);
            void            CreateInternalWindow(PRThread *aThread);

private:
                            ~nsToolkit();
            void            CreateUIThread(void);

public:
    
    static LRESULT CALLBACK WindowProc(HWND hWnd, 
                                        UINT Msg, 
                                        WPARAM WParam, 
                                        LPARAM lParam);

protected:
    
    HWND        mDispatchWnd;
    
    PRThread    *mGuiThread;
    nsCOMPtr<nsITimer> mD3D9Timer;

public:
    static HINSTANCE mDllInstance;
    
    static bool      mIsWinXP;

    static bool InitVersionInfo();
    static void Startup(HINSTANCE hModule);
    static void Shutdown();
    static void StartAllowingD3D9();

    static MouseTrailer *gMouseTrailer;
};

class  nsWindow;






 

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

#endif  
