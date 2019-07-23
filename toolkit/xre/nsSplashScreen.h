





































#ifndef NS_SPLASHSCREEN_H_
#define NS_SPLASHSCREEN_H_

#include "prtypes.h"






class nsSplashScreen {
public:
    
    
    
    static nsSplashScreen* GetOrCreate();
    static nsSplashScreen* Get();
public:
    
    
    virtual void Open() = 0;
    virtual void Close() = 0;

    
    virtual void Update(PRInt32 progress) = 0;

    PRBool IsOpen() { return mIsOpen; }

protected:
    nsSplashScreen() : mIsOpen(PR_FALSE) { }
    PRBool mIsOpen;
};

extern "C" {
    nsSplashScreen *NS_GetSplashScreen(PRBool create);
    typedef nsSplashScreen* (*NS_GetSplashScreenPtr) (PRBool);
}

#endif 
