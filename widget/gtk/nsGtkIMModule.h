






#ifndef __nsGtkIMModule_h__
#define __nsGtkIMModule_h__

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIWidget.h"
#include "mozilla/EventForwards.h"

class nsWindow;

class nsGtkIMModule
{
protected:
    typedef mozilla::widget::InputContext InputContext;
    typedef mozilla::widget::InputContextAction InputContextAction;

public:
    nsrefcnt AddRef()
    {
        NS_PRECONDITION(int32_t(mRefCnt) >= 0, "mRefCnt is negative");
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
    
    
    
    explicit nsGtkIMModule(nsWindow* aOwnerWindow);
    ~nsGtkIMModule();

    
    
    bool IsEnabled() const;

    
    void OnFocusWindow(nsWindow* aWindow);
    
    void OnBlurWindow(nsWindow* aWindow);
    
    void OnDestroyWindow(nsWindow* aWindow);
    
    void OnFocusChangeInGecko(bool aFocus);
    
    
    void OnSelectionChange(nsWindow* aCaller);

    
    
    
    
    
    bool OnKeyEvent(nsWindow* aWindow, GdkEventKey* aEvent,
                      bool aKeyDownEventWasSent = false);

    
    nsresult EndIMEComposition(nsWindow* aCaller);
    void SetInputContext(nsWindow* aCaller,
                         const InputContext* aContext,
                         const InputContextAction* aAction);
    InputContext GetInputContext();
    void OnUpdateComposition();

protected:
    
    
    
    
    
    
    nsWindow* mOwnerWindow;

    
    nsWindow* mLastFocusedWindow;

    
    GtkIMContext* mContext;

    
    
    
    
    GtkIMContext* mSimpleContext;

    
    
    
    GtkIMContext* mDummyContext;

    
    
    
    GtkIMContext* mComposingContext;

    
    
    InputContext mInputContext;

    
    
    
    
    uint32_t mCompositionStart;

    
    
    nsString mDispatchedCompositionString;

    
    
    nsString mSelectedString;

    
    
    GdkEventKey* mProcessingKeyEvent;

    
    uint32_t mCompositionTargetOffset;

    
    enum eCompositionState {
        eCompositionState_NotComposing,
        eCompositionState_CompositionStartDispatched,
        eCompositionState_CompositionChangeEventDispatched,
        eCompositionState_CommitCompositionChangeEventDispatched
    };
    eCompositionState mCompositionState;

    bool IsComposing()
    {
        return (mCompositionState != eCompositionState_NotComposing);
    }

    bool EditorHasCompositionString()
    {
        return (mCompositionState ==
                    eCompositionState_CompositionChangeEventDispatched);
    }

    







    bool IsValidContext(GtkIMContext* aContext) const;

#ifdef PR_LOGGING
    const char* GetCompositionStateName()
    {
        switch (mCompositionState) {
            case eCompositionState_NotComposing:
                return "NotComposing";
            case eCompositionState_CompositionStartDispatched:
                return "CompositionStartDispatched";
            case eCompositionState_CompositionChangeEventDispatched:
                return "CompositionChangeEventDispatched";
            case eCompositionState_CommitCompositionChangeEventDispatched:
                return "CommitCompositionChangeEventDispatched";
            default:
                return "InvaildState";
        }
    }
#endif 


    
    
    bool mIsIMFocused;
    
    
    
    bool mFilterKeyEvent;
    
    
    
    
    
    bool mKeyDownEventWasSent;

    
    
    
    static nsGtkIMModule* sLastFocusedModule;

    
    
    
    static bool sUseSimpleContext;

    
    
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

    







    GtkIMContext* GetCurrentContext() const;

    


    GtkIMContext* GetActiveContext() const
    {
        return mComposingContext ? mComposingContext : GetCurrentContext();
    }

    
    bool IsDestroyed() { return !mOwnerWindow; }

    
    void Focus();

    
    void Blur();

    
    void Init();

    
    
    void ResetIME();

    
    void GetCompositionString(GtkIMContext* aContext,
                              nsAString& aCompositionString);

    







    already_AddRefed<mozilla::TextRangeArray>
        CreateTextRangeArray(GtkIMContext* aContext,
                             const nsAString& aLastDispatchedData);

    







    void SetCursorPosition(GtkIMContext* aContext, uint32_t aTargetOffset);

    
    uint32_t GetSelectionOffset(nsWindow* aWindow);

    
    nsresult GetCurrentParagraph(nsAString& aText, uint32_t& aCursorPos);

    







    nsresult DeleteText(GtkIMContext* aContext,
                        int32_t aOffset,
                        uint32_t aNChars);

    
    void InitEvent(mozilla::WidgetGUIEvent& aEvent);

    
    void PrepareToDestroyContext(GtkIMContext *aContext);

    









    






    bool DispatchCompositionStart(GtkIMContext* aContext);

    







    bool DispatchCompositionChangeEvent(GtkIMContext* aContext,
                                        const nsAString& aCompositionString);

    









    bool DispatchCompositionEventsForCommit(GtkIMContext* aContext,
                                            const nsAString& aCommitString);
};

#endif 
