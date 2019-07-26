




#ifndef GFX_FONT_INFO_LOADER_H
#define GFX_FONT_INFO_LOADER_H

#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsITimer.h"



class gfxFontInfoLoader {
public:

    
    
    
    
    
    
    
    
    typedef enum {
        stateInitial,
        stateTimerOnDelay,
        stateTimerOnInterval,
        stateTimerOff
    } TimerState;

    gfxFontInfoLoader() :
        mInterval(0), mState(stateInitial)
    {
    }

    virtual ~gfxFontInfoLoader();

    
    void StartLoader(uint32_t aDelay, uint32_t aInterval);

    
    void CancelLoader();

protected:
    class ShutdownObserver : public nsIObserver
    {
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSIOBSERVER

        ShutdownObserver(gfxFontInfoLoader *aLoader)
            : mLoader(aLoader)
        { }

        virtual ~ShutdownObserver()
        { }

    protected:
        gfxFontInfoLoader *mLoader;
    };

    
    virtual void InitLoader() = 0;

    
    virtual bool RunLoader() = 0;

    
    virtual void FinishLoader() = 0;

    
    static void LoaderTimerCallback(nsITimer *aTimer, void *aThis) {
        gfxFontInfoLoader *loader = static_cast<gfxFontInfoLoader*>(aThis);
        loader->LoaderTimerFire();
    }

    void LoaderTimerFire();

    void RemoveShutdownObserver();

    nsCOMPtr<nsITimer> mTimer;
    nsCOMPtr<nsIObserver> mObserver;
    uint32_t mInterval;
    TimerState mState;
};

#endif 
