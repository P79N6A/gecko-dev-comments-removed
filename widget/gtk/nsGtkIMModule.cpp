





#include "mozilla/Logging.h"
#include "prtime.h"

#include "nsGtkIMModule.h"
#include "nsWindow.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/Likely.h"
#include "mozilla/MiscEvents.h"
#include "mozilla/Preferences.h"
#include "mozilla/TextEvents.h"
#include "WritingModes.h"

using namespace mozilla;
using namespace mozilla::widget;

PRLogModuleInfo* gGtkIMLog = nullptr;

static const char*
GetBoolName(bool aBool)
{
  return aBool ? "true" : "false";
}

static const char*
GetRangeTypeName(uint32_t aRangeType)
{
    switch (aRangeType) {
        case NS_TEXTRANGE_RAWINPUT:
            return "NS_TEXTRANGE_RAWINPUT";
        case NS_TEXTRANGE_CONVERTEDTEXT:
            return "NS_TEXTRANGE_CONVERTEDTEXT";
        case NS_TEXTRANGE_SELECTEDRAWTEXT:
            return "NS_TEXTRANGE_SELECTEDRAWTEXT";
        case NS_TEXTRANGE_SELECTEDCONVERTEDTEXT:
            return "NS_TEXTRANGE_SELECTEDCONVERTEDTEXT";
        case NS_TEXTRANGE_CARETPOSITION:
            return "NS_TEXTRANGE_CARETPOSITION";
        default:
            return "UNKNOWN SELECTION TYPE!!";
    }
}

static const char*
GetEnabledStateName(uint32_t aState)
{
    switch (aState) {
        case IMEState::DISABLED:
            return "DISABLED";
        case IMEState::ENABLED:
            return "ENABLED";
        case IMEState::PASSWORD:
            return "PASSWORD";
        case IMEState::PLUGIN:
            return "PLUG_IN";
        default:
            return "UNKNOWN ENABLED STATUS!!";
    }
}

static const char*
GetEventType(GdkEventKey* aKeyEvent)
{
    switch (aKeyEvent->type) {
        case GDK_KEY_PRESS:
            return "GDK_KEY_PRESS";
        case GDK_KEY_RELEASE:
            return "GDK_KEY_RELEASE";
        default:
            return "Unknown";
    }
}

class GetWritingModeName : public nsAutoCString
{
public:
  explicit GetWritingModeName(const WritingMode& aWritingMode)
  {
    if (!aWritingMode.IsVertical()) {
      AssignLiteral("Horizontal");
      return;
    }
    if (aWritingMode.IsVerticalLR()) {
      AssignLiteral("Vertical (LTR)");
      return;
    }
    AssignLiteral("Vertical (RTL)");
  }
  virtual ~GetWritingModeName() {}
};

const static bool kUseSimpleContextDefault = MOZ_WIDGET_GTK == 2;





nsGtkIMModule* nsGtkIMModule::sLastFocusedModule = nullptr;
bool nsGtkIMModule::sUseSimpleContext;

nsGtkIMModule::nsGtkIMModule(nsWindow* aOwnerWindow)
    : mOwnerWindow(aOwnerWindow)
    , mLastFocusedWindow(nullptr)
    , mContext(nullptr)
    , mSimpleContext(nullptr)
    , mDummyContext(nullptr)
    , mComposingContext(nullptr)
    , mCompositionStart(UINT32_MAX)
    , mProcessingKeyEvent(nullptr)
    , mCompositionState(eCompositionState_NotComposing)
    , mIsIMFocused(false)
    , mIsDeletingSurrounding(false)
{
    if (!gGtkIMLog) {
        gGtkIMLog = PR_NewLogModule("nsGtkIMModuleWidgets");
    }
    static bool sFirstInstance = true;
    if (sFirstInstance) {
        sFirstInstance = false;
        sUseSimpleContext =
            Preferences::GetBool(
                "intl.ime.use_simple_context_on_password_field",
                kUseSimpleContextDefault);
    }
    Init();
}

void
nsGtkIMModule::Init()
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): Init, mOwnerWindow=%p",
         this, mOwnerWindow));

    MozContainer* container = mOwnerWindow->GetMozContainer();
    NS_PRECONDITION(container, "container is null");
    GdkWindow* gdkWindow = gtk_widget_get_window(GTK_WIDGET(container));

    
    

    
    mContext = gtk_im_multicontext_new();
    gtk_im_context_set_client_window(mContext, gdkWindow);
    g_signal_connect(mContext, "preedit_changed",
                     G_CALLBACK(nsGtkIMModule::OnChangeCompositionCallback),
                     this);
    g_signal_connect(mContext, "retrieve_surrounding",
                     G_CALLBACK(nsGtkIMModule::OnRetrieveSurroundingCallback),
                     this);
    g_signal_connect(mContext, "delete_surrounding",
                     G_CALLBACK(nsGtkIMModule::OnDeleteSurroundingCallback),
                     this);
    g_signal_connect(mContext, "commit",
                     G_CALLBACK(nsGtkIMModule::OnCommitCompositionCallback),
                     this);
    g_signal_connect(mContext, "preedit_start",
                     G_CALLBACK(nsGtkIMModule::OnStartCompositionCallback),
                     this);
    g_signal_connect(mContext, "preedit_end",
                     G_CALLBACK(nsGtkIMModule::OnEndCompositionCallback),
                     this);

    
    if (sUseSimpleContext) {
        mSimpleContext = gtk_im_context_simple_new();
        gtk_im_context_set_client_window(mSimpleContext, gdkWindow);
        g_signal_connect(mSimpleContext, "preedit_changed",
            G_CALLBACK(&nsGtkIMModule::OnChangeCompositionCallback),
            this);
        g_signal_connect(mSimpleContext, "retrieve_surrounding",
            G_CALLBACK(&nsGtkIMModule::OnRetrieveSurroundingCallback),
            this);
        g_signal_connect(mSimpleContext, "delete_surrounding",
            G_CALLBACK(&nsGtkIMModule::OnDeleteSurroundingCallback),
            this);
        g_signal_connect(mSimpleContext, "commit",
            G_CALLBACK(&nsGtkIMModule::OnCommitCompositionCallback),
            this);
        g_signal_connect(mSimpleContext, "preedit_start",
            G_CALLBACK(nsGtkIMModule::OnStartCompositionCallback),
            this);
        g_signal_connect(mSimpleContext, "preedit_end",
            G_CALLBACK(nsGtkIMModule::OnEndCompositionCallback),
            this);
    }

    
    mDummyContext = gtk_im_multicontext_new();
    gtk_im_context_set_client_window(mDummyContext, gdkWindow);
}

nsGtkIMModule::~nsGtkIMModule()
{
    if (this == sLastFocusedModule) {
        sLastFocusedModule = nullptr;
    }
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p) was gone", this));
}

void
nsGtkIMModule::OnDestroyWindow(nsWindow* aWindow)
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): OnDestroyWindow, aWindow=%p, mLastFocusedWindow=%p, mOwnerWindow=%p, mLastFocusedModule=%p",
         this, aWindow, mLastFocusedWindow, mOwnerWindow, sLastFocusedModule));

    NS_PRECONDITION(aWindow, "aWindow must not be null");

    if (mLastFocusedWindow == aWindow) {
        EndIMEComposition(aWindow);
        if (mIsIMFocused) {
            Blur();
        }
        mLastFocusedWindow = nullptr;
    }

    if (mOwnerWindow != aWindow) {
        return;
    }

    if (sLastFocusedModule == this) {
        sLastFocusedModule = nullptr;
    }

    







    if (mContext) {
        PrepareToDestroyContext(mContext);
        gtk_im_context_set_client_window(mContext, nullptr);
        g_object_unref(mContext);
        mContext = nullptr;
    }

    if (mSimpleContext) {
        gtk_im_context_set_client_window(mSimpleContext, nullptr);
        g_object_unref(mSimpleContext);
        mSimpleContext = nullptr;
    }

    if (mDummyContext) {
        
        
        gtk_im_context_set_client_window(mDummyContext, nullptr);
        g_object_unref(mDummyContext);
        mDummyContext = nullptr;
    }

    if (NS_WARN_IF(mComposingContext)) {
        g_object_unref(mComposingContext);
        mComposingContext = nullptr;
    }

    mOwnerWindow = nullptr;
    mLastFocusedWindow = nullptr;
    mInputContext.mIMEState.mEnabled = IMEState::DISABLED;

    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("    SUCCEEDED, Completely destroyed"));
}



















void
nsGtkIMModule::PrepareToDestroyContext(GtkIMContext *aContext)
{
#if (MOZ_WIDGET_GTK == 2)
    GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT(aContext);
    GtkIMContext *slave = multicontext->slave;
#else
    GtkIMContext *slave = nullptr; 
#endif
    if (!slave) {
        return;
    }

    GType slaveType = G_TYPE_FROM_INSTANCE(slave);
    const gchar *im_type_name = g_type_name(slaveType);
    if (strcmp(im_type_name, "GtkIMContextIIIM") == 0) {
        
        static gpointer gtk_iiim_context_class =
            g_type_class_ref(slaveType);
        
        (void)gtk_iiim_context_class;
    }
}

void
nsGtkIMModule::OnFocusWindow(nsWindow* aWindow)
{
    if (MOZ_UNLIKELY(IsDestroyed())) {
        return;
    }

    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): OnFocusWindow, aWindow=%p, mLastFocusedWindow=%p",
         this, aWindow, mLastFocusedWindow));
    mLastFocusedWindow = aWindow;
    Focus();
}

void
nsGtkIMModule::OnBlurWindow(nsWindow* aWindow)
{
    if (MOZ_UNLIKELY(IsDestroyed())) {
        return;
    }

    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): OnBlurWindow, aWindow=%p, mLastFocusedWindow=%p, "
         "mIsIMFocused=%s",
         this, aWindow, mLastFocusedWindow, GetBoolName(mIsIMFocused)));

    if (!mIsIMFocused || mLastFocusedWindow != aWindow) {
        return;
    }

    Blur();
}

bool
nsGtkIMModule::OnKeyEvent(nsWindow* aCaller, GdkEventKey* aEvent,
                          bool aKeyDownEventWasSent )
{
    NS_PRECONDITION(aEvent, "aEvent must be non-null");

    if (!mInputContext.mIMEState.MaybeEditable() ||
        MOZ_UNLIKELY(IsDestroyed())) {
        return false;
    }

    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): OnKeyEvent, aCaller=%p, aKeyDownEventWasSent=%s, "
         "mCompositionState=%s, current context=%p, active context=%p, "
         "aEvent(%p): { type=%s, keyval=%s, unicode=0x%X }",
         this, aCaller, GetBoolName(aKeyDownEventWasSent),
         GetCompositionStateName(), GetCurrentContext(), GetActiveContext(),
         aEvent, GetEventType(aEvent), gdk_keyval_name(aEvent->keyval),
         gdk_keyval_to_unicode(aEvent->keyval)));

    if (aCaller != mLastFocusedWindow) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, the caller isn't focused window, mLastFocusedWindow=%p",
             mLastFocusedWindow));
        return false;
    }

    
    
    GtkIMContext* currentContext = GetCurrentContext();
    if (MOZ_UNLIKELY(!currentContext)) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, there are no context"));
        return false;
    }

    mKeyDownEventWasSent = aKeyDownEventWasSent;
    mFilterKeyEvent = true;
    mProcessingKeyEvent = aEvent;
    gboolean isFiltered =
        gtk_im_context_filter_keypress(currentContext, aEvent);
    mProcessingKeyEvent = nullptr;

    
    
    
    
    
    bool filterThisEvent = isFiltered && mFilterKeyEvent;

    if (IsComposingOnCurrentContext() && !isFiltered) {
        if (aEvent->type == GDK_KEY_PRESS) {
            if (!mDispatchedCompositionString.IsEmpty()) {
                
                
                filterThisEvent = true;
            } else {
                
                
                
                
                
                
                
                DispatchCompositionCommitEvent(currentContext, &EmptyString());
                filterThisEvent = false;
            }
        } else {
            
            
            filterThisEvent = true;
        }
    }

    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("    filterThisEvent=%s (isFiltered=%s, mFilterKeyEvent=%s), "
         "mCompositionState=%s",
         GetBoolName(filterThisEvent), GetBoolName(isFiltered),
         GetBoolName(mFilterKeyEvent), GetCompositionStateName()));

    return filterThisEvent;
}

void
nsGtkIMModule::OnFocusChangeInGecko(bool aFocus)
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): OnFocusChangeInGecko, aFocus=%s, "
         "mCompositionState=%s, mIsIMFocused=%s",
         this, GetBoolName(aFocus), GetCompositionStateName(),
         GetBoolName(mIsIMFocused)));

    
    mSelectedString.Truncate();
    mSelection.Clear();
}

void
nsGtkIMModule::ResetIME()
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): ResetIME, mCompositionState=%s, mIsIMFocused=%s",
         this, GetCompositionStateName(), GetBoolName(mIsIMFocused)));

    GtkIMContext* activeContext = GetActiveContext();
    if (MOZ_UNLIKELY(!activeContext)) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, there are no context"));
        return;
    }

    nsRefPtr<nsGtkIMModule> kungFuDeathGrip(this);
    nsRefPtr<nsWindow> lastFocusedWindow(mLastFocusedWindow);

    gtk_im_context_reset(activeContext);

    
    
    if (!lastFocusedWindow ||
        NS_WARN_IF(lastFocusedWindow != mLastFocusedWindow) ||
        lastFocusedWindow->Destroyed()) {
        return;
    }

    nsAutoString compositionString;
    GetCompositionString(activeContext, compositionString);

    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): ResetIME() called gtk_im_context_reset(), "
         "activeContext=%p, mCompositionState=%s, compositionString=%s, "
         "mIsIMFocused=%s",
         this, activeContext, GetCompositionStateName(),
         NS_ConvertUTF16toUTF8(compositionString).get(),
         GetBoolName(mIsIMFocused)));

    
    
    
    
    
    if (IsComposing() && compositionString.IsEmpty()) {
        
        DispatchCompositionCommitEvent(activeContext, &EmptyString());
    }
}

nsresult
nsGtkIMModule::EndIMEComposition(nsWindow* aCaller)
{
    if (MOZ_UNLIKELY(IsDestroyed())) {
        return NS_OK;
    }

    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): EndIMEComposition, aCaller=%p, "
         "mCompositionState=%s",
         this, aCaller, GetCompositionStateName()));

    if (aCaller != mLastFocusedWindow) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    WARNING: the caller isn't focused window, mLastFocusedWindow=%p",
             mLastFocusedWindow));
        return NS_OK;
    }

    if (!IsComposing()) {
        return NS_OK;
    }

    
    
    
    
    
    
    
    
    ResetIME();

    return NS_OK;
}

void
nsGtkIMModule::OnUpdateComposition()
{
    if (MOZ_UNLIKELY(IsDestroyed())) {
        return;
    }

    SetCursorPosition(GetActiveContext());
}

void
nsGtkIMModule::SetInputContext(nsWindow* aCaller,
                               const InputContext* aContext,
                               const InputContextAction* aAction)
{
    if (MOZ_UNLIKELY(IsDestroyed())) {
        return;
    }

    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): SetInputContext, aCaller=%p, aState=%s mHTMLInputType=%s",
         this, aCaller, GetEnabledStateName(aContext->mIMEState.mEnabled),
         NS_ConvertUTF16toUTF8(aContext->mHTMLInputType).get()));

    if (aCaller != mLastFocusedWindow) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, the caller isn't focused window, mLastFocusedWindow=%p",
             mLastFocusedWindow));
        return;
    }

    if (!mContext) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, there are no context"));
        return;
    }


    if (sLastFocusedModule != this) {
        mInputContext = *aContext;
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    SUCCEEDED, but we're not active"));
        return;
    }

    bool changingEnabledState =
        aContext->mIMEState.mEnabled != mInputContext.mIMEState.mEnabled ||
        aContext->mHTMLInputType != mInputContext.mHTMLInputType;

    
    if (changingEnabledState && mInputContext.mIMEState.MaybeEditable()) {
        EndIMEComposition(mLastFocusedWindow);
        Blur();
    }

    mInputContext = *aContext;

    if (changingEnabledState) {
#if (MOZ_WIDGET_GTK == 3)
        static bool sInputPurposeSupported = !gtk_check_version(3, 6, 0);
        if (sInputPurposeSupported && mInputContext.mIMEState.MaybeEditable()) {
            GtkIMContext* currentContext = GetCurrentContext();
            if (currentContext) {
                GtkInputPurpose purpose = GTK_INPUT_PURPOSE_FREE_FORM;
                const nsString& inputType = mInputContext.mHTMLInputType;
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                if (mInputContext.mIMEState.mEnabled == IMEState::PASSWORD) {
                    purpose = GTK_INPUT_PURPOSE_PASSWORD;
                } else if (inputType.EqualsLiteral("email")) {
                    purpose = GTK_INPUT_PURPOSE_EMAIL;
                } else if (inputType.EqualsLiteral("url")) {
                    purpose = GTK_INPUT_PURPOSE_URL;
                } else if (inputType.EqualsLiteral("tel")) {
                    purpose = GTK_INPUT_PURPOSE_PHONE;
                } else if (inputType.EqualsLiteral("number")) {
                    purpose = GTK_INPUT_PURPOSE_NUMBER;
                }

                g_object_set(currentContext, "input-purpose", purpose, nullptr);
            }
        }
#endif 

        
        
        
        
        Focus();

        
        
    }
}

InputContext
nsGtkIMModule::GetInputContext()
{
    mInputContext.mIMEState.mOpen = IMEState::OPEN_STATE_NOT_SUPPORTED;
    return mInputContext;
}

GtkIMContext*
nsGtkIMModule::GetCurrentContext() const
{
    if (IsEnabled()) {
        return mContext;
    }
    if (mInputContext.mIMEState.mEnabled == IMEState::PASSWORD) {
        return mSimpleContext;
    }
    return mDummyContext;
}

bool
nsGtkIMModule::IsValidContext(GtkIMContext* aContext) const
{
    if (!aContext) {
        return false;
    }
    return aContext == mContext ||
           aContext == mSimpleContext ||
           aContext == mDummyContext;
}

bool
nsGtkIMModule::IsEnabled() const
{
    return mInputContext.mIMEState.mEnabled == IMEState::ENABLED ||
           mInputContext.mIMEState.mEnabled == IMEState::PLUGIN ||
           (!sUseSimpleContext &&
            mInputContext.mIMEState.mEnabled == IMEState::PASSWORD);
}

void
nsGtkIMModule::Focus()
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): Focus, sLastFocusedModule=%p",
         this, sLastFocusedModule));

    if (mIsIMFocused) {
        NS_ASSERTION(sLastFocusedModule == this,
                     "We're not active, but the IM was focused?");
        return;
    }

    GtkIMContext* currentContext = GetCurrentContext();
    if (!currentContext) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, there are no context"));
        return;
    }

    if (sLastFocusedModule && sLastFocusedModule != this) {
        sLastFocusedModule->Blur();
    }

    sLastFocusedModule = this;

    gtk_im_context_focus_in(currentContext);
    mIsIMFocused = true;

    if (!IsEnabled()) {
        
        
        Blur();
    }
}

void
nsGtkIMModule::Blur()
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): Blur, mIsIMFocused=%s",
         this, GetBoolName(mIsIMFocused)));

    if (!mIsIMFocused) {
        return;
    }

    GtkIMContext* currentContext = GetCurrentContext();
    if (!currentContext) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, there are no context"));
        return;
    }

    gtk_im_context_focus_out(currentContext);
    mIsIMFocused = false;
}

void
nsGtkIMModule::OnSelectionChange(nsWindow* aCaller,
                                 const IMENotification& aIMENotification)
{
    mSelection.Assign(aIMENotification);

    if (MOZ_UNLIKELY(IsDestroyed())) {
        return;
    }

    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): OnSelectionChange(aCaller=0x%p), "
         "mCompositionState=%s, mIsDeletingSurrounding=%s",
         this, aCaller, GetCompositionStateName(),
         mIsDeletingSurrounding ? "true" : "false"));

    if (aCaller != mLastFocusedWindow) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    WARNING: the caller isn't focused window, "
             "mLastFocusedWindow=%p",
             mLastFocusedWindow));
        return;
    }

    
    
    
    
    if (mCompositionState == eCompositionState_CompositionStartDispatched) {
        if (NS_WARN_IF(!mSelection.IsValid())) {
            MOZ_LOG(gGtkIMLog, LogLevel::Info,
                ("    ERROR: new offset is too large, cannot keep composing"));
        } else {
            
            mCompositionStart = mSelection.mOffset;
            
            MOZ_LOG(gGtkIMLog, LogLevel::Info,
                ("    NOTE: mCompositionStart is updated to %u, "
                 "the selection change doesn't cause resetting IM context",
                 mCompositionStart));
            
            return;
        }
        
    }

    
    
    if (mIsDeletingSurrounding) {
        return;
    }

    ResetIME();
}


void
nsGtkIMModule::OnStartCompositionCallback(GtkIMContext *aContext,
                                          nsGtkIMModule* aModule)
{
    aModule->OnStartCompositionNative(aContext);
}

void
nsGtkIMModule::OnStartCompositionNative(GtkIMContext *aContext)
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): OnStartCompositionNative, aContext=%p, "
         "current context=%p",
         this, aContext, GetCurrentContext()));

    
    if (GetCurrentContext() != aContext) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, given context doesn't match"));
        return;
    }

    mComposingContext = static_cast<GtkIMContext*>(g_object_ref(aContext));

    if (!DispatchCompositionStart(aContext)) {
        return;
    }
    mCompositionTargetRange.mOffset = mCompositionStart;
    mCompositionTargetRange.mLength = 0;
}


void
nsGtkIMModule::OnEndCompositionCallback(GtkIMContext *aContext,
                                        nsGtkIMModule* aModule)
{
    aModule->OnEndCompositionNative(aContext);
}

void
nsGtkIMModule::OnEndCompositionNative(GtkIMContext *aContext)
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): OnEndCompositionNative, aContext=%p",
         this, aContext));

    
    
    
    if (!IsValidContext(aContext)) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, given context doesn't match with any context"));
        return;
    }

    g_object_unref(mComposingContext);
    mComposingContext = nullptr;

    if (!IsComposing()) {
        
        return;
    }

    
    DispatchCompositionCommitEvent(aContext);
}


void
nsGtkIMModule::OnChangeCompositionCallback(GtkIMContext *aContext,
                                           nsGtkIMModule* aModule)
{
    aModule->OnChangeCompositionNative(aContext);
}

void
nsGtkIMModule::OnChangeCompositionNative(GtkIMContext *aContext)
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): OnChangeCompositionNative, aContext=%p",
         this, aContext));

    
    
    
    if (!IsValidContext(aContext)) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, given context doesn't match with any context"));
        return;
    }

    nsAutoString compositionString;
    GetCompositionString(aContext, compositionString);
    if (!IsComposing() && compositionString.IsEmpty()) {
        mDispatchedCompositionString.Truncate();
        return; 
    }

    
    DispatchCompositionChangeEvent(aContext, compositionString);
}


gboolean
nsGtkIMModule::OnRetrieveSurroundingCallback(GtkIMContext  *aContext,
                                             nsGtkIMModule *aModule)
{
    return aModule->OnRetrieveSurroundingNative(aContext);
}

gboolean
nsGtkIMModule::OnRetrieveSurroundingNative(GtkIMContext *aContext)
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): OnRetrieveSurroundingNative, aContext=%p, "
         "current context=%p",
         this, aContext, GetCurrentContext()));

    
    if (GetCurrentContext() != aContext) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, given context doesn't match"));
        return FALSE;
    }

    nsAutoString uniStr;
    uint32_t cursorPos;
    if (NS_FAILED(GetCurrentParagraph(uniStr, cursorPos))) {
        return FALSE;
    }

    NS_ConvertUTF16toUTF8 utf8Str(nsDependentSubstring(uniStr, 0, cursorPos));
    uint32_t cursorPosInUTF8 = utf8Str.Length();
    AppendUTF16toUTF8(nsDependentSubstring(uniStr, cursorPos), utf8Str);
    gtk_im_context_set_surrounding(aContext, utf8Str.get(), utf8Str.Length(),
                                   cursorPosInUTF8);
    return TRUE;
}


gboolean
nsGtkIMModule::OnDeleteSurroundingCallback(GtkIMContext  *aContext,
                                           gint           aOffset,
                                           gint           aNChars,
                                           nsGtkIMModule *aModule)
{
    return aModule->OnDeleteSurroundingNative(aContext, aOffset, aNChars);
}

gboolean
nsGtkIMModule::OnDeleteSurroundingNative(GtkIMContext  *aContext,
                                         gint           aOffset,
                                         gint           aNChars)
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): OnDeleteSurroundingNative, aContext=%p, "
         "current context=%p",
         this, aContext, GetCurrentContext()));

    
    if (GetCurrentContext() != aContext) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, given context doesn't match"));
        return FALSE;
    }

    AutoRestore<bool> saveDeletingSurrounding(mIsDeletingSurrounding);
    mIsDeletingSurrounding = true;
    if (NS_SUCCEEDED(DeleteText(aContext, aOffset, (uint32_t)aNChars))) {
        return TRUE;
    }

    
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("    FAILED, cannot delete text"));
    return FALSE;
}
                         

void
nsGtkIMModule::OnCommitCompositionCallback(GtkIMContext *aContext,
                                           const gchar *aString,
                                           nsGtkIMModule* aModule)
{
    aModule->OnCommitCompositionNative(aContext, aString);
}

void
nsGtkIMModule::OnCommitCompositionNative(GtkIMContext *aContext,
                                         const gchar *aUTF8Char)
{
    const gchar emptyStr = 0;
    const gchar *commitString = aUTF8Char ? aUTF8Char : &emptyStr;

    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): OnCommitCompositionNative, aContext=%p, "
         "current context=%p, active context=%p, commitString=\"%s\", "
         "mProcessingKeyEvent=%p, IsComposingOn(aContext)=%s",
         this, aContext, GetCurrentContext(), GetActiveContext(), commitString,
         mProcessingKeyEvent, GetBoolName(IsComposingOn(aContext))));

    
    if (!IsValidContext(aContext)) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, given context doesn't match"));
        return;
    }

    
    
    
    
    
    if (!IsComposingOn(aContext) && !commitString[0]) {
        return;
    }

    
    
    
    
    
    if (!IsComposingOn(aContext) && mProcessingKeyEvent &&
        aContext == GetCurrentContext()) {
        char keyval_utf8[8]; 
        gint keyval_utf8_len;
        guint32 keyval_unicode;

        keyval_unicode = gdk_keyval_to_unicode(mProcessingKeyEvent->keyval);
        keyval_utf8_len = g_unichar_to_utf8(keyval_unicode, keyval_utf8);
        keyval_utf8[keyval_utf8_len] = '\0';

        if (!strcmp(commitString, keyval_utf8)) {
            MOZ_LOG(gGtkIMLog, LogLevel::Info,
                ("GtkIMModule(%p): OnCommitCompositionNative, we'll send normal key event",
                 this));
            mFilterKeyEvent = false;
            return;
        }
    }

    NS_ConvertUTF8toUTF16 str(commitString);
    
    DispatchCompositionCommitEvent(aContext, &str);
}

void
nsGtkIMModule::GetCompositionString(GtkIMContext* aContext,
                                    nsAString& aCompositionString)
{
    gchar *preedit_string;
    gint cursor_pos;
    PangoAttrList *feedback_list;
    gtk_im_context_get_preedit_string(aContext, &preedit_string,
                                      &feedback_list, &cursor_pos);
    if (preedit_string && *preedit_string) {
        CopyUTF8toUTF16(preedit_string, aCompositionString);
    } else {
        aCompositionString.Truncate();
    }

    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): GetCompositionString, result=\"%s\"",
         this, preedit_string));

    pango_attr_list_unref(feedback_list);
    g_free(preedit_string);
}

bool
nsGtkIMModule::DispatchCompositionStart(GtkIMContext* aContext)
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): DispatchCompositionStart, aContext=%p",
         this, aContext));

    if (IsComposing()) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    WARNING, we're already in composition"));
        return true;
    }

    if (!mLastFocusedWindow) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, there are no focused window in this module"));
        return false;
    }

    if (NS_WARN_IF(!EnsureToCacheSelection())) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, cannot query the selection offset"));
        return false;
    }

    
    
    
    
    mCompositionStart = mSelection.mOffset;
    mDispatchedCompositionString.Truncate();

    if (mProcessingKeyEvent && !mKeyDownEventWasSent &&
        mProcessingKeyEvent->type == GDK_KEY_PRESS) {
        
        
        nsCOMPtr<nsIWidget> kungFuDeathGrip = mLastFocusedWindow;
        bool isCancelled;
        mLastFocusedWindow->DispatchKeyDownEvent(mProcessingKeyEvent,
                                                 &isCancelled);
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    keydown event is dispatched"));
        if (static_cast<nsWindow*>(kungFuDeathGrip.get())->IsDestroyed() ||
            kungFuDeathGrip != mLastFocusedWindow) {
            MOZ_LOG(gGtkIMLog, LogLevel::Info,
                ("    NOTE, the focused widget was destroyed/changed by keydown event"));
            return false;
        }
    }

    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("    mCompositionStart=%u", mCompositionStart));
    mCompositionState = eCompositionState_CompositionStartDispatched;
    WidgetCompositionEvent compEvent(true, NS_COMPOSITION_START,
                                     mLastFocusedWindow);
    InitEvent(compEvent);
    nsCOMPtr<nsIWidget> kungFuDeathGrip = mLastFocusedWindow;
    nsEventStatus status;
    mLastFocusedWindow->DispatchEvent(&compEvent, status);
    if (static_cast<nsWindow*>(kungFuDeathGrip.get())->IsDestroyed() ||
        kungFuDeathGrip != mLastFocusedWindow) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    NOTE, the focused widget was destroyed/changed by compositionstart event"));
        return false;
    }

    return true;
}

bool
nsGtkIMModule::DispatchCompositionChangeEvent(
                   GtkIMContext* aContext,
                   const nsAString& aCompositionString)
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): DispatchCompositionChangeEvent, aContext=%p",
         this, aContext));

    if (!mLastFocusedWindow) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, there are no focused window in this module"));
        return false;
    }

    if (!IsComposing()) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    The composition wasn't started, force starting..."));
        nsCOMPtr<nsIWidget> kungFuDeathGrip = mLastFocusedWindow;
        if (!DispatchCompositionStart(aContext)) {
            return false;
        }
    }

    nsEventStatus status;
    nsRefPtr<nsWindow> lastFocusedWindow = mLastFocusedWindow;

    
    
    if (mCompositionState == eCompositionState_CompositionStartDispatched) {
        if (NS_WARN_IF(!EnsureToCacheSelection(&mSelectedString))) {
            
        } else {
            
            
            mCompositionStart = mSelection.mOffset;
        }
    }

    WidgetCompositionEvent compositionChangeEvent(true, NS_COMPOSITION_CHANGE,
                                                  mLastFocusedWindow);
    InitEvent(compositionChangeEvent);

    uint32_t targetOffset = mCompositionStart;

    compositionChangeEvent.mData =
      mDispatchedCompositionString = aCompositionString;

    compositionChangeEvent.mRanges =
      CreateTextRangeArray(aContext, mDispatchedCompositionString);
    targetOffset += compositionChangeEvent.mRanges->TargetClauseOffset();

    mCompositionState = eCompositionState_CompositionChangeEventDispatched;

    mLastFocusedWindow->DispatchEvent(&compositionChangeEvent, status);
    if (lastFocusedWindow->IsDestroyed() ||
        lastFocusedWindow != mLastFocusedWindow) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    NOTE, the focused widget was destroyed/changed by "
             "compositionchange event"));
        return false;
    }

    
    
    
    mCompositionTargetRange.mOffset = targetOffset;
    mCompositionTargetRange.mLength =
        compositionChangeEvent.mRanges->TargetClauseLength();

    return true;
}

bool
nsGtkIMModule::DispatchCompositionCommitEvent(
                   GtkIMContext* aContext,
                   const nsAString* aCommitString)
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): DispatchCompositionCommitEvent, aContext=%p, "
         "aCommitString=%p, (\"%s\")",
         this, aContext, aCommitString,
         aCommitString ? NS_ConvertUTF16toUTF8(*aCommitString).get() : ""));

    if (!mLastFocusedWindow) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, there are no focused window in this module"));
        return false;
    }

    if (!IsComposing()) {
        if (!aCommitString || aCommitString->IsEmpty()) {
            MOZ_LOG(gGtkIMLog, LogLevel::Info,
                ("    FAILED, there is no composition and empty commit "
                 "string"));
            return true;
        }
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    The composition wasn't started, force starting..."));
        nsCOMPtr<nsIWidget> kungFuDeathGrip(mLastFocusedWindow);
        if (!DispatchCompositionStart(aContext)) {
            return false;
        }
    }

    nsRefPtr<nsWindow> lastFocusedWindow(mLastFocusedWindow);

    uint32_t message = aCommitString ? NS_COMPOSITION_COMMIT :
                                       NS_COMPOSITION_COMMIT_AS_IS;
    mCompositionState = eCompositionState_NotComposing;
    mCompositionStart = UINT32_MAX;
    mCompositionTargetRange.Clear();
    mDispatchedCompositionString.Truncate();

    WidgetCompositionEvent compositionCommitEvent(true, message,
                                                  mLastFocusedWindow);
    InitEvent(compositionCommitEvent);
    if (message == NS_COMPOSITION_COMMIT) {
        compositionCommitEvent.mData = *aCommitString;
    }

    nsEventStatus status = nsEventStatus_eIgnore;
    mLastFocusedWindow->DispatchEvent(&compositionCommitEvent, status);

    if (lastFocusedWindow->IsDestroyed() ||
        lastFocusedWindow != mLastFocusedWindow) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    NOTE, the focused widget was destroyed/changed by "
             "compositioncommit event"));
        return false;
    }

    return true;
}

already_AddRefed<TextRangeArray>
nsGtkIMModule::CreateTextRangeArray(GtkIMContext* aContext,
                                    const nsAString& aLastDispatchedData)
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): CreateTextRangeArray, aContext=%p, "
         "aLastDispatchedData=\"%s\" (length=%u)",
         this, aContext, NS_ConvertUTF16toUTF8(aLastDispatchedData).get(),
         aLastDispatchedData.Length()));

    nsRefPtr<TextRangeArray> textRangeArray = new TextRangeArray();

    gchar *preedit_string;
    gint cursor_pos;
    PangoAttrList *feedback_list;
    gtk_im_context_get_preedit_string(aContext, &preedit_string,
                                      &feedback_list, &cursor_pos);
    if (!preedit_string || !*preedit_string) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    preedit_string is null"));
        pango_attr_list_unref(feedback_list);
        g_free(preedit_string);
        return textRangeArray.forget();
    }

    PangoAttrIterator* iter;
    iter = pango_attr_list_get_iterator(feedback_list);
    if (!iter) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, iterator couldn't be allocated"));
        pango_attr_list_unref(feedback_list);
        g_free(preedit_string);
        return textRangeArray.forget();
    }

    








    do {
        PangoAttribute* attrUnderline =
            pango_attr_iterator_get(iter, PANGO_ATTR_UNDERLINE);
        PangoAttribute* attrForeground =
            pango_attr_iterator_get(iter, PANGO_ATTR_FOREGROUND);
        if (!attrUnderline && !attrForeground) {
            continue;
        }

        
        gint start, end;
        pango_attr_iterator_range(iter, &start, &end);

        TextRange range;
        
        if (attrUnderline && attrForeground) {
            range.mRangeType = NS_TEXTRANGE_SELECTEDCONVERTEDTEXT;
        }
        
        else if (attrUnderline) {
            range.mRangeType = NS_TEXTRANGE_CONVERTEDTEXT;
        }
        
        else if (attrForeground) {
            range.mRangeType = NS_TEXTRANGE_SELECTEDRAWTEXT;
        } else {
            range.mRangeType = NS_TEXTRANGE_RAWINPUT;
        }

        gunichar2* uniStr = nullptr;
        if (start == 0) {
            range.mStartOffset = 0;
        } else {
            glong uniStrLen;
            uniStr = g_utf8_to_utf16(preedit_string, start,
                                     nullptr, &uniStrLen, nullptr);
            if (uniStr) {
                range.mStartOffset = uniStrLen;
                g_free(uniStr);
                uniStr = nullptr;
            }
        }

        glong uniStrLen;
        uniStr = g_utf8_to_utf16(preedit_string + start, end - start,
                                 nullptr, &uniStrLen, nullptr);
        if (!uniStr) {
            range.mEndOffset = range.mStartOffset;
        } else {
            range.mEndOffset = range.mStartOffset + uniStrLen;
            g_free(uniStr);
            uniStr = nullptr;
        }

        textRangeArray->AppendElement(range);

        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    mStartOffset=%u, mEndOffset=%u, mRangeType=%s",
             range.mStartOffset, range.mEndOffset,
             GetRangeTypeName(range.mRangeType)));
    } while (pango_attr_iterator_next(iter));

    TextRange range;
    if (cursor_pos < 0) {
        range.mStartOffset = 0;
    } else if (uint32_t(cursor_pos) > aLastDispatchedData.Length()) {
        range.mStartOffset = aLastDispatchedData.Length();
    } else {
        range.mStartOffset = uint32_t(cursor_pos);
    }
    range.mEndOffset = range.mStartOffset;
    range.mRangeType = NS_TEXTRANGE_CARETPOSITION;
    textRangeArray->AppendElement(range);

    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("    mStartOffset=%u, mEndOffset=%u, mRangeType=%s",
         range.mStartOffset, range.mEndOffset,
         GetRangeTypeName(range.mRangeType)));

    pango_attr_iterator_destroy(iter);
    pango_attr_list_unref(feedback_list);
    g_free(preedit_string);

    return textRangeArray.forget();
}

void
nsGtkIMModule::SetCursorPosition(GtkIMContext* aContext)
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): SetCursorPosition, aContext=%p, "
         "mCompositionTargetRange={ mOffset=%u, mLength=%u }"
         "mSelection.mWritingMode=%s",
         this, aContext, mCompositionTargetRange.mOffset,
         mCompositionTargetRange.mLength,
         GetWritingModeName(mSelection.mWritingMode).get()));

    if (!mCompositionTargetRange.IsValid()) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, mCompositionTargetRange is invalid"));
        return;
    }

    if (!mLastFocusedWindow) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, there are no focused window"));
        return;
    }

    if (MOZ_UNLIKELY(!aContext)) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, there are no context"));
        return;
    }

    WidgetQueryContentEvent charRect(true, NS_QUERY_TEXT_RECT,
                                     mLastFocusedWindow);
    if (mSelection.mWritingMode.IsVertical()) {
        
        
        uint32_t length = mCompositionTargetRange.mLength ?
            mCompositionTargetRange.mLength : 1;
        charRect.InitForQueryTextRect(mCompositionTargetRange.mOffset, length);
    } else {
        charRect.InitForQueryTextRect(mCompositionTargetRange.mOffset, 1);
    }
    InitEvent(charRect);
    nsEventStatus status;
    mLastFocusedWindow->DispatchEvent(&charRect, status);
    if (!charRect.mSucceeded) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, NS_QUERY_TEXT_RECT was failed"));
        return;
    }

    nsWindow* rootWindow =
        static_cast<nsWindow*>(mLastFocusedWindow->GetTopLevelWidget());

    
    gint rootX, rootY;
    gdk_window_get_origin(rootWindow->GetGdkWindow(), &rootX, &rootY);

    
    gint ownerX, ownerY;
    gdk_window_get_origin(mOwnerWindow->GetGdkWindow(), &ownerX, &ownerY);

    
    GdkRectangle area;
    area.x = charRect.mReply.mRect.x + rootX - ownerX;
    area.y = charRect.mReply.mRect.y + rootY - ownerY;
    area.width  = 0;
    area.height = charRect.mReply.mRect.height;

    gtk_im_context_set_cursor_location(aContext, &area);
}

nsresult
nsGtkIMModule::GetCurrentParagraph(nsAString& aText, uint32_t& aCursorPos)
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): GetCurrentParagraph, mCompositionState=%s",
         this, GetCompositionStateName()));

    if (!mLastFocusedWindow) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, there are no focused window in this module"));
        return NS_ERROR_NULL_POINTER;
    }

    nsEventStatus status;

    uint32_t selOffset = mCompositionStart;
    uint32_t selLength = mSelectedString.Length();

    
    
    if (!EditorHasCompositionString()) {
        
        if (NS_WARN_IF(!EnsureToCacheSelection())) {
            MOZ_LOG(gGtkIMLog, LogLevel::Info,
                ("    FAILED, due to no valid selection information"));
            return NS_ERROR_FAILURE;
        }

        selOffset = mSelection.mOffset;
        selLength = mSelection.mLength;
    }

    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("        selOffset=%u, selLength=%u",
         selOffset, selLength));

    
    
    
    if (selOffset > INT32_MAX || selLength > INT32_MAX ||
        selOffset + selLength > INT32_MAX) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, The selection is out of range"));
        return NS_ERROR_FAILURE;
    }

    
    WidgetQueryContentEvent queryTextContentEvent(true,
                                                  NS_QUERY_TEXT_CONTENT,
                                                  mLastFocusedWindow);
    queryTextContentEvent.InitForQueryTextContent(0, UINT32_MAX);
    mLastFocusedWindow->DispatchEvent(&queryTextContentEvent, status);
    NS_ENSURE_TRUE(queryTextContentEvent.mSucceeded, NS_ERROR_FAILURE);

    nsAutoString textContent(queryTextContentEvent.mReply.mString);
    if (selOffset + selLength > textContent.Length()) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, The selection is invalid, textContent.Length()=%u",
             textContent.Length()));
        return NS_ERROR_FAILURE;
    }

    
    
    
    if (EditorHasCompositionString() &&
        mDispatchedCompositionString != mSelectedString) {
        textContent.Replace(mCompositionStart,
            mDispatchedCompositionString.Length(), mSelectedString);
    }

    
    int32_t parStart = (selOffset == 0) ? 0 :
        textContent.RFind("\n", false, selOffset - 1, -1) + 1;
    int32_t parEnd = textContent.Find("\n", false, selOffset + selLength, -1);
    if (parEnd < 0) {
        parEnd = textContent.Length();
    }
    aText = nsDependentSubstring(textContent, parStart, parEnd - parStart);
    aCursorPos = selOffset - uint32_t(parStart);

    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("    aText=%s, aText.Length()=%u, aCursorPos=%u",
         NS_ConvertUTF16toUTF8(aText).get(),
         aText.Length(), aCursorPos));

    return NS_OK;
}

nsresult
nsGtkIMModule::DeleteText(GtkIMContext* aContext,
                          int32_t aOffset,
                          uint32_t aNChars)
{
    MOZ_LOG(gGtkIMLog, LogLevel::Info,
        ("GtkIMModule(%p): DeleteText, aContext=%p, aOffset=%d, aNChars=%d, "
         "mCompositionState=%s",
         this, aContext, aOffset, aNChars, GetCompositionStateName()));

    if (!mLastFocusedWindow) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, there are no focused window in this module"));
        return NS_ERROR_NULL_POINTER;
    }

    if (!aNChars) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, aNChars must not be zero"));
        return NS_ERROR_INVALID_ARG;
    }

    nsRefPtr<nsWindow> lastFocusedWindow(mLastFocusedWindow);
    nsEventStatus status;

    
    
    uint32_t selOffset;
    bool wasComposing = IsComposing();
    bool editorHadCompositionString = EditorHasCompositionString();
    if (wasComposing) {
        selOffset = mCompositionStart;
        if (!DispatchCompositionCommitEvent(aContext, &mSelectedString)) {
            MOZ_LOG(gGtkIMLog, LogLevel::Info,
                ("    FAILED, quitting from DeletText"));
            return NS_ERROR_FAILURE;
        }
    } else {
        if (NS_WARN_IF(!EnsureToCacheSelection())) {
            MOZ_LOG(gGtkIMLog, LogLevel::Info,
                ("    FAILED, due to no valid selection information"));
            return NS_ERROR_FAILURE;
        }
        selOffset = mSelection.mOffset;
    }

    
    WidgetQueryContentEvent queryTextContentEvent(true,
                                                  NS_QUERY_TEXT_CONTENT,
                                                  mLastFocusedWindow);
    queryTextContentEvent.InitForQueryTextContent(0, UINT32_MAX);
    mLastFocusedWindow->DispatchEvent(&queryTextContentEvent, status);
    NS_ENSURE_TRUE(queryTextContentEvent.mSucceeded, NS_ERROR_FAILURE);
    if (queryTextContentEvent.mReply.mString.IsEmpty()) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, there is no contents"));
        return NS_ERROR_FAILURE;
    }

    NS_ConvertUTF16toUTF8 utf8Str(
        nsDependentSubstring(queryTextContentEvent.mReply.mString,
                             0, selOffset));
    glong offsetInUTF8Characters =
        g_utf8_strlen(utf8Str.get(), utf8Str.Length()) + aOffset;
    if (offsetInUTF8Characters < 0) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, aOffset is too small for current cursor pos "
             "(computed offset: %d)",
             offsetInUTF8Characters));
        return NS_ERROR_FAILURE;
    }

    AppendUTF16toUTF8(
        nsDependentSubstring(queryTextContentEvent.mReply.mString, selOffset),
        utf8Str);
    glong countOfCharactersInUTF8 =
        g_utf8_strlen(utf8Str.get(), utf8Str.Length());
    glong endInUTF8Characters =
        offsetInUTF8Characters + aNChars;
    if (countOfCharactersInUTF8 < endInUTF8Characters) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, aNChars is too large for current contents "
             "(content length: %d, computed end offset: %d)",
             countOfCharactersInUTF8, endInUTF8Characters));
        return NS_ERROR_FAILURE;
    }

    gchar* charAtOffset =
        g_utf8_offset_to_pointer(utf8Str.get(), offsetInUTF8Characters);
    gchar* charAtEnd =
        g_utf8_offset_to_pointer(utf8Str.get(), endInUTF8Characters);

    
    WidgetSelectionEvent selectionEvent(true, NS_SELECTION_SET,
                                        mLastFocusedWindow);

    nsDependentCSubstring utf8StrBeforeOffset(utf8Str, 0,
                                              charAtOffset - utf8Str.get());
    selectionEvent.mOffset =
        NS_ConvertUTF8toUTF16(utf8StrBeforeOffset).Length();

    nsDependentCSubstring utf8DeletingStr(utf8Str,
                                          utf8StrBeforeOffset.Length(),
                                          charAtEnd - charAtOffset);
    selectionEvent.mLength =
        NS_ConvertUTF8toUTF16(utf8DeletingStr).Length();

    selectionEvent.mReversed = false;
    selectionEvent.mExpandToClusterBoundary = false;
    lastFocusedWindow->DispatchEvent(&selectionEvent, status);

    if (!selectionEvent.mSucceeded ||
        lastFocusedWindow != mLastFocusedWindow ||
        lastFocusedWindow->Destroyed()) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, setting selection caused focus change "
             "or window destroyed"));
        return NS_ERROR_FAILURE;
    }

    
    WidgetContentCommandEvent contentCommandEvent(true,
                                                  NS_CONTENT_COMMAND_DELETE,
                                                  mLastFocusedWindow);
    mLastFocusedWindow->DispatchEvent(&contentCommandEvent, status);

    if (!contentCommandEvent.mSucceeded ||
        lastFocusedWindow != mLastFocusedWindow ||
        lastFocusedWindow->Destroyed()) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, deleting the selection caused focus change "
             "or window destroyed"));
        return NS_ERROR_FAILURE;
    }

    if (!wasComposing) {
        return NS_OK;
    }

    
    if (!DispatchCompositionStart(aContext)) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, resterting composition start"));
        return NS_ERROR_FAILURE;
    }

    if (!editorHadCompositionString) {
        return NS_OK;
    }

    nsAutoString compositionString;
    GetCompositionString(aContext, compositionString);
    if (!DispatchCompositionChangeEvent(aContext, compositionString)) {
        MOZ_LOG(gGtkIMLog, LogLevel::Info,
            ("    FAILED, restoring composition string"));
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

void
nsGtkIMModule::InitEvent(WidgetGUIEvent& aEvent)
{
    aEvent.time = PR_Now() / 1000;
}

bool
nsGtkIMModule::EnsureToCacheSelection(nsAString* aSelectedString)
{
    if (aSelectedString) {
        aSelectedString->Truncate();
    }

    if (mSelection.IsValid() &&
        (!mSelection.Collapsed() || !aSelectedString)) {
       return true;
    }

    if (NS_WARN_IF(!mLastFocusedWindow)) {
        MOZ_LOG(gGtkIMLog, LogLevel::Error,
                ("GtkIMModule(%p): EnsureToCacheSelection(), FAILED, due to "
                 "no focused window", this));
        return false;
    }

    nsEventStatus status;
    WidgetQueryContentEvent selection(true, NS_QUERY_SELECTED_TEXT,
                                      mLastFocusedWindow);
    InitEvent(selection);
    mLastFocusedWindow->DispatchEvent(&selection, status);
    if (NS_WARN_IF(!selection.mSucceeded)) {
        MOZ_LOG(gGtkIMLog, LogLevel::Error,
                ("GtkIMModule(%p): EnsureToCacheSelection(), FAILED, due to "
                 "failure of query selection event", this));
        return false;
    }

    mSelection.Assign(selection);
    if (!mSelection.IsValid()) {
        MOZ_LOG(gGtkIMLog, LogLevel::Error,
                ("GtkIMModule(%p): EnsureToCacheSelection(), FAILED, due to "
                 "failure of query selection event (invalid result)", this));
        return false;
    }

    if (!mSelection.Collapsed() && aSelectedString) {
        aSelectedString->Assign(selection.mReply.mString);
    }

    MOZ_LOG(gGtkIMLog, LogLevel::Debug,
            ("GtkIMModule(%p): EnsureToCacheSelection(), Succeeded, mSelection="
             "{ mOffset=%u, mLength=%u, mWritingMode=%s }",
             this, mSelection.mOffset, mSelection.mLength,
             GetWritingModeName(mSelection.mWritingMode).get()));
    return true;
}





void
nsGtkIMModule::Selection::Assign(const IMENotification& aIMENotification)
{
    MOZ_ASSERT(aIMENotification.mMessage == NOTIFY_IME_OF_SELECTION_CHANGE);
    mOffset = aIMENotification.mSelectionChangeData.mOffset;
    mLength = aIMENotification.mSelectionChangeData.mLength;
    mWritingMode = aIMENotification.mSelectionChangeData.GetWritingMode();
}

void
nsGtkIMModule::Selection::Assign(const WidgetQueryContentEvent& aEvent)
{
    MOZ_ASSERT(aEvent.message == NS_QUERY_SELECTED_TEXT);
    MOZ_ASSERT(aEvent.mSucceeded);
    mOffset = aEvent.mReply.mOffset;
    mLength = aEvent.mReply.mString.Length();
    mWritingMode = aEvent.GetWritingMode();
}
