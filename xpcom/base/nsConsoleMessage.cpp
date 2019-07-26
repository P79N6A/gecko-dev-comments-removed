








#include "nsConsoleMessage.h"
#include "nsReadableUtils.h"
#include "jsapi.h"
#include "nsGlobalWindow.h"
#include "nsPIDOMWindow.h"
#include "nsILoadContext.h"
#include "nsIDocShell.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsConsoleMessage, nsIConsoleMessage)

nsConsoleMessage::nsConsoleMessage()
    :  mMessage(),
       mCategory(),
       mOuterWindowID(0),
       mInnerWindowID(0),
       mTimeStamp(0),
       mIsFromPrivateWindow(false)
{
}

nsConsoleMessage::nsConsoleMessage(const PRUnichar *message)
{
  nsString m;
  m.Assign(message);
  InitMessage(m, nullptr, 0);
}

NS_IMETHODIMP
nsConsoleMessage::InitMessage(const nsAString& message,
                              const char *category,
                              uint64_t innerWindowID)
{
  mTimeStamp = JS_Now() / 1000;
  mMessage.Assign(message);
  mCategory.Assign(category);
  mInnerWindowID = innerWindowID;

  if (innerWindowID) {
    nsGlobalWindow* window = nsGlobalWindow::GetInnerWindowWithId(innerWindowID);
    if (window) {
      nsPIDOMWindow* outer = window->GetOuterWindow();
      if (outer)
        mOuterWindowID = outer->WindowID();

      nsIDocShell* docShell = window->GetDocShell();
      nsCOMPtr<nsILoadContext> loadContext = do_QueryInterface(docShell);

      if (loadContext) {
        
        
        nsIPrincipal* winPrincipal = window->GetPrincipal();
        mIsFromPrivateWindow = loadContext->UsePrivateBrowsing() &&
                               !nsContentUtils::IsSystemPrincipal(winPrincipal);
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsConsoleMessage::GetMessageMoz(PRUnichar **result)
{
  *result = ToNewUnicode(mMessage);

  return NS_OK;
}

NS_IMETHODIMP
nsConsoleMessage::GetCategory(char **result)
{
  *result = ToNewCString(mCategory);
  return NS_OK;
}

NS_IMETHODIMP
nsConsoleMessage::GetOuterWindowID(uint64_t *aOuterWindowID)
{
  *aOuterWindowID = mOuterWindowID;
  return NS_OK;
}

NS_IMETHODIMP
nsConsoleMessage::GetInnerWindowID(uint64_t *aInnerWindowID)
{
  *aInnerWindowID = mInnerWindowID;
  return NS_OK;
}

NS_IMETHODIMP
nsConsoleMessage::GetTimeStamp(int64_t *aTimeStamp)
{
  *aTimeStamp = mTimeStamp;
  return NS_OK;
}

NS_IMETHODIMP
nsConsoleMessage::GetIsFromPrivateWindow(bool *aIsFromPrivateWindow)
{
  *aIsFromPrivateWindow = mIsFromPrivateWindow;
  return NS_OK;
}

NS_IMETHODIMP
nsConsoleMessage::ToString(nsACString&  aResult)
{
  if (!mCategory.IsEmpty())
    aResult.Assign(mCategory + NS_LITERAL_CSTRING(": ") +
                   NS_ConvertUTF16toUTF8(mMessage));
  else
    CopyUTF16toUTF8(mMessage, aResult);

  return NS_OK;
}
