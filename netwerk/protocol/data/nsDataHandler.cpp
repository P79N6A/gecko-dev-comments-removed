




































#include "nspr.h"
#include "nsDataChannel.h"
#include "nsDataHandler.h"
#include "nsIURL.h"
#include "nsCRT.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIProgressEventSink.h"
#include "nsNetCID.h"
#include "nsNetError.h"

static NS_DEFINE_CID(kSimpleURICID, NS_SIMPLEURI_CID);



nsDataHandler::nsDataHandler() {
}

nsDataHandler::~nsDataHandler() {
}

NS_IMPL_ISUPPORTS1(nsDataHandler, nsIProtocolHandler)

nsresult
nsDataHandler::Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult) {

    nsDataHandler* ph = new nsDataHandler();
    if (ph == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(ph);
    nsresult rv = ph->QueryInterface(aIID, aResult);
    NS_RELEASE(ph);
    return rv;
}
    



NS_IMETHODIMP
nsDataHandler::GetScheme(nsACString &result) {
    result.AssignLiteral("data");
    return NS_OK;
}

NS_IMETHODIMP
nsDataHandler::GetDefaultPort(PRInt32 *result) {
    
    *result = -1;
    return NS_OK;
}

NS_IMETHODIMP
nsDataHandler::GetProtocolFlags(PRUint32 *result) {
    *result = URI_NORELATIVE | URI_NOAUTH | URI_INHERITS_SECURITY_CONTEXT |
        URI_LOADABLE_BY_ANYONE | URI_NON_PERSISTABLE | URI_IS_LOCAL_RESOURCE;
    return NS_OK;
}

NS_IMETHODIMP
nsDataHandler::NewURI(const nsACString &aSpec,
                      const char *aCharset, 
                      nsIURI *aBaseURI,
                      nsIURI **result) {
    nsresult rv;
    nsRefPtr<nsIURI> uri;

    nsCString spec(aSpec);

    if (aBaseURI && !spec.IsEmpty() && spec[0] == '#') {
        
        
        rv = aBaseURI->Clone(getter_AddRefs(uri));
        if (NS_FAILED(rv))
            return rv;
        rv = uri->SetRef(spec);
    } else {
        
        nsCAutoString contentType, contentCharset, dataBuffer, hashRef;
        PRBool base64;
        rv = ParseURI(spec, contentType, contentCharset, base64, dataBuffer, hashRef);
        if (NS_FAILED(rv))
            return rv;

        
        
        if (base64 || (strncmp(contentType.get(),"text/",5) != 0 &&
                       contentType.Find("xml") == kNotFound)) {
            
            spec.StripWhitespace();
        }

        uri = do_CreateInstance(kSimpleURICID, &rv);
        if (NS_FAILED(rv))
            return rv;
        rv = uri->SetSpec(spec);
    }

    if (NS_FAILED(rv))
        return rv;

    uri.forget(result);
    return rv;
}

NS_IMETHODIMP
nsDataHandler::NewChannel(nsIURI* uri, nsIChannel* *result) {
    NS_ENSURE_ARG_POINTER(uri);
    nsDataChannel* channel = new nsDataChannel(uri);
    if (!channel)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(channel);

    nsresult rv = channel->Init();
    if (NS_FAILED(rv)) {
        NS_RELEASE(channel);
        return rv;
    }

    *result = channel;
    return NS_OK;
}

NS_IMETHODIMP 
nsDataHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval) {
    
    *_retval = PR_FALSE;
    return NS_OK;
}

nsresult
nsDataHandler::ParseURI(nsCString& spec,
                        nsCString& contentType,
                        nsCString& contentCharset,
                        PRBool&    isBase64,
                        nsCString& dataBuffer,
                        nsCString& hashRef) {
    isBase64 = PR_FALSE;

    
    char *buffer = (char *) PL_strcasestr(spec.BeginWriting(), "data:");
    if (!buffer) {
        
        return NS_ERROR_MALFORMED_URI;
    }
    buffer += 5;

    
    char *comma = strchr(buffer, ',');
    if (!comma)
        return NS_ERROR_MALFORMED_URI;

    *comma = '\0';

    
    char *base64 = PL_strcasestr(buffer, ";base64");
    if (base64) {
        isBase64 = PR_TRUE;
        *base64 = '\0';
    }

    if (comma == buffer) {
        
        contentType.AssignLiteral("text/plain");
        contentCharset.AssignLiteral("US-ASCII");
    } else {
        
        char *semiColon = (char *) strchr(buffer, ';');
        if (semiColon)
            *semiColon = '\0';
        
        if (semiColon == buffer || base64 == buffer) {
            
            contentType.AssignLiteral("text/plain");
        } else {
            contentType = buffer;
            ToLowerCase(contentType);
        }

        if (semiColon) {
            char *charset = PL_strcasestr(semiColon + 1, "charset=");
            if (charset)
                contentCharset = charset + sizeof("charset=") - 1;

            *semiColon = ';';
        }
    }

    *comma = ',';
    if (isBase64)
        *base64 = ';';

    contentType.StripWhitespace();
    contentCharset.StripWhitespace();

    
    char *data = comma + 1;
    char *hash = strchr(data, '#');
    if (!hash) {
        dataBuffer.Assign(data);
        hashRef.Truncate();
    } else {
        dataBuffer.Assign(data, hash - data);
        hashRef.Assign(hash);
    }

    return NS_OK;
}
