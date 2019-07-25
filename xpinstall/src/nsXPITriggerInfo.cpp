







































#include "jscntxt.h"
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
#include "nsIX509Cert.h"





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
        nsCOMPtr<nsISupports> certificate;
        aPrincipal->GetCertificate(getter_AddRefs(certificate));

        nsCOMPtr<nsIX509Cert> x509 = do_QueryInterface(certificate);
        if (x509) {
            x509->GetCommonName(mCertName);
            if (mCertName.Length() > 0)
                return;
        }

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
        delete item;
    }
    mItems.Clear();

    if ( mCx && !JSVAL_IS_NULL(mCbval) ) {
        JS_BeginRequest(mCx);
        JS_RemoveValueRoot(mCx, &mCbval );
        JS_EndRequest(mCx);
    }

    MOZ_COUNT_DTOR(nsXPITriggerInfo);
}

void nsXPITriggerInfo::SaveCallback( JSContext *aCx, jsval aVal )
{
    NS_ASSERTION( mCx == 0, "callback set twice, memory leak" );
    
    
    if (!(JS_GetOptions(aCx) & JSOPTION_PRIVATE_IS_NSISUPPORTS))
        return;
    mContextWrapper = static_cast<nsISupports *>(JS_GetContextPrivate(aCx));
    if (!mContextWrapper)
        return;

    mCx = aCx;
    mCbval = aVal;
    mThread = do_GetCurrentThread();

    if ( !JSVAL_IS_NULL(mCbval) ) {
        JS_BeginRequest(mCx);
        JS_AddValueRoot(mCx, &mCbval );
        JS_EndRequest(mCx);
    }
}

XPITriggerEvent::~XPITriggerEvent()
{
    JS_BeginRequest(cx);
    JS_RemoveValueRoot(cx, &cbval);
    JS_EndRequest(cx);
}

NS_IMETHODIMP
XPITriggerEvent::Run()
{
    JSAutoRequest ar(cx);

    
    
    
    JSObject* innerGlobal = JS_GetGlobalForObject(cx, JSVAL_TO_OBJECT(cbval));
    jsval components;
    if (!JS_LookupProperty(cx, innerGlobal, "Components", &components) ||
        !JSVAL_IS_OBJECT(components))
        return 0;

    
    jsval args[2] = { JSVAL_NULL, JSVAL_NULL };
    js::AutoArrayRooter tvr(cx, JS_ARRAY_LENGTH(args), args);

    
    JSString *str = JS_NewUCStringCopyZ(cx, reinterpret_cast<const jschar*>(URL.get()));
    if (!str)
        return 0;
    args[0] = STRING_TO_JSVAL(str);

    
    if (!JS_NewNumberValue(cx, status, &args[1]))
        return 0;

    class StackPushGuard {
        nsCOMPtr<nsIJSContextStack> mStack;
    public:
        StackPushGuard(JSContext *cx)
          : mStack(do_GetService("@mozilla.org/js/xpc/ContextStack;1"))
        {
            if (mStack)
                mStack->Push(cx);
        }

        ~StackPushGuard()
        {
            if (mStack)
                mStack->Pop(nsnull);
        }
    } stackPushGuard(cx);

    nsCOMPtr<nsIScriptSecurityManager> secman =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
    if (!secman)
    {
        JS_ReportError(cx, "Could not get script security manager service");
        return 0;
    }

    nsCOMPtr<nsIPrincipal> principal;
    nsresult rv = secman->GetSubjectPrincipal(getter_AddRefs(principal));

    if (NS_FAILED(rv) || !principal)
    {
         JS_ReportError(cx, "Could not get principal from script security manager");
         return 0;
    }

    PRBool equals = PR_FALSE;
    principal->Equals(princ, &equals);
    if (!equals)
    {
        JS_ReportError(cx, "Principal of callback context is different than InstallTriggers");
        return 0;
    }

    jsval ret;
    JS_CallFunctionValue(cx,
                         JS_GetGlobalObject(cx),
                         cbval,
                         2,
                         args,
                         &ret);
    return 0;
}


void nsXPITriggerInfo::SendStatus(const PRUnichar* URL, PRInt32 status)
{
    nsresult rv;

    if ( mCx && mContextWrapper && !JSVAL_IS_NULL(mCbval) )
    {
        
        nsRefPtr<XPITriggerEvent> event = new XPITriggerEvent();
        if (event)
        {
            event->URL      = URL;
            event->status   = status;
            event->cx       = mCx;
            event->princ    = mPrincipal;

            event->cbval    = mCbval;
            JS_BeginRequest(event->cx);
            JS_AddNamedValueRoot(event->cx, &event->cbval,
                            "XPITriggerEvent::cbval" );
            JS_EndRequest(event->cx);

            
            
            event->ref      = mContextWrapper;

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
