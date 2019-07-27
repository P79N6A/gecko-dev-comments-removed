





#ifndef nsIOfflineStorage_h__
#define nsIOfflineStorage_h__

#include "mozilla/dom/quota/PersistenceType.h"

#define NS_OFFLINESTORAGE_IID \
  {0x91c57bf2, 0x0eda, 0x4db6, {0x9f, 0xf6, 0xcb, 0x38, 0x26, 0x8d, 0xb3, 0x01}}

namespace mozilla {
namespace dom {

class ContentParent;

namespace quota {

class Client;

}
}
}

class nsIOfflineStorage : public nsISupports
{
public:
  typedef mozilla::dom::ContentParent ContentParent;
  typedef mozilla::dom::quota::Client Client;
  typedef mozilla::dom::quota::PersistenceType PersistenceType;

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_OFFLINESTORAGE_IID)

  NS_IMETHOD_(const nsACString&)
  Id() = 0;

  NS_IMETHOD_(Client*)
  GetClient() = 0;

  NS_IMETHOD_(bool)
  IsOwnedByProcess(ContentParent* aOwner) = 0;

  NS_IMETHOD_(PersistenceType)
  Type()
  {
    return mPersistenceType;
  }

  NS_IMETHOD_(const nsACString&)
  Group()
  {
    return mGroup;
  }

  NS_IMETHOD_(const nsACString&)
  Origin() = 0;

  
  
  NS_IMETHOD_(nsresult)
  Close() = 0;

  
  
  NS_IMETHOD_(void)
  Invalidate() = 0;

protected:
  nsIOfflineStorage()
  : mPersistenceType(mozilla::dom::quota::PERSISTENCE_TYPE_INVALID)
  { }

  virtual ~nsIOfflineStorage()
  { }

  PersistenceType mPersistenceType;
  nsCString mGroup;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIOfflineStorage, NS_OFFLINESTORAGE_IID)

#define NS_DECL_NSIOFFLINESTORAGE                                              \
  NS_IMETHOD_(const nsACString&)                                               \
  Id() override;                                                               \
                                                                               \
  NS_IMETHOD_(Client*)                                                         \
  GetClient() override;                                                        \
                                                                               \
  NS_IMETHOD_(bool)                                                            \
  IsOwnedByProcess(ContentParent* aOwner) override;                            \
                                                                               \
  NS_IMETHOD_(const nsACString&)                                               \
  Origin() override;                                                           \
                                                                               \
  NS_IMETHOD_(nsresult)                                                        \
  Close() override;                                                            \
                                                                               \
  NS_IMETHOD_(void)                                                            \
  Invalidate() override;

#endif 
