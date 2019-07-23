







































#ifndef nsTreeBoxObject_h___
#define nsTreeBoxObject_h___

#include "nsBoxObject.h"
#include "nsITreeView.h"
#include "nsITreeBoxObject.h"

class nsTreeBoxObject : public nsITreeBoxObject, public nsBoxObject
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSITREEBOXOBJECT

  nsTreeBoxObject();
  ~nsTreeBoxObject();

  nsITreeBoxObject* GetTreeBody();
  nsITreeBoxObject* GetCachedTreeBody() { return mTreeBody; }

  
  virtual void Clear();
  virtual void ClearCachedValues();

protected:
  nsITreeBoxObject* mTreeBody;
  nsCOMPtr<nsITreeView> mView;
};

#endif
