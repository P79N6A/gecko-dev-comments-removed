





#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsNetUtil.h"
#include "nsEscape.h"
#include "nsCRT.h"

#include "nsIPlatformCharset.h"
#include "nsIFile.h"
#include <algorithm>

#ifdef MOZ_TOOLKIT_SEARCH
#include "nsIBrowserSearchService.h"
#endif

#include "nsIURIFixup.h"
#include "nsDefaultURIFixup.h"
#include "mozilla/Preferences.h"
#include "nsIObserverService.h"

using namespace mozilla;


NS_IMPL_ISUPPORTS1(nsDefaultURIFixup, nsIURIFixup)

nsDefaultURIFixup::nsDefaultURIFixup()
{
  
}


nsDefaultURIFixup::~nsDefaultURIFixup()
{
  
}


NS_IMETHODIMP
nsDefaultURIFixup::CreateExposableURI(nsIURI *aURI, nsIURI **aReturn)
{
    NS_ENSURE_ARG_POINTER(aURI);
    NS_ENSURE_ARG_POINTER(aReturn);

    bool isWyciwyg = false;
    aURI->SchemeIs("wyciwyg", &isWyciwyg);

    nsAutoCString userPass;
    aURI->GetUserPass(userPass);

    
    if (!isWyciwyg && userPass.IsEmpty())
    {
        *aReturn = aURI;
        NS_ADDREF(*aReturn);
        return NS_OK;
    }

    
    nsCOMPtr<nsIURI> uri;
    if (isWyciwyg)
    {
        nsAutoCString path;
        nsresult rv = aURI->GetPath(path);
        NS_ENSURE_SUCCESS(rv, rv);

        uint32_t pathLength = path.Length();
        if (pathLength <= 2)
        {
            return NS_ERROR_FAILURE;
        }

        
        
        
        int32_t slashIndex = path.FindChar('/', 2);
        if (slashIndex == kNotFound)
        {
            return NS_ERROR_FAILURE;
        }

        
        nsAutoCString charset;
        aURI->GetOriginCharset(charset);

        rv = NS_NewURI(getter_AddRefs(uri),
                   Substring(path, slashIndex + 1, pathLength - slashIndex - 1),
                   charset.get());
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else
    {
        
        nsresult rv = aURI->Clone(getter_AddRefs(uri));
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    if (Preferences::GetBool("browser.fixup.hide_user_pass", true))
    {
        uri->SetUserPass(EmptyCString());
    }

    
    *aReturn = uri;
    NS_ADDREF(*aReturn);
    return NS_OK;
}


NS_IMETHODIMP
nsDefaultURIFixup::CreateFixupURI(const nsACString& aStringURI, uint32_t aFixupFlags, nsIURI **aURI)
{
    NS_ENSURE_ARG(!aStringURI.IsEmpty());
    NS_ENSURE_ARG_POINTER(aURI);

    nsresult rv;
    *aURI = nullptr;

    nsAutoCString uriString(aStringURI);
    uriString.Trim(" ");  

    
    uriString.StripChars("\r\n");

    NS_ENSURE_TRUE(!uriString.IsEmpty(), NS_ERROR_FAILURE);

    nsCOMPtr<nsIIOService> ioService = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsAutoCString scheme;
    ioService->ExtractScheme(aStringURI, scheme);
    
    
    
    

    if (scheme.LowerCaseEqualsLiteral("view-source"))
    {
        nsCOMPtr<nsIURI> uri;
        uint32_t newFixupFlags = aFixupFlags & ~FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP;

        rv =  CreateFixupURI(Substring(uriString,
                                       sizeof("view-source:") - 1,
                                       uriString.Length() -
                                         (sizeof("view-source:") - 1)),
                             newFixupFlags, getter_AddRefs(uri));
        if (NS_FAILED(rv))
            return NS_ERROR_FAILURE;
        nsAutoCString spec;
        uri->GetSpec(spec);
        uriString.Assign(NS_LITERAL_CSTRING("view-source:") + spec);
    }
    else {
        
        FileURIFixup(uriString, aURI);
        if(*aURI)
            return NS_OK;

#if defined(XP_WIN) || defined(XP_OS2)
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (scheme.IsEmpty() ||
            scheme.LowerCaseEqualsLiteral("http") ||
            scheme.LowerCaseEqualsLiteral("https") ||
            scheme.LowerCaseEqualsLiteral("ftp"))
        {
            
            
            
            
            
            nsAutoCString::iterator start;
            nsAutoCString::iterator end;
            uriString.BeginWriting(start);
            uriString.EndWriting(end);
            while (start != end) {
                if (*start == '?' || *start == '#' || *start == '/')
                    break;
                if (*start == '\\')
                    *start = '/';
                ++start;
            }
        }
#endif
    }

    
    
    bool bAsciiURI = IsASCII(uriString);
    bool useUTF8 = (aFixupFlags & FIXUP_FLAG_USE_UTF8) ||
                   Preferences::GetBool("browser.fixup.use-utf8", false);
    bool bUseNonDefaultCharsetForURI =
                        !bAsciiURI && !useUTF8 &&
                        (scheme.IsEmpty() ||
                         scheme.LowerCaseEqualsLiteral("http") ||
                         scheme.LowerCaseEqualsLiteral("https") ||
                         scheme.LowerCaseEqualsLiteral("ftp") ||
                         scheme.LowerCaseEqualsLiteral("file"));

    
    
    nsCOMPtr<nsIProtocolHandler> ourHandler, extHandler;
    
    ioService->GetProtocolHandler(scheme.get(), getter_AddRefs(ourHandler));
    extHandler = do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX"default");
    
    if (ourHandler != extHandler || !PossiblyHostPortUrl(uriString)) {
        
        rv = NS_NewURI(aURI, uriString,
                       bUseNonDefaultCharsetForURI ? GetCharsetForUrlBar() : nullptr);

        if (!*aURI && rv != NS_ERROR_MALFORMED_URI) {
            return rv;
        }
    }
    
    if (*aURI) {
        if (aFixupFlags & FIXUP_FLAGS_MAKE_ALTERNATE_URI)
            MakeAlternateURI(*aURI);
        return NS_OK;
    }

    
    
    bool fixupKeywords = false;
    if (aFixupFlags & FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP) {
        nsresult rv = Preferences::GetBool("keyword.enabled", &fixupKeywords);
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
        if (fixupKeywords)
        {
            KeywordURIFixup(uriString, aURI);
            if(*aURI)
                return NS_OK;
        }
    }

    
    
    
    
    
    if (StringBeginsWith(uriString, NS_LITERAL_CSTRING("://")))
    {
        uriString = StringTail(uriString, uriString.Length() - 3);
    }
    else if (StringBeginsWith(uriString, NS_LITERAL_CSTRING("//")))
    {
        uriString = StringTail(uriString, uriString.Length() - 2);
    }

    
    
    
    
    
    
    
    
    
    int32_t schemeDelim = uriString.Find("://",0);
    int32_t firstDelim = uriString.FindCharInSet("/:");
    if (schemeDelim <= 0 ||
        (firstDelim != -1 && schemeDelim > firstDelim)) {
        
        int32_t hostPos = uriString.FindCharInSet("/:?#");
        if (hostPos == -1) 
            hostPos = uriString.Length();

        
        nsAutoCString hostSpec;
        uriString.Left(hostSpec, hostPos);

        
        if (IsLikelyFTP(hostSpec))
            uriString.Assign(NS_LITERAL_CSTRING("ftp://") + uriString);
        else 
            uriString.Assign(NS_LITERAL_CSTRING("http://") + uriString);

        
        if (!bAsciiURI && !useUTF8)
          bUseNonDefaultCharsetForURI = true;
    } 

    rv = NS_NewURI(aURI, uriString, bUseNonDefaultCharsetForURI ? GetCharsetForUrlBar() : nullptr);

    
    

    if (*aURI && aFixupFlags & FIXUP_FLAGS_MAKE_ALTERNATE_URI) {
        MakeAlternateURI(*aURI);
    }

    
    
    if (!*aURI && fixupKeywords)
    {
        KeywordToURI(aStringURI, aURI);
        if(*aURI)
            return NS_OK;
    }

    return rv;
}

NS_IMETHODIMP nsDefaultURIFixup::KeywordToURI(const nsACString& aKeyword,
                                              nsIURI **aURI)
{
    *aURI = nullptr;
    NS_ENSURE_STATE(Preferences::GetRootBranch());

    
    nsAutoCString keyword(aKeyword);
    if (StringBeginsWith(keyword, NS_LITERAL_CSTRING("?"))) {
        keyword.Cut(0, 1);
    }
    keyword.Trim(" ");

    nsAdoptingCString url = Preferences::GetLocalizedCString("keyword.URL");
    if (!url) {
        
        url = Preferences::GetCString("keyword.URL");
    }

    
    if (!url.IsEmpty()) {
        
        nsAutoCString spec;
        if (!NS_Escape(keyword, spec, url_XPAlphas)) {
            return NS_ERROR_OUT_OF_MEMORY;
        }

        spec.Insert(url, 0);

        nsresult rv = NS_NewURI(aURI, spec);
        if (NS_FAILED(rv))
            return rv;

        nsCOMPtr<nsIObserverService> obsSvc = mozilla::services::GetObserverService();
        if (obsSvc) {
            obsSvc->NotifyObservers(*aURI,
                                    "defaultURIFixup-using-keyword-pref",
                                    nullptr);
        }
        return NS_OK;
    }

#ifdef MOZ_TOOLKIT_SEARCH
    
    nsCOMPtr<nsIBrowserSearchService> searchSvc = do_GetService("@mozilla.org/browser/search-service;1");
    if (searchSvc) {
        nsCOMPtr<nsISearchEngine> defaultEngine;
        searchSvc->GetOriginalDefaultEngine(getter_AddRefs(defaultEngine));
        if (defaultEngine) {
            nsCOMPtr<nsISearchSubmission> submission;
            
            
            defaultEngine->GetSubmission(NS_ConvertUTF8toUTF16(keyword),
                                         EmptyString(),
                                         NS_LITERAL_STRING("keyword"),
                                         getter_AddRefs(submission));
            if (submission) {
                
                
                
                nsCOMPtr<nsIInputStream> postData;
                submission->GetPostData(getter_AddRefs(postData));
                if (postData) {
                    return NS_ERROR_NOT_AVAILABLE;
                }

                
                
                
                
                
                
                
                nsCOMPtr<nsIObserverService> obsSvc = mozilla::services::GetObserverService();
                if (obsSvc) {
                  nsAutoString name;
                  defaultEngine->GetName(name);
                  obsSvc->NotifyObservers(nullptr, "keyword-search", name.get());
                }

                return submission->GetUri(aURI);
            }
        }
    }
#endif

    
    return NS_ERROR_NOT_AVAILABLE;
}

bool nsDefaultURIFixup::MakeAlternateURI(nsIURI *aURI)
{
    if (!Preferences::GetRootBranch())
    {
        return false;
    }
    if (!Preferences::GetBool("browser.fixup.alternate.enabled", true))
    {
        return false;
    }

    
    bool isHttp = false;
    aURI->SchemeIs("http", &isHttp);
    if (!isHttp) {
        return false;
    }

    
    nsAutoCString userpass;
    aURI->GetUserPass(userpass);
    if (!userpass.IsEmpty()) {
        return false;
    }

    nsAutoCString oldHost;
    nsAutoCString newHost;
    aURI->GetHost(oldHost);

    
    int32_t numDots = 0;
    nsReadingIterator<char> iter;
    nsReadingIterator<char> iterEnd;
    oldHost.BeginReading(iter);
    oldHost.EndReading(iterEnd);
    while (iter != iterEnd) {
        if (*iter == '.')
            numDots++;
        ++iter;
    }


    
    

    nsAutoCString prefix("www.");
    nsAdoptingCString prefPrefix =
        Preferences::GetCString("browser.fixup.alternate.prefix");
    if (prefPrefix)
    {
        prefix.Assign(prefPrefix);
    }

    nsAutoCString suffix(".com");
    nsAdoptingCString prefSuffix =
        Preferences::GetCString("browser.fixup.alternate.suffix");
    if (prefSuffix)
    {
        suffix.Assign(prefSuffix);
    }
    
    if (numDots == 0)
    {
        newHost.Assign(prefix);
        newHost.Append(oldHost);
        newHost.Append(suffix);
    }
    else if (numDots == 1)
    {
        if (!prefix.IsEmpty() &&
                oldHost.EqualsIgnoreCase(prefix.get(), prefix.Length())) {
            newHost.Assign(oldHost);
            newHost.Append(suffix);
        }
        else if (!suffix.IsEmpty()) {
            newHost.Assign(prefix);
            newHost.Append(oldHost);
        }
        else
        {
            
            return false;
        }
    }
    else
    {
        
        return false;
    }

    if (newHost.IsEmpty()) {
        return false;
    }

    
    aURI->SetHost(newHost);
    return true;
}





bool nsDefaultURIFixup::IsLikelyFTP(const nsCString &aHostSpec)
{
    bool likelyFTP = false;
    if (aHostSpec.EqualsIgnoreCase("ftp", 3)) {
        nsACString::const_iterator iter;
        nsACString::const_iterator end;
        aHostSpec.BeginReading(iter);
        aHostSpec.EndReading(end);
        iter.advance(3); 

        while (iter != end)
        {
            if (*iter == '.') {
                
                ++iter;
                while (iter != end)
                {
                    if (*iter == '.') {
                        likelyFTP = true;
                        break;
                    }
                    ++iter;
                }
                break;
            }
            else if (!nsCRT::IsAsciiDigit(*iter)) {
                break;
            }
            ++iter;
        }
    }
    return likelyFTP;
}

nsresult nsDefaultURIFixup::FileURIFixup(const nsACString& aStringURI, 
                                         nsIURI** aURI)
{
    nsAutoCString uriSpecOut;

    nsresult rv = ConvertFileToStringURI(aStringURI, uriSpecOut);
    if (NS_SUCCEEDED(rv))
    {
        
        if(NS_SUCCEEDED(NS_NewURI(aURI, uriSpecOut.get(), nullptr)))
            return NS_OK;
    } 
    return NS_ERROR_FAILURE;
}

nsresult nsDefaultURIFixup::ConvertFileToStringURI(const nsACString& aIn,
                                                   nsCString& aOut)
{
    bool attemptFixup = false;

#if defined(XP_WIN) || defined(XP_OS2)
    
    if(kNotFound != aIn.FindChar('\\') ||
       (aIn.Length() == 2 && (aIn.Last() == ':' || aIn.Last() == '|')))
    {
        attemptFixup = true;
    }
#elif defined(XP_UNIX)
    
    if(aIn.First() == '/')
    {
        attemptFixup = true;
    }
#else
    
#endif

    if (attemptFixup)
    {
        
        

        
        
        

        nsCOMPtr<nsIFile> filePath;
        nsresult rv;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        NS_ConvertUTF8toUTF16 in(aIn);
        if (PossiblyByteExpandedFileName(in)) {
          
          rv = NS_NewNativeLocalFile(NS_LossyConvertUTF16toASCII(in), false, getter_AddRefs(filePath));
        }
        else {
          
          rv = NS_NewLocalFile(in, false, getter_AddRefs(filePath));
        }

        if (NS_SUCCEEDED(rv))
        {
            NS_GetURLSpecFromFile(filePath, aOut);
            return NS_OK;
        }
    }

    return NS_ERROR_FAILURE;
}

bool nsDefaultURIFixup::PossiblyHostPortUrl(const nsACString &aUrl)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    

    nsACString::const_iterator iterBegin;
    nsACString::const_iterator iterEnd;
    aUrl.BeginReading(iterBegin);
    aUrl.EndReading(iterEnd);
    nsACString::const_iterator iter = iterBegin;

    while (iter != iterEnd)
    {
        uint32_t chunkSize = 0;
        
        while (iter != iterEnd &&
               (*iter == '-' ||
                nsCRT::IsAsciiAlpha(*iter) ||
                nsCRT::IsAsciiDigit(*iter)))
        {
            ++chunkSize;
            ++iter;
        }
        if (chunkSize == 0 || iter == iterEnd)
        {
            return false;
        }
        if (*iter == ':')
        {
            
            break;
        }
        if (*iter != '.')
        {
            
            return false;
        }
        ++iter;
    }
    if (iter == iterEnd)
    {
        
        return false;
    }
    ++iter;

    
    

    uint32_t digitCount = 0;
    while (iter != iterEnd && digitCount <= 5)
    {
        if (nsCRT::IsAsciiDigit(*iter))
        {
            digitCount++;
        }
        else if (*iter == '/')
        {
            break;
        }
        else
        {
            
            return false;
        }
        ++iter;
    }
    if (digitCount == 0 || digitCount > 5)
    {
        
        return false;
    }

    
    return true;
}

bool nsDefaultURIFixup::PossiblyByteExpandedFileName(const nsAString& aIn)
{
    
    
    
    
    

    nsReadingIterator<PRUnichar> iter;
    nsReadingIterator<PRUnichar> iterEnd;
    aIn.BeginReading(iter);
    aIn.EndReading(iterEnd);
    while (iter != iterEnd)
    {
        if (*iter >= 0x0080 && *iter <= 0x00FF)
            return true;
        ++iter;
    }
    return false;
}

const char * nsDefaultURIFixup::GetFileSystemCharset()
{
  if (mFsCharset.IsEmpty())
  {
    nsresult rv;
    nsAutoCString charset;
    nsCOMPtr<nsIPlatformCharset> plat(do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv))
      rv = plat->GetCharset(kPlatformCharsetSel_FileName, charset);

    if (charset.IsEmpty())
      mFsCharset.AssignLiteral("ISO-8859-1");
    else
      mFsCharset.Assign(charset);
  }

  return mFsCharset.get();
}

const char * nsDefaultURIFixup::GetCharsetForUrlBar()
{
  const char *charset = GetFileSystemCharset();
  return charset;
}

nsresult nsDefaultURIFixup::KeywordURIFixup(const nsACString & aURIString, 
                                            nsIURI** aURI)
{
    
    
    
    
    
    
    

    
    
    
    
    
    
    
    

    
    
    
    
    uint32_t dotLoc   = uint32_t(aURIString.FindChar('.'));
    uint32_t colonLoc = uint32_t(aURIString.FindChar(':'));
    uint32_t spaceLoc = uint32_t(aURIString.FindChar(' '));
    if (spaceLoc == 0) {
        
        spaceLoc = uint32_t(kNotFound);
    }
    uint32_t qMarkLoc = uint32_t(aURIString.FindChar('?'));
    uint32_t quoteLoc = std::min(uint32_t(aURIString.FindChar('"')),
                               uint32_t(aURIString.FindChar('\'')));

    if (((spaceLoc < dotLoc || quoteLoc < dotLoc) &&
         (spaceLoc < colonLoc || quoteLoc < colonLoc) &&
         (spaceLoc < qMarkLoc || quoteLoc < qMarkLoc)) ||
        qMarkLoc == 0)
    {
        KeywordToURI(aURIString, aURI);
    }

    if(*aURI)
        return NS_OK;

    return NS_ERROR_FAILURE;
}


nsresult NS_NewURIFixup(nsIURIFixup **aURIFixup)
{
    nsDefaultURIFixup *fixup = new nsDefaultURIFixup;
    if (fixup == nullptr)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    return fixup->QueryInterface(NS_GET_IID(nsIURIFixup), (void **) aURIFixup);
}

