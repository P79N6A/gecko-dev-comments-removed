








#include "nsConsoleMessage.h"
#include "jsapi.h"

NS_IMPL_ISUPPORTS1(nsConsoleMessage, nsIConsoleMessage)

nsConsoleMessage::nsConsoleMessage()
    :  mTimeStamp(0),
       mMessage()
{
}

nsConsoleMessage::nsConsoleMessage(const PRUnichar *message)
{
  mTimeStamp = JS_Now() / 1000;
  mMessage.Assign(message);
}

NS_IMETHODIMP
nsConsoleMessage::GetMessageMoz(PRUnichar **result)
{
  *result = ToNewUnicode(mMessage);

  return NS_OK;
}

NS_IMETHODIMP
nsConsoleMessage::GetTimeStamp(int64_t *aTimeStamp)
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
