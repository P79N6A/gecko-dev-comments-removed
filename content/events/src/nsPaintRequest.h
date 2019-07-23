





































#ifndef NSPAINTREQUEST_H_
#define NSPAINTREQUEST_H_

#include "nsIDOMPaintRequest.h"
#include "nsIDOMPaintRequestList.h"
#include "nsPresContext.h"
#include "nsClientRect.h"

class nsPaintRequest : public nsIDOMPaintRequest
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

class nsPaintRequestList : public nsIDOMPaintRequestList
{
public:
  nsPaintRequestList() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMPAINTREQUESTLIST
  
  void Append(nsIDOMPaintRequest* aElement) { mArray.AppendObject(aElement); }

  nsIDOMPaintRequest* GetItemAt(PRUint32 aIndex)
  {
    return mArray.SafeObjectAt(aIndex);
  }

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
};

#endif 
