






































#include "nsFileProtocolHandler.h"
#include "nsFileChannel.h"
#include "nsInputStreamChannel.h"
#include "nsStandardURL.h"
#include "nsURLHelper.h"
#include "nsNetCID.h"

#include "nsIServiceManager.h"
#include "nsIURL.h"

#include "nsNetUtil.h"


#ifdef XP_WIN
#ifndef WINCE



#include <shlobj.h>
#include <intshcut.h>
#endif
#include "nsIFileURL.h"
#ifdef CompareString
#undef CompareString
#endif
#endif


#ifdef XP_OS2
#include "prio.h"
#include "nsIFileURL.h"
#include "nsILocalFileOS2.h"
#endif


#ifdef XP_UNIX
#include "nsINIParser.h"
#define DESKTOP_ENTRY_SECTION "Desktop Entry"
#endif



nsFileProtocolHandler::nsFileProtocolHandler()
{
}

nsresult
nsFileProtocolHandler::Init()
{
    return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsFileProtocolHandler,
                              nsIFileProtocolHandler,
                              nsIProtocolHandler,
                              nsISupportsWeakReference)




#if defined(XP_WIN)
NS_IMETHODIMP
nsFileProtocolHandler::ReadURLFile(nsIFile* aFile, nsIURI** aURI)
{

#if _MSC_VER < 1200 || defined (WINCE)
    return NS_ERROR_NOT_AVAILABLE;
#else
    nsAutoString path;
    nsresult rv = aFile->GetPath(path);
    if (NS_FAILED(rv))
        return rv;

    if (path.Length() < 4)
        return NS_ERROR_NOT_AVAILABLE;
    if (!StringTail(path, 4).LowerCaseEqualsLiteral(".url"))
        return NS_ERROR_NOT_AVAILABLE;

    HRESULT result;

    rv = NS_ERROR_NOT_AVAILABLE;

    IUniformResourceLocatorW* urlLink = nsnull;
    result = ::CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                                IID_IUniformResourceLocatorW, (void**)&urlLink);
    if (SUCCEEDED(result) && urlLink) {
        IPersistFile* urlFile = nsnull;
        result = urlLink->QueryInterface(IID_IPersistFile, (void**)&urlFile);
        if (SUCCEEDED(result) && urlFile) {
            result = urlFile->Load(path.get(), STGM_READ);
            if (SUCCEEDED(result) ) {
                LPWSTR lpTemp = nsnull;

                
                
                result = urlLink->GetURL(&lpTemp);
                if (SUCCEEDED(result) && lpTemp) {
                    rv = NS_NewURI(aURI, nsDependentString(lpTemp));
                    
                    CoTaskMemFree(lpTemp);
                }
            }
            urlFile->Release();
        }
        urlLink->Release();
    }
    return rv;

#endif 
}

#elif defined(XP_OS2)
NS_IMETHODIMP
nsFileProtocolHandler::ReadURLFile(nsIFile* aFile, nsIURI** aURI)
{
    nsresult rv;

    nsCOMPtr<nsILocalFileOS2> os2File (do_QueryInterface(aFile, &rv));
    if (NS_FAILED(rv))
        return NS_ERROR_NOT_AVAILABLE;

    
    PRBool isUrl;
    rv = os2File->IsFileType(NS_LITERAL_CSTRING("UniformResourceLocator"),
                             &isUrl);
    if (NS_FAILED(rv) || !isUrl)
        return NS_ERROR_NOT_AVAILABLE;

    
    PRFileDesc *file;
    rv = os2File->OpenNSPRFileDesc(PR_RDONLY, 0, &file);
    if (NS_FAILED(rv))
        return NS_ERROR_NOT_AVAILABLE;

    PRInt64 fileSize;
    os2File->GetFileSize(&fileSize);
    rv = NS_ERROR_NOT_AVAILABLE;

    
    
    char * buffer = (char*)NS_Alloc(fileSize+1);
    if (buffer) {
        PRInt32 cnt = PR_Read(file, buffer, fileSize);
        if (cnt > 0) {
            buffer[cnt] = '\0';
            if (NS_SUCCEEDED(NS_NewURI(aURI, nsDependentCString(buffer))))
                rv = NS_OK;
        }
        NS_Free(buffer);
    }
    PR_Close(file);

    return rv;
}

#elif defined(XP_UNIX) && !defined(__SYMBIAN32__)
NS_IMETHODIMP
nsFileProtocolHandler::ReadURLFile(nsIFile* aFile, nsIURI** aURI)
{
    
    
    nsCAutoString leafName;
    nsresult rv = aFile->GetNativeLeafName(leafName);
    if (NS_FAILED(rv) ||
	!StringEndsWith(leafName, NS_LITERAL_CSTRING(".desktop")))
        return NS_ERROR_NOT_AVAILABLE;

    nsCOMPtr<nsILocalFile> file(do_QueryInterface(aFile, &rv));
    if (NS_FAILED(rv))
        return rv;

    nsINIParser parser;
    rv = parser.Init(file);
    if (NS_FAILED(rv))
        return rv;

    nsCAutoString type;
    parser.GetString(DESKTOP_ENTRY_SECTION, "Type", type);
    if (!type.EqualsLiteral("Link"))
        return NS_ERROR_NOT_AVAILABLE;

    nsCAutoString url;
    rv = parser.GetString(DESKTOP_ENTRY_SECTION, "URL", url);
    if (NS_FAILED(rv) || url.IsEmpty())
        return NS_ERROR_NOT_AVAILABLE;

    return NS_NewURI(aURI, url);
}

#else 
NS_IMETHODIMP
nsFileProtocolHandler::ReadURLFile(nsIFile* aFile, nsIURI** aURI)
{
    return NS_ERROR_NOT_AVAILABLE;
}
#endif 

NS_IMETHODIMP
nsFileProtocolHandler::GetScheme(nsACString &result)
{
    result.AssignLiteral("file");
    return NS_OK;
}

NS_IMETHODIMP
nsFileProtocolHandler::GetDefaultPort(PRInt32 *result)
{
    *result = -1;        
    return NS_OK;
}

NS_IMETHODIMP
nsFileProtocolHandler::GetProtocolFlags(PRUint32 *result)
{
    *result = URI_NOAUTH | URI_IS_LOCAL_FILE | URI_IS_LOCAL_RESOURCE;
    return NS_OK;
}

NS_IMETHODIMP
nsFileProtocolHandler::NewURI(const nsACString &spec,
                              const char *charset,
                              nsIURI *baseURI,
                              nsIURI **result)
{
    nsCOMPtr<nsIStandardURL> url = new nsStandardURL(PR_TRUE);
    if (!url)
        return NS_ERROR_OUT_OF_MEMORY;

    const nsACString *specPtr = &spec;

#if defined(XP_WIN) || defined(XP_OS2)
    nsCAutoString buf;
    if (net_NormalizeFileURL(spec, buf))
        specPtr = &buf;
#endif

    nsresult rv = url->Init(nsIStandardURL::URLTYPE_NO_AUTHORITY, -1,
                            *specPtr, charset, baseURI);
    if (NS_FAILED(rv)) return rv;

    return CallQueryInterface(url, result);
}

NS_IMETHODIMP
nsFileProtocolHandler::NewChannel(nsIURI *uri, nsIChannel **result)
{
    nsFileChannel *chan = new nsFileChannel(uri);
    if (!chan)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(chan);

    nsresult rv = chan->Init();
    if (NS_FAILED(rv)) {
        NS_RELEASE(chan);
        return rv;
    }

    *result = chan;
    return NS_OK;
}

NS_IMETHODIMP 
nsFileProtocolHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *result)
{
    
    *result = PR_FALSE;
    return NS_OK;
}




NS_IMETHODIMP
nsFileProtocolHandler::NewFileURI(nsIFile *file, nsIURI **result)
{
    NS_ENSURE_ARG_POINTER(file);
    nsresult rv;

    nsCOMPtr<nsIFileURL> url = new nsStandardURL(PR_TRUE);
    if (!url)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    rv = url->SetFile(file);
    if (NS_FAILED(rv)) return rv;

    return CallQueryInterface(url, result);
}

NS_IMETHODIMP
nsFileProtocolHandler::GetURLSpecFromFile(nsIFile *file, nsACString &result)
{
    NS_ENSURE_ARG_POINTER(file);
    return net_GetURLSpecFromFile(file, result);
}

NS_IMETHODIMP
nsFileProtocolHandler::GetURLSpecFromActualFile(nsIFile *file, 
                                                nsACString &result)
{
    NS_ENSURE_ARG_POINTER(file);
    return net_GetURLSpecFromActualFile(file, result);
}

NS_IMETHODIMP
nsFileProtocolHandler::GetURLSpecFromDir(nsIFile *file, nsACString &result)
{
    NS_ENSURE_ARG_POINTER(file);
    return net_GetURLSpecFromDir(file, result);
}

NS_IMETHODIMP
nsFileProtocolHandler::GetFileFromURLSpec(const nsACString &spec, nsIFile **result)
{
    return net_GetFileFromURLSpec(spec, result);
}
