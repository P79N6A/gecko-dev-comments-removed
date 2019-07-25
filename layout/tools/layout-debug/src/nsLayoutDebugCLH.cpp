






































#include "nsLayoutDebugCLH.h"
#include "nsString.h"
#include "plstr.h"
#include "nsCOMPtr.h"
#include "nsIWindowWatcher.h"
#include "nsIServiceManager.h"
#include "nsIDOMWindow.h"
#include "nsISupportsArray.h"
#include "nsISupportsPrimitives.h"
#include "nsICommandLine.h"

nsLayoutDebugCLH::nsLayoutDebugCLH()
{
}

nsLayoutDebugCLH::~nsLayoutDebugCLH()
{
}

NS_IMPL_ISUPPORTS1(nsLayoutDebugCLH, ICOMMANDLINEHANDLER)

NS_IMETHODIMP
nsLayoutDebugCLH::Handle(nsICommandLine* aCmdLine)
{
    nsresult rv;

    PRInt32 idx;
    rv = aCmdLine->FindFlag(NS_LITERAL_STRING("layoutdebug"), PR_FALSE, &idx);
    NS_ENSURE_SUCCESS(rv, rv);
    if (idx < 0)
      return NS_OK;

    PRInt32 length;
    aCmdLine->GetLength(&length);

    nsAutoString url;
    if (idx + 1 < length) {
        rv = aCmdLine->GetArgument(idx + 1, url);
        NS_ENSURE_SUCCESS(rv, rv);
        if (!url.IsEmpty() && url.CharAt(0) == '-')
            url.Truncate();
    }

    aCmdLine->RemoveArguments(idx, idx + !url.IsEmpty());

    nsCOMPtr<nsISupportsArray> argsArray =
        do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!url.IsEmpty())
    {
        nsCOMPtr<nsISupportsString> scriptableURL =
            do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
        NS_ENSURE_TRUE(scriptableURL, NS_ERROR_FAILURE);
  
        scriptableURL->SetData(url);
        argsArray->AppendElement(scriptableURL);
    }

    nsCOMPtr<nsIWindowWatcher> wwatch =
        do_GetService(NS_WINDOWWATCHER_CONTRACTID);
    NS_ENSURE_TRUE(wwatch, NS_ERROR_FAILURE);

    nsCOMPtr<nsIDOMWindow> opened;
    wwatch->OpenWindow(nsnull, "chrome://layoutdebug/content/",
                       "_blank", "chrome,dialog=no,all", argsArray,
                       getter_AddRefs(opened));
    aCmdLine->SetPreventDefault(PR_TRUE);
    return NS_OK;
}

NS_IMETHODIMP
nsLayoutDebugCLH::GetHelpInfo(nsACString& aResult)
{
    aResult.Assign(NS_LITERAL_CSTRING("  -layoutdebug [<url>] Start with Layout Debugger\n"));
    return NS_OK;
}

