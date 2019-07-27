






#ifndef __nsGtkIMModule_h__
#define __nsGtkIMModule_h__

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIWidget.h"
#include "mozilla/CheckedInt.h"
#include "mozilla/EventForwards.h"
#include "WritingModes.h"

class nsWindow;

class nsGtkIMModule
{
protected:
    typedef mozilla::widget::IMENotification IMENotification;
    typedef mozilla::widget::InputContext InputContext;
    typedef mozilla::widget::InputContextAction InputContextAction;

public:
    
    
    
    explicit nsGtkIMModule(nsWindow* aOwnerWindow);

    NS_INLINE_DECL_REFCOUNTING(nsGtkIMModule)

    
    
    bool IsEnabled() const;

    
    void OnFocusWindow(nsWindow* aWindow);
    
    void OnBlurWindow(nsWindow* aWindow);
    
    void OnDestroyWindow(nsWindow* aWindow);
    
    void OnFocusChangeInGecko(bool aFocus);
    
    
    void OnSelectionChange(nsWindow* aCaller,
                           const IMENotification& aIMENotification);

    
    
    
    
    
    bool OnKeyEvent(nsWindow* aWindow, GdkEventKey* aEvent,
                      bool aKeyDownEventWasSent = false);

    
    nsresult EndIMEComposition(nsWindow* aCaller);
    void SetInputContext(nsWindow* aCaller,
                         const InputContext* aContext,
                         const InputContextAction* aAction);
    InputContext GetInputContext();
    void OnUpdateComposition();
    void OnLayoutChange();

protected:
    ~nsGtkIMModule();

    
    
    
    
    
    
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

    struct Range
    {
        uint32_t mOffset;
        uint32_t mLength;

        Range()
            : mOffset(UINT32_MAX)
            , mLength(UINT32_MAX)
        {
        }

        bool IsValid() const { return mOffset != UINT32_MAX; }
        void Clear()
        {
            mOffset = UINT32_MAX;
            mLength = UINT32_MAX;
        }
    };

    
    Range mCompositionTargetRange;

    
    enum eCompositionState {
        eCompositionState_NotComposing,
        eCompositionState_CompositionStartDispatched,
        eCompositionState_CompositionChangeEventDispatched
    };
    eCompositionState mCompositionState;

    bool IsComposing() const
    {
        return (mCompositionState != eCompositionState_NotComposing);
    }

    bool IsComposingOn(GtkIMContext* aContext) const
    {
        return IsComposing() && mComposingContext == aContext;
    }

    bool IsComposingOnCurrentContext() const
    {
        return IsComposingOn(GetCurrentContext());
    }

    bool EditorHasCompositionString()
    {
        return (mCompositionState ==
                    eCompositionState_CompositionChangeEventDispatched);
    }

    







    bool IsValidContext(GtkIMContext* aContext) const;

    const char* GetCompositionStateName()
    {
        switch (mCompositionState) {
            case eCompositionState_NotComposing:
                return "NotComposing";
            case eCompositionState_CompositionStartDispatched:
                return "CompositionStartDispatched";
            case eCompositionState_CompositionChangeEventDispatched:
                return "CompositionChangeEventDispatched";
            default:
                return "InvaildState";
        }
    }

    struct Selection final
    {
        uint32_t mOffset;
        uint32_t mLength;
        WritingMode mWritingMode;

        Selection()
            : mOffset(UINT32_MAX)
            , mLength(UINT32_MAX)
        {
        }

        void Clear()
        {
            mOffset = UINT32_MAX;
            mLength = UINT32_MAX;
            mWritingMode = WritingMode();
        }

        void Assign(const IMENotification& aIMENotification);
        void Assign(const WidgetQueryContentEvent& aSelectedTextEvent);

        bool IsValid() const { return mOffset != UINT32_MAX; }
        bool Collapsed() const { return !mLength; }
        uint32_t EndOffset() const
        {
            if (NS_WARN_IF(!IsValid())) {
                return UINT32_MAX;
            }
            CheckedInt<uint32_t> endOffset =
                CheckedInt<uint32_t>(mOffset) + mLength;
            if (NS_WARN_IF(!endOffset.isValid())) {
                return UINT32_MAX;
            }
            return endOffset.value();
        }
    } mSelection;
    bool EnsureToCacheSelection(nsAString* aSelectedString = nullptr);

    
    
    bool mIsIMFocused;
    
    
    
    bool mFilterKeyEvent;
    
    
    
    
    
    bool mKeyDownEventWasSent;
    
    
    bool mIsDeletingSurrounding;
    
    
    bool mLayoutChanged;

    
    
    
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

    




    void SetCursorPosition(GtkIMContext* aContext);

    
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

    










    bool DispatchCompositionCommitEvent(
             GtkIMContext* aContext,
             const nsAString* aCommitString = nullptr);
};

#endif 
