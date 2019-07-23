







































#include "prlog.h"
#include "nsStyleSheetService.h"
#include "nsIStyleSheet.h"
#include "nsICSSLoader.h"
#include "nsICSSStyleSheet.h"
#include "nsIURI.h"
#include "nsContentCID.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsISupportsPrimitives.h"
#include "nsNetUtil.h"
#include "nsIObserverService.h"

static NS_DEFINE_CID(kCSSLoaderCID, NS_CSS_LOADER_CID);

nsStyleSheetService *nsStyleSheetService::gInstance = nsnull;

nsStyleSheetService::nsStyleSheetService()
{
  PR_STATIC_ASSERT(0 == AGENT_SHEET && 1 == USER_SHEET);
  NS_ASSERTION(!gInstance, "Someone is using CreateInstance instead of GetService");
  gInstance = this;
  nsLayoutStatics::AddRef();
}

nsStyleSheetService::~nsStyleSheetService()
{
  gInstance = nsnull;
  nsLayoutStatics::Release();
}

NS_IMPL_ISUPPORTS1(nsStyleSheetService, nsIStyleSheetService)

void
nsStyleSheetService::RegisterFromEnumerator(nsICategoryManager  *aManager,
                                            const char          *aCategory,
                                            nsISimpleEnumerator *aEnumerator,
                                            PRUint32             aSheetType)
{
  if (!aEnumerator)
    return;

  PRBool hasMore;
  while (NS_SUCCEEDED(aEnumerator->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> element;
    if (NS_FAILED(aEnumerator->GetNext(getter_AddRefs(element))))
      break;

    nsCOMPtr<nsISupportsCString> icStr = do_QueryInterface(element);
    NS_ASSERTION(icStr,
                 "category manager entries must be nsISupportsCStrings");

    nsCAutoString name;
    icStr->GetData(name);

    nsXPIDLCString spec;
    aManager->GetCategoryEntry(aCategory, name.get(), getter_Copies(spec));

    nsCOMPtr<nsIURI> uri;
    NS_NewURI(getter_AddRefs(uri), spec);
    if (uri)
      LoadAndRegisterSheetInternal(uri, aSheetType);
  }
}

PRInt32
nsStyleSheetService::FindSheetByURI(const nsCOMArray<nsIStyleSheet> &sheets,
                                    nsIURI *sheetURI)
{
  for (PRInt32 i = sheets.Count() - 1; i >= 0; i-- ) {
    PRBool bEqual;
    nsCOMPtr<nsIURI> uri;
    if (NS_SUCCEEDED(sheets[i]->GetSheetURI(getter_AddRefs(uri)))
        && uri
        && NS_SUCCEEDED(uri->Equals(sheetURI, &bEqual))
        && bEqual) {
      return i;
    }
  }

  return -1;
}

nsresult
nsStyleSheetService::Init()
{
  
  

  nsCOMPtr<nsICategoryManager> catMan =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID);

  NS_ENSURE_TRUE(catMan, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsISimpleEnumerator> sheets;
  catMan->EnumerateCategory("agent-style-sheets", getter_AddRefs(sheets));
  RegisterFromEnumerator(catMan, "agent-style-sheets", sheets, AGENT_SHEET);

  catMan->EnumerateCategory("user-style-sheets", getter_AddRefs(sheets));
  RegisterFromEnumerator(catMan, "user-style-sheets", sheets, USER_SHEET);

  return NS_OK;
}

NS_IMETHODIMP
nsStyleSheetService::LoadAndRegisterSheet(nsIURI *aSheetURI,
                                          PRUint32 aSheetType)
{
  nsresult rv = LoadAndRegisterSheetInternal(aSheetURI, aSheetType);
  if (NS_SUCCEEDED(rv)) {
    const char* message = (aSheetType == AGENT_SHEET) ?
      "agent-sheet-added" : "user-sheet-added";
    nsCOMPtr<nsIObserverService> serv =
      do_GetService("@mozilla.org/observer-service;1");
    if (serv) {
      
      
      const nsCOMArray<nsIStyleSheet> & sheets = mSheets[aSheetType];
      serv->NotifyObservers(sheets[sheets.Count() - 1], message, nsnull);
    }
  }
  return rv;
}

nsresult
nsStyleSheetService::LoadAndRegisterSheetInternal(nsIURI *aSheetURI,
                                                  PRUint32 aSheetType)
{
  NS_ENSURE_ARG(aSheetType == AGENT_SHEET || aSheetType == USER_SHEET);
  NS_ENSURE_ARG_POINTER(aSheetURI);

  nsCOMPtr<nsICSSLoader> loader = do_CreateInstance(kCSSLoaderCID);
  nsCOMPtr<nsICSSStyleSheet> sheet;
  
  nsresult rv = loader->LoadSheetSync(aSheetURI, aSheetType == AGENT_SHEET,
                                      PR_TRUE, getter_AddRefs(sheet));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mSheets[aSheetType].AppendObject(sheet)) {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }

  return rv;
}

NS_IMETHODIMP
nsStyleSheetService::SheetRegistered(nsIURI *sheetURI,
                                     PRUint32 aSheetType, PRBool *_retval)
{
  NS_ENSURE_ARG(aSheetType == AGENT_SHEET || aSheetType == USER_SHEET);
  NS_ENSURE_ARG_POINTER(sheetURI);
  NS_PRECONDITION(_retval, "Null out param");

  *_retval = (FindSheetByURI(mSheets[aSheetType], sheetURI) >= 0);

  return NS_OK;
}

NS_IMETHODIMP
nsStyleSheetService::UnregisterSheet(nsIURI *sheetURI, PRUint32 aSheetType)
{
  NS_ENSURE_ARG(aSheetType == AGENT_SHEET || aSheetType == USER_SHEET);
  NS_ENSURE_ARG_POINTER(sheetURI);

  PRInt32 foundIndex = FindSheetByURI(mSheets[aSheetType], sheetURI);
  NS_ENSURE_TRUE(foundIndex >= 0, NS_ERROR_INVALID_ARG);
  nsCOMPtr<nsIStyleSheet> sheet = mSheets[aSheetType][foundIndex];
  mSheets[aSheetType].RemoveObjectAt(foundIndex);
  
  const char* message = (aSheetType == AGENT_SHEET) ?
      "agent-sheet-removed" : "user-sheet-removed";
  nsCOMPtr<nsIObserverService> serv =
    do_GetService("@mozilla.org/observer-service;1");
  if (serv) {
    serv->NotifyObservers(sheet, message, nsnull);
  }
  
  return NS_OK;
}
