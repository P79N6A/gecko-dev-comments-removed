





































#ifndef NSCLIENTRECT_H_
#define NSCLIENTRECT_H_

#include "nsIDOMClientRect.h"
#include "nsIDOMClientRectList.h"
#include "nsCOMArray.h"
#include "nsRect.h"

class nsPresContext;

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

  void SetLayoutRect(const nsRect& aLayoutRect, nsPresContext* aPresContext);

protected:
  float mX, mY, mWidth, mHeight;
};

class nsClientRectList : public nsIDOMClientRectList
{
public:
  nsClientRectList() {}

  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOMCLIENTRECTLIST
  
  void Append(nsIDOMClientRect* aElement) { mArray.AppendObject(aElement); }

protected:
  virtual ~nsClientRectList() {}

  nsCOMArray<nsIDOMClientRect> mArray;
};

#endif 
