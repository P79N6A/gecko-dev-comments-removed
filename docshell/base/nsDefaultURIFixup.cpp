





#include "nsNetUtil.h"
#include "nsCRT.h"

#include "nsIFile.h"
#include <algorithm>

#ifdef MOZ_TOOLKIT_SEARCH
#include "nsIBrowserSearchService.h"
#endif

#include "nsIURIFixup.h"
#include "nsDefaultURIFixup.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "mozilla/ipc/URIUtils.h"
#include "nsIObserverService.h"
#include "nsXULAppAPI.h"


#include "nsCExternalHandlerService.h"
#include "nsIExternalProtocolService.h"

using namespace mozilla;


NS_IMPL_ISUPPORTS(nsDefaultURIFixup, nsIURIFixup)

static bool sInitializedPrefCaches = false;
static bool sFixTypos = true;
static bool sDNSFirstForSingleWords = false;
static bool sFixupKeywords = true;

nsDefaultURIFixup::nsDefaultURIFixup()
{
}

nsDefaultURIFixup::~nsDefaultURIFixup()
{
}


NS_IMETHODIMP
nsDefaultURIFixup::CreateExposableURI(nsIURI* aURI, nsIURI** aReturn)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(aReturn);

  bool isWyciwyg = false;
  aURI->SchemeIs("wyciwyg", &isWyciwyg);

  nsAutoCString userPass;
  aURI->GetUserPass(userPass);

  
  if (!isWyciwyg && userPass.IsEmpty()) {
    *aReturn = aURI;
    NS_ADDREF(*aReturn);
    return NS_OK;
  }

  
  nsCOMPtr<nsIURI> uri;
  if (isWyciwyg) {
    nsAutoCString path;
    nsresult rv = aURI->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t pathLength = path.Length();
    if (pathLength <= 2) {
      return NS_ERROR_FAILURE;
    }

    
    
    
    int32_t slashIndex = path.FindChar('/', 2);
    if (slashIndex == kNotFound) {
      return NS_ERROR_FAILURE;
    }

    
    
    nsAutoCString charset;
    aURI->GetOriginCharset(charset);

    rv = NS_NewURI(getter_AddRefs(uri),
                   Substring(path, slashIndex + 1, pathLength - slashIndex - 1),
                   charset.get());
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    
    nsresult rv = aURI->Clone(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (Preferences::GetBool("browser.fixup.hide_user_pass", true)) {
    uri->SetUserPass(EmptyCString());
  }

  uri.forget(aReturn);
  return NS_OK;
}


NS_IMETHODIMP
nsDefaultURIFixup::CreateFixupURI(const nsACString& aStringURI,
                                  uint32_t aFixupFlags,
                                  nsIInputStream** aPostData, nsIURI** aURI)
{
  nsCOMPtr<nsIURIFixupInfo> fixupInfo;
  nsresult rv = GetFixupURIInfo(aStringURI, aFixupFlags, aPostData,
                                getter_AddRefs(fixupInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  fixupInfo->GetPreferredURI(aURI);
  return rv;
}

NS_IMETHODIMP
nsDefaultURIFixup::GetFixupURIInfo(const nsACString& aStringURI,
                                   uint32_t aFixupFlags,
                                   nsIInputStream** aPostData,
                                   nsIURIFixupInfo** aInfo)
{
  NS_ENSURE_ARG(!aStringURI.IsEmpty());

  nsresult rv;

  nsAutoCString uriString(aStringURI);

  
  uriString.StripChars("\r\n");
  
  uriString.Trim(" ");

  NS_ENSURE_TRUE(!uriString.IsEmpty(), NS_ERROR_FAILURE);

  nsRefPtr<nsDefaultURIFixupInfo> info = new nsDefaultURIFixupInfo(uriString);
  NS_ADDREF(*aInfo = info);

  nsCOMPtr<nsIIOService> ioService =
    do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsAutoCString scheme;
  ioService->ExtractScheme(aStringURI, scheme);

  
  
  

  if (scheme.LowerCaseEqualsLiteral("view-source")) {
    nsCOMPtr<nsIURIFixupInfo> uriInfo;
    uint32_t newFixupFlags = aFixupFlags & ~FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP;

    rv = GetFixupURIInfo(Substring(uriString,
                                   sizeof("view-source:") - 1,
                                   uriString.Length() -
                                   (sizeof("view-source:") - 1)),
                         newFixupFlags, aPostData, getter_AddRefs(uriInfo));
    if (NS_FAILED(rv)) {
      return NS_ERROR_FAILURE;
    }
    nsAutoCString spec;
    nsCOMPtr<nsIURI> uri;
    uriInfo->GetPreferredURI(getter_AddRefs(uri));
    if (!uri) {
      return NS_ERROR_FAILURE;
    }
    uri->GetSpec(spec);
    uriString.AssignLiteral("view-source:");
    uriString.Append(spec);
  } else {
    
    nsCOMPtr<nsIURI> uri;
    FileURIFixup(uriString, getter_AddRefs(uri));
    
    
    if (uri) {
      uri.swap(info->mFixedURI);
      info->mPreferredURI = info->mFixedURI;
      info->mFixupChangedProtocol = true;
      return NS_OK;
    }

#if defined(XP_WIN)
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (scheme.IsEmpty() ||
        scheme.LowerCaseEqualsLiteral("http") ||
        scheme.LowerCaseEqualsLiteral("https") ||
        scheme.LowerCaseEqualsLiteral("ftp")) {
      
      
      
      
      
      nsAutoCString::iterator start;
      nsAutoCString::iterator end;
      uriString.BeginWriting(start);
      uriString.EndWriting(end);
      while (start != end) {
        if (*start == '?' || *start == '#' || *start == '/') {
          break;
        }
        if (*start == '\\') {
          *start = '/';
        }
        ++start;
      }
    }
#endif
  }

  if (!sInitializedPrefCaches) {
    
    rv = Preferences::AddBoolVarCache(&sFixTypos,
                                      "browser.fixup.typo.scheme",
                                      sFixTypos);
    MOZ_ASSERT(NS_SUCCEEDED(rv),
               "Failed to observe \"browser.fixup.typo.scheme\"");

    rv = Preferences::AddBoolVarCache(&sDNSFirstForSingleWords,
                                      "browser.fixup.dns_first_for_single_words",
                                      sDNSFirstForSingleWords);
    MOZ_ASSERT(NS_SUCCEEDED(rv),
               "Failed to observe \"browser.fixup.dns_first_for_single_words\"");

    rv = Preferences::AddBoolVarCache(&sFixupKeywords, "keyword.enabled",
                                      sFixupKeywords);
    MOZ_ASSERT(NS_SUCCEEDED(rv), "Failed to observe \"keyword.enabled\"");
    sInitializedPrefCaches = true;
  }

  
  if (sFixTypos && (aFixupFlags & FIXUP_FLAG_FIX_SCHEME_TYPOS)) {
    
    if (scheme.IsEmpty() ||
        scheme.LowerCaseEqualsLiteral("http") ||
        scheme.LowerCaseEqualsLiteral("https") ||
        scheme.LowerCaseEqualsLiteral("ftp") ||
        scheme.LowerCaseEqualsLiteral("file")) {
      
    } else if (scheme.LowerCaseEqualsLiteral("ttp")) {
      
      uriString.Replace(0, 3, "http");
      scheme.AssignLiteral("http");
      info->mFixupChangedProtocol = true;
    } else if (scheme.LowerCaseEqualsLiteral("ttps")) {
      
      uriString.Replace(0, 4, "https");
      scheme.AssignLiteral("https");
      info->mFixupChangedProtocol = true;
    } else if (scheme.LowerCaseEqualsLiteral("tps")) {
      
      uriString.Replace(0, 3, "https");
      scheme.AssignLiteral("https");
      info->mFixupChangedProtocol = true;
    } else if (scheme.LowerCaseEqualsLiteral("ps")) {
      
      uriString.Replace(0, 2, "https");
      scheme.AssignLiteral("https");
      info->mFixupChangedProtocol = true;
    } else if (scheme.LowerCaseEqualsLiteral("ile")) {
      
      uriString.Replace(0, 3, "file");
      scheme.AssignLiteral("file");
      info->mFixupChangedProtocol = true;
    } else if (scheme.LowerCaseEqualsLiteral("le")) {
      
      uriString.Replace(0, 2, "file");
      scheme.AssignLiteral("file");
      info->mFixupChangedProtocol = true;
    }
  }

  
  
  nsCOMPtr<nsIProtocolHandler> ourHandler, extHandler;

  ioService->GetProtocolHandler(scheme.get(), getter_AddRefs(ourHandler));
  extHandler = do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "default");

  if (ourHandler != extHandler || !PossiblyHostPortUrl(uriString)) {
    
    rv = NS_NewURI(getter_AddRefs(info->mFixedURI), uriString, nullptr);

    if (!info->mFixedURI && rv != NS_ERROR_MALFORMED_URI) {
      return rv;
    }
  }

  if (info->mFixedURI && ourHandler == extHandler && sFixupKeywords &&
      (aFixupFlags & FIXUP_FLAG_FIX_SCHEME_TYPOS)) {
    nsCOMPtr<nsIExternalProtocolService> extProtService =
      do_GetService(NS_EXTERNALPROTOCOLSERVICE_CONTRACTID);
    if (extProtService) {
      bool handlerExists = false;
      rv = extProtService->ExternalProtocolHandlerExists(scheme.get(),
                                                         &handlerExists);
      if (NS_FAILED(rv)) {
        return rv;
      }
      
      
      
      
      if (!handlerExists) {
        TryKeywordFixupForURIInfo(uriString, info, aPostData);
      }
    }
  }

  if (info->mFixedURI) {
    if (!info->mPreferredURI) {
      if (aFixupFlags & FIXUP_FLAGS_MAKE_ALTERNATE_URI) {
        info->mFixupCreatedAlternateURI = MakeAlternateURI(info->mFixedURI);
      }
      info->mPreferredURI = info->mFixedURI;
    }
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIURI> uriWithProtocol;
  bool inputHadDuffProtocol = false;

  
  
  
  
  
  if (StringBeginsWith(uriString, NS_LITERAL_CSTRING("://"))) {
    uriString = StringTail(uriString, uriString.Length() - 3);
    inputHadDuffProtocol = true;
  } else if (StringBeginsWith(uriString, NS_LITERAL_CSTRING("//"))) {
    uriString = StringTail(uriString, uriString.Length() - 2);
    inputHadDuffProtocol = true;
  }

  
  
  
  rv = FixupURIProtocol(uriString, info, getter_AddRefs(uriWithProtocol));
  if (uriWithProtocol) {
    info->mFixedURI = uriWithProtocol;
  }

  
  
  if (sFixupKeywords && (aFixupFlags & FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP) &&
      !inputHadDuffProtocol) {
    if (NS_SUCCEEDED(KeywordURIFixup(uriString, info, aPostData)) &&
        info->mPreferredURI) {
      return NS_OK;
    }
  }

  
  

  if (info->mFixedURI && aFixupFlags & FIXUP_FLAGS_MAKE_ALTERNATE_URI) {
    info->mFixupCreatedAlternateURI = MakeAlternateURI(info->mFixedURI);
  }

  
  
  if (info->mFixedURI) {
    if (aFixupFlags & FIXUP_FLAG_REQUIRE_WHITELISTED_HOST) {
      nsAutoCString asciiHost;
      if (NS_SUCCEEDED(info->mFixedURI->GetAsciiHost(asciiHost)) &&
          !asciiHost.IsEmpty()) {
        uint32_t dotLoc = uint32_t(asciiHost.FindChar('.'));

        if ((dotLoc == uint32_t(kNotFound) ||
             dotLoc == asciiHost.Length() - 1)) {
          if (IsDomainWhitelisted(asciiHost, dotLoc)) {
            info->mPreferredURI = info->mFixedURI;
          }
        } else {
          info->mPreferredURI = info->mFixedURI;
        }
      }
    } else {
      info->mPreferredURI = info->mFixedURI;
    }

    return NS_OK;
  }

  
  
  if (sFixupKeywords && (aFixupFlags & FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP)) {
    rv = TryKeywordFixupForURIInfo(aStringURI, info, aPostData);
  }

  return rv;
}

NS_IMETHODIMP
nsDefaultURIFixup::KeywordToURI(const nsACString& aKeyword,
                                nsIInputStream** aPostData,
                                nsIURIFixupInfo** aInfo)
{
  nsRefPtr<nsDefaultURIFixupInfo> info = new nsDefaultURIFixupInfo(aKeyword);
  NS_ADDREF(*aInfo = info);

  if (aPostData) {
    *aPostData = nullptr;
  }
  NS_ENSURE_STATE(Preferences::GetRootBranch());

  
  nsAutoCString keyword(aKeyword);
  if (StringBeginsWith(keyword, NS_LITERAL_CSTRING("?"))) {
    keyword.Cut(0, 1);
  }
  keyword.Trim(" ");

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    dom::ContentChild* contentChild = dom::ContentChild::GetSingleton();
    if (!contentChild) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    ipc::OptionalInputStreamParams postData;
    ipc::OptionalURIParams uri;
    nsAutoString providerName;
    if (!contentChild->SendKeywordToURI(keyword, &providerName, &postData,
                                        &uri)) {
      return NS_ERROR_FAILURE;
    }

    CopyUTF8toUTF16(keyword, info->mKeywordAsSent);
    info->mKeywordProviderName = providerName;

    if (aPostData) {
      nsTArray<ipc::FileDescriptor> fds;
      nsCOMPtr<nsIInputStream> temp = DeserializeInputStream(postData, fds);
      temp.forget(aPostData);

      MOZ_ASSERT(fds.IsEmpty());
    }

    nsCOMPtr<nsIURI> temp = DeserializeURI(uri);
    info->mPreferredURI = temp.forget();
    return NS_OK;
  }

#ifdef MOZ_TOOLKIT_SEARCH
  
  nsCOMPtr<nsIBrowserSearchService> searchSvc =
    do_GetService("@mozilla.org/browser/search-service;1");
  if (searchSvc) {
    nsCOMPtr<nsISearchEngine> defaultEngine;
    searchSvc->GetDefaultEngine(getter_AddRefs(defaultEngine));
    if (defaultEngine) {
      nsCOMPtr<nsISearchSubmission> submission;
      nsAutoString responseType;
      
      
      NS_NAMED_LITERAL_STRING(mozKeywordSearch,
                              "application/x-moz-keywordsearch");
      bool supportsResponseType = false;
      defaultEngine->SupportsResponseType(mozKeywordSearch,
                                          &supportsResponseType);
      if (supportsResponseType) {
        responseType.Assign(mozKeywordSearch);
      }

      NS_ConvertUTF8toUTF16 keywordW(keyword);
      defaultEngine->GetSubmission(keywordW,
                                   responseType,
                                   NS_LITERAL_STRING("keyword"),
                                   getter_AddRefs(submission));

      if (submission) {
        nsCOMPtr<nsIInputStream> postData;
        submission->GetPostData(getter_AddRefs(postData));
        if (aPostData) {
          postData.forget(aPostData);
        } else if (postData) {
          
          
          
          
          return NS_ERROR_FAILURE;
        }

        defaultEngine->GetName(info->mKeywordProviderName);
        info->mKeywordAsSent = keywordW;
        return submission->GetUri(getter_AddRefs(info->mPreferredURI));
      }
    }
  }
#endif

  
  return NS_ERROR_NOT_AVAILABLE;
}


nsresult
nsDefaultURIFixup::TryKeywordFixupForURIInfo(const nsACString& aURIString,
                                             nsDefaultURIFixupInfo* aFixupInfo,
                                             nsIInputStream** aPostData)
{
  nsCOMPtr<nsIURIFixupInfo> keywordInfo;
  nsresult rv = KeywordToURI(aURIString, aPostData,
                             getter_AddRefs(keywordInfo));
  if (NS_SUCCEEDED(rv)) {
    keywordInfo->GetKeywordProviderName(aFixupInfo->mKeywordProviderName);
    keywordInfo->GetKeywordAsSent(aFixupInfo->mKeywordAsSent);
    keywordInfo->GetPreferredURI(getter_AddRefs(aFixupInfo->mPreferredURI));
  }
  return rv;
}

bool
nsDefaultURIFixup::MakeAlternateURI(nsIURI* aURI)
{
  if (!Preferences::GetRootBranch()) {
    return false;
  }
  if (!Preferences::GetBool("browser.fixup.alternate.enabled", true)) {
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
    if (*iter == '.') {
      numDots++;
    }
    ++iter;
  }

  
  

  nsAutoCString prefix("www.");
  nsAdoptingCString prefPrefix =
    Preferences::GetCString("browser.fixup.alternate.prefix");
  if (prefPrefix) {
    prefix.Assign(prefPrefix);
  }

  nsAutoCString suffix(".com");
  nsAdoptingCString prefSuffix =
    Preferences::GetCString("browser.fixup.alternate.suffix");
  if (prefSuffix) {
    suffix.Assign(prefSuffix);
  }

  if (numDots == 0) {
    newHost.Assign(prefix);
    newHost.Append(oldHost);
    newHost.Append(suffix);
  } else if (numDots == 1) {
    if (!prefix.IsEmpty() &&
        oldHost.EqualsIgnoreCase(prefix.get(), prefix.Length())) {
      newHost.Assign(oldHost);
      newHost.Append(suffix);
    } else if (!suffix.IsEmpty()) {
      newHost.Assign(prefix);
      newHost.Append(oldHost);
    } else {
      
      return false;
    }
  } else {
    
    return false;
  }

  if (newHost.IsEmpty()) {
    return false;
  }

  
  aURI->SetHost(newHost);
  return true;
}





bool
nsDefaultURIFixup::IsLikelyFTP(const nsCString& aHostSpec)
{
  bool likelyFTP = false;
  if (aHostSpec.EqualsIgnoreCase("ftp", 3)) {
    nsACString::const_iterator iter;
    nsACString::const_iterator end;
    aHostSpec.BeginReading(iter);
    aHostSpec.EndReading(end);
    iter.advance(3); 

    while (iter != end) {
      if (*iter == '.') {
        
        ++iter;
        while (iter != end) {
          if (*iter == '.') {
            likelyFTP = true;
            break;
          }
          ++iter;
        }
        break;
      } else if (!nsCRT::IsAsciiDigit(*iter)) {
        break;
      }
      ++iter;
    }
  }
  return likelyFTP;
}

nsresult
nsDefaultURIFixup::FileURIFixup(const nsACString& aStringURI, nsIURI** aURI)
{
  nsAutoCString uriSpecOut;

  nsresult rv = ConvertFileToStringURI(aStringURI, uriSpecOut);
  if (NS_SUCCEEDED(rv)) {
    
    if (NS_SUCCEEDED(NS_NewURI(aURI, uriSpecOut.get(), nullptr))) {
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsDefaultURIFixup::ConvertFileToStringURI(const nsACString& aIn,
                                          nsCString& aResult)
{
  bool attemptFixup = false;

#if defined(XP_WIN)
  
  if (kNotFound != aIn.FindChar('\\') ||
      (aIn.Length() == 2 && (aIn.Last() == ':' || aIn.Last() == '|'))) {
    attemptFixup = true;
  }
#elif defined(XP_UNIX)
  
  if (aIn.First() == '/') {
    attemptFixup = true;
  }
#else
  
#endif

  if (attemptFixup) {
    
    

    
    
    

    nsCOMPtr<nsIFile> filePath;
    nsresult rv;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    NS_ConvertUTF8toUTF16 in(aIn);
    if (PossiblyByteExpandedFileName(in)) {
      
      rv = NS_NewNativeLocalFile(NS_LossyConvertUTF16toASCII(in), false,
                                 getter_AddRefs(filePath));
    } else {
      
      rv = NS_NewLocalFile(in, false, getter_AddRefs(filePath));
    }

    if (NS_SUCCEEDED(rv)) {
      NS_GetURLSpecFromFile(filePath, aResult);
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsDefaultURIFixup::FixupURIProtocol(const nsACString& aURIString,
                                    nsDefaultURIFixupInfo* aFixupInfo,
                                    nsIURI** aURI)
{
  nsAutoCString uriString(aURIString);
  *aURI = nullptr;

  
  
  
  
  
  
  
  
  
  int32_t schemeDelim = uriString.Find("://", 0);
  int32_t firstDelim = uriString.FindCharInSet("/:");
  if (schemeDelim <= 0 ||
      (firstDelim != -1 && schemeDelim > firstDelim)) {
    
    int32_t hostPos = uriString.FindCharInSet("/:?#");
    if (hostPos == -1) {
      hostPos = uriString.Length();
    }

    
    nsAutoCString hostSpec;
    uriString.Left(hostSpec, hostPos);

    
    if (IsLikelyFTP(hostSpec)) {
      uriString.InsertLiteral("ftp://", 0);
    } else {
      uriString.InsertLiteral("http://", 0);
    }
    aFixupInfo->mFixupChangedProtocol = true;
  } 

  return NS_NewURI(aURI, uriString, nullptr);
}

bool
nsDefaultURIFixup::PossiblyHostPortUrl(const nsACString& aUrl)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  

  nsACString::const_iterator iterBegin;
  nsACString::const_iterator iterEnd;
  aUrl.BeginReading(iterBegin);
  aUrl.EndReading(iterEnd);
  nsACString::const_iterator iter = iterBegin;

  while (iter != iterEnd) {
    uint32_t chunkSize = 0;
    
    while (iter != iterEnd &&
           (*iter == '-' ||
            nsCRT::IsAsciiAlpha(*iter) ||
            nsCRT::IsAsciiDigit(*iter))) {
      ++chunkSize;
      ++iter;
    }
    if (chunkSize == 0 || iter == iterEnd) {
      return false;
    }
    if (*iter == ':') {
      
      break;
    }
    if (*iter != '.') {
      
      return false;
    }
    ++iter;
  }
  if (iter == iterEnd) {
    
    return false;
  }
  ++iter;

  
  

  uint32_t digitCount = 0;
  while (iter != iterEnd && digitCount <= 5) {
    if (nsCRT::IsAsciiDigit(*iter)) {
      digitCount++;
    } else if (*iter == '/') {
      break;
    } else {
      
      return false;
    }
    ++iter;
  }
  if (digitCount == 0 || digitCount > 5) {
    
    return false;
  }

  
  return true;
}

bool
nsDefaultURIFixup::PossiblyByteExpandedFileName(const nsAString& aIn)
{
  
  
  
  
  

  nsReadingIterator<char16_t> iter;
  nsReadingIterator<char16_t> iterEnd;
  aIn.BeginReading(iter);
  aIn.EndReading(iterEnd);
  while (iter != iterEnd) {
    if (*iter >= 0x0080 && *iter <= 0x00FF) {
      return true;
    }
    ++iter;
  }
  return false;
}

nsresult
nsDefaultURIFixup::KeywordURIFixup(const nsACString& aURIString,
                                   nsDefaultURIFixupInfo* aFixupInfo,
                                   nsIInputStream** aPostData)
{
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  

  
  
  
  

  uint32_t firstDotLoc = uint32_t(kNotFound);
  uint32_t lastDotLoc = uint32_t(kNotFound);
  uint32_t firstColonLoc = uint32_t(kNotFound);
  uint32_t firstQuoteLoc = uint32_t(kNotFound);
  uint32_t firstSpaceLoc = uint32_t(kNotFound);
  uint32_t firstQMarkLoc = uint32_t(kNotFound);
  uint32_t lastLSBracketLoc = uint32_t(kNotFound);
  uint32_t lastSlashLoc = uint32_t(kNotFound);
  uint32_t pos = 0;
  uint32_t foundDots = 0;
  uint32_t foundColons = 0;
  uint32_t foundDigits = 0;
  uint32_t foundRSBrackets = 0;
  bool looksLikeIpv6 = true;
  bool hasAsciiAlpha = false;

  nsACString::const_iterator iterBegin;
  nsACString::const_iterator iterEnd;
  aURIString.BeginReading(iterBegin);
  aURIString.EndReading(iterEnd);
  nsACString::const_iterator iter = iterBegin;

  while (iter != iterEnd) {
    if (pos >= 1 && foundRSBrackets == 0) {
      if (!(lastLSBracketLoc == 0 &&
            (*iter == ':' ||
             *iter == '.' ||
             *iter == ']' ||
             (*iter >= 'a' && *iter <= 'f') ||
             (*iter >= 'A' && *iter <= 'F') ||
             nsCRT::IsAsciiDigit(*iter)))) {
        looksLikeIpv6 = false;
      }
    }

    
    
    if ((iter.size_forward() == 1 || (lastSlashLoc == uint32_t(kNotFound) && *iter == '/')) &&
        
        (foundDots == 2 || foundDots == 3) &&
        
        (foundDots + foundDigits == pos ||
         
         
         (foundColons == 1 && firstColonLoc > lastDotLoc &&
          foundDots + foundDigits + foundColons == pos))) {
      
      
      return NS_OK;
    }

    if (*iter == '.') {
      ++foundDots;
      lastDotLoc = pos;
      if (firstDotLoc == uint32_t(kNotFound)) {
        firstDotLoc = pos;
      }
    } else if (*iter == ':') {
      ++foundColons;
      if (firstColonLoc == uint32_t(kNotFound)) {
        firstColonLoc = pos;
      }
    } else if (*iter == ' ' && firstSpaceLoc == uint32_t(kNotFound)) {
      firstSpaceLoc = pos;
    } else if (*iter == '?' && firstQMarkLoc == uint32_t(kNotFound)) {
      firstQMarkLoc = pos;
    } else if ((*iter == '\'' || *iter == '"') &&
               firstQuoteLoc == uint32_t(kNotFound)) {
      firstQuoteLoc = pos;
    } else if (*iter == '[') {
      lastLSBracketLoc = pos;
    } else if (*iter == ']') {
      foundRSBrackets++;
    } else if (*iter == '/') {
      lastSlashLoc = pos;
    } else if (nsCRT::IsAsciiAlpha(*iter)) {
      hasAsciiAlpha = true;
    } else if (nsCRT::IsAsciiDigit(*iter)) {
      ++foundDigits;
    }

    pos++;
    iter++;
  }

  if (lastLSBracketLoc > 0 || foundRSBrackets != 1) {
    looksLikeIpv6 = false;
  }

  
  
  if (looksLikeIpv6) {
    return NS_OK;
  }

  nsAutoCString asciiHost;
  nsAutoCString host;

  bool isValidAsciiHost =
    aFixupInfo->mFixedURI &&
    NS_SUCCEEDED(aFixupInfo->mFixedURI->GetAsciiHost(asciiHost)) &&
    !asciiHost.IsEmpty();

  bool isValidHost =
    aFixupInfo->mFixedURI &&
    NS_SUCCEEDED(aFixupInfo->mFixedURI->GetHost(host)) &&
    !host.IsEmpty();

  nsresult rv = NS_OK;
  
  
  
  if (((firstSpaceLoc < firstDotLoc || firstQuoteLoc < firstDotLoc) &&
       (firstSpaceLoc < firstColonLoc || firstQuoteLoc < firstColonLoc) &&
       (firstSpaceLoc < firstQMarkLoc || firstQuoteLoc < firstQMarkLoc)) ||
      firstQMarkLoc == 0) {
    rv = TryKeywordFixupForURIInfo(aFixupInfo->mOriginalInput, aFixupInfo,
                                   aPostData);
    
    
  } else if (isValidAsciiHost && isValidHost && !hasAsciiAlpha &&
             host.EqualsIgnoreCase(asciiHost.get())) {
    if (!sDNSFirstForSingleWords) {
      rv = TryKeywordFixupForURIInfo(aFixupInfo->mOriginalInput, aFixupInfo,
                                     aPostData);
    }
  }
  
  
  else if ((firstDotLoc == uint32_t(kNotFound) ||
            (foundDots == 1 && (firstDotLoc == 0 ||
                                firstDotLoc == aURIString.Length() - 1))) &&
           firstColonLoc == uint32_t(kNotFound) &&
           firstQMarkLoc == uint32_t(kNotFound)) {
    if (isValidAsciiHost && IsDomainWhitelisted(asciiHost, firstDotLoc)) {
      return NS_OK;
    }

    
    
    if (firstDotLoc == uint32_t(kNotFound) &&
        lastSlashLoc != uint32_t(kNotFound) &&
        hasAsciiAlpha && isValidAsciiHost) {
      return NS_OK;
    }

    
    
    rv = TryKeywordFixupForURIInfo(aFixupInfo->mOriginalInput, aFixupInfo,
                                   aPostData);
  }
  return rv;
}

bool
nsDefaultURIFixup::IsDomainWhitelisted(const nsAutoCString aAsciiHost,
                                       const uint32_t aDotLoc)
{
  if (sDNSFirstForSingleWords) {
    return true;
  }
  
  
  
  

  nsAutoCString pref("browser.fixup.domainwhitelist.");

  if (aDotLoc == aAsciiHost.Length() - 1) {
    pref.Append(Substring(aAsciiHost, 0, aAsciiHost.Length() - 1));
  } else {
    pref.Append(aAsciiHost);
  }

  return Preferences::GetBool(pref.get(), false);
}


NS_IMPL_ISUPPORTS(nsDefaultURIFixupInfo, nsIURIFixupInfo)

nsDefaultURIFixupInfo::nsDefaultURIFixupInfo(const nsACString& aOriginalInput)
  : mFixupChangedProtocol(false)
  , mFixupCreatedAlternateURI(false)
{
  mOriginalInput = aOriginalInput;
}

nsDefaultURIFixupInfo::~nsDefaultURIFixupInfo()
{
}

NS_IMETHODIMP
nsDefaultURIFixupInfo::GetConsumer(nsISupports** aConsumer)
{
  *aConsumer = mConsumer;
  NS_IF_ADDREF(*aConsumer);
  return NS_OK;
}

NS_IMETHODIMP
nsDefaultURIFixupInfo::SetConsumer(nsISupports* aConsumer)
{
  mConsumer = aConsumer;
  return NS_OK;
}

NS_IMETHODIMP
nsDefaultURIFixupInfo::GetPreferredURI(nsIURI** aPreferredURI)
{
  *aPreferredURI = mPreferredURI;
  NS_IF_ADDREF(*aPreferredURI);
  return NS_OK;
}

NS_IMETHODIMP
nsDefaultURIFixupInfo::GetFixedURI(nsIURI** aFixedURI)
{
  *aFixedURI = mFixedURI;
  NS_IF_ADDREF(*aFixedURI);
  return NS_OK;
}

NS_IMETHODIMP
nsDefaultURIFixupInfo::GetKeywordProviderName(nsAString& aResult)
{
  aResult = mKeywordProviderName;
  return NS_OK;
}

NS_IMETHODIMP
nsDefaultURIFixupInfo::GetKeywordAsSent(nsAString& aResult)
{
  aResult = mKeywordAsSent;
  return NS_OK;
}

NS_IMETHODIMP
nsDefaultURIFixupInfo::GetFixupChangedProtocol(bool* aResult)
{
  *aResult = mFixupChangedProtocol;
  return NS_OK;
}

NS_IMETHODIMP
nsDefaultURIFixupInfo::GetFixupCreatedAlternateURI(bool* aResult)
{
  *aResult = mFixupCreatedAlternateURI;
  return NS_OK;
}

NS_IMETHODIMP
nsDefaultURIFixupInfo::GetOriginalInput(nsACString& aResult)
{
  aResult = mOriginalInput;
  return NS_OK;
}
