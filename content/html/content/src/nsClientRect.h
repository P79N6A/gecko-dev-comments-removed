





































#ifndef NSCLIENTRECT_H_
#define NSCLIENTRECT_H_

#include "nsIDOMClientRect.h"
#include "nsIDOMClientRectList.h"
#include "nsCOMArray.h"
#include "nsRect.h"
#include "nsCOMPtr.h"
#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"

class nsClientRect : public nsIDOMClientRect
{
public:
  NS_DECL_ISUPPORTS

  nsClientRect();
  void SetRect(float aX, float aY, float aWidth, float aHeight) {
    mX = aX; mY = aY; mWidth = aWidth; mHeight = aHeight;
  }
  virtual ~nsClientRect() {}
  
  NS_DECL_NSIDOMCLIENTRECT

  void SetLayoutRect(const nsRect& aLayoutRect);

protected:
  float mX, mY, mWidth, mHeight;
};

class nsClientRectList MOZ_FINAL : public nsIDOMClientRectList,
                                   public nsWrapperCache
{
public:
  nsClientRectList(nsISupports *aParent) : mParent(aParent)
  {
    SetIsProxy();
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsClientRectList)

  NS_DECL_NSIDOMCLIENTRECTLIST
  
  virtual JSObject* WrapObject(JSContext *cx, XPCWrappedNativeScope *scope,
                               bool *triedToWrap);

  nsISupports* GetParentObject()
  {
    return mParent;
  }

  void Append(nsIDOMClientRect* aElement) { mArray.AppendObject(aElement); }

  static nsClientRectList* FromSupports(nsISupports* aSupports)
  {
#ifdef DEBUG
    {
      nsCOMPtr<nsIDOMClientRectList> list_qi = do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(list_qi == static_cast<nsIDOMClientRectList*>(aSupports),
                   "Uh, fix QI!");
    }
#endif

    return static_cast<nsClientRectList*>(aSupports);
  }

protected:
  virtual ~nsClientRectList() {}

  nsCOMArray<nsIDOMClientRect> mArray;
  nsCOMPtr<nsISupports> mParent;
};

#endif 
