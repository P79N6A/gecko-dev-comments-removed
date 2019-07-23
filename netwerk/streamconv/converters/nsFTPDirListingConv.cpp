





































#include "nsFTPDirListingConv.h"
#include "nsMemory.h"
#include "plstr.h"
#include "prlog.h"
#include "nsIServiceManager.h"
#include "nsIGenericFactory.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsCOMPtr.h"
#include "nsEscape.h"
#include "nsNetUtil.h"
#include "nsStringStream.h"
#include "nsIComponentManager.h"
#include "nsDateTimeFormatCID.h"
#include "nsIStreamListener.h"
#include "nsCRT.h"
#include "nsMimeTypes.h"
#include "nsAutoPtr.h"

#include "ParseFTPList.h"

#if defined(PR_LOGGING)











PRLogModuleInfo* gFTPDirListConvLog = nsnull;

#endif 


NS_IMPL_ISUPPORTS3(nsFTPDirListingConv,
                   nsIStreamConverter,
                   nsIStreamListener, 
                   nsIRequestObserver)



NS_IMETHODIMP
nsFTPDirListingConv::Convert(nsIInputStream *aFromStream,
                             const char *aFromType,
                             const char *aToType,
                             nsISupports *aCtxt, nsIInputStream **_retval) {
    return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
nsFTPDirListingConv::AsyncConvertData(const char *aFromType, const char *aToType,
                                      nsIStreamListener *aListener, nsISupports *aCtxt) {
    NS_ASSERTION(aListener && aFromType && aToType, "null pointer passed into FTP dir listing converter");

    
    
    mFinalListener = aListener;
    NS_ADDREF(mFinalListener);

    PR_LOG(gFTPDirListConvLog, PR_LOG_DEBUG, 
        ("nsFTPDirListingConv::AsyncConvertData() converting FROM raw, TO application/http-index-format\n"));

    return NS_OK;
}



NS_IMETHODIMP
nsFTPDirListingConv::OnDataAvailable(nsIRequest* request, nsISupports *ctxt,
                                  nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count) {
    NS_ASSERTION(request, "FTP dir listing stream converter needs a request");
    
    nsresult rv;

    nsCOMPtr<nsIChannel> channel = do_QueryInterface(request, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    PRUint32 read, streamLen;

    rv = inStr->Available(&streamLen);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoArrayPtr<char> buffer(new char[streamLen + 1]);
    NS_ENSURE_TRUE(buffer, NS_ERROR_OUT_OF_MEMORY);

    rv = inStr->Read(buffer, streamLen, &read);
    NS_ENSURE_SUCCESS(rv, rv);

    
    buffer[streamLen] = '\0';

    PR_LOG(gFTPDirListConvLog, PR_LOG_DEBUG, ("nsFTPDirListingConv::OnData(request = %x, ctxt = %x, inStr = %x, sourceOffset = %d, count = %d)\n", request, ctxt, inStr, sourceOffset, count));

    if (!mBuffer.IsEmpty()) {
        
        
        mBuffer.Append(buffer);

        buffer = new char[mBuffer.Length()+1];
        NS_ENSURE_TRUE(buffer, NS_ERROR_OUT_OF_MEMORY);

        strncpy(buffer, mBuffer.get(), mBuffer.Length()+1);
        mBuffer.Truncate();
    }

#ifndef DEBUG_dougt
    PR_LOG(gFTPDirListConvLog, PR_LOG_DEBUG, ("::OnData() received the following %d bytes...\n\n%s\n\n", streamLen, buffer.get()) );
#else
    printf("::OnData() received the following %d bytes...\n\n%s\n\n", streamLen, buffer);
#endif 

    nsCAutoString indexFormat;
    if (!mSentHeading) {
        
        nsCOMPtr<nsIURI> uri;
        rv = channel->GetURI(getter_AddRefs(uri));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = GetHeaders(indexFormat, uri);
        NS_ENSURE_SUCCESS(rv, rv);

        mSentHeading = PR_TRUE;
    }

    char *line = buffer;
    line = DigestBufferLines(line, indexFormat);

#ifndef DEBUG_dougt
    PR_LOG(gFTPDirListConvLog, PR_LOG_DEBUG, ("::OnData() sending the following %d bytes...\n\n%s\n\n", 
        indexFormat.Length(), indexFormat.get()) );
#else
    char *unescData = ToNewCString(indexFormat);
    NS_ENSURE_TRUE(unescData, NS_ERROR_OUT_OF_MEMORY);
    
    nsUnescape(unescData);
    printf("::OnData() sending the following %d bytes...\n\n%s\n\n", indexFormat.Length(), unescData);
    nsMemory::Free(unescData);
#endif 

    
    if (line && *line) {
        mBuffer.Append(line);
        PR_LOG(gFTPDirListConvLog, PR_LOG_DEBUG, ("::OnData() buffering the following %d bytes...\n\n%s\n\n",
            PL_strlen(line), line) );
    }

    
    nsCOMPtr<nsIInputStream> inputData;

    rv = NS_NewCStringInputStream(getter_AddRefs(inputData), indexFormat);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mFinalListener->OnDataAvailable(request, ctxt, inputData, 0, indexFormat.Length());

    return rv;
}



NS_IMETHODIMP
nsFTPDirListingConv::OnStartRequest(nsIRequest* request, nsISupports *ctxt) {
    
    
    return mFinalListener->OnStartRequest(request, ctxt);
}

NS_IMETHODIMP
nsFTPDirListingConv::OnStopRequest(nsIRequest* request, nsISupports *ctxt,
                                   nsresult aStatus) {
    

    return mFinalListener->OnStopRequest(request, ctxt, aStatus);
}



nsFTPDirListingConv::nsFTPDirListingConv() {
    mFinalListener      = nsnull;
    mSentHeading        = PR_FALSE;
}

nsFTPDirListingConv::~nsFTPDirListingConv() {
    NS_IF_RELEASE(mFinalListener);
}

nsresult
nsFTPDirListingConv::Init() {
#if defined(PR_LOGGING)
    
    
    
    
    if (nsnull == gFTPDirListConvLog) {
        gFTPDirListConvLog = PR_NewLogModule("nsFTPDirListingConv");
    }
#endif 

    return NS_OK;
}

nsresult
nsFTPDirListingConv::GetHeaders(nsACString& headers,
                                nsIURI* uri)
{
    nsresult rv = NS_OK;
    
    headers.AppendLiteral("300: ");

    
    nsCAutoString pw;
    nsCAutoString spec;
    uri->GetPassword(pw);
    if (!pw.IsEmpty()) {
         rv = uri->SetPassword(EmptyCString());
         if (NS_FAILED(rv)) return rv;
         rv = uri->GetAsciiSpec(spec);
         if (NS_FAILED(rv)) return rv;
         headers.Append(spec);
         rv = uri->SetPassword(pw);
         if (NS_FAILED(rv)) return rv;
    } else {
        rv = uri->GetAsciiSpec(spec);
        if (NS_FAILED(rv)) return rv;
        
        headers.Append(spec);
    }
    headers.Append(char(nsCRT::LF));
    

    
    headers.AppendLiteral("200: filename content-length last-modified file-type\n");
    
    return rv;
}

char *
nsFTPDirListingConv::DigestBufferLines(char *aBuffer, nsCString &aString) {
    char *line = aBuffer;
    char *eol;
    PRBool cr = PR_FALSE;

    
    while ( line && (eol = PL_strchr(line, nsCRT::LF)) ) {
        
        if (eol > line && *(eol-1) == nsCRT::CR) {
            eol--;
            *eol = '\0';
            cr = PR_TRUE;
        } else {
            *eol = '\0';
            cr = PR_FALSE;
        }

        list_state state;
        list_result result;

        int type = ParseFTPList(line, &state, &result );

        
        
        if ((type != 'd' && type != 'f' && type != 'l') || 
            (result.fe_type == 'd' && result.fe_fname[0] == '.' &&
            (result.fe_fnlen == 1 || (result.fe_fnlen == 2 &&  result.fe_fname[1] == '.'))) )
        {
            if (cr)
                line = eol+2;
            else
                line = eol+1;
            
            continue;
        }

        
        aString.AppendLiteral("201: ");
        

        
	if (state.lstyle != 'U' && state.lstyle != 'W') {
            const char* offset = strstr(result.fe_fname, " -> ");
            if (offset) {
                result.fe_fnlen = offset - result.fe_fname;
            }
        }

        nsCAutoString buf;
        aString.Append('\"');
        aString.Append(NS_EscapeURL(Substring(result.fe_fname, 
                                              result.fe_fname+result.fe_fnlen),
                                    esc_Minimal|esc_OnlyASCII|esc_Forced,buf));
        aString.AppendLiteral("\" ");
 
        
        
        if (type != 'd') 
        {
            for (int i = 0; i < int(sizeof(result.fe_size)); ++i)
            {
                if (result.fe_size[i] != '\0')
                    aString.Append((const char*)&result.fe_size[i], 1);
            }
            
            aString.Append(' ');
        }
        else
            aString.AppendLiteral("0 ");


        
        char buffer[256] = "";
        
        
        
        
        PR_FormatTimeUSEnglish(buffer, sizeof(buffer),
                               "%a, %d %b %Y %H:%M:%S", &result.fe_time );

        char *escapedDate = nsEscape(buffer, url_Path);
        aString.Append(escapedDate);
        nsMemory::Free(escapedDate);
        aString.Append(' ');

        
        if (type == 'd')
            aString.AppendLiteral("DIRECTORY");
        else if (type == 'l')
            aString.AppendLiteral("SYMBOLIC-LINK");
        else
            aString.AppendLiteral("FILE");
        
        aString.Append(' ');

        aString.Append(char(nsCRT::LF)); 
        

        if (cr)
            line = eol+2;
        else
            line = eol+1;
    } 

    return line;
}

nsresult
NS_NewFTPDirListingConv(nsFTPDirListingConv** aFTPDirListingConv)
{
    NS_PRECONDITION(aFTPDirListingConv != nsnull, "null ptr");
    if (! aFTPDirListingConv)
        return NS_ERROR_NULL_POINTER;

    *aFTPDirListingConv = new nsFTPDirListingConv();
    if (! *aFTPDirListingConv)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aFTPDirListingConv);
    return (*aFTPDirListingConv)->Init();
}
