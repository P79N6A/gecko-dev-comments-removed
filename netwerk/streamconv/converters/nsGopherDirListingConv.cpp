








































#include "plstr.h"
#include "nsMemory.h"
#include "nsCRT.h"
#include "nsIServiceManager.h"
#include "nsIGenericFactory.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsEscape.h"
#include "nsIStreamListener.h"
#include "nsIStreamConverter.h"
#include "nsStringStream.h"
#include "nsIRequestObserver.h"
#include "nsNetUtil.h"
#include "nsMimeTypes.h"

#include "nsGopherDirListingConv.h"


NS_IMPL_THREADSAFE_ISUPPORTS3(nsGopherDirListingConv,
                              nsIStreamConverter,
                              nsIStreamListener,
                              nsIRequestObserver)



#define CONV_BUF_SIZE (4*1024)

NS_IMETHODIMP
nsGopherDirListingConv::Convert(nsIInputStream *aFromStream,
                                const char *aFromType,
                                const char *aToType,
                                nsISupports *aCtxt, nsIInputStream **_retval) {
    return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
nsGopherDirListingConv::AsyncConvertData(const char *aFromType,
                                         const char *aToType,
                                         nsIStreamListener *aListener,
                                         nsISupports *aCtxt) {
    NS_ASSERTION(aListener && aFromType && aToType,
                 "null pointer passed into gopher dir listing converter");

    
    
    mFinalListener = aListener;
    
    return NS_OK;
}


NS_IMETHODIMP
nsGopherDirListingConv::OnDataAvailable(nsIRequest *request,
                                        nsISupports *ctxt,
                                        nsIInputStream *inStr,
                                        PRUint32 sourceOffset,
                                        PRUint32 count) {
    nsresult rv;

    PRUint32 read, streamLen;
    nsCAutoString indexFormat;

    rv = inStr->Available(&streamLen);
    if (NS_FAILED(rv)) return rv;

    char *buffer = (char*)nsMemory::Alloc(streamLen + 1);
    if (!buffer)
        return NS_ERROR_OUT_OF_MEMORY;
    rv = inStr->Read(buffer, streamLen, &read);
    if (NS_FAILED(rv))
        return rv;

    
    buffer[streamLen] = '\0';

    if (!mBuffer.IsEmpty()) {
        
        
        mBuffer.Append(buffer);
        nsMemory::Free(buffer);
        buffer = ToNewCString(mBuffer);
        mBuffer.Truncate();
    }

    if (!mSentHeading) {
        nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
        NS_ENSURE_STATE(channel);

        nsCOMPtr<nsIURI> uri;
        channel->GetURI(getter_AddRefs(uri));
        NS_ENSURE_STATE(uri);

        
        nsCAutoString spec;
        rv = uri->GetAsciiSpec(spec);
        if (NS_FAILED(rv))
            return rv;

        
        
        indexFormat.AppendLiteral("300: ");
        indexFormat.Append(spec);
        indexFormat.Append(char(nsCRT::LF));
        

        
        indexFormat.AppendLiteral("200: description filename file-type\n");
        
        
        mSentHeading = PR_TRUE;
    }
    char *line = DigestBufferLines(buffer, indexFormat);
    
    if (line && *line) {
        mBuffer.Append(line);
    }
    
    nsMemory::Free(buffer);
    
    
    nsCOMPtr<nsIInputStream> inputData;
    
    rv = NS_NewCStringInputStream(getter_AddRefs(inputData), indexFormat);
    if (NS_FAILED(rv))
        return rv;
    
    rv = mFinalListener->OnDataAvailable(request, ctxt, inputData, 0,
                                         indexFormat.Length());
    return rv;
}


NS_IMETHODIMP
nsGopherDirListingConv::OnStartRequest(nsIRequest *request, nsISupports *ctxt) {
    
    
    return mFinalListener->OnStartRequest(request, ctxt);
}

NS_IMETHODIMP
nsGopherDirListingConv::OnStopRequest(nsIRequest *request, nsISupports *ctxt,
                                      nsresult aStatus) {
    return mFinalListener->OnStopRequest(request, ctxt, aStatus);
}


nsGopherDirListingConv::nsGopherDirListingConv() {
    mSentHeading = PR_FALSE;
}

char*
nsGopherDirListingConv::DigestBufferLines(char* aBuffer, nsCAutoString& aString) {
    char *line = aBuffer;
    char *eol;
    PRBool cr = PR_FALSE;

    
    while (line && (eol = PL_strchr(line, nsCRT::LF)) ) {
        
        if (eol > line && *(eol-1) == nsCRT::CR) {
            eol--;
            *eol = '\0';
            cr = PR_TRUE;
        } else {
            *eol = '\0';
            cr = PR_FALSE;
        }

        if (line[0]=='.' && line[1]=='\0') {
            if (cr)
                line = eol+2;
            else
                line = eol+1;
            continue;
        }

        char type;
        nsCAutoString desc, selector, host;
        PRInt32 port = GOPHER_PORT;

        type = line[0];
        line++;
        char* tabPos = PL_strchr(line,'\t');

        
        if (tabPos) {
            
            if (tabPos != line) {
                char* descStr = PL_strndup(line,tabPos-line);
                if (!descStr)
                    return nsnull;
                char* escName = nsEscape(descStr,url_Path);
                if (!escName) {
                    PL_strfree(descStr);
                    return nsnull;
                }
                desc = escName;
                NS_Free(escName);
                PL_strfree(descStr);
            } else {
                desc = "%20";
            }
            line = tabPos+1;
            tabPos = PL_strchr(line,'\t');
        }

        
        if (tabPos) {
            char* sel = PL_strndup(line,tabPos-line);
            if (!sel)
                return nsnull;
            char* escName = nsEscape(sel,url_Path);
            if (!escName) {
                PL_strfree(sel);
                return nsnull;
            }
            selector = escName;
            NS_Free(escName);
            PL_strfree(sel);
            line = tabPos+1;
            tabPos = PL_strchr(line,'\t');
        }

        

        if (tabPos) {
            host.Assign(line, tabPos - line);
            line = tabPos+1;
            tabPos = PL_strchr(line,'\t');
            if (tabPos == NULL)
                tabPos = PL_strchr(line,'\0');

            
            nsCAutoString portStr(line, tabPos - line);
            port = atol(portStr.get());
            line = tabPos+1;
        }
        
        
        nsCAutoString filename;
        if (type != '8' && type != 'T') {
            filename.AssignLiteral("gopher://");
            filename.Append(host);
            if (port != GOPHER_PORT) {
                filename.Append(':');
                filename.AppendInt(port);
            }
            filename.Append('/');
            filename.Append(type);
            filename.Append(selector);
        } else {
            
            
            
            if (type == '8')
                
                filename.AssignLiteral("telnet://");
            else
                
                filename.AssignLiteral("tn3270://");
            if (!selector.IsEmpty()) {
                filename.Append(selector);
                filename.Append('@');
            }
            filename.Append(host);
            if (port != 23) { 
                filename.Append(':');
                filename.AppendInt(port);
            }
        }

        if (tabPos) {
            





            if (type != '3' && type != 'i') {
                aString.AppendLiteral("201: ");
                aString.Append(desc);
                aString.Append(' ');
                aString.Append(filename);
                aString.Append(' ');
                if (type == '1')
                    aString.AppendLiteral("DIRECTORY");
                else
                    aString.AppendLiteral("FILE");
                aString.Append(char(nsCRT::LF));
            } else if(type == 'i'){
                aString.AppendLiteral("101: ");
                aString.Append(desc);
                aString.Append(char(nsCRT::LF));
            }
        } else {
            NS_WARNING("Error parsing gopher directory response.\n");
        }
        
        if (cr)
            line = eol+2;
        else
            line = eol+1;
    }
    return line;
}
