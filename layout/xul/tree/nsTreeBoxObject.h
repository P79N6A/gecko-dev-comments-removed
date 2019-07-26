




#ifndef nsTreeBoxObject_h___
#define nsTreeBoxObject_h___

#include "mozilla/Attributes.h"
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

  nsTreeBodyFrame* GetTreeBody(bool aFlushLayout = false);
  nsTreeBodyFrame* GetCachedTreeBody() { return mTreeBody; }

  
  virtual void Clear() MOZ_OVERRIDE;
  virtual void ClearCachedValues() MOZ_OVERRIDE;

protected:
  nsTreeBodyFrame* mTreeBody;
  nsCOMPtr<nsITreeView> mView;
};

#endif
