




#include "nsPluginPlayPreviewInfo.h"
#include "nsWildCard.h"

using namespace mozilla;

nsPluginPlayPreviewInfo::nsPluginPlayPreviewInfo(const char* aMimeType,
                                                 bool aIgnoreCTP,
                                                 const char* aRedirectURL,
                                                 const char* aWhitelist)
  : mMimeType(aMimeType), mIgnoreCTP(aIgnoreCTP), mRedirectURL(aRedirectURL),
    mWhitelist(aWhitelist) {}

nsPluginPlayPreviewInfo::nsPluginPlayPreviewInfo(
  const nsPluginPlayPreviewInfo* aSource)
{
  MOZ_ASSERT(aSource);

  mMimeType = aSource->mMimeType;
  mIgnoreCTP = aSource->mIgnoreCTP;
  mRedirectURL = aSource->mRedirectURL;
  mWhitelist = aSource->mWhitelist;
}

nsPluginPlayPreviewInfo::~nsPluginPlayPreviewInfo()
{
}

NS_IMPL_ISUPPORTS(nsPluginPlayPreviewInfo, nsIPluginPlayPreviewInfo)

NS_IMETHODIMP
nsPluginPlayPreviewInfo::GetMimeType(nsACString& aMimeType)
{
  aMimeType = mMimeType;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginPlayPreviewInfo::GetIgnoreCTP(bool* aIgnoreCTP)
{
  *aIgnoreCTP = mIgnoreCTP;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginPlayPreviewInfo::GetRedirectURL(nsACString& aRedirectURL)
{
  aRedirectURL = mRedirectURL;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginPlayPreviewInfo::GetWhitelist(nsACString& aWhitelist)
{
  aWhitelist = mWhitelist;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginPlayPreviewInfo::CheckWhitelist(const nsACString& aPageURI,
                                        const nsACString& aObjectURI,
                                        bool *_retval)
{
  if (mWhitelist.Length() == 0) {
    
    *_retval = true;
    return NS_OK;
  }

  
  
  
  
  nsACString::const_iterator start, end;
  mWhitelist.BeginReading(start);
  mWhitelist.EndReading(end);

  nsAutoCString pageURI(aPageURI);
  nsAutoCString objectURI(aObjectURI);
  nsACString::const_iterator pos = start, entryStart, entryEnd;
  nsACString::const_iterator pagePatternStart, pagePatternEnd;
  nsACString::const_iterator objectPatternStart, objectPatternEnd;
  int matchResult;
  bool matched, didMatching;
  while (pos != end) {
    matched = true;
    didMatching = false;
    entryStart = pos;
    
    while (pos != end && *pos != ',') {
      pos++;
    }
    entryEnd = pos;
    if (entryStart != entryEnd && *entryStart == '@') {
      
      pagePatternStart = entryStart;
      pagePatternStart++;
      pagePatternEnd = pagePatternStart;
      while (pagePatternEnd != entryEnd && *pagePatternEnd != ' ') {
        pagePatternEnd++;
      }
      nsAutoCString pagePattern(Substring(pagePatternStart, pagePatternEnd));
      matchResult = NS_WildCardMatch(pageURI.get(), pagePattern.get(), true);
      matched &= matchResult == MATCH;
      didMatching = true;
      objectPatternStart = pagePatternEnd;
    } else {
      objectPatternStart = entryStart;
    }
    while (objectPatternStart != entryEnd && *objectPatternStart == ' ') {
      objectPatternStart++;
    }
    if (objectPatternStart != entryEnd) {
      
      objectPatternEnd = entryEnd;
      --objectPatternEnd;
      while (objectPatternStart != objectPatternEnd &&
             *objectPatternEnd == ' ') {
        objectPatternEnd--;
      };
      objectPatternEnd++;
      nsAutoCString objectPattern(Substring(objectPatternStart,
                                            objectPatternEnd));
      matchResult = NS_WildCardMatch(objectURI.get(), objectPattern.get(), true);
      matched &= matchResult == MATCH;
      didMatching = true;
    }
    
    if (didMatching && matched) {
      *_retval = true;
      return NS_OK;
    }
    if (pos == end) {
      break;
    }
    pos++;
  }

  *_retval = false;
  return NS_OK;
}
