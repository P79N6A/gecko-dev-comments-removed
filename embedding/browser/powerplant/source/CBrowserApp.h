






































#pragma once

#include <PP_Prefix.h>
#include <LApplication.h>

#include "nsError.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"

#ifndef USE_PROFILES
class nsProfileDirServiceProvider;
#endif

class CBrowserApp : public PP_PowerPlant::LApplication

#ifdef USE_PROFILES
                      ,public nsIObserver
                      ,public nsSupportsWeakReference
#endif

{
    friend class CStartUpTask;
    
public:
                            CBrowserApp();  
    virtual                 ~CBrowserApp();   

#ifdef USE_PROFILES
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
#endif

    virtual void            AdjustCursor(const EventRecord& inMacEvent);

    virtual void            HandleAppleEvent(const AppleEvent&  inAppleEvent,
                                             AppleEvent&        outAEReply,
                                             AEDesc&            outResult,
                                             long               inAENumber);

    
    virtual Boolean         ObeyCommand(PP_PowerPlant::CommandT inCommand, void* ioParam);  


    
    virtual void            FindCommandStatus(PP_PowerPlant::CommandT inCommand,
                                              Boolean &outEnabled, Boolean &outUsesMark,
                                              UInt16 &outMark, Str255 outName);

   virtual Boolean          AttemptQuitSelf(SInt32 inSaveOption);


protected:

    virtual nsresult        OverrideComponents();
    virtual void            MakeMenuBar();
    virtual void						Initialize();

#if TARGET_CARBON
    virtual void            InstallCarbonEventHandlers();
    static pascal OSStatus  AppEventHandler(EventHandlerCallRef myHandlerChain,
                                            EventRef event,
                                            void* userData);
#endif

    virtual nsresult        InitializePrefs();
    virtual void            OnStartUp();

    virtual Boolean         SelectFileObject(PP_PowerPlant::CommandT    inCommand,
                                             FSSpec& outSpec);

#ifdef USE_PROFILES
    Boolean                 ConfirmProfileSwitch();
#else
    nsProfileDirServiceProvider* mProfDirServiceProvider;
#endif

};
