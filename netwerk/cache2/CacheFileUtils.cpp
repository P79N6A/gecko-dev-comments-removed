



#include "CacheLog.h"
#include "CacheFileUtils.h"
#include "LoadContextInfo.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsString.h"


namespace mozilla {
namespace net {
namespace CacheFileUtils {

namespace { 




class KeyParser
{
public:
  KeyParser(nsACString::const_iterator aCaret, nsACString::const_iterator aEnd)
    : caret(aCaret)
    , end(aEnd)
    
    , appId(nsILoadContextInfo::NO_APP_ID)
    , isPrivate(false)
    , isInBrowser(false)
    , isAnonymous(false)
    
    , cacheKey(aEnd)
    , lastTag(0)
  {
  }

private:
  
  nsACString::const_iterator caret;
  
  nsACString::const_iterator const end;

  
  uint32_t appId;
  bool isPrivate;
  bool isInBrowser;
  bool isAnonymous;
  nsCString idEnhance;
  
  nsACString::const_iterator cacheKey;

  
  char lastTag;

  bool ParseTags()
  {
    
    if (caret == end)
      return true;

    
    char const tag = *caret++;
    
    if (!(lastTag < tag || tag == ':'))
      return false;

    lastTag = tag;

    switch (tag) {
    case ':':
      
      
      cacheKey = caret;
      caret = end;
      return true;
    case 'p':
      isPrivate = true;
      break;
    case 'b':
      isInBrowser = true;
      break;
    case 'a':
      isAnonymous = true;
      break;
    case 'i': {
      nsAutoCString appIdString;
      if (!ParseValue(&appIdString))
        return false;

      nsresult rv;
      int64_t appId64 = appIdString.ToInteger64(&rv);
      if (NS_FAILED(rv))
        return false; 
      if (appId64 < 0 || appId64 > PR_UINT32_MAX)
        return false; 
      appId = static_cast<uint32_t>(appId64);

      break;
    }
    case '~':
      if (!ParseValue(&idEnhance))
        return false;
      break;
    default:
      if (!ParseValue()) 
        return false;
      break;
    }

    
    return ParseNextTagOrEnd();
  }

  bool ParseNextTagOrEnd()
  {
    
    if (caret == end || *caret++ != ',')
      return false;

    
    return ParseTags();
  }

  bool ParseValue(nsACString * result = nullptr)
  {
    
    if (caret == end)
      return false;

    
    nsACString::const_iterator val = caret;
    nsACString::const_iterator comma = end;
    bool escape = false;
    while (caret != end) {
      nsACString::const_iterator at = caret;
      ++caret; 

      if (*at == ',') {
        if (comma != end) {
          
          comma = end;
          escape = true;
        } else {
          comma = at;
        }
        continue;
      }

      if (comma != end) {
        
        break;
      }
    }

    
    
    

    caret = comma;
    if (result) {
      if (escape) {
        
        nsAutoCString _result(Substring(val, caret));
        _result.ReplaceSubstring(NS_LITERAL_CSTRING(",,"), NS_LITERAL_CSTRING(","));
        result->Assign(_result);
      } else {
        result->Assign(Substring(val, caret));
      }
    }

    return caret != end;
  }

public:
  already_AddRefed<LoadContextInfo> Parse()
  {
    nsRefPtr<LoadContextInfo> info;
    if (ParseTags())
      info = GetLoadContextInfo(isPrivate, appId, isInBrowser, isAnonymous);

    return info.forget();
  }

  void URISpec(nsACString &result)
  {
    
    result.Assign(Substring(cacheKey, end));
  }

  void IdEnhance(nsACString &result)
  {
    result.Assign(idEnhance);
  }
};

} 

already_AddRefed<nsILoadContextInfo>
ParseKey(const nsCSubstring &aKey,
         nsCSubstring *aIdEnhance,
         nsCSubstring *aURISpec)
{
  nsACString::const_iterator caret, end;
  aKey.BeginReading(caret);
  aKey.EndReading(end);

  KeyParser parser(caret, end);
  nsRefPtr<LoadContextInfo> info = parser.Parse();

  if (info) {
    if (aIdEnhance)
      parser.IdEnhance(*aIdEnhance);
    if (aURISpec)
      parser.URISpec(*aURISpec);
  }

  return info.forget();
}

void
AppendKeyPrefix(nsILoadContextInfo* aInfo, nsACString &_retval)
{
  







  if (aInfo->IsAnonymous()) {
    _retval.Append(NS_LITERAL_CSTRING("a,"));
  }

  if (aInfo->IsInBrowserElement()) {
    _retval.Append(NS_LITERAL_CSTRING("b,"));
  }

  if (aInfo->AppId() != nsILoadContextInfo::NO_APP_ID) {
    _retval.Append('i');
    _retval.AppendInt(aInfo->AppId());
    _retval.Append(',');
  }

  if (aInfo->IsPrivate()) {
    _retval.Append(NS_LITERAL_CSTRING("p,"));
  }
}

void
AppendTagWithValue(nsACString & aTarget, char const aTag, nsCSubstring const & aValue)
{
  aTarget.Append(aTag);

  
  
  if (!aValue.IsEmpty()) {
    if (aValue.FindChar(',') == kNotFound) {
      
      aTarget.Append(aValue);
    } else {
      nsAutoCString escapedValue(aValue);
      escapedValue.ReplaceSubstring(
        NS_LITERAL_CSTRING(","), NS_LITERAL_CSTRING(",,"));
      aTarget.Append(escapedValue);
    }
  }

  aTarget.Append(',');
}

} 
} 
} 
