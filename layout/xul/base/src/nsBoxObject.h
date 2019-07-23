




































#ifndef nsBoxObject_h_
#define nsBoxObject_h_

#include "nsCOMPtr.h"
#include "nsIBoxObject.h"
#include "nsPIBoxObject.h"
#include "nsPoint.h"
#include "nsAutoPtr.h"
#include "nsHashKeys.h"
#include "nsInterfaceHashtable.h"
#include "nsCycleCollectionParticipant.h"

class nsIFrame;
class nsIDocShell;
struct nsIntRect;

class nsBoxObject : public nsPIBoxObject
{
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsBoxObject)
  NS_DECL_NSIBOXOBJECT

public:
  nsBoxObject();
  virtual ~nsBoxObject();

  
  virtual nsresult Init(nsIContent* aContent);
  virtual void Clear();
  virtual void ClearCachedValues();

  nsIFrame* GetFrame(PRBool aFlushLayout);
  nsIPresShell* GetPresShell(PRBool aFlushLayout);
  nsresult GetOffsetRect(nsIntRect& aRect);
  nsresult GetScreenPosition(nsIntPoint& aPoint);

  
  
  static nsresult GetPreviousSibling(nsIFrame* aParentFrame, nsIFrame* aFrame,
                                     nsIDOMElement** aResult);

protected:

  nsAutoPtr<nsInterfaceHashtable<nsStringHashKey,nsISupports> > mPropertyTable; 

  nsIContent* mContent; 
};

#endif
