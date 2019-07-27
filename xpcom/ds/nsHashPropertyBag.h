





#ifndef nsHashPropertyBag_h___
#define nsHashPropertyBag_h___

#include "nsIVariant.h"
#include "nsIWritablePropertyBag.h"
#include "nsIWritablePropertyBag2.h"

#include "nsCycleCollectionParticipant.h"
#include "nsInterfaceHashtable.h"

class nsHashPropertyBagBase
  : public nsIWritablePropertyBag
  , public nsIWritablePropertyBag2
{
public:
  nsHashPropertyBagBase() {}

  NS_DECL_NSIPROPERTYBAG
  NS_DECL_NSIPROPERTYBAG2

  NS_DECL_NSIWRITABLEPROPERTYBAG
  NS_DECL_NSIWRITABLEPROPERTYBAG2

protected:
  
  nsInterfaceHashtable<nsStringHashKey, nsIVariant> mPropertyHash;
};

class nsHashPropertyBag : public nsHashPropertyBagBase
{
public:
  nsHashPropertyBag() {}
  NS_DECL_THREADSAFE_ISUPPORTS

protected:
  virtual ~nsHashPropertyBag() {}
};


class nsHashPropertyBagCC MOZ_FINAL : public nsHashPropertyBagBase
{
public:
  nsHashPropertyBagCC() {}
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsHashPropertyBagCC,
                                           nsIWritablePropertyBag)
protected:
  virtual ~nsHashPropertyBagCC() {}
};

#endif 
