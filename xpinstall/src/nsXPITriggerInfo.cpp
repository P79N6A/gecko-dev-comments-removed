







































#include "nscore.h"
#include "plstr.h"
#include "nsXPITriggerInfo.h"
#include "nsNetUtil.h"
#include "nsDebug.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "nsIServiceManager.h"
#include "nsIJSContextStack.h"
#include "nsIScriptSecurityManager.h"
#include "nsICryptoHash.h"





nsXPITriggerItem::nsXPITriggerItem( const PRUnichar* aName,
                                    const PRUnichar* aURL,
                                    const PRUnichar* aIconURL,
                                    const char* aHash,
                                    PRInt32 aFlags)
    : mName(aName), mURL(aURL), mIconURL(aIconURL), mHashFound(PR_FALSE), mFlags(aFlags)
{
    MOZ_COUNT_CTOR(nsXPITriggerItem);

    
    PRInt32 qmark = mURL.FindChar('?');
    if ( qmark != kNotFound )
    {
        mArguments = Substring( mURL, qmark+1, mURL.Length() );
    }

    
    if ( mName.IsEmpty() )
    {
        
        
        
        

        PRInt32 namestart = mURL.RFindChar( '/', qmark );

        
        namestart = ( namestart==kNotFound ) ? 0 : namestart + 1;

        PRInt32 length;
        if (qmark == kNotFound)
            length =  mURL.Length();      
        else
            length = (qmark - namestart); 

        mName = Substring( mURL, namestart, length );
    }

    
    if (aHash)
    {
        mHashFound = PR_TRUE;

        char * colon = PL_strchr(aHash, ':');
        if (colon)
        {
            mHasher = do_CreateInstance("@mozilla.org/security/hash;1");
            if (!mHasher) return;

            *colon = '\0'; 
            nsresult rv = mHasher->InitWithString(nsDependentCString(aHash));
            *colon = ':';  

            if (NS_SUCCEEDED(rv))
                mHash = colon+1;
        }
    }
}

nsXPITriggerItem::~nsXPITriggerItem()
{
    MOZ_COUNT_DTOR(nsXPITriggerItem);
}

PRBool nsXPITriggerItem::IsRelativeURL()
{
    PRInt32 cpos = mURL.FindChar(':');
    if (cpos == kNotFound)
        return PR_TRUE;

    PRInt32 spos = mURL.FindChar('/');
    return (cpos > spos);
}

const PRUnichar*
nsXPITriggerItem::GetSafeURLString()
{
    
    if (mSafeURL.IsEmpty() && !mURL.IsEmpty())
    {
        nsCOMPtr<nsIURI> uri;
        NS_NewURI(getter_AddRefs(uri), mURL);
        if (uri)
        {
            nsCAutoString spec;
            uri->SetUserPass(EmptyCString());
            uri->GetSpec(spec);
            mSafeURL = NS_ConvertUTF8toUTF16(spec);
        }
    }

    return mSafeURL.get();
}

void
nsXPITriggerItem::SetPrincipal(nsIPrincipal* aPrincipal)
{
    mPrincipal = aPrincipal;

    
    
    
    
    if (!aPrincipal)
        return;

    PRBool hasCert;
    aPrincipal->GetHasCertificate(&hasCert);
    if (hasCert) {
        nsCAutoString prettyName;
        
        
        
        aPrincipal->GetPrettyName(prettyName);
        CopyUTF8toUTF16(prettyName, mCertName);
    }
}





nsXPITriggerInfo::nsXPITriggerInfo()
  : mCx(0), mCbval(JSVAL_NULL)
{
    MOZ_COUNT_CTOR(nsXPITriggerInfo);
}

nsXPITriggerInfo::~nsXPITriggerInfo()
{
    nsXPITriggerItem* item;

    for(PRUint32 i=0; i < Size(); i++)
    {
        item = Get(i);
        if (item)
            delete item;
    }
    mItems.Clear();

    if ( mCx && !JSVAL_IS_NULL(mCbval) ) {
        JS_BeginRequest(mCx);
        JS_RemoveRoot( mCx, &mCbval );
        JS_EndRequest(mCx);
    }

    MOZ_COUNT_DTOR(nsXPITriggerInfo);
}

void nsXPITriggerInfo::SaveCallback( JSContext *aCx, jsval aVal )
{
    NS_ASSERTION( mCx == 0, "callback set twice, memory leak" );
    mCx = aCx;
    JSObject *obj = JS_GetGlobalObject( mCx );

    JSClass* clazz;

#ifdef JS_THREADSAFE
    clazz = ::JS_GetClass(aCx, obj);
#else
    clazz = ::JS_GetClass(obj);
#endif

    if (clazz &&
        (clazz->flags & JSCLASS_HAS_PRIVATE) &&
        (clazz->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS)) {
      mGlobalWrapper =
        do_QueryInterface((nsISupports*)::JS_GetPrivate(aCx, obj));
    }

    mCbval = aVal;
    mThread = do_GetCurrentThread();

    if ( !JSVAL_IS_NULL(mCbval) ) {
        JS_BeginRequest(mCx);
        JS_AddRoot( mCx, &mCbval );
        JS_EndRequest(mCx);
    }
}

XPITriggerEvent::~XPITriggerEvent()
{
    JS_BeginRequest(cx);
    JS_RemoveRoot(cx, &cbval);
    JS_EndRequest(cx);
}

NS_IMETHODIMP
XPITriggerEvent::Run()
{
    jsval  ret;
    void*  mark;
    jsval* args;

    JS_BeginRequest(cx);
    args = JS_PushArguments(cx, &mark, "Wi",
                            URL.get(),
                            status);
    if ( args )
    {
        
        
        
        

        const char *errorStr = nsnull;

        nsCOMPtr<nsIJSContextStack> stack =
            do_GetService("@mozilla.org/js/xpc/ContextStack;1");
        if (stack)
            stack->Push(cx);

        nsCOMPtr<nsIScriptSecurityManager> secman =
            do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);

        if (!secman)
        {
            errorStr = "Could not get script security manager service";
        }

        nsCOMPtr<nsIPrincipal> principal;
        if (!errorStr)
        {
            secman->GetSubjectPrincipal(getter_AddRefs(principal));
            if (!principal)
            {
                errorStr = "Could not get principal from script security manager";
            }
        }

        if (!errorStr)
        {
            PRBool equals = PR_FALSE;
            principal->Equals(princ, &equals);

            if (!equals)
            {
                errorStr = "Principal of callback context is different than InstallTriggers";
            }
        }

        if (errorStr)
        {
            JS_ReportError(cx, errorStr);
        }
        else
        {
            JS_CallFunctionValue(cx,
                                 JSVAL_TO_OBJECT(global),
                                 cbval,
                                 2,
                                 args,
                                 &ret);
        }

        if (stack)
            stack->Pop(nsnull);

        JS_PopArguments(cx, mark);
    }
    JS_EndRequest(cx);

    return 0;
}


void nsXPITriggerInfo::SendStatus(const PRUnichar* URL, PRInt32 status)
{
    nsresult rv;

    if ( mCx && mGlobalWrapper && !JSVAL_IS_NULL(mCbval) )
    {
        
        nsRefPtr<XPITriggerEvent> event = new XPITriggerEvent();
        if (event)
        {
            event->URL      = URL;
            event->status   = status;
            event->cx       = mCx;
            event->princ    = mPrincipal;

            JSObject *obj = nsnull;

            mGlobalWrapper->GetJSObject(&obj);

            event->global   = OBJECT_TO_JSVAL(obj);

            event->cbval    = mCbval;
            JS_BeginRequest(event->cx);
            JS_AddNamedRoot(event->cx, &event->cbval,
                            "XPITriggerEvent::cbval" );
            JS_EndRequest(event->cx);

            
            
            event->ref      = mGlobalWrapper;

            rv = mThread->Dispatch(event, NS_DISPATCH_NORMAL);
        }
        else
            rv = NS_ERROR_OUT_OF_MEMORY;

        if ( NS_FAILED( rv ) )
        {
            
            
            NS_WARNING("failed to dispatch XPITriggerEvent");
        }
    }
}
