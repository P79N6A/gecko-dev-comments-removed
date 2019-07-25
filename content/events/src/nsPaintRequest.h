




#ifndef NSPAINTREQUEST_H_
#define NSPAINTREQUEST_H_

#include "nsIDOMPaintRequest.h"
#include "nsIDOMPaintRequestList.h"
#include "nsPresContext.h"
#include "nsIDOMEvent.h"
#include "dombindings.h"
#include "mozilla/Attributes.h"

class nsPaintRequest MOZ_FINAL : public nsIDOMPaintRequest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMPAINTREQUEST

  nsPaintRequest() { mRequest.mFlags = 0; }

  void SetRequest(const nsInvalidateRequestList::Request& aRequest)
  { mRequest = aRequest; }

private:
  ~nsPaintRequest() {}

  nsInvalidateRequestList::Request mRequest;
};

class nsPaintRequestList MOZ_FINAL : public nsIDOMPaintRequestList,
                                     public nsWrapperCache
{
public:
  nsPaintRequestList(nsIDOMEvent *aParent) : mParent(aParent)
  {
    SetIsDOMBinding();
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsPaintRequestList)
  NS_DECL_NSIDOMPAINTREQUESTLIST
  
  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope,
                               bool *triedToWrap)
  {
    return mozilla::dom::oldproxybindings::PaintRequestList::create(cx, scope, this,
                                                           triedToWrap);
  }

  nsISupports* GetParentObject()
  {
    return mParent;
  }

  void Append(nsIDOMPaintRequest* aElement) { mArray.AppendObject(aElement); }

  static nsPaintRequestList* FromSupports(nsISupports* aSupports)
  {
#ifdef DEBUG
    {
      nsCOMPtr<nsIDOMPaintRequestList> list_qi = do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(list_qi == static_cast<nsIDOMPaintRequestList*>(aSupports),
                   "Uh, fix QI!");
    }
#endif

    return static_cast<nsPaintRequestList*>(aSupports);
  }

private:
  ~nsPaintRequestList() {}

  nsCOMArray<nsIDOMPaintRequest> mArray;
  nsCOMPtr<nsIDOMEvent> mParent;
};

#endif 
