





































#ifndef TOOLKIT_H      
#define TOOLKIT_H

#include "nsdefs.h"
#include "prmon.h"
#include "nsIToolkit.h"
#ifdef DEBUG
#include <stdio.h>
#endif

struct MethodInfo;





 

class nsToolkit : public nsIToolkit
{

  public:

            NS_DECL_ISUPPORTS

                            nsToolkit();
            NS_IMETHOD      Init(PRThread *aThread);
            void            CallMethod(MethodInfo *info);
            MRESULT         SendMsg(HWND hwnd, ULONG msg, MPARAM mp1 = 0, MPARAM mp2 = 0);
            
            PRBool          IsGuiThread(void)      { return (PRBool)(mGuiThread == PR_GetCurrentThread());}
            PRThread*       GetGuiThread(void)       { return mGuiThread;   }
            HWND            GetDispatchWindow(void)  { return mDispatchWnd; }
            void            CreateInternalWindow(PRThread *aThread);

private:
                            ~nsToolkit();
            void            CreateUIThread(void);

public:

protected:
    
    HWND        mDispatchWnd;
    
    PRThread    *mGuiThread;
    
    PRMonitor *mMonitor;
};

#define WM_CALLMETHOD   (WM_USER+1)

inline void nsToolkit::CallMethod(MethodInfo *info)
{
    NS_PRECONDITION(::WinIsWindow((HAB)0, mDispatchWnd), "Invalid window handle");
    ::WinSendMsg(mDispatchWnd, WM_CALLMETHOD, (MPARAM)0, MPFROMP(info));
}

#endif  
