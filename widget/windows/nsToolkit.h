




#ifndef nsToolkit_h__
#define nsToolkit_h__

#include "nsdefs.h"

#include "nsITimer.h"
#include "nsCOMPtr.h"
#include <windows.h>


#ifndef GET_X_LPARAM
#define GET_X_LPARAM(pt) (short(LOWORD(pt)))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(pt) (short(HIWORD(pt)))
#endif





 

class nsToolkit
{
public:
    nsToolkit();

private:
    ~nsToolkit();

public:
    static nsToolkit* GetToolkit();

    static HINSTANCE mDllInstance;

    static void Startup(HMODULE hModule);
    static void Shutdown();
    static void StartAllowingD3D9();

protected:
    static nsToolkit* gToolkit;

    nsCOMPtr<nsITimer> mD3D9Timer;
};

#endif  
