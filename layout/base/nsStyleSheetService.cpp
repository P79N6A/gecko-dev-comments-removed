







#include "nsStyleSheetService.h"
#include "nsIStyleSheet.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/unused.h"
#include "mozilla/css/Loader.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/ipc/URIUtils.h"
#include "nsCSSStyleSheet.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsICategoryManager.h"
#include "nsISupportsPrimitives.h"
#include "nsNetUtil.h"
#include "nsIObserverService.h"
#include "nsLayoutStatics.h"
#include "nsIMemoryReporter.h"

using namespace mozilla;

class LayoutStyleSheetServiceReporter MOZ_FINAL
  : public mozilla::MemoryUniReporter
{
public:
  LayoutStyleSheetServiceReporter()
    : MemoryUniReporter("explicit/layout/style-sheet-service",
                         KIND_HEAP, UNITS_BYTES,
"Memory used for style sheets held by the style sheet service.")
  {}
private:
  int64_t Amount() MOZ_OVERRIDE
  {
    return nsStyleSheetService::gInstance
         ? nsStyleSheetService::gInstance->SizeOfIncludingThis(MallocSizeOf)
         : 0;
  }
};

nsStyleSheetService *nsStyleSheetService::gInstance = nullptr;

nsStyleSheetService::nsStyleSheetService()
{
  PR_STATIC_ASSERT(0 == AGENT_SHEET && 1 == USER_SHEET && 2 == AUTHOR_SHEET);
  NS_ASSERTION(!gInstance, "Someone is using CreateInstance instead of GetService");
  gInstance = this;
  nsLayoutStatics::AddRef();

  mReporter = new LayoutStyleSheetServiceReporter();
  NS_RegisterMemoryReporter(mReporter);
}

nsStyleSheetService::~nsStyleSheetService()
{
  NS_UnregisterMemoryReporter(mReporter);

  gInstance = nullptr;
  nsLayoutStatics::Release();
}

NS_IMPL_ISUPPORTS1(nsStyleSheetService, nsIStyleSheetService)

void
nsStyleSheetService::RegisterFromEnumerator(nsICategoryManager  *aManager,
                                            const char          *aCategory,
                                            nsISimpleEnumerator *aEnumerator,
                                            uint32_t             aSheetType)
{
  if (!aEnumerator)
    return;

  bool hasMore;
  while (NS_SUCCEEDED(aEnumerator->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> element;
    if (NS_FAILED(aEnumerator->GetNext(getter_AddRefs(element))))
      break;

    nsCOMPtr<nsISupportsCString> icStr = do_QueryInterface(element);
    NS_ASSERTION(icStr,
                 "category manager entries must be nsISupportsCStrings");

    nsAutoCString name;
    icStr->GetData(name);

    nsXPIDLCString spec;
    aManager->GetCategoryEntry(aCategory, name.get(), getter_Copies(spec));

    nsCOMPtr<nsIURI> uri;
    NS_NewURI(getter_AddRefs(uri), spec);
    if (uri)
      LoadAndRegisterSheetInternal(uri, aSheetType);
  }
}

int32_t
nsStyleSheetService::FindSheetByURI(const nsCOMArray<nsIStyleSheet> &sheets,
                                    nsIURI *sheetURI)
{
  for (int32_t i = sheets.Count() - 1; i >= 0; i-- ) {
    bool bEqual;
    nsIURI* uri = sheets[i]->GetSheetURI();
    if (uri
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
  
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    return NS_OK;
  }

  
  

  nsCOMPtr<nsICategoryManager> catMan =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID);

  NS_ENSURE_TRUE(catMan, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsISimpleEnumerator> sheets;
  catMan->EnumerateCategory("agent-style-sheets", getter_AddRefs(sheets));
  RegisterFromEnumerator(catMan, "agent-style-sheets", sheets, AGENT_SHEET);

  catMan->EnumerateCategory("user-style-sheets", getter_AddRefs(sheets));
  RegisterFromEnumerator(catMan, "user-style-sheets", sheets, USER_SHEET);

  catMan->EnumerateCategory("author-style-sheets", getter_AddRefs(sheets));
  RegisterFromEnumerator(catMan, "author-style-sheets", sheets, AUTHOR_SHEET);

  return NS_OK;
}

NS_IMETHODIMP
nsStyleSheetService::LoadAndRegisterSheet(nsIURI *aSheetURI,
                                          uint32_t aSheetType)
{
  nsresult rv = LoadAndRegisterSheetInternal(aSheetURI, aSheetType);
  if (NS_SUCCEEDED(rv)) {
    const char* message;
    switch (aSheetType) {
      case AGENT_SHEET:
        message = "agent-sheet-added";
        break;
      case USER_SHEET:
        message = "user-sheet-added";
        break;
      case AUTHOR_SHEET:
        message = "author-sheet-added";
        break;
      default:
        return NS_ERROR_INVALID_ARG;
    }
    nsCOMPtr<nsIObserverService> serv = services::GetObserverService();
    if (serv) {
      
      
      const nsCOMArray<nsIStyleSheet> & sheets = mSheets[aSheetType];
      serv->NotifyObservers(sheets[sheets.Count() - 1], message, nullptr);
    }

    if (XRE_GetProcessType() == GeckoProcessType_Default) {
      nsTArray<dom::ContentParent*> children;
      dom::ContentParent::GetAll(children);

      if (children.IsEmpty()) {
        return rv;
      }

      ipc::URIParams uri;
      SerializeURI(aSheetURI, uri);

      for (uint32_t i = 0; i < children.Length(); i++) {
        unused << children[i]->SendLoadAndRegisterSheet(uri, aSheetType);
      }
    }
  }
  return rv;
}

nsresult
nsStyleSheetService::LoadAndRegisterSheetInternal(nsIURI *aSheetURI,
                                                  uint32_t aSheetType)
{
  NS_ENSURE_ARG(aSheetType == AGENT_SHEET ||
                aSheetType == USER_SHEET ||
                aSheetType == AUTHOR_SHEET);
  NS_ENSURE_ARG_POINTER(aSheetURI);

  nsRefPtr<css::Loader> loader = new css::Loader();

  nsRefPtr<nsCSSStyleSheet> sheet;
  
  nsresult rv = loader->LoadSheetSync(aSheetURI, aSheetType == AGENT_SHEET,
                                      true, getter_AddRefs(sheet));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mSheets[aSheetType].AppendObject(sheet)) {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }

  return rv;
}

NS_IMETHODIMP
nsStyleSheetService::SheetRegistered(nsIURI *sheetURI,
                                     uint32_t aSheetType, bool *_retval)
{
  NS_ENSURE_ARG(aSheetType == AGENT_SHEET ||
                aSheetType == USER_SHEET ||
                aSheetType == AUTHOR_SHEET);
  NS_ENSURE_ARG_POINTER(sheetURI);
  NS_PRECONDITION(_retval, "Null out param");

  *_retval = (FindSheetByURI(mSheets[aSheetType], sheetURI) >= 0);

  return NS_OK;
}

NS_IMETHODIMP
nsStyleSheetService::UnregisterSheet(nsIURI *aSheetURI, uint32_t aSheetType)
{
  NS_ENSURE_ARG(aSheetType == AGENT_SHEET ||
                aSheetType == USER_SHEET ||
                aSheetType == AUTHOR_SHEET);
  NS_ENSURE_ARG_POINTER(aSheetURI);

  int32_t foundIndex = FindSheetByURI(mSheets[aSheetType], aSheetURI);
  NS_ENSURE_TRUE(foundIndex >= 0, NS_ERROR_INVALID_ARG);
  nsCOMPtr<nsIStyleSheet> sheet = mSheets[aSheetType][foundIndex];
  mSheets[aSheetType].RemoveObjectAt(foundIndex);

  const char* message;
  switch (aSheetType) {
    case AGENT_SHEET:
      message = "agent-sheet-removed";
      break;
    case USER_SHEET:
      message = "user-sheet-removed";
      break;
    case AUTHOR_SHEET:
      message = "author-sheet-removed";
      break;
  }

  nsCOMPtr<nsIObserverService> serv = services::GetObserverService();
  if (serv)
    serv->NotifyObservers(sheet, message, nullptr);

  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    nsTArray<dom::ContentParent*> children;
    dom::ContentParent::GetAll(children);

    if (children.IsEmpty()) {
      return NS_OK;
    }

    ipc::URIParams uri;
    SerializeURI(aSheetURI, uri);

    for (uint32_t i = 0; i < children.Length(); i++) {
      unused << children[i]->SendUnregisterSheet(uri, aSheetType);
    }
  }

  return NS_OK;
}


nsStyleSheetService *
nsStyleSheetService::GetInstance()
{
  static bool first = true;
  if (first) {
    
    nsCOMPtr<nsIStyleSheetService> dummy =
      do_GetService(NS_STYLESHEETSERVICE_CONTRACTID);
    first = false;
  }

  return gInstance;
}

static size_t
SizeOfElementIncludingThis(nsIStyleSheet* aElement,
                           MallocSizeOf aMallocSizeOf, void *aData)
{
    return aElement->SizeOfIncludingThis(aMallocSizeOf);
}

size_t
nsStyleSheetService::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);
  n += mSheets[AGENT_SHEET].SizeOfExcludingThis(SizeOfElementIncludingThis,
                                                aMallocSizeOf);
  n += mSheets[USER_SHEET].SizeOfExcludingThis(SizeOfElementIncludingThis,
                                               aMallocSizeOf);
  n += mSheets[AUTHOR_SHEET].SizeOfExcludingThis(SizeOfElementIncludingThis,
                                                 aMallocSizeOf);
  return n;
}


