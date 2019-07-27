









#include "nsConsoleMessage.h"
#include "jsapi.h"

NS_IMPL_ISUPPORTS(nsConsoleMessage, nsIConsoleMessage)

nsConsoleMessage::nsConsoleMessage()
  : mTimeStamp(0)
  , mMessage()
{
}

nsConsoleMessage::nsConsoleMessage(const char16_t* aMessage)
{
  mTimeStamp = JS_Now() / 1000;
  mMessage.Assign(aMessage);
}

NS_IMETHODIMP
nsConsoleMessage::GetMessageMoz(char16_t** aResult)
{
  *aResult = ToNewUnicode(mMessage);

  return NS_OK;
}

NS_IMETHODIMP
nsConsoleMessage::GetLogLevel(uint32_t* aLogLevel)
{
  *aLogLevel = nsConsoleMessage::info;
  return NS_OK;
}

NS_IMETHODIMP
nsConsoleMessage::GetTimeStamp(int64_t* aTimeStamp)
{
  *aTimeStamp = mTimeStamp;
  return NS_OK;
}

NS_IMETHODIMP
nsConsoleMessage::ToString(nsACString&  aResult)
{
  CopyUTF16toUTF8(mMessage, aResult);

  return NS_OK;
}
