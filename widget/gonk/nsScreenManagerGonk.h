















#ifndef nsScreenManagerGonk_h___
#define nsScreenManagerGonk_h___

#include "mozilla/Hal.h"
#include "nsCOMPtr.h"

#include "nsBaseScreen.h"
#include "nsIScreenManager.h"

class nsRunnable;
class nsWindow;

class nsScreenGonk : public nsBaseScreen
{
    typedef mozilla::hal::ScreenConfiguration ScreenConfiguration;

public:
    nsScreenGonk(uint32_t aId, ANativeWindow* aNativeWindow);

    ~nsScreenGonk();

    NS_IMETHOD GetId(uint32_t* aId);
    NS_IMETHOD GetRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
    NS_IMETHOD GetAvailRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
    NS_IMETHOD GetPixelDepth(int32_t* aPixelDepth);
    NS_IMETHOD GetColorDepth(int32_t* aColorDepth);
    NS_IMETHOD GetRotation(uint32_t* aRotation);
    NS_IMETHOD SetRotation(uint32_t  aRotation);

    uint32_t GetId();
    nsIntRect GetRect();
    float GetDpi();
    ANativeWindow* GetNativeWindow();
    nsIntRect GetNaturalBounds();
    uint32_t EffectiveScreenRotation();
    ScreenConfiguration GetConfiguration();
    bool IsPrimaryScreen();

    void RegisterWindow(nsWindow* aWindow);
    void UnregisterWindow(nsWindow* aWindow);
    void BringToTop(nsWindow* aWindow);

    const nsTArray<nsWindow*>& GetTopWindows() const
    {
        return mTopWindows;
    }

protected:
    uint32_t mId;
    int32_t mColorDepth;
    android::sp<ANativeWindow> mNativeWindow;
    float mDpi;
    nsIntRect mNaturalBounds; 
    nsIntRect mVirtualBounds; 
    uint32_t mScreenRotation;
    uint32_t mPhysicalScreenRotation;
    nsTArray<nsWindow*> mTopWindows;
};

class nsScreenManagerGonk final : public nsIScreenManager
{
public:
    enum {
        
        PRIMARY_SCREEN_TYPE = 0,

        
        PRIMARY_SCREEN_ID = 0,
    };

public:
    nsScreenManagerGonk();

    NS_DECL_ISUPPORTS
    NS_DECL_NSISCREENMANAGER

    static already_AddRefed<nsScreenManagerGonk> GetInstance();
    static already_AddRefed<nsScreenGonk> GetPrimaryScreen();

    void Initialize();
    void DisplayEnabled(bool aEnabled);

    void AddScreen(uint32_t aDisplayType);
    void RemoveScreen(uint32_t aDisplayType);

protected:
    ~nsScreenManagerGonk();
    void VsyncControl(bool aEnabled);
    uint32_t GetIdFromType(uint32_t aDisplayType);

    bool mInitialized;
    nsTArray<nsRefPtr<nsScreenGonk>> mScreens;
    nsRefPtr<nsRunnable> mScreenOnEvent;
    nsRefPtr<nsRunnable> mScreenOffEvent;
};

#endif
