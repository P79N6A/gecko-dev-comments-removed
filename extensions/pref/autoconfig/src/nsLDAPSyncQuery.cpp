






































#if defined(MOZ_LDAP_XPCOM)

#include "nsLDAPSyncQuery.h"
#include "nsIServiceManager.h"
#include "nsIProxyObjectManager.h"
#include "nsXPIDLString.h"
#include "nsILDAPErrors.h"
#include "nsThreadUtils.h"
#include "nsReadableUtils.h"
#include "nsILDAPMessage.h"



NS_IMPL_THREADSAFE_ISUPPORTS2(nsLDAPSyncQuery, nsILDAPSyncQuery, nsILDAPMessageListener)



nsLDAPSyncQuery::nsLDAPSyncQuery() :
    mFinished(PR_FALSE), 
    mAttrCount(0), mAttrs(0), mProtocolVersion(nsILDAPConnection::VERSION3)
{
}



nsLDAPSyncQuery::~nsLDAPSyncQuery()
{
}





NS_IMETHODIMP 
nsLDAPSyncQuery::OnLDAPMessage(nsILDAPMessage *aMessage)
{
    PRInt32 messageType;

    
    
    if (!aMessage) {
        return NS_OK;
    }

    
    
    nsresult rv = aMessage->GetType(&messageType);
    if (NS_FAILED(rv)) {
        NS_ERROR("nsLDAPSyncQuery::OnLDAPMessage(): unexpected "
                 "error in aMessage->GetType()");
        FinishLDAPQuery();
        return NS_ERROR_UNEXPECTED;
    }

    switch (messageType) {

    case nsILDAPMessage::RES_BIND:

        
        
        return OnLDAPBind(aMessage);

    case nsILDAPMessage::RES_SEARCH_ENTRY:
        
        
        
        return OnLDAPSearchEntry(aMessage);

    case nsILDAPMessage::RES_SEARCH_RESULT:

        
        
        return OnLDAPSearchResult(aMessage);

    default:
        
        
        
        
        
        
        
        NS_ERROR("nsLDAPSyncQuery::OnLDAPMessage(): unexpected "
                 "LDAP message received");
        return NS_OK;
    }
}



NS_IMETHODIMP
nsLDAPSyncQuery::OnLDAPInit(nsILDAPConnection *aConn, nsresult aStatus)
{
    nsresult rv;        
    nsCOMPtr<nsILDAPMessageListener> selfProxy;

    
    
    mOperation = do_CreateInstance("@mozilla.org/network/ldap-operation;1", 
                                   &rv);
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    
    
    rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                              NS_GET_IID(nsILDAPMessageListener), 
                              static_cast<nsILDAPMessageListener *>(this),
                              NS_PROXY_ASYNC | NS_PROXY_ALWAYS, 
                              getter_AddRefs(selfProxy));
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    
    
    rv = mOperation->Init(mConnection, selfProxy, nsnull);
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_UNEXPECTED; 
    }

    
    
    rv = mOperation->SimpleBind(EmptyCString()); 
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }
    
    return NS_OK;
}

nsresult
nsLDAPSyncQuery::OnLDAPBind(nsILDAPMessage *aMessage)
{

    PRInt32 errCode;

    mOperation = 0;  

    
    
    nsresult rv = aMessage->GetErrorCode(&errCode);
    if (NS_FAILED(rv)) {
        
        NS_ERROR("nsLDAPSyncQuery::OnLDAPBind(): couldn't get "
                 "error code from aMessage");
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }


    
    
    if (errCode != nsILDAPErrors::SUCCESS) {
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    
    
    return StartLDAPSearch();
}

nsresult
nsLDAPSyncQuery::OnLDAPSearchEntry(nsILDAPMessage *aMessage)
{
    nsresult rv;       

    
    
    
    for (PRUint32 i = 0; i < mAttrCount; i++) {

        PRUnichar **vals;
        PRUint32 valueCount;

        
        
        
        rv = aMessage->GetValues(mAttrs[i], &valueCount, &vals);
        if (NS_FAILED(rv)) {
            NS_WARNING("nsLDAPSyncQuery:OnLDAPSearchEntry(): "
                       "aMessage->GetValues() failed\n");
            FinishLDAPQuery();
            return rv;;
        }

        
        
        for (PRUint32 j = 0; j < valueCount; j++) {
            mResults.Append(PRUnichar('\n'));
            mResults.AppendASCII(mAttrs[i]);
            mResults.Append(PRUnichar('='));
            mResults.Append(vals[j]);
        }
        
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(valueCount, vals);

    }

    return NS_OK;
}


nsresult
nsLDAPSyncQuery::OnLDAPSearchResult(nsILDAPMessage *aMessage)
{
    
    
    
    FinishLDAPQuery();
    
    
    
    if (mAttrCount > 0)
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(mAttrCount, mAttrs);

    return NS_OK;
}

nsresult
nsLDAPSyncQuery::StartLDAPSearch()
{
    nsresult rv; 
    nsCOMPtr<nsILDAPMessageListener> selfProxy; 


    
    
    mOperation = 
        do_CreateInstance("@mozilla.org/network/ldap-operation;1", &rv);

    if (NS_FAILED(rv)) {
        NS_ERROR("nsLDAPSyncQuery::StartLDAPSearch(): couldn't "
                 "create @mozilla.org/network/ldap-operation;1");
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    
    
    rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD, 
                              NS_GET_IID(nsILDAPMessageListener),
                              static_cast<nsILDAPMessageListener *>(this),
                              NS_PROXY_ASYNC | NS_PROXY_ALWAYS,
                              getter_AddRefs(selfProxy));
    if (NS_FAILED(rv)) {
        NS_ERROR("nsLDAPSyncQuery::StartLDAPSearch(): couldn't "
                 "create proxy to this object for callback");
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    
    
    rv = mOperation->Init(mConnection, selfProxy, nsnull);
    if (NS_FAILED(rv)) {
        NS_ERROR("nsLDAPSyncQuery::StartLDAPSearch(): couldn't "
                 "initialize LDAP operation");
        FinishLDAPQuery();
        return NS_ERROR_UNEXPECTED;
    }

    
    
    nsCAutoString urlFilter;
    rv = mServerURL->GetFilter(urlFilter);
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_UNEXPECTED;
    }

    
    
    nsCAutoString dn;
    rv = mServerURL->GetDn(dn);
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_UNEXPECTED;
    }

    
    
    PRInt32 scope;
    rv = mServerURL->GetScope(&scope);
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_UNEXPECTED;
    }

    
    rv = mServerURL->GetAttributes(&mAttrCount, &mAttrs);
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_UNEXPECTED;
    }


    
    
    rv = mOperation->SearchExt(dn, scope, urlFilter, mAttrCount,
                               const_cast<const char **>(mAttrs), 0, 0);

    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}



nsresult nsLDAPSyncQuery::InitConnection()
{
    nsCOMPtr<nsILDAPMessageListener> selfProxy;
    nsresult rv;        
    
    
    
    mConnection = do_CreateInstance("@mozilla.org/network/ldap-connection;1",
                                    &rv);
    if (NS_FAILED(rv)) {
        NS_ERROR("nsLDAPSyncQuery::InitConnection(): could "
                 "not create @mozilla.org/network/ldap-connection;1");
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    
    
    if (!mServerURL) {
        NS_ERROR("nsLDAPSyncQuery::InitConnection(): mServerURL "
                 "is NULL");
        FinishLDAPQuery();
        return NS_ERROR_NOT_INITIALIZED;
    }

    
    
    nsCAutoString host;
    rv = mServerURL->GetAsciiHost(host);
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    
    
    PRInt32 port;
    rv = mServerURL->GetPort(&port);
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }
        
    PRUint32 options;
    rv = mServerURL->GetOptions(&options);
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    
    
    rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                              NS_GET_IID(nsILDAPMessageListener), 
                              static_cast<nsILDAPMessageListener *>(this), 
                              NS_PROXY_ASYNC | NS_PROXY_ALWAYS, 
                              getter_AddRefs(selfProxy));
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        NS_ERROR("nsLDAPSyncQuery::InitConnection(): couldn't "
                 "create proxy to this object for callback");
        return NS_ERROR_FAILURE;
    }

    rv = mConnection->Init(host.get(), port, 
                           (options & nsILDAPURL::OPT_SECURE) 
                           ? PR_TRUE : PR_FALSE, EmptyCString(), selfProxy,
                           nsnull, mProtocolVersion);
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_UNEXPECTED; 
    }

    return NS_OK;
}

void
nsLDAPSyncQuery::FinishLDAPQuery()
{
    
    
    
    mFinished = PR_TRUE;
    
    
    
    mConnection = 0;
    mOperation = 0;
    mServerURL = 0;
 
}


NS_IMETHODIMP nsLDAPSyncQuery::GetQueryResults(nsILDAPURL *aServerURL,
                                               PRUint32 aProtocolVersion,
                                               PRUnichar **_retval)
{
    nsresult rv;
    
    if (!aServerURL) {
        NS_ERROR("nsLDAPSyncQuery::GetQueryResults() called without LDAP URL");
        return NS_ERROR_FAILURE;
    }
    mServerURL = aServerURL;
    mProtocolVersion = aProtocolVersion;

    nsCOMPtr<nsIThread> currentThread = do_GetCurrentThread();

    
    
    
    
    
    rv = InitConnection();
    if (NS_FAILED(rv))
        return rv;
    
    
    
    
    
    
    
    
    
    
    
    while (!mFinished)
        NS_ENSURE_STATE(NS_ProcessNextEvent(currentThread));

    
    
    if (!mResults.IsEmpty()) {
        *_retval = ToNewUnicode(mResults);
        if (!_retval)
          rv = NS_ERROR_OUT_OF_MEMORY;
    }
    return rv;

}
#endif
