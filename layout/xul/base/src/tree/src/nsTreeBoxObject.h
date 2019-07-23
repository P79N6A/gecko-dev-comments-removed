







































#ifndef nsTreeBoxObject_h___
#define nsTreeBoxObject_h___

#include "nsBoxObject.h"
#include "nsITreeView.h"
#include "nsITreeBoxObject.h"

class nsTreeBodyFrame;

class nsTreeBoxObject : public nsITreeBoxObject, public nsBoxObject
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsTreeBoxObject, nsBoxObject)
  NS_DECL_NSITREEBOXOBJECT

  nsTreeBoxObject();
  ~nsTreeBoxObject();

  nsTreeBodyFrame* GetTreeBody();
  nsTreeBodyFrame* GetCachedTreeBody() { return mTreeBody; }

  
  virtual void Clear();
  virtual void ClearCachedValues();

protected:
  nsTreeBodyFrame* mTreeBody;
  nsCOMPtr<nsITreeView> mView;
};

#endif
