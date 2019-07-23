








































#include "nsConsoleMessage.h"
#include "nsReadableUtils.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsConsoleMessage, nsIConsoleMessage)

nsConsoleMessage::nsConsoleMessage() 
{
}

nsConsoleMessage::nsConsoleMessage(const PRUnichar *message) 
{
	mMessage.Assign(message);
}

NS_IMETHODIMP
nsConsoleMessage::GetMessageMoz(PRUnichar **result) {
    *result = ToNewUnicode(mMessage);

    return NS_OK;
}








