







































#ifdef MOZ_PLATFORM_MAEMO
#define MAEMO_CHANGES
#endif

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "nsGtkIMModule.h"
#include "nsWindow.h"
#include "mozilla/Preferences.h"

#ifdef MOZ_PLATFORM_MAEMO
#include "nsServiceManagerUtils.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#endif

using namespace mozilla;
using namespace mozilla::widget;

#ifdef PR_LOGGING
PRLogModuleInfo* gGtkIMLog = nsnull;

static const char*
GetRangeTypeName(PRUint32 aRangeType)
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
GetEnabledStateName(PRUint32 aState)
{
    switch (aState) {
        case InputContext::IME_DISABLED:
            return "DISABLED";
        case InputContext::IME_ENABLED:
            return "ENABLED";
        case InputContext::IME_PASSWORD:
            return "PASSWORD";
        case InputContext::IME_PLUGIN:
            return "PLUG_IN";
        default:
            return "UNKNOWN ENABLED STATUS!!";
    }
}
#endif

nsGtkIMModule* nsGtkIMModule::sLastFocusedModule = nsnull;

#ifdef MOZ_PLATFORM_MAEMO
static bool gIsVirtualKeyboardOpened = false;
#endif

nsGtkIMModule::nsGtkIMModule(nsWindow* aOwnerWindow) :
    mOwnerWindow(aOwnerWindow), mLastFocusedWindow(nsnull),
    mContext(nsnull),
#ifndef NS_IME_ENABLED_ON_PASSWORD_FIELD
    mSimpleContext(nsnull),
#endif
    mDummyContext(nsnull),
    mCompositionStart(PR_UINT32_MAX), mProcessingKeyEvent(nsnull),
    mIsComposing(false), mIsIMFocused(false),
    mIgnoreNativeCompositionEvent(false)
{
#ifdef PR_LOGGING
    if (!gGtkIMLog) {
        gGtkIMLog = PR_NewLogModule("nsGtkIMModuleWidgets");
    }
#endif
    mInputContext.mIMEEnabled = InputContext::IME_ENABLED;
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
    GdkWindow* gdkWindow = GTK_WIDGET(container)->window;

    
    

    
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

#ifndef NS_IME_ENABLED_ON_PASSWORD_FIELD
    
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
#endif 

    
    mDummyContext = gtk_im_multicontext_new();
    gtk_im_context_set_client_window(mDummyContext, gdkWindow);
}

nsGtkIMModule::~nsGtkIMModule()
{
    if (this == sLastFocusedModule) {
        sLastFocusedModule = nsnull;
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
        mLastFocusedWindow = nsnull;
    }

    if (mOwnerWindow != aWindow) {
        return;
    }

    if (sLastFocusedModule == this) {
        sLastFocusedModule = nsnull;
    }

    







    if (mContext) {
        PrepareToDestroyContext(mContext);
        gtk_im_context_set_client_window(mContext, nsnull);
        g_object_unref(mContext);
        mContext = nsnull;
    }

#ifndef NS_IME_ENABLED_ON_PASSWORD_FIELD
    if (mSimpleContext) {
        gtk_im_context_set_client_window(mSimpleContext, nsnull);
        g_object_unref(mSimpleContext);
        mSimpleContext = nsnull;
    }
#endif 

    if (mDummyContext) {
        
        
        gtk_im_context_set_client_window(mDummyContext, nsnull);
        g_object_unref(mDummyContext);
        mDummyContext = nsnull;
    }

    mOwnerWindow = nsnull;
    mLastFocusedWindow = nsnull;
    mInputContext.mIMEEnabled = InputContext::IME_DISABLED;

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("    SUCCEEDED, Completely destroyed"));
}



















void
nsGtkIMModule::PrepareToDestroyContext(GtkIMContext *aContext)
{
    MozContainer* container = mOwnerWindow->GetMozContainer();
    NS_PRECONDITION(container, "The container of the window is null");

    GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT(aContext);
    GtkIMContext *slave = multicontext->slave;
    if (!slave) {
        return;
    }

    GType slaveType = G_TYPE_FROM_INSTANCE(slave);
    const gchar *im_type_name = g_type_name(slaveType);
    if (strcmp(im_type_name, "GtkIMContextXIM") == 0) {
        if (gtk_check_version(2, 12, 1) == NULL) {
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
            G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, signal_data);

        
        
        
        static gpointer gtk_xim_context_class =
            g_type_class_ref(slaveType);
        
        gtk_xim_context_class = gtk_xim_context_class;
    } else if (strcmp(im_type_name, "GtkIMContextIIIM") == 0) {
        
        static gpointer gtk_iiim_context_class =
            g_type_class_ref(slaveType);
        
        gtk_iiim_context_class = gtk_iiim_context_class;
    }
}

void
nsGtkIMModule::OnFocusWindow(nsWindow* aWindow)
{
    if (NS_UNLIKELY(IsDestroyed())) {
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
    if (NS_UNLIKELY(IsDestroyed())) {
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

    if (!IsEditable() || NS_UNLIKELY(IsDestroyed())) {
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
    if (NS_UNLIKELY(!im)) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no context"));
        return false;
    }

    mKeyDownEventWasSent = aKeyDownEventWasSent;
    mFilterKeyEvent = true;
    mProcessingKeyEvent = aEvent;
    gboolean isFiltered = gtk_im_context_filter_keypress(im, aEvent);
    mProcessingKeyEvent = nsnull;

    
    
    
    
    
    bool filterThisEvent = isFiltered && mFilterKeyEvent;

    if (mIsComposing && !isFiltered) {
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
        ("GtkIMModule(%p): OnFocusChangeInGecko, aFocus=%s mIsComposing=%s, mIsIMFocused=%s, mIgnoreNativeCompositionEvent=%s",
         this, aFocus ? "YES" : "NO", mIsComposing ? "YES" : "NO",
         mIsIMFocused ? "YES" : "NO",
         mIgnoreNativeCompositionEvent ? "YES" : "NO"));
    if (aFocus) {
        
        
        mIgnoreNativeCompositionEvent = false;
    }
}

void
nsGtkIMModule::ResetIME()
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): ResetIME, mIsComposing=%s, mIsIMFocused=%s",
         this, mIsComposing ? "YES" : "NO", mIsIMFocused ? "YES" : "NO"));

    GtkIMContext *im = GetContext();
    if (NS_UNLIKELY(!im)) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no context"));
        return;
    }

    mIgnoreNativeCompositionEvent = true;
    gtk_im_context_reset(im);
}

nsresult
nsGtkIMModule::ResetInputState(nsWindow* aCaller)
{
    if (NS_UNLIKELY(IsDestroyed())) {
        return NS_OK;
    }

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): ResetInputState, aCaller=%p, mIsComposing=%s",
         this, aCaller, mIsComposing ? "YES" : "NO"));

    if (aCaller != mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    WARNING: the caller isn't focused window, mLastFocusedWindow=%p",
             mLastFocusedWindow));
        return NS_OK;
    }

    if (!mIsComposing) {
        return NS_OK;
    }

    
    ResetIME();
    CommitCompositionBy(mDispatchedCompositionString);

    return NS_OK;
}

nsresult
nsGtkIMModule::CancelIMEComposition(nsWindow* aCaller)
{
    if (NS_UNLIKELY(IsDestroyed())) {
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

    if (!mIsComposing) {
        return NS_OK;
    }

    GtkIMContext *im = GetContext();
    if (NS_UNLIKELY(!im)) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no context"));
        return NS_OK;
    }

    ResetIME();
    CommitCompositionBy(EmptyString());

    return NS_OK;
}

nsresult
nsGtkIMModule::SetInputMode(nsWindow* aCaller, const InputContext* aContext)
{
    if (aContext->mIMEEnabled == mInputContext.mIMEEnabled ||
        NS_UNLIKELY(IsDestroyed())) {
        return NS_OK;
    }

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): SetInputMode, aCaller=%p, aState=%s mHTMLInputType=%s",
         this, aCaller, GetEnabledStateName(aContext->mIMEEnabled),
         NS_ConvertUTF16toUTF8(aContext->mHTMLInputType).get()));

    if (aCaller != mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, the caller isn't focused window, mLastFocusedWindow=%p",
             mLastFocusedWindow));
        return NS_OK;
    }

    if (!mContext) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no context"));
        return NS_ERROR_NOT_AVAILABLE;
    }


    if (sLastFocusedModule != this) {
        mInputContext = *aContext;
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    SUCCEEDED, but we're not active"));
        return NS_OK;
    }

    
    if (IsEditable()) {
        ResetInputState(mLastFocusedWindow);
        Blur();
    }

    mInputContext = *aContext;

    
    
    
    
    Focus();

#if (MOZ_PLATFORM_MAEMO == 5)
    GtkIMContext *im = GetContext();
    if (im) {
        if (IsEnabled()) {
            
            
            if (mInputContext.mIMEEnabled != InputContext::IME_DISABLED && 
                mInputContext.mIMEEnabled != InputContext::IME_PLUGIN) {

                bool useStrictPolicy =
                    Preferences::GetBool("content.ime.strict_policy", false);
                if (useStrictPolicy && !mInputContext.FocusMovedByUser() && 
                    mInputContext.FocusMovedInContentProcess()) {
                    return NS_OK;
                }
            }

            
            
            
            int mode;
            g_object_get(im, "hildon-input-mode", &mode, NULL);

            if (mInputContext.mIMEEnabled == InputContext::IME_ENABLED ||
                mInputContext.mIMEEnabled == InputContext::IME_PLUGIN) {
                mode &= ~HILDON_GTK_INPUT_MODE_INVISIBLE;
            } else if (mInputContext.mIMEEnabled == InputContext::IME_PASSWORD) {
               mode |= HILDON_GTK_INPUT_MODE_INVISIBLE;
            }

            
            mode &= ~HILDON_GTK_INPUT_MODE_AUTOCAP;

            
            mode &= ~HILDON_GTK_INPUT_MODE_DICTIONARY;

            g_object_set(im, "hildon-input-mode",
                         (HildonGtkInputMode)mode, NULL);
            gIsVirtualKeyboardOpened = true;
            hildon_gtk_im_context_show(im);
        } else {
            gIsVirtualKeyboardOpened = false;
            hildon_gtk_im_context_hide(im);
        }
    }

    nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();
    if (observerService) {
        nsAutoString rectBuf;
        PRInt32 x, y, w, h;
        gdk_window_get_position(aCaller->GetGdkWindow(), &x, &y);
        gdk_window_get_size(aCaller->GetGdkWindow(), &w, &h);
        rectBuf.Assign(NS_LITERAL_STRING("{\"left\": "));
        rectBuf.AppendInt(x);
        rectBuf.Append(NS_LITERAL_STRING(", \"top\": "));
        rectBuf.AppendInt(y);
        rectBuf.Append(NS_LITERAL_STRING(", \"right\": "));
        rectBuf.AppendInt(w);
        rectBuf.Append(NS_LITERAL_STRING(", \"bottom\": "));
        rectBuf.AppendInt(h);
        rectBuf.Append(NS_LITERAL_STRING("}"));
        observerService->NotifyObservers(nsnull, "softkb-change",
                                         rectBuf.get());
    }
#endif

    return NS_OK;
}

nsresult
nsGtkIMModule::GetInputMode(InputContext* aContext)
{
    NS_ENSURE_ARG_POINTER(aContext);
    *aContext = mInputContext;
    return NS_OK;
}


bool
nsGtkIMModule::IsVirtualKeyboardOpened()
{
#ifdef MOZ_PLATFORM_MAEMO
    return gIsVirtualKeyboardOpened;
#else
    return false;
#endif
}

GtkIMContext*
nsGtkIMModule::GetContext()
{
    if (IsEnabled()) {
        return mContext;
    }

#ifndef NS_IME_ENABLED_ON_PASSWORD_FIELD
    if (mInputContext.mIMEEnabled == InputContext::IME_PASSWORD) {
        return mSimpleContext;
    }
#endif 

    return mDummyContext;
}

bool
nsGtkIMModule::IsEnabled()
{
    return mInputContext.mIMEEnabled == InputContext::IME_ENABLED ||
#ifdef NS_IME_ENABLED_ON_PASSWORD_FIELD
           mInputContext.mIMEEnabled == InputContext::IME_PASSWORD ||
#endif 
           mInputContext.mIMEEnabled == InputContext::IME_PLUGIN;
}

bool
nsGtkIMModule::IsEditable()
{
    return mInputContext.mIMEEnabled == InputContext::IME_ENABLED ||
           mInputContext.mIMEEnabled == InputContext::IME_PLUGIN ||
           mInputContext.mIMEEnabled == InputContext::IME_PASSWORD;
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

    if (!mIsComposing || shouldIgnoreThisEvent) {
        
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
    if (!mIsComposing && compositionString.IsEmpty()) {
        mDispatchedCompositionString.Truncate();
        return; 
    }

    
    DispatchTextEvent(compositionString, true);
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
    PRUint32 cursorPos;
    if (NS_FAILED(GetCurrentParagraph(uniStr, cursorPos))) {
        return FALSE;
    }

    glong wbytes;
    gchar *utf8_str = g_utf16_to_utf8((const gunichar2 *)uniStr.get(),
                                      uniStr.Length(), NULL, &wbytes, NULL);
    if (utf8_str == NULL) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    failed to convert utf16 string to utf8"));
        return FALSE;
    }
    gtk_im_context_set_surrounding(aContext, utf8_str, wbytes,
        g_utf8_offset_to_pointer(utf8_str, cursorPos) - utf8_str);
    g_free(utf8_str);

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

    if (NS_SUCCEEDED(DeleteText(aOffset, (PRUint32)aNChars))) {
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

    
    
    
    
    
    if (!mIsComposing && !commitString[0]) {
        return;
    }

    if (ShouldIgnoreNativeCompositionEvent()) {
        return;
    }

    
    
    
    if (!mIsComposing && mProcessingKeyEvent) {
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

    if (!DispatchTextEvent(aString, false)) {
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

    if (mIsComposing) {
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
    nsQueryContentEvent selection(true, NS_QUERY_SELECTED_TEXT,
                                  mLastFocusedWindow);
    InitEvent(selection);
    mLastFocusedWindow->DispatchEvent(&selection, status);

    if (!selection.mSucceeded || selection.mReply.mOffset == PR_UINT32_MAX) {
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
    mIsComposing = true;
    nsCompositionEvent compEvent(true, NS_COMPOSITION_START,
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

    if (!mIsComposing) {
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

    nsCompositionEvent compEvent(true, NS_COMPOSITION_END,
                                 mLastFocusedWindow);
    InitEvent(compEvent);
    compEvent.data = mDispatchedCompositionString;
    nsEventStatus status;
    nsCOMPtr<nsIWidget> kungFuDeathGrip = mLastFocusedWindow;
    mLastFocusedWindow->DispatchEvent(&compEvent, status);
    mIsComposing = false;
    mCompositionStart = PR_UINT32_MAX;
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
                                 bool aCheckAttr)
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): DispatchTextEvent, aCheckAttr=%s",
         this, aCheckAttr ? "TRUE" : "FALSE"));

    if (!mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no focused window in this module"));
        return false;
    }

    if (!mIsComposing) {
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
      nsCompositionEvent compositionUpdate(true, NS_COMPOSITION_UPDATE,
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

    nsTextEvent textEvent(true, NS_TEXT_TEXT, mLastFocusedWindow);
    InitEvent(textEvent);

    PRUint32 targetOffset = mCompositionStart;

    nsAutoTArray<nsTextRange, 4> textRanges;
    if (aCheckAttr) {
        
        
        SetTextRangeList(textRanges);
        for (PRUint32 i = 0; i < textRanges.Length(); i++) {
            nsTextRange& range = textRanges[i];
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
nsGtkIMModule::SetTextRangeList(nsTArray<nsTextRange> &aTextRangeList)
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

        nsTextRange range;
        
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

        gunichar2* uniStr = nsnull;
        if (start == 0) {
            range.mStartOffset = 0;
        } else {
            glong uniStrLen;
            uniStr = g_utf8_to_utf16(preedit_string, start,
                                     NULL, &uniStrLen, NULL);
            if (uniStr) {
                range.mStartOffset = uniStrLen;
                g_free(uniStr);
                uniStr = nsnull;
            }
        }

        glong uniStrLen;
        uniStr = g_utf8_to_utf16(preedit_string + start, end - start,
                                 NULL, &uniStrLen, NULL);
        if (!uniStr) {
            range.mEndOffset = range.mStartOffset;
        } else {
            range.mEndOffset = range.mStartOffset + uniStrLen;
            g_free(uniStr);
            uniStr = nsnull;
        }

        aTextRangeList.AppendElement(range);

        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    mStartOffset=%u, mEndOffset=%u, mRangeType=%s",
             range.mStartOffset, range.mEndOffset,
             GetRangeTypeName(range.mRangeType)));
    } while (pango_attr_iterator_next(iter));

    nsTextRange range;
    if (cursor_pos < 0) {
        range.mStartOffset = 0;
    } else if (PRUint32(cursor_pos) > mDispatchedCompositionString.Length()) {
        range.mStartOffset = mDispatchedCompositionString.Length();
    } else {
        range.mStartOffset = PRUint32(cursor_pos);
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
nsGtkIMModule::SetCursorPosition(PRUint32 aTargetOffset)
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): SetCursorPosition, aTargetOffset=%u",
         this, aTargetOffset));

    if (aTargetOffset == PR_UINT32_MAX) {
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

    nsQueryContentEvent charRect(true, NS_QUERY_TEXT_RECT,
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
nsGtkIMModule::GetCurrentParagraph(nsAString& aText, PRUint32& aCursorPos)
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): GetCurrentParagraph", this));

    if (!mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no focused window in this module"));
        return NS_ERROR_NULL_POINTER;
    }

    nsEventStatus status;

    
    nsQueryContentEvent querySelectedTextEvent(true,
                                               NS_QUERY_SELECTED_TEXT,
                                               mLastFocusedWindow);
    mLastFocusedWindow->DispatchEvent(&querySelectedTextEvent, status);
    NS_ENSURE_TRUE(querySelectedTextEvent.mSucceeded, NS_ERROR_FAILURE);

    PRUint32 selOffset = querySelectedTextEvent.mReply.mOffset;
    PRUint32 selLength = querySelectedTextEvent.mReply.mString.Length();

    
    
    
    if (selOffset > PR_INT32_MAX || selLength > PR_INT32_MAX ||
        selOffset + selLength > PR_INT32_MAX) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, The selection is out of range"));
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("        selOffset=%u, selLength=%u",
             selOffset, selLength));
        return NS_ERROR_FAILURE;
    }

    
    nsQueryContentEvent queryTextContentEvent(true,
                                              NS_QUERY_TEXT_CONTENT,
                                              mLastFocusedWindow);
    queryTextContentEvent.InitForQueryTextContent(0, PR_UINT32_MAX);
    mLastFocusedWindow->DispatchEvent(&queryTextContentEvent, status);
    NS_ENSURE_TRUE(queryTextContentEvent.mSucceeded, NS_ERROR_FAILURE);

    nsAutoString textContent(queryTextContentEvent.mReply.mString);
    if (selOffset + selLength > textContent.Length()) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, The selection is invalid"));
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("        selOffset=%u, selLength=%u, textContent.Length()=%u",
             selOffset, selLength, textContent.Length()));
        return NS_ERROR_FAILURE;
    }

    
    PRInt32 parStart = (selOffset == 0) ? 0 :
        textContent.RFind("\n", false, selOffset - 1, -1) + 1;
    PRInt32 parEnd = textContent.Find("\n", false, selOffset + selLength, -1);
    if (parEnd < 0) {
        parEnd = textContent.Length();
    }
    aText = nsDependentSubstring(textContent, parStart, parEnd - parStart);
    aCursorPos = selOffset - PRUint32(parStart);

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("    aText.Length()=%u, aCursorPos=%u", aText.Length(), aCursorPos));

    return NS_OK;
}

nsresult
nsGtkIMModule::DeleteText(const PRInt32 aOffset, const PRUint32 aNChars)
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): DeleteText, aOffset=%d, aNChars=%d",
         this, aOffset, aNChars));

    if (!mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no focused window in this module"));
        return NS_ERROR_NULL_POINTER;
    }

    nsEventStatus status;

    
    nsQueryContentEvent querySelectedTextEvent(true,
                                               NS_QUERY_SELECTED_TEXT,
                                               mLastFocusedWindow);
    mLastFocusedWindow->DispatchEvent(&querySelectedTextEvent, status);
    NS_ENSURE_TRUE(querySelectedTextEvent.mSucceeded, NS_ERROR_FAILURE);

    
    nsSelectionEvent selectionEvent(true, NS_SELECTION_SET,
                                    mLastFocusedWindow);
    selectionEvent.mOffset = querySelectedTextEvent.mReply.mOffset + aOffset;
    selectionEvent.mLength = aNChars;
    selectionEvent.mReversed = false;
    selectionEvent.mExpandToClusterBoundary = false;
    mLastFocusedWindow->DispatchEvent(&selectionEvent, status);
    NS_ENSURE_TRUE(selectionEvent.mSucceeded, NS_ERROR_FAILURE);

    
    nsContentCommandEvent contentCommandEvent(true,
                                              NS_CONTENT_COMMAND_DELETE,
                                              mLastFocusedWindow);
    mLastFocusedWindow->DispatchEvent(&contentCommandEvent, status);
    NS_ENSURE_TRUE(contentCommandEvent.mSucceeded, NS_ERROR_FAILURE);

    return NS_OK;
}

void
nsGtkIMModule::InitEvent(nsGUIEvent &aEvent)
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
