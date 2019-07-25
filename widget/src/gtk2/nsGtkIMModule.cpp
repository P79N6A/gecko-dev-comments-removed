







































#ifdef MOZ_PLATFORM_MAEMO
#define MAEMO_CHANGES
#endif

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "nsGtkIMModule.h"
#include "nsWindow.h"

#ifdef MOZ_PLATFORM_MAEMO
#include "nsServiceManagerUtils.h"
#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "mozilla/Services.h"
#endif

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
        case nsIWidget::IME_STATUS_DISABLED:
            return "DISABLED";
        case nsIWidget::IME_STATUS_ENABLED:
            return "ENABLED";
        case nsIWidget::IME_STATUS_PASSWORD:
            return "PASSWORD";
        case nsIWidget::IME_STATUS_PLUGIN:
            return "PLUG_IN";
        default:
            return "UNKNOWN ENABLED STATUS!!";
    }
}
#endif

nsGtkIMModule* nsGtkIMModule::sLastFocusedModule = nsnull;

#ifdef MOZ_PLATFORM_MAEMO
static PRBool gIsVirtualKeyboardOpened = PR_FALSE;
#endif

nsGtkIMModule::nsGtkIMModule(nsWindow* aOwnerWindow) :
    mOwnerWindow(aOwnerWindow), mLastFocusedWindow(nsnull),
    mContext(nsnull),
#ifndef NS_IME_ENABLED_ON_PASSWORD_FIELD
    mSimpleContext(nsnull),
#endif
    mDummyContext(nsnull),
    mCompositionStart(PR_UINT32_MAX), mProcessingKeyEvent(nsnull),
    mIsComposing(PR_FALSE), mIsIMFocused(PR_FALSE),
    mIgnoreNativeCompositionEvent(PR_FALSE)
{
#ifdef PR_LOGGING
    if (!gGtkIMLog) {
        gGtkIMLog = PR_NewLogModule("nsGtkIMModuleWidgets");
    }
#endif
    mIMEContext.mStatus = nsIWidget::IME_STATUS_ENABLED;
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
        g_object_unref(G_OBJECT(mContext));
        mContext = nsnull;
    }

#ifndef NS_IME_ENABLED_ON_PASSWORD_FIELD
    if (mSimpleContext) {
        gtk_im_context_set_client_window(mSimpleContext, nsnull);
        g_object_unref(G_OBJECT(mSimpleContext));
        mSimpleContext = nsnull;
    }
#endif 

    if (mDummyContext) {
        
        
        gtk_im_context_set_client_window(mDummyContext, nsnull);
        g_object_unref(G_OBJECT(mDummyContext));
        mDummyContext = nsnull;
    }

    mOwnerWindow = nsnull;
    mLastFocusedWindow = nsnull;
    mIMEContext.mStatus = nsIWidget::IME_STATUS_DISABLED;

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
         this, aWindow));
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

PRBool
nsGtkIMModule::OnKeyEvent(nsWindow* aCaller, GdkEventKey* aEvent,
                          PRBool aKeyDownEventWasSent )
{
    NS_PRECONDITION(aEvent, "aEvent must be non-null");

    if (!IsEditable() || NS_UNLIKELY(IsDestroyed())) {
        return PR_FALSE;
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
        return PR_FALSE;
    }

    GtkIMContext* im = GetContext();
    if (NS_UNLIKELY(!im)) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no context"));
        return PR_FALSE;
    }

    mKeyDownEventWasSent = aKeyDownEventWasSent;
    mFilterKeyEvent = PR_TRUE;
    mProcessingKeyEvent = aEvent;
    gboolean isFiltered = gtk_im_context_filter_keypress(im, aEvent);
    mProcessingKeyEvent = nsnull;

    
    
    
    
    
    PRBool filterThisEvent = isFiltered && mFilterKeyEvent;

    if (mIsComposing && !isFiltered) {
        if (aEvent->type == GDK_KEY_PRESS) {
            if (!mCompositionString.IsEmpty()) {
                
                
                filterThisEvent = PR_TRUE;
            } else {
                
                
                
                
                
                
                
                CommitCompositionBy(EmptyString());
                filterThisEvent = PR_FALSE;
            }
        } else {
            
            
            filterThisEvent = PR_TRUE;
        }
    }

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("    filterThisEvent=%s (isFiltered=%s, mFilterKeyEvent=%s)",
         filterThisEvent ? "TRUE" : "FALSE", isFiltered ? "YES" : "NO",
         mFilterKeyEvent ? "YES" : "NO"));

    return filterThisEvent;
}

void
nsGtkIMModule::OnFocusChangeInGecko(PRBool aFocus)
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): OnFocusChangeInGecko, aFocus=%s mIsComposing=%s, mIsIMFocused=%s, mIgnoreNativeCompositionEvent=%s",
         this, aFocus ? "YES" : "NO", mIsComposing ? "YES" : "NO",
         mIsIMFocused ? "YES" : "NO",
         mIgnoreNativeCompositionEvent ? "YES" : "NO"));
    if (aFocus) {
        
        
        mIgnoreNativeCompositionEvent = PR_FALSE;
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

    mIgnoreNativeCompositionEvent = PR_TRUE;
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
    CommitCompositionBy(mCompositionString);

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
nsGtkIMModule::SetInputMode(nsWindow* aCaller, const IMEContext* aContext)
{
    if (aContext->mStatus == mIMEContext.mStatus || NS_UNLIKELY(IsDestroyed())) {
        return NS_OK;
    }

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): SetInputMode, aCaller=%p, aState=%s mHTMLInputType=%s",
         this, aCaller, GetEnabledStateName(aContext->mStatus), aContext->mHTMLInputType.get()));

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
        mIMEContext = *aContext;
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    SUCCEEDED, but we're not active"));
        return NS_OK;
    }

    
    if (IsEditable()) {
        ResetInputState(mLastFocusedWindow);
        Blur();
    }

    mIMEContext = *aContext;

    
    
    
    
    Focus();

#if (MOZ_PLATFORM_MAEMO == 5)
    GtkIMContext *im = GetContext();
    if (im) {
        if (IsEnabled()) {
            
            
            if (mIMEContext.mStatus != nsIWidget::IME_STATUS_DISABLED && 
                mIMEContext.mStatus != nsIWidget::IME_STATUS_PLUGIN) {

                nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));

                PRBool useStrictPolicy = PR_FALSE;
                if (NS_SUCCEEDED(prefs->GetBoolPref("content.ime.strict_policy", &useStrictPolicy))) {
                    if (useStrictPolicy && !mIMEContext.FocusMovedByUser() && 
                        mIMEContext.FocusMovedInContentProcess()) {
                        return NS_OK;
                    }
                }
            }

            
            
            
            int mode;
            g_object_get(G_OBJECT(im), "hildon-input-mode", &mode, NULL);

            if (mIMEContext.mStatus == nsIWidget::IME_STATUS_ENABLED ||
                mIMEContext.mStatus == nsIWidget::IME_STATUS_PLUGIN) {
                mode &= ~HILDON_GTK_INPUT_MODE_INVISIBLE;
            } else if (mIMEContext.mStatus == nsIWidget::IME_STATUS_PASSWORD) {
               mode |= HILDON_GTK_INPUT_MODE_INVISIBLE;
            }

            
            mode &= ~HILDON_GTK_INPUT_MODE_AUTOCAP;

            
            mode &= ~HILDON_GTK_INPUT_MODE_DICTIONARY;

            g_object_set(G_OBJECT(im), "hildon-input-mode",
                         (HildonGtkInputMode)mode, NULL);
            gIsVirtualKeyboardOpened = PR_TRUE;
            hildon_gtk_im_context_show(im);
        } else {
            gIsVirtualKeyboardOpened = PR_FALSE;
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
nsGtkIMModule::GetInputMode(IMEContext* aContext)
{
    NS_ENSURE_ARG_POINTER(aContext);
    *aContext = mIMEContext;
    return NS_OK;
}


PRBool
nsGtkIMModule::IsVirtualKeyboardOpened()
{
#ifdef MOZ_PLATFORM_MAEMO
    return gIsVirtualKeyboardOpened;
#else
    return PR_FALSE;
#endif
}

GtkIMContext*
nsGtkIMModule::GetContext()
{
    if (IsEnabled()) {
        return mContext;
    }

#ifndef NS_IME_ENABLED_ON_PASSWORD_FIELD
    if (mIMEContext.mStatus == nsIWidget::IME_STATUS_PASSWORD) {
        return mSimpleContext;
    }
#endif 

    return mDummyContext;
}

PRBool
nsGtkIMModule::IsEnabled()
{
    return mIMEContext.mStatus == nsIWidget::IME_STATUS_ENABLED ||
#ifdef NS_IME_ENABLED_ON_PASSWORD_FIELD
           mIMEContext.mStatus == nsIWidget::IME_STATUS_PASSWORD ||
#endif 
           mIMEContext.mStatus == nsIWidget::IME_STATUS_PLUGIN;
}

PRBool
nsGtkIMModule::IsEditable()
{
    return mIMEContext.mStatus == nsIWidget::IME_STATUS_ENABLED ||
           mIMEContext.mStatus == nsIWidget::IME_STATUS_PLUGIN ||
           mIMEContext.mStatus == nsIWidget::IME_STATUS_PASSWORD;
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
            ("    FAILED, there are no context",
             this));
        return;
    }

    if (sLastFocusedModule && sLastFocusedModule != this) {
        sLastFocusedModule->Blur();
    }

    sLastFocusedModule = this;

    gtk_im_context_focus_in(im);
    mIsIMFocused = PR_TRUE;

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
    mIsIMFocused = PR_FALSE;
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

    PRBool shouldIgnoreThisEvent = ShouldIgnoreNativeCompositionEvent();

    
    
    
    
    mIgnoreNativeCompositionEvent = PR_FALSE;

    if (!mIsComposing || shouldIgnoreThisEvent) {
        
        return;
    }

#ifdef PR_LOGGING
    if (!mCompositionString.IsEmpty()) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    WARNING, the composition string is still there"));
    }
#endif
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

    GetCompositionString(mCompositionString);
    if (!mIsComposing && mCompositionString.IsEmpty()) {
        return; 
    }

    DispatchTextEvent(PR_TRUE); 
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
            mFilterKeyEvent = PR_FALSE;
            return;
        }
    }

    NS_ConvertUTF8toUTF16 str(commitString);
    CommitCompositionBy(str); 
}

PRBool
nsGtkIMModule::CommitCompositionBy(const nsAString& aString)
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): CommitCompositionBy, aString=\"%s\", mCompositionString=\"%s\"",
         this, NS_ConvertUTF16toUTF8(aString).get(),
         NS_ConvertUTF16toUTF8(mCompositionString).get()));

    mCompositionString = aString;
    if (!DispatchTextEvent(PR_FALSE)) {
        return PR_FALSE;
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

PRBool
nsGtkIMModule::DispatchCompositionStart()
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): DispatchCompositionStart", this));

    if (mIsComposing) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    WARNING, we're already in composition"));
        return PR_TRUE;
    }

    if (!mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no focused window in this module"));
        return PR_FALSE;
    }

    nsEventStatus status;
    nsQueryContentEvent selection(PR_TRUE, NS_QUERY_SELECTED_TEXT,
                                  mLastFocusedWindow);
    InitEvent(selection);
    mLastFocusedWindow->DispatchEvent(&selection, status);

    if (!selection.mSucceeded || selection.mReply.mOffset == PR_UINT32_MAX) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, cannot query the selection offset"));
        return PR_FALSE;
    }

    mCompositionStart = selection.mReply.mOffset;

    if (mProcessingKeyEvent && !mKeyDownEventWasSent &&
        mProcessingKeyEvent->type == GDK_KEY_PRESS) {
        
        
        nsCOMPtr<nsIWidget> kungFuDeathGrip = mLastFocusedWindow;
        PRBool isCancelled;
        mLastFocusedWindow->DispatchKeyDownEvent(mProcessingKeyEvent,
                                                 &isCancelled);
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    keydown event is dispatched"));
        if (static_cast<nsWindow*>(kungFuDeathGrip.get())->IsDestroyed() ||
            kungFuDeathGrip != mLastFocusedWindow) {
            PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
                ("    NOTE, the focused widget was destroyed/changed by keydown event"));
            return PR_FALSE;
        }
    }

    if (mIgnoreNativeCompositionEvent) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    WARNING, mIgnoreNativeCompositionEvent is already TRUE, but we forcedly reset"));
        mIgnoreNativeCompositionEvent = PR_FALSE;
    }

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("    mCompositionStart=%lu", mCompositionStart));
    mIsComposing = PR_TRUE;
    nsCompositionEvent compEvent(PR_TRUE, NS_COMPOSITION_START,
                                 mLastFocusedWindow);
    InitEvent(compEvent);
    nsCOMPtr<nsIWidget> kungFuDeathGrip = mLastFocusedWindow;
    mLastFocusedWindow->DispatchEvent(&compEvent, status);
    if (static_cast<nsWindow*>(kungFuDeathGrip.get())->IsDestroyed() ||
        kungFuDeathGrip != mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    NOTE, the focused widget was destroyed/changed by compositionstart event"));
        return PR_FALSE;
    }

    return PR_TRUE;
}

PRBool
nsGtkIMModule::DispatchCompositionEnd()
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): DispatchCompositionEnd", this));

    if (!mIsComposing) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    WARNING, we have alrady finished the composition"));
        return PR_FALSE;
    }

    if (!mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no focused window in this module"));
        return PR_FALSE;
    }

    nsCompositionEvent compEvent(PR_TRUE, NS_COMPOSITION_END,
                                 mLastFocusedWindow);
    InitEvent(compEvent);
    nsEventStatus status;
    nsCOMPtr<nsIWidget> kungFuDeathGrip = mLastFocusedWindow;
    mLastFocusedWindow->DispatchEvent(&compEvent, status);
    mIsComposing = PR_FALSE;
    mCompositionStart = PR_UINT32_MAX;
    mCompositionString.Truncate();
    if (static_cast<nsWindow*>(kungFuDeathGrip.get())->IsDestroyed() ||
        kungFuDeathGrip != mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    NOTE, the focused widget was destroyed/changed by compositionend event"));
        return PR_FALSE;
    }

    return PR_TRUE;
}

PRBool
nsGtkIMModule::DispatchTextEvent(PRBool aCheckAttr)
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): DispatchTextEvent, aCheckAttr=%s",
         this, aCheckAttr ? "TRUE" : "FALSE"));

    if (!mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    FAILED, there are no focused window in this module"));
        return PR_FALSE;
    }

    if (!mIsComposing) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    The composition wasn't started, force starting..."));
        nsCOMPtr<nsIWidget> kungFuDeathGrip = mLastFocusedWindow;
        if (!DispatchCompositionStart()) {
            return PR_FALSE;
        }
    }

    nsTextEvent textEvent(PR_TRUE, NS_TEXT_TEXT, mLastFocusedWindow);
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
    textEvent.theText = mCompositionString.get();

    nsEventStatus status;
    nsCOMPtr<nsIWidget> kungFuDeathGrip = mLastFocusedWindow;
    mLastFocusedWindow->DispatchEvent(&textEvent, status);
    if (static_cast<nsWindow*>(kungFuDeathGrip.get())->IsDestroyed() ||
        kungFuDeathGrip != mLastFocusedWindow) {
        PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
            ("    NOTE, the focused widget was destroyed/changed by text event"));
        return PR_FALSE;
    }

    SetCursorPosition(targetOffset);

    return PR_TRUE;
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
            ("    mStartOffset=%lu, mEndOffset=%lu, mRangeType=%s",
             range.mStartOffset, range.mEndOffset,
             GetRangeTypeName(range.mRangeType)));
    } while (pango_attr_iterator_next(iter));

    nsTextRange range;
    if (cursor_pos < 0) {
        range.mStartOffset = 0;
    } else if (PRUint32(cursor_pos) > mCompositionString.Length()) {
        range.mStartOffset = mCompositionString.Length();
    } else {
        range.mStartOffset = PRUint32(cursor_pos);
    }
    range.mEndOffset = range.mStartOffset;
    range.mRangeType = NS_TEXTRANGE_CARETPOSITION;
    aTextRangeList.AppendElement(range);

    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("    mStartOffset=%lu, mEndOffset=%lu, mRangeType=%s",
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
        ("GtkIMModule(%p): SetCursorPosition, aTargetOffset=%lu",
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

    nsQueryContentEvent charRect(PR_TRUE, NS_QUERY_TEXT_RECT,
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

    
    nsQueryContentEvent querySelectedTextEvent(PR_TRUE,
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

    
    nsQueryContentEvent queryTextContentEvent(PR_TRUE,
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
        textContent.RFind("\n", PR_FALSE, selOffset - 1, -1) + 1;
    PRInt32 parEnd = textContent.Find("\n", PR_FALSE, selOffset + selLength, -1);
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

    
    nsQueryContentEvent querySelectedTextEvent(PR_TRUE,
                                               NS_QUERY_SELECTED_TEXT,
                                               mLastFocusedWindow);
    mLastFocusedWindow->DispatchEvent(&querySelectedTextEvent, status);
    NS_ENSURE_TRUE(querySelectedTextEvent.mSucceeded, NS_ERROR_FAILURE);

    
    nsSelectionEvent selectionEvent(PR_TRUE, NS_SELECTION_SET,
                                    mLastFocusedWindow);
    selectionEvent.mOffset = querySelectedTextEvent.mReply.mOffset + aOffset;
    selectionEvent.mLength = aNChars;
    selectionEvent.mReversed = PR_FALSE;
    selectionEvent.mExpandToClusterBoundary = PR_FALSE;
    mLastFocusedWindow->DispatchEvent(&selectionEvent, status);
    NS_ENSURE_TRUE(selectionEvent.mSucceeded, NS_ERROR_FAILURE);

    
    nsContentCommandEvent contentCommandEvent(PR_TRUE,
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

PRBool
nsGtkIMModule::ShouldIgnoreNativeCompositionEvent()
{
    PR_LOG(gGtkIMLog, PR_LOG_ALWAYS,
        ("GtkIMModule(%p): ShouldIgnoreNativeCompositionEvent, mLastFocusedWindow=%p, mIgnoreNativeCompositionEvent=%s",
         this, mLastFocusedWindow,
         mIgnoreNativeCompositionEvent ? "YES" : "NO"));

    if (!mLastFocusedWindow) {
        return PR_TRUE; 
    }

    return mIgnoreNativeCompositionEvent;
}
