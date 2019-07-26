





#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"
#include "prtime.h"

#include "nsGtkIMModule.h"
#include "nsWindow.h"
#include "mozilla/Likely.h"
#include "mozilla/MiscEvents.h"
#include "mozilla/Preferences.h"
#include "mozilla/TextEvents.h"

using namespace mozilla;
using namespace mozilla::widget;

#ifdef PR_LOGGING
PRLogModuleInfo* gGtkIMLog = nullptr;

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
#endif

nsGtkIMModule* nsGtkIMModule::sLastFocusedModule = nullptr;

nsGtkIMModule::nsGtkIMModule(nsWindow* aOwnerWindow) :
    mOwnerWindow(aOwnerWindow), mLastFocusedWindow(nullptr),
    mContext(nullptr),
    mSimpleContext(nullptr),
    mDummyContext(nullptr),
    mCompositionStart(UINT32_MAX), mProcessingKeyEvent(nullptr),
    mCompositionState(eCompositionState_NotComposing),
    mIsIMFocused(false), mIgnoreNativeCompositionEvent(false)
{
#ifdef PR_LOGGING
    if (!gGtkIMLog) {
        gGtkIMLog = PR_NewLogModule("nsGtkIMModuleWidgets");
    }
#endif
    Init();
}

void
nsGtkIMModule::Init()
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
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

    
    mDummyContext = gtk_im_multicontext_new();
    gtk_im_context_set_client_window(mDummyContext, gdkWindow);
}

nsGtkIMModule::~nsGtkIMModule()
{
    if (this == sLastFocusedModule) {
        sLastFocusedModule = nullptr;
    }
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p) was gone", this));
}

void
nsGtkIMModule::OnDestroyWindow(nsWindow* aWindow)
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): OnDestroyWindow, aWindow=%p, mLastFocusedWindow=%p, mOwnerWindow=%p, mLastFocusedModule=%p",
         this, aWindow, mLastFocusedWindow, mOwnerWindow, sLastFocusedModule));

    NS_PRECONDITION(aWindow, "aWindow must not be null");

    if (mLastFocusedWindow == aWindow) {
        CancelIMEComposition(aWindow);
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

    mOwnerWindow = nullptr;
    mLastFocusedWindow = nullptr;
    mInputContext.mIMEState.mEnabled = IMEState::DISABLED;

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("    SUCCEEDED, Completely destroyed"));
}



















void
nsGtkIMModule::PrepareToDestroyContext(GtkIMContext *aContext)
{
    MozContainer* container = mOwnerWindow->GetMozContainer();
    NS_PRECONDITION(container, "The container of the window is null");

    GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT(aContext);
#if (MOZ_WIDGET_GTK == 2)
    GtkIMContext *slave = multicontext->slave;
#else
    GtkIMContext *slave = nullptr; 
#endif
    if (!slave) {
        return;
    }

    GType slaveType = G_TYPE_FROM_INSTANCE(slave);
    const gchar *im_type_name = g_type_name(slaveType);
    if (strcmp(im_type_name, "GtkIMContextXIM") == 0) {
        if (gtk_check_version(2, 12, 1) == nullptr) {
            return; 
        }

        struct GtkIMContextXIM
        {
            GtkIMContext parent;
            gpointer private_data;
            
        };

        gpointer signal_data =
            reinterpret_cast<GtkIMContextXIM*>(slave)->private_data;
        if (!signal_data) {
            return;
        }

        g_signal_handlers_disconnect_matched(
            gtk_widget_get_display(GTK_WIDGET(container)),
            G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, signal_data);

        
        
        
        static gpointer gtk_xim_context_class =
            g_type_class_ref(slaveType);
        
        (void)gtk_xim_context_class;
    } else if (strcmp(im_type_name, "GtkIMContextIIIM") == 0) {
        
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

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
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

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): OnBlurWindow, aWindow=%p, mLastFocusedWindow=%p, mIsIMFocused=%s",
         this, aWindow, mLastFocusedWindow, mIsIMFocused ? "YES" : "NO"));

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

    if (!IsEditable() || MOZ_UNLIKELY(IsDestroyed())) {
        return false;
    }

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): OnKeyEvent, aCaller=%p, aKeyDownEventWasSent=%s",
         this, aCaller, aKeyDownEventWasSent ? "TRUE" : "FALSE"));
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("    aEvent: type=%s, keyval=%s, unicode=0x%X",
         aEvent->type == GDK_KEY_PRESS ? "GDK_KEY_PRESS" :
         aEvent->type == GDK_KEY_RELEASE ? "GDK_KEY_RELEASE" : "Unknown",
         gdk_keyval_name(aEvent->keyval),
         gdk_keyval_to_unicode(aEvent->keyval)));

    if (aCaller != mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, the caller isn't focused window, mLastFocusedWindow=%p",
             mLastFocusedWindow));
        return false;
    }

    GtkIMContext* im = GetContext();
    if (MOZ_UNLIKELY(!im)) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no context"));
        return false;
    }

    mKeyDownEventWasSent = aKeyDownEventWasSent;
    mFilterKeyEvent = true;
    mProcessingKeyEvent = aEvent;
    gboolean isFiltered = gtk_im_context_filter_keypress(im, aEvent);
    mProcessingKeyEvent = nullptr;

    
    
    
    
    
    bool filterThisEvent = isFiltered && mFilterKeyEvent;

    if (IsComposing() && !isFiltered) {
        if (aEvent->type == GDK_KEY_PRESS) {
            if (!mDispatchedCompositionString.IsEmpty()) {
                
                
                filterThisEvent = true;
            } else {
                
                
                
                
                
                
                
                CommitCompositionBy(EmptyString());
                filterThisEvent = false;
            }
        } else {
            
            
            filterThisEvent = true;
        }
    }

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("    filterThisEvent=%s (isFiltered=%s, mFilterKeyEvent=%s)",
         filterThisEvent ? "TRUE" : "FALSE", isFiltered ? "YES" : "NO",
         mFilterKeyEvent ? "YES" : "NO"));

    return filterThisEvent;
}

void
nsGtkIMModule::OnFocusChangeInGecko(bool aFocus)
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): OnFocusChangeInGecko, aFocus=%s, "
         "mCompositionState=%s, mIsIMFocused=%s, "
         "mIgnoreNativeCompositionEvent=%s",
         this, aFocus ? "YES" : "NO", GetCompositionStateName(),
         mIsIMFocused ? "YES" : "NO",
         mIgnoreNativeCompositionEvent ? "YES" : "NO"));

    
    mSelectedString.Truncate();

    if (aFocus) {
        
        
        mIgnoreNativeCompositionEvent = false;
    }
}

void
nsGtkIMModule::ResetIME()
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): ResetIME, mCompositionState=%s, mIsIMFocused=%s",
         this, GetCompositionStateName(), mIsIMFocused ? "YES" : "NO"));

    GtkIMContext *im = GetContext();
    if (MOZ_UNLIKELY(!im)) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no context"));
        return;
    }

    mIgnoreNativeCompositionEvent = true;
    gtk_im_context_reset(im);
}

nsresult
nsGtkIMModule::CommitIMEComposition(nsWindow* aCaller)
{
    if (MOZ_UNLIKELY(IsDestroyed())) {
        return NS_OK;
    }

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): CommitIMEComposition, aCaller=%p, "
         "mCompositionState=%s",
         this, aCaller, GetCompositionStateName()));

    if (aCaller != mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    WARNING: the caller isn't focused window, mLastFocusedWindow=%p",
             mLastFocusedWindow));
        return NS_OK;
    }

    if (!IsComposing()) {
        return NS_OK;
    }

    
    ResetIME();
    CommitCompositionBy(mDispatchedCompositionString);

    return NS_OK;
}

nsresult
nsGtkIMModule::CancelIMEComposition(nsWindow* aCaller)
{
    if (MOZ_UNLIKELY(IsDestroyed())) {
        return NS_OK;
    }

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): CancelIMEComposition, aCaller=%p",
         this, aCaller));

    if (aCaller != mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, the caller isn't focused window, mLastFocusedWindow=%p",
             mLastFocusedWindow));
        return NS_OK;
    }

    if (!IsComposing()) {
        return NS_OK;
    }

    GtkIMContext *im = GetContext();
    if (MOZ_UNLIKELY(!im)) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no context"));
        return NS_OK;
    }

    ResetIME();
    CommitCompositionBy(EmptyString());

    return NS_OK;
}

void
nsGtkIMModule::SetInputContext(nsWindow* aCaller,
                               const InputContext* aContext,
                               const InputContextAction* aAction)
{
    if (MOZ_UNLIKELY(IsDestroyed())) {
        return;
    }

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): SetInputContext, aCaller=%p, aState=%s mHTMLInputType=%s",
         this, aCaller, GetEnabledStateName(aContext->mIMEState.mEnabled),
         NS_ConvertUTF16toUTF8(aContext->mHTMLInputType).get()));

    if (aCaller != mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, the caller isn't focused window, mLastFocusedWindow=%p",
             mLastFocusedWindow));
        return;
    }

    if (!mContext) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no context"));
        return;
    }


    if (sLastFocusedModule != this) {
        mInputContext = *aContext;
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    SUCCEEDED, but we're not active"));
        return;
    }

    bool changingEnabledState =
        aContext->mIMEState.mEnabled != mInputContext.mIMEState.mEnabled;

    
    if (changingEnabledState && IsEditable()) {
        CommitIMEComposition(mLastFocusedWindow);
        Blur();
    }

    mInputContext = *aContext;

    
    
    
    
    if (changingEnabledState) {
        Focus();
    }
}

InputContext
nsGtkIMModule::GetInputContext()
{
    mInputContext.mIMEState.mOpen = IMEState::OPEN_STATE_NOT_SUPPORTED;
    return mInputContext;
}


bool
nsGtkIMModule::IsVirtualKeyboardOpened()
{
    return false;
}

GtkIMContext*
nsGtkIMModule::GetContext()
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
nsGtkIMModule::IsEnabled()
{
    return mInputContext.mIMEState.mEnabled == IMEState::ENABLED ||
           mInputContext.mIMEState.mEnabled == IMEState::PLUGIN;
}

bool
nsGtkIMModule::IsEditable()
{
    return mInputContext.mIMEState.mEnabled == IMEState::ENABLED ||
           mInputContext.mIMEState.mEnabled == IMEState::PLUGIN ||
           mInputContext.mIMEState.mEnabled == IMEState::PASSWORD;
}

void
nsGtkIMModule::Focus()
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): Focus, sLastFocusedModule=%p",
         this, sLastFocusedModule));

    if (mIsIMFocused) {
        NS_ASSERTION(sLastFocusedModule == this,
                     "We're not active, but the IM was focused?");
        return;
    }

    GtkIMContext *im = GetContext();
    if (!im) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no context"));
        return;
    }

    if (sLastFocusedModule && sLastFocusedModule != this) {
        sLastFocusedModule->Blur();
    }

    sLastFocusedModule = this;

    gtk_im_context_focus_in(im);
    mIsIMFocused = true;

    if (!IsEnabled()) {
        
        
        Blur();
    }
}

void
nsGtkIMModule::Blur()
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): Blur, mIsIMFocused=%s",
         this, mIsIMFocused ? "YES" : "NO"));

    if (!mIsIMFocused) {
        return;
    }

    GtkIMContext *im = GetContext();
    if (!im) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no context"));
        return;
    }

    gtk_im_context_focus_out(im);
    mIsIMFocused = false;
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
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): OnStartCompositionNative, aContext=%p",
         this, aContext));

    
    if (GetContext() != aContext) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, given context doesn't match, GetContext()=%p",
             GetContext()));
        return;
    }

    if (!DispatchCompositionStart()) {
        return;
    }
    SetCursorPosition(mCompositionStart);
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
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): OnEndCompositionNative, aContext=%p",
         this, aContext));

    
    if (GetContext() != aContext) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, given context doesn't match, GetContext()=%p",
             GetContext()));
        return;
    }

    bool shouldIgnoreThisEvent = ShouldIgnoreNativeCompositionEvent();

    
    
    
    
    mIgnoreNativeCompositionEvent = false;

    if (!IsComposing() || shouldIgnoreThisEvent) {
        
        return;
    }

    
    DispatchCompositionEnd();
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
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): OnChangeCompositionNative, aContext=%p",
         this, aContext));

    
    if (GetContext() != aContext) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, given context doesn't match, GetContext()=%p",
             GetContext()));
        return;
    }

    if (ShouldIgnoreNativeCompositionEvent()) {
        return;
    }

    nsAutoString compositionString;
    GetCompositionString(compositionString);
    if (!IsComposing() && compositionString.IsEmpty()) {
        mDispatchedCompositionString.Truncate();
        return; 
    }

    
    DispatchTextEvent(compositionString, false);
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
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): OnRetrieveSurroundingNative, aContext=%p, current context=%p",
         this, aContext, GetContext()));

    if (GetContext() != aContext) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, given context doesn't match, GetContext()=%p",
             GetContext()));
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
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): OnDeleteSurroundingNative, aContext=%p, current context=%p",
         this, aContext, GetContext()));

    if (GetContext() != aContext) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, given context doesn't match, GetContext()=%p",
             GetContext()));
        return FALSE;
    }

    if (NS_SUCCEEDED(DeleteText(aOffset, (uint32_t)aNChars))) {
        return TRUE;
    }

    
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
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

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): OnCommitCompositionNative, aContext=%p, current context=%p, commitString=\"%s\"",
         this, aContext, GetContext(), commitString));

    
    if (GetContext() != aContext) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, given context doesn't match, GetContext()=%p",
             GetContext()));
        return;
    }

    
    
    
    
    
    if (!IsComposing() && !commitString[0]) {
        return;
    }

    if (ShouldIgnoreNativeCompositionEvent()) {
        return;
    }

    
    
    
    if (!IsComposing() && mProcessingKeyEvent) {
        char keyval_utf8[8]; 
        gint keyval_utf8_len;
        guint32 keyval_unicode;

        keyval_unicode = gdk_keyval_to_unicode(mProcessingKeyEvent->keyval);
        keyval_utf8_len = g_unichar_to_utf8(keyval_unicode, keyval_utf8);
        keyval_utf8[keyval_utf8_len] = '\0';

        if (!strcmp(commitString, keyval_utf8)) {
            PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
                ("GtkIMModule(%p): OnCommitCompositionNative, we'll send normal key event",
                 this));
            mFilterKeyEvent = false;
            return;
        }
    }

    NS_ConvertUTF8toUTF16 str(commitString);
    CommitCompositionBy(str); 
}

bool
nsGtkIMModule::CommitCompositionBy(const nsAString& aString)
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): CommitCompositionBy, aString=\"%s\", "
         "mDispatchedCompositionString=\"%s\"",
         this, NS_ConvertUTF16toUTF8(aString).get(),
         NS_ConvertUTF16toUTF8(mDispatchedCompositionString).get()));

    if (!DispatchTextEvent(aString, true)) {
        return false;
    }
    
    
    return DispatchCompositionEnd(); 
}

void
nsGtkIMModule::GetCompositionString(nsAString &aCompositionString)
{
    gchar *preedit_string;
    gint cursor_pos;
    PangoAttrList *feedback_list;
    gtk_im_context_get_preedit_string(GetContext(), &preedit_string,
                                      &feedback_list, &cursor_pos);
    if (preedit_string && *preedit_string) {
        CopyUTF8toUTF16(preedit_string, aCompositionString);
    } else {
        aCompositionString.Truncate();
    }

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): GetCompositionString, result=\"%s\"",
         this, preedit_string));

    pango_attr_list_unref(feedback_list);
    g_free(preedit_string);
}

bool
nsGtkIMModule::DispatchCompositionStart()
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): DispatchCompositionStart", this));

    if (IsComposing()) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    WARNING, we're already in composition"));
        return true;
    }

    if (!mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no focused window in this module"));
        return false;
    }

    nsEventStatus status;
    WidgetQueryContentEvent selection(true, NS_QUERY_SELECTED_TEXT,
                                      mLastFocusedWindow);
    InitEvent(selection);
    mLastFocusedWindow->DispatchEvent(&selection, status);

    if (!selection.mSucceeded || selection.mReply.mOffset == UINT32_MAX) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, cannot query the selection offset"));
        return false;
    }

    
    
    
    
    mCompositionStart = selection.mReply.mOffset;
    mDispatchedCompositionString.Truncate();

    if (mProcessingKeyEvent && !mKeyDownEventWasSent &&
        mProcessingKeyEvent->type == GDK_KEY_PRESS) {
        
        
        nsCOMPtr<nsIWidget> kungFuDeathGrip = mLastFocusedWindow;
        bool isCancelled;
        mLastFocusedWindow->DispatchKeyDownEvent(mProcessingKeyEvent,
                                                 &isCancelled);
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    keydown event is dispatched"));
        if (static_cast<nsWindow*>(kungFuDeathGrip.get())->IsDestroyed() ||
            kungFuDeathGrip != mLastFocusedWindow) {
            PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
                ("    NOTE, the focused widget was destroyed/changed by keydown event"));
            return false;
        }
    }

    if (mIgnoreNativeCompositionEvent) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    WARNING, mIgnoreNativeCompositionEvent is already TRUE, but we forcedly reset"));
        mIgnoreNativeCompositionEvent = false;
    }

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("    mCompositionStart=%u", mCompositionStart));
    mCompositionState = eCompositionState_CompositionStartDispatched;
    WidgetCompositionEvent compEvent(true, NS_COMPOSITION_START,
                                     mLastFocusedWindow);
    InitEvent(compEvent);
    nsCOMPtr<nsIWidget> kungFuDeathGrip = mLastFocusedWindow;
    mLastFocusedWindow->DispatchEvent(&compEvent, status);
    if (static_cast<nsWindow*>(kungFuDeathGrip.get())->IsDestroyed() ||
        kungFuDeathGrip != mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    NOTE, the focused widget was destroyed/changed by compositionstart event"));
        return false;
    }

    return true;
}

bool
nsGtkIMModule::DispatchCompositionEnd()
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): DispatchCompositionEnd, "
         "mDispatchedCompositionString=\"%s\"",
         this, NS_ConvertUTF16toUTF8(mDispatchedCompositionString).get()));

    if (!IsComposing()) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    WARNING, we have alrady finished the composition"));
        return false;
    }

    if (!mLastFocusedWindow) {
        mDispatchedCompositionString.Truncate();
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no focused window in this module"));
        return false;
    }

    WidgetCompositionEvent compEvent(true, NS_COMPOSITION_END,
                                     mLastFocusedWindow);
    InitEvent(compEvent);
    compEvent.data = mDispatchedCompositionString;
    nsEventStatus status;
    nsCOMPtr<nsIWidget> kungFuDeathGrip = mLastFocusedWindow;
    mLastFocusedWindow->DispatchEvent(&compEvent, status);
    mCompositionState = eCompositionState_NotComposing;
    mCompositionStart = UINT32_MAX;
    mDispatchedCompositionString.Truncate();
    if (static_cast<nsWindow*>(kungFuDeathGrip.get())->IsDestroyed() ||
        kungFuDeathGrip != mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    NOTE, the focused widget was destroyed/changed by compositionend event"));
        return false;
    }

    return true;
}

bool
nsGtkIMModule::DispatchTextEvent(const nsAString &aCompositionString,
                                 bool aIsCommit)
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): DispatchTextEvent, aIsCommit=%s",
         this, aIsCommit ? "TRUE" : "FALSE"));

    if (!mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no focused window in this module"));
        return false;
    }

    if (!IsComposing()) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    The composition wasn't started, force starting..."));
        nsCOMPtr<nsIWidget> kungFuDeathGrip = mLastFocusedWindow;
        if (!DispatchCompositionStart()) {
            return false;
        }
    }

    nsEventStatus status;
    nsRefPtr<nsWindow> lastFocusedWindow = mLastFocusedWindow;

    if (aCompositionString != mDispatchedCompositionString) {
      WidgetCompositionEvent compositionUpdate(true, NS_COMPOSITION_UPDATE,
                                               mLastFocusedWindow);
      InitEvent(compositionUpdate);
      compositionUpdate.data = aCompositionString;
      mDispatchedCompositionString = aCompositionString;
      mLastFocusedWindow->DispatchEvent(&compositionUpdate, status);
      if (lastFocusedWindow->IsDestroyed() ||
          lastFocusedWindow != mLastFocusedWindow) {
          PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
              ("    NOTE, the focused widget was destroyed/changed by compositionupdate"));
          return false;
      }
    }

    
    if (mCompositionState == eCompositionState_CompositionStartDispatched) {
        
        
        WidgetQueryContentEvent querySelectedTextEvent(true,
                                                       NS_QUERY_SELECTED_TEXT,
                                                       mLastFocusedWindow);
        mLastFocusedWindow->DispatchEvent(&querySelectedTextEvent, status);
        if (querySelectedTextEvent.mSucceeded) {
            mSelectedString = querySelectedTextEvent.mReply.mString;
            mCompositionStart = querySelectedTextEvent.mReply.mOffset;
        }
    }

    WidgetTextEvent textEvent(true, NS_TEXT_TEXT, mLastFocusedWindow);
    InitEvent(textEvent);

    uint32_t targetOffset = mCompositionStart;

    nsAutoTArray<TextRange, 4> textRanges;
    if (!aIsCommit) {
        
        
        SetTextRangeList(textRanges);
        for (uint32_t i = 0; i < textRanges.Length(); i++) {
            TextRange& range = textRanges[i];
            if (range.mRangeType == NS_TEXTRANGE_SELECTEDRAWTEXT ||
                range.mRangeType == NS_TEXTRANGE_SELECTEDCONVERTEDTEXT) {
                targetOffset += range.mStartOffset;
                break;
            }
        }
    }

    textEvent.rangeCount = textRanges.Length();
    textEvent.rangeArray = textRanges.Elements();
    textEvent.theText = mDispatchedCompositionString.get();

    mCompositionState = aIsCommit ?
        eCompositionState_CommitTextEventDispatched :
        eCompositionState_TextEventDispatched;

    mLastFocusedWindow->DispatchEvent(&textEvent, status);
    if (lastFocusedWindow->IsDestroyed() ||
        lastFocusedWindow != mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    NOTE, the focused widget was destroyed/changed by text event"));
        return false;
    }

    SetCursorPosition(targetOffset);

    return true;
}

void
nsGtkIMModule::SetTextRangeList(nsTArray<TextRange> &aTextRangeList)
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): SetTextRangeList", this));

    NS_PRECONDITION(aTextRangeList.IsEmpty(), "aTextRangeList must be empty");

    gchar *preedit_string;
    gint cursor_pos;
    PangoAttrList *feedback_list;
    gtk_im_context_get_preedit_string(GetContext(), &preedit_string,
                                      &feedback_list, &cursor_pos);
    if (!preedit_string || !*preedit_string) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    preedit_string is null"));
        pango_attr_list_unref(feedback_list);
        g_free(preedit_string);
        return;
    }

    PangoAttrIterator* iter;
    iter = pango_attr_list_get_iterator(feedback_list);
    if (!iter) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, iterator couldn't be allocated"));
        pango_attr_list_unref(feedback_list);
        g_free(preedit_string);
        return;
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

        aTextRangeList.AppendElement(range);

        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    mStartOffset=%u, mEndOffset=%u, mRangeType=%s",
             range.mStartOffset, range.mEndOffset,
             GetRangeTypeName(range.mRangeType)));
    } while (pango_attr_iterator_next(iter));

    TextRange range;
    if (cursor_pos < 0) {
        range.mStartOffset = 0;
    } else if (uint32_t(cursor_pos) > mDispatchedCompositionString.Length()) {
        range.mStartOffset = mDispatchedCompositionString.Length();
    } else {
        range.mStartOffset = uint32_t(cursor_pos);
    }
    range.mEndOffset = range.mStartOffset;
    range.mRangeType = NS_TEXTRANGE_CARETPOSITION;
    aTextRangeList.AppendElement(range);

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("    mStartOffset=%u, mEndOffset=%u, mRangeType=%s",
         range.mStartOffset, range.mEndOffset,
         GetRangeTypeName(range.mRangeType)));

    pango_attr_iterator_destroy(iter);
    pango_attr_list_unref(feedback_list);
    g_free(preedit_string);
}

void
nsGtkIMModule::SetCursorPosition(uint32_t aTargetOffset)
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): SetCursorPosition, aTargetOffset=%u",
         this, aTargetOffset));

    if (aTargetOffset == UINT32_MAX) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, aTargetOffset is wrong offset"));
        return;
    }

    if (!mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no focused window"));
        return;
    }

    GtkIMContext *im = GetContext();
    if (!im) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no context"));
        return;
    }

    WidgetQueryContentEvent charRect(true, NS_QUERY_TEXT_RECT,
                                     mLastFocusedWindow);
    charRect.InitForQueryTextRect(aTargetOffset, 1);
    InitEvent(charRect);
    nsEventStatus status;
    mLastFocusedWindow->DispatchEvent(&charRect, status);
    if (!charRect.mSucceeded) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
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

    gtk_im_context_set_cursor_location(im, &area);
}

nsresult
nsGtkIMModule::GetCurrentParagraph(nsAString& aText, uint32_t& aCursorPos)
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): GetCurrentParagraph, mCompositionState=%s",
         this, GetCompositionStateName()));

    if (!mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no focused window in this module"));
        return NS_ERROR_NULL_POINTER;
    }

    nsEventStatus status;

    uint32_t selOffset = mCompositionStart;
    uint32_t selLength = mSelectedString.Length();

    
    
    if (!EditorHasCompositionString()) {
        
        WidgetQueryContentEvent querySelectedTextEvent(true,
                                                       NS_QUERY_SELECTED_TEXT,
                                                       mLastFocusedWindow);
        mLastFocusedWindow->DispatchEvent(&querySelectedTextEvent, status);
        NS_ENSURE_TRUE(querySelectedTextEvent.mSucceeded, NS_ERROR_FAILURE);

        selOffset = querySelectedTextEvent.mReply.mOffset;
        selLength = querySelectedTextEvent.mReply.mString.Length();
    }

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("        selOffset=%u, selLength=%u",
         selOffset, selLength));

    
    
    
    if (selOffset > INT32_MAX || selLength > INT32_MAX ||
        selOffset + selLength > INT32_MAX) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
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
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
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

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("    aText=%s, aText.Length()=%u, aCursorPos=%u",
         NS_ConvertUTF16toUTF8(aText).get(),
         aText.Length(), aCursorPos));

    return NS_OK;
}

nsresult
nsGtkIMModule::DeleteText(const int32_t aOffset, const uint32_t aNChars)
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): DeleteText, aOffset=%d, aNChars=%d, "
         "mCompositionState=%s",
         this, aOffset, aNChars, GetCompositionStateName()));

    if (!mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no focused window in this module"));
        return NS_ERROR_NULL_POINTER;
    }

    if (!aNChars) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
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
        if (editorHadCompositionString &&
            !DispatchTextEvent(mSelectedString, false)) {
            PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
                ("    FAILED, quitting from DeletText"));
            return NS_ERROR_FAILURE;
        }
        if (!DispatchCompositionEnd()) {
            PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
                ("    FAILED, quitting from DeletText"));
            return NS_ERROR_FAILURE;
        }
    } else {
        
        WidgetQueryContentEvent querySelectedTextEvent(true,
                                                       NS_QUERY_SELECTED_TEXT,
                                                       mLastFocusedWindow);
        lastFocusedWindow->DispatchEvent(&querySelectedTextEvent, status);
        NS_ENSURE_TRUE(querySelectedTextEvent.mSucceeded, NS_ERROR_FAILURE);

        selOffset = querySelectedTextEvent.mReply.mOffset;
    }

    
    WidgetQueryContentEvent queryTextContentEvent(true,
                                                  NS_QUERY_TEXT_CONTENT,
                                                  mLastFocusedWindow);
    queryTextContentEvent.InitForQueryTextContent(0, UINT32_MAX);
    mLastFocusedWindow->DispatchEvent(&queryTextContentEvent, status);
    NS_ENSURE_TRUE(queryTextContentEvent.mSucceeded, NS_ERROR_FAILURE);
    if (queryTextContentEvent.mReply.mString.IsEmpty()) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there is no contents"));
        return NS_ERROR_FAILURE;
    }

    NS_ConvertUTF16toUTF8 utf8Str(
        nsDependentSubstring(queryTextContentEvent.mReply.mString,
                             0, selOffset));
    glong offsetInUTF8Characters =
        g_utf8_strlen(utf8Str.get(), utf8Str.Length()) + aOffset;
    if (offsetInUTF8Characters < 0) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
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
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
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
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
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
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, deleting the selection caused focus change "
             "or window destroyed"));
        return NS_ERROR_FAILURE;
    }

    if (!wasComposing) {
        return NS_OK;
    }

    
    if (!DispatchCompositionStart()) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, resterting composition start"));
        return NS_ERROR_FAILURE;
    }

    if (!editorHadCompositionString) {
        return NS_OK;
    }

    nsAutoString compositionString;
    GetCompositionString(compositionString);
    if (!DispatchTextEvent(compositionString, true)) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
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
nsGtkIMModule::ShouldIgnoreNativeCompositionEvent()
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): ShouldIgnoreNativeCompositionEvent, mLastFocusedWindow=%p, mIgnoreNativeCompositionEvent=%s",
         this, mLastFocusedWindow,
         mIgnoreNativeCompositionEvent ? "YES" : "NO"));

    if (!mLastFocusedWindow) {
        return true; 
    }

    return mIgnoreNativeCompositionEvent;
}
