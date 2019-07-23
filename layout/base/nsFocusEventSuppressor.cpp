




































#include "nsIFocusEventSuppressor.h"
#include "nsTArray.h"

class nsFocusEventSuppressorService : public nsIFocusEventSuppressorService
{
public:
  NS_DECL_ISUPPORTS
  virtual void AddObserverCallback(nsFocusEventSuppressorCallback aCallback)
  {
    NS_AddFocusSuppressorCallback(aCallback);
  }
  virtual void Suppress()
  {
    NS_SuppressFocusEvent();
  }
  virtual void Unsuppress()
  {
    NS_UnsuppressFocusEvent();
  }
};

static nsTArray<nsFocusEventSuppressorCallback>* sCallbacks = nsnull;
static PRUint32 sFocusSuppressCount = 0;

NS_IMPL_ADDREF(nsFocusEventSuppressorService)
NS_IMPL_RELEASE(nsFocusEventSuppressorService)

NS_INTERFACE_MAP_BEGIN(nsFocusEventSuppressorService)
  NS_INTERFACE_MAP_ENTRY(nsIFocusEventSuppressorService)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

void
NS_AddFocusSuppressorCallback(nsFocusEventSuppressorCallback aCallback)
{
  if (aCallback) {
    if (!sCallbacks) {
      sCallbacks = new nsTArray<nsFocusEventSuppressorCallback>(2);
      if (!sCallbacks) {
        NS_WARNING("Out of memory!");
        return;
      }
    } else if (sCallbacks->Contains(aCallback)) {
      return;
    }
    sCallbacks->AppendElement(aCallback);
  }
}

void
NS_SuppressFocusEvent()
{
  ++sFocusSuppressCount;
  if (sFocusSuppressCount == 1 && sCallbacks) {
    for (PRUint32 i = 0; i < sCallbacks->Length(); ++i) {
      sCallbacks->ElementAt(i)(PR_TRUE);
    }
  }
}

void
NS_UnsuppressFocusEvent()
{
  --sFocusSuppressCount;
  if (sFocusSuppressCount == 0 && sCallbacks) {
    for (PRUint32 i = 0; i < sCallbacks->Length(); ++i) {
      sCallbacks->ElementAt(i)(PR_FALSE);
    }
  }
}

void
NS_ShutdownFocusSuppressor()
{
  delete sCallbacks;
  sCallbacks = nsnull;
}

nsresult
NS_NewFocusEventSuppressorService(nsIFocusEventSuppressorService** aResult)
{
  nsIFocusEventSuppressorService* it = new nsFocusEventSuppressorService();
  NS_ENSURE_TRUE(it, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aResult = it);
  return NS_OK;
}
