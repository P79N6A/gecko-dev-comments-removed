





































#ifndef NSTEXTRECTANGLE_H_
#define NSTEXTRECTANGLE_H_

#include "nsIDOMTextRectangle.h"
#include "nsIDOMTextRectangleList.h"
#include "nsCOMArray.h"

class nsTextRectangle : public nsIDOMTextRectangle
{
public:
  NS_DECL_ISUPPORTS

  nsTextRectangle();
  void SetRect(float aX, float aY, float aWidth, float aHeight) {
    mX = aX; mY = aY; mWidth = aWidth; mHeight = aHeight;
  }
  virtual ~nsTextRectangle() {}
  
  NS_DECL_NSIDOMTEXTRECTANGLE

protected:
  float mX, mY, mWidth, mHeight;
};

class nsTextRectangleList : public nsIDOMTextRectangleList
{
public:
  nsTextRectangleList() {}

  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOMTEXTRECTANGLELIST
  
  void Append(nsIDOMTextRectangle* aElement) { mArray.AppendObject(aElement); }

protected:
  virtual ~nsTextRectangleList() {}

  nsCOMArray<nsIDOMTextRectangle> mArray;
};

#endif 
