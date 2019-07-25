






































#ifndef __nsGtkIMModule_h__
#define __nsGtkIMModule_h__

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsGUIEvent.h"



#ifdef MOZ_PLATFORM_MAEMO
#define NS_IME_ENABLED_ON_PASSWORD_FIELD 1
#endif

class nsWindow;

class nsGtkIMModule
{
public:
    nsrefcnt AddRef()
    {
        NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "mRefCnt is negative");
        ++mRefCnt;
        NS_LOG_ADDREF(this, mRefCnt, "nsGtkIMModule", sizeof(*this));
        return mRefCnt;
    }
    nsrefcnt Release()
    {
        NS_PRECONDITION(mRefCnt != 0, "mRefCnt is alrady zero");
        --mRefCnt;
        NS_LOG_RELEASE(this, mRefCnt, "nsGtkIMModule");
        if (mRefCnt == 0) {
            mRefCnt = 1; 
            delete this;
            return 0;
        }
        return mRefCnt;
    }

protected:
    nsAutoRefCnt mRefCnt;

public:
    
    
    
    nsGtkIMModule(nsWindow* aOwnerWindow);
    ~nsGtkIMModule();

    
    
    bool IsEnabled();

    
    void OnFocusWindow(nsWindow* aWindow);
    
    void OnBlurWindow(nsWindow* aWindow);
    
    void OnDestroyWindow(nsWindow* aWindow);
    
    void OnFocusChangeInGecko(bool aFocus);

    
    
    
    
    
    bool OnKeyEvent(nsWindow* aWindow, GdkEventKey* aEvent,
                      bool aKeyDownEventWasSent = false);

    
    nsresult ResetInputState(nsWindow* aCaller);
    nsresult SetInputMode(nsWindow* aCaller, const IMEContext* aContext);
    nsresult GetInputMode(IMEContext* aContext);
    nsresult CancelIMEComposition(nsWindow* aCaller);

    
    
    static bool IsVirtualKeyboardOpened();

protected:
    
    
    
    
    
    
    nsWindow* mOwnerWindow;

    
    nsWindow* mLastFocusedWindow;

    
    GtkIMContext       *mContext;

#ifndef NS_IME_ENABLED_ON_PASSWORD_FIELD
    
    
    
    
    GtkIMContext       *mSimpleContext;
#endif 

    
    
    
    GtkIMContext       *mDummyContext;

    
    
    IMEContext mIMEContext;

    
    
    
    
    PRUint32 mCompositionStart;

    
    
    nsString mDispatchedCompositionString;

    
    
    GdkEventKey* mProcessingKeyEvent;


    
    
    
    
    bool mIsComposing;
    
    
    bool mIsIMFocused;
    
    
    
    bool mFilterKeyEvent;
    
    
    
    
    bool mIgnoreNativeCompositionEvent;
    
    
    
    
    
    bool mKeyDownEventWasSent;

    
    
    
    static nsGtkIMModule* sLastFocusedModule;

    
    
    static gboolean OnRetrieveSurroundingCallback(GtkIMContext  *aContext,
                                                  nsGtkIMModule *aModule);
    static gboolean OnDeleteSurroundingCallback(GtkIMContext  *aContext,
                                                gint           aOffset,
                                                gint           aNChars,
                                                nsGtkIMModule *aModule);
    static void OnCommitCompositionCallback(GtkIMContext *aContext,
                                            const gchar *aString,
                                            nsGtkIMModule* aModule);
    static void OnChangeCompositionCallback(GtkIMContext *aContext,
                                            nsGtkIMModule* aModule);
    static void OnStartCompositionCallback(GtkIMContext *aContext,
                                           nsGtkIMModule* aModule);
    static void OnEndCompositionCallback(GtkIMContext *aContext,
                                         nsGtkIMModule* aModule);

    
    gboolean OnRetrieveSurroundingNative(GtkIMContext  *aContext);
    gboolean OnDeleteSurroundingNative(GtkIMContext  *aContext,
                                       gint           aOffset,
                                       gint           aNChars);
    void OnCommitCompositionNative(GtkIMContext *aContext,
                                   const gchar *aString);
    void OnChangeCompositionNative(GtkIMContext *aContext);
    void OnStartCompositionNative(GtkIMContext *aContext);
    void OnEndCompositionNative(GtkIMContext *aContext);


    
    
    GtkIMContext* GetContext();

    
    
    
    
    bool IsEditable();

    
    bool IsDestroyed() { return !mOwnerWindow; }

    
    void Focus();

    
    void Blur();

    
    void Init();

    
    
    void ResetIME();

    
    void GetCompositionString(nsAString &aCompositionString);

    
    void SetTextRangeList(nsTArray<nsTextRange> &aTextRangeList);

    
    void SetCursorPosition(PRUint32 aTargetOffset);

    
    PRUint32 GetSelectionOffset(nsWindow* aWindow);

    
    nsresult GetCurrentParagraph(nsAString& aText, PRUint32& aCursorPos);

    
    nsresult DeleteText(const PRInt32 aOffset, const PRUint32 aNChars);

    
    void InitEvent(nsGUIEvent& aEvent);

    
    void PrepareToDestroyContext(GtkIMContext *aContext);

    bool ShouldIgnoreNativeCompositionEvent();

    










    
    bool CommitCompositionBy(const nsAString& aString);

    
    bool DispatchCompositionStart();
    bool DispatchCompositionEnd();

    
    
    bool DispatchTextEvent(const nsAString& aCompositionString,
                             bool aCheckAttr);

};

#endif 
