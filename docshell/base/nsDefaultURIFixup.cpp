







































#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsNetUtil.h"
#include "nsEscape.h"
#include "nsCRT.h"

#include "nsIPrefService.h"
#include "nsIPrefLocalizedString.h"
#include "nsIPlatformCharset.h"
#include "nsILocalFile.h"

#ifdef MOZ_TOOLKIT_SEARCH
#include "nsIBrowserSearchService.h"
#endif

#include "nsIURIFixup.h"
#include "nsDefaultURIFixup.h"


NS_IMPL_ISUPPORTS1(nsDefaultURIFixup, nsIURIFixup)

nsDefaultURIFixup::nsDefaultURIFixup()
{
  

  
  mPrefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID);
}


nsDefaultURIFixup::~nsDefaultURIFixup()
{
  
}


NS_IMETHODIMP
nsDefaultURIFixup::CreateExposableURI(nsIURI *aURI, nsIURI **aReturn)
{
    NS_ENSURE_ARG_POINTER(aURI);
    NS_ENSURE_ARG_POINTER(aReturn);

    PRBool isWyciwyg = PR_FALSE;
    aURI->SchemeIs("wyciwyg", &isWyciwyg);

    nsCAutoString userPass;
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
        nsCAutoString path;
        nsresult rv = aURI->GetPath(path);
        NS_ENSURE_SUCCESS(rv, rv);

        PRUint32 pathLength = path.Length();
        if (pathLength <= 2)
        {
            return NS_ERROR_FAILURE;
        }

        
        
        
        PRInt32 slashIndex = path.FindChar('/', 2);
        if (slashIndex == kNotFound)
        {
            return NS_ERROR_FAILURE;
        }

        
        nsCAutoString charset;
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

    
    PRBool hideUserPass = PR_TRUE;
    if (mPrefBranch)
    {
        mPrefBranch->GetBoolPref("browser.fixup.hide_user_pass", &hideUserPass);
    }
    if (hideUserPass)
        uri->SetUserPass(EmptyCString());

    
    *aReturn = uri;
    NS_ADDREF(*aReturn);
    return NS_OK;
}


NS_IMETHODIMP
nsDefaultURIFixup::CreateFixupURI(const nsACString& aStringURI, PRUint32 aFixupFlags, nsIURI **aURI)
{
    NS_ENSURE_ARG(!aStringURI.IsEmpty());
    NS_ENSURE_ARG_POINTER(aURI);

    nsresult rv;
    *aURI = nsnull;

    nsCAutoString uriString(aStringURI);
    uriString.Trim(" ");  

    
    uriString.StripChars("\r\n");

    NS_ENSURE_TRUE(!uriString.IsEmpty(), NS_ERROR_FAILURE);

    nsCOMPtr<nsIIOService> ioService = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCAutoString scheme;
    ioService->ExtractScheme(aStringURI, scheme);
    
    
    
    

    if (scheme.LowerCaseEqualsLiteral("view-source"))
    {
        nsCOMPtr<nsIURI> uri;
        PRUint32 newFixupFlags = aFixupFlags & ~FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP;

        rv =  CreateFixupURI(Substring(uriString,
                                       sizeof("view-source:") - 1,
                                       uriString.Length() -
                                         (sizeof("view-source:") - 1)),
                             newFixupFlags, getter_AddRefs(uri));
        if (NS_FAILED(rv))
            return NS_ERROR_FAILURE;
        nsCAutoString spec;
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
            
            
            
            
            
            nsCAutoString::iterator start;
            nsCAutoString::iterator end;
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

    
    
    PRBool bAsciiURI = IsASCII(uriString);
    PRBool bUseNonDefaultCharsetForURI =
                        !bAsciiURI &&
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
                       bUseNonDefaultCharsetForURI ? GetCharsetForUrlBar() : nsnull);

        if (!*aURI && rv != NS_ERROR_MALFORMED_URI) {
            return rv;
        }
    }
    
    if (*aURI) {
        if (aFixupFlags & FIXUP_FLAGS_MAKE_ALTERNATE_URI)
            MakeAlternateURI(*aURI);
        return NS_OK;
    }

    
    
    PRBool fixupKeywords = PR_FALSE;
    if (aFixupFlags & FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP) {
        if (mPrefBranch)
        {
            NS_ENSURE_SUCCESS(mPrefBranch->GetBoolPref("keyword.enabled", &fixupKeywords), NS_ERROR_FAILURE);
        }
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

    
    
    
    
    
    
    
    
    
    PRInt32 schemeDelim = uriString.Find("://",0);
    PRInt32 firstDelim = uriString.FindCharInSet("/:");
    if (schemeDelim <= 0 ||
        (firstDelim != -1 && schemeDelim > firstDelim)) {
        
        PRInt32 hostPos = uriString.FindCharInSet("/:?#");
        if (hostPos == -1) 
            hostPos = uriString.Length();

        
        nsCAutoString hostSpec;
        uriString.Left(hostSpec, hostPos);

        
        if (IsLikelyFTP(hostSpec))
            uriString.Assign(NS_LITERAL_CSTRING("ftp://") + uriString);
        else 
            uriString.Assign(NS_LITERAL_CSTRING("http://") + uriString);

        
        if (!bAsciiURI)
          bUseNonDefaultCharsetForURI = PR_TRUE;
    } 

    rv = NS_NewURI(aURI, uriString, bUseNonDefaultCharsetForURI ? GetCharsetForUrlBar() : nsnull);

    
    

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
    *aURI = nsnull;
    NS_ENSURE_STATE(mPrefBranch);

    
    nsCAutoString keyword(aKeyword);
    if (StringBeginsWith(keyword, NS_LITERAL_CSTRING("?"))) {
        keyword.Cut(0, 1);
    }
    keyword.Trim(" ");

    nsXPIDLCString url;
    nsCOMPtr<nsIPrefLocalizedString> keywordURL;
    mPrefBranch->GetComplexValue("keyword.URL", 
                                 NS_GET_IID(nsIPrefLocalizedString),
                                 getter_AddRefs(keywordURL));

    if (keywordURL) {
        nsXPIDLString wurl;
        keywordURL->GetData(getter_Copies(wurl));
        CopyUTF16toUTF8(wurl, url);
    } else {
        
        mPrefBranch->GetCharPref("keyword.URL", getter_Copies(url));
    }

    
    if (!url.IsEmpty()) {
        
        nsCAutoString spec;
        if (!NS_Escape(keyword, spec, url_XPAlphas)) {
            return NS_ERROR_OUT_OF_MEMORY;
        }

        spec.Insert(url, 0);

        return NS_NewURI(aURI, spec);
    }

#ifdef MOZ_TOOLKIT_SEARCH
    
    nsCOMPtr<nsIBrowserSearchService> searchSvc = do_GetService("@mozilla.org/browser/search-service;1");
    if (searchSvc) {
        nsCOMPtr<nsISearchEngine> defaultEngine;
        searchSvc->GetOriginalDefaultEngine(getter_AddRefs(defaultEngine));
        if (defaultEngine) {
            nsCOMPtr<nsISearchSubmission> submission;
            
            
            
            
            
            defaultEngine->GetSubmission(NS_ConvertUTF8toUTF16(keyword),
                                         NS_LITERAL_STRING("application/x-moz-keywordsearch"),
                                         getter_AddRefs(submission));
            
            
            if (!submission) {
                defaultEngine->GetSubmission(NS_ConvertUTF8toUTF16(keyword),
                                             EmptyString(),
                                             getter_AddRefs(submission));
            }

            if (submission) {
                
                
                
                nsCOMPtr<nsIInputStream> postData;
                submission->GetPostData(getter_AddRefs(postData));
                if (postData) {
                    return NS_ERROR_NOT_AVAILABLE;
                }

                return submission->GetUri(aURI);
            }
        }
    }
#endif

    
    return NS_ERROR_NOT_AVAILABLE;
}

PRBool nsDefaultURIFixup::MakeAlternateURI(nsIURI *aURI)
{
    if (!mPrefBranch)
    {
        return PR_FALSE;
    }
    PRBool makeAlternate = PR_TRUE;
    mPrefBranch->GetBoolPref("browser.fixup.alternate.enabled", &makeAlternate);
    if (!makeAlternate)
    {
        return PR_FALSE;
    }

    
    PRBool isHttp = PR_FALSE;
    aURI->SchemeIs("http", &isHttp);
    if (!isHttp) {
        return PR_FALSE;
    }

    
    nsCAutoString userpass;
    aURI->GetUserPass(userpass);
    if (!userpass.IsEmpty()) {
        return PR_FALSE;
    }

    nsCAutoString oldHost;
    nsCAutoString newHost;
    aURI->GetHost(oldHost);

    
    PRInt32 numDots = 0;
    nsReadingIterator<char> iter;
    nsReadingIterator<char> iterEnd;
    oldHost.BeginReading(iter);
    oldHost.EndReading(iterEnd);
    while (iter != iterEnd) {
        if (*iter == '.')
            numDots++;
        ++iter;
    }


    nsresult rv;

    
    

    nsCAutoString prefix("www.");
    nsXPIDLCString prefPrefix;
    rv = mPrefBranch->GetCharPref("browser.fixup.alternate.prefix", getter_Copies(prefPrefix));
    if (NS_SUCCEEDED(rv))
    {
        prefix.Assign(prefPrefix);
    }

    nsCAutoString suffix(".com");
    nsXPIDLCString prefSuffix;
    rv = mPrefBranch->GetCharPref("browser.fixup.alternate.suffix", getter_Copies(prefSuffix));
    if (NS_SUCCEEDED(rv))
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
            
            return PR_FALSE;
        }
    }
    else
    {
        
        return PR_FALSE;
    }

    if (newHost.IsEmpty()) {
        return PR_FALSE;
    }

    
    aURI->SetHost(newHost);
    return PR_TRUE;
}





PRBool nsDefaultURIFixup::IsLikelyFTP(const nsCString &aHostSpec)
{
    PRBool likelyFTP = PR_FALSE;
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
                        likelyFTP = PR_TRUE;
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
    nsCAutoString uriSpecOut;

    nsresult rv = ConvertFileToStringURI(aStringURI, uriSpecOut);
    if (NS_SUCCEEDED(rv))
    {
        
        if(NS_SUCCEEDED(NS_NewURI(aURI, uriSpecOut.get(), nsnull)))
            return NS_OK;
    } 
    return NS_ERROR_FAILURE;
}

nsresult nsDefaultURIFixup::ConvertFileToStringURI(const nsACString& aIn,
                                                   nsCString& aOut)
{
    PRBool attemptFixup = PR_FALSE;

#if defined(XP_WIN) || defined(XP_OS2)
    
    if(kNotFound != aIn.FindChar('\\') ||
       (aIn.Length() == 2 && (aIn.Last() == ':' || aIn.Last() == '|')))
    {
        attemptFixup = PR_TRUE;
    }
#elif defined(XP_UNIX)
    
    if(aIn.First() == '/')
    {
        attemptFixup = PR_TRUE;
    }
#else
    
#endif

    if (attemptFixup)
    {
        
        

        
        
        

        nsCOMPtr<nsILocalFile> filePath;
        nsresult rv;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        NS_ConvertUTF8toUTF16 in(aIn);
        if (PossiblyByteExpandedFileName(in)) {
          
          rv = NS_NewNativeLocalFile(NS_LossyConvertUTF16toASCII(in), PR_FALSE, getter_AddRefs(filePath));
        }
        else {
          
          rv = NS_NewLocalFile(in, PR_FALSE, getter_AddRefs(filePath));
        }

        if (NS_SUCCEEDED(rv))
        {
            NS_GetURLSpecFromFile(filePath, aOut);
            return NS_OK;
        }
    }

    return NS_ERROR_FAILURE;
}

PRBool nsDefaultURIFixup::PossiblyHostPortUrl(const nsACString &aUrl)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    

    nsACString::const_iterator iterBegin;
    nsACString::const_iterator iterEnd;
    aUrl.BeginReading(iterBegin);
    aUrl.EndReading(iterEnd);
    nsACString::const_iterator iter = iterBegin;

    while (iter != iterEnd)
    {
        PRUint32 chunkSize = 0;
        
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
            return PR_FALSE;
        }
        if (*iter == ':')
        {
            
            break;
        }
        if (*iter != '.')
        {
            
            return PR_FALSE;
        }
        ++iter;
    }
    if (iter == iterEnd)
    {
        
        return PR_FALSE;
    }
    ++iter;

    
    

    PRUint32 digitCount = 0;
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
            
            return PR_FALSE;
        }
        ++iter;
    }
    if (digitCount == 0 || digitCount > 5)
    {
        
        return PR_FALSE;
    }

    
    return PR_TRUE;
}

PRBool nsDefaultURIFixup::PossiblyByteExpandedFileName(const nsAString& aIn)
{
    
    
    
    
    

    nsReadingIterator<PRUnichar> iter;
    nsReadingIterator<PRUnichar> iterEnd;
    aIn.BeginReading(iter);
    aIn.EndReading(iterEnd);
    while (iter != iterEnd)
    {
        if (*iter >= 0x0080 && *iter <= 0x00FF)
            return PR_TRUE;
        ++iter;
    }
    return PR_FALSE;
}

const char * nsDefaultURIFixup::GetFileSystemCharset()
{
  if (mFsCharset.IsEmpty())
  {
    nsresult rv;
    nsCAutoString charset;
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
    
    
    
    
    
    
    

    
    
    
    
    
    
    
    

    
    
    
    
    PRUint32 dotLoc   = PRUint32(aURIString.FindChar('.'));
    PRUint32 colonLoc = PRUint32(aURIString.FindChar(':'));
    PRUint32 spaceLoc = PRUint32(aURIString.FindChar(' '));
    if (spaceLoc == 0) {
        
        spaceLoc = PRUint32(kNotFound);
    }
    PRUint32 qMarkLoc = PRUint32(aURIString.FindChar('?'));
    PRUint32 quoteLoc = NS_MIN(PRUint32(aURIString.FindChar('"')),
                               PRUint32(aURIString.FindChar('\'')));

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
    if (fixup == nsnull)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    return fixup->QueryInterface(NS_GET_IID(nsIURIFixup), (void **) aURIFixup);
}

