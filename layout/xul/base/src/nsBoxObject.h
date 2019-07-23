




































#ifndef nsBoxObject_h_
#define nsBoxObject_h_

#include "nsCOMPtr.h"
#include "nsIBoxObject.h"
#include "nsPIBoxObject.h"
#include "nsPoint.h"
#include "nsAutoPtr.h"
#include "nsHashKeys.h"
#include "nsInterfaceHashtable.h"

class nsIFrame;
class nsIDocShell;
struct nsRect;

class nsBoxObject : public nsPIBoxObject
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIBOXOBJECT

public:
  nsBoxObject();
  virtual ~nsBoxObject();

  
  virtual nsresult Init(nsIContent* aContent);
  virtual void Clear();
  virtual void ClearCachedValues();

  nsIFrame* GetFrame(PRBool aFlushLayout);
  nsIPresShell* GetPresShell(PRBool aFlushLayout);
  nsresult GetOffsetRect(nsRect& aRect);
  nsresult GetScreenPosition(nsIntPoint& aPoint);

  
  
  static nsresult GetPreviousSibling(nsIFrame* aParentFrame, nsIFrame* aFrame,
                                     nsIDOMElement** aResult);

protected:

  nsAutoPtr<nsInterfaceHashtable<nsStringHashKey,nsISupports> > mPropertyTable; 

  nsIContent* mContent; 
};

#endif
