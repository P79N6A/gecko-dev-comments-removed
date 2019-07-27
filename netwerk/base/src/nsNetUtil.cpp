





#include "nsNetUtil.h"
#include "nsHttp.h"

bool NS_IsReasonableHTTPHeaderValue(const nsACString& aValue)
{
  return mozilla::net::nsHttp::IsReasonableHeaderValue(aValue);
}

bool NS_IsValidHTTPToken(const nsACString& aToken)
{
  return mozilla::net::nsHttp::IsValidToken(aToken);
}
