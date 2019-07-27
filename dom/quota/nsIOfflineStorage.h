





#ifndef nsIOfflineStorage_h__
#define nsIOfflineStorage_h__

#include "mozilla/dom/quota/PersistenceType.h"

#define NS_OFFLINESTORAGE_IID \
  {0x3ae00063, 0x6c13, 0x4afd, \
  { 0x86, 0x7d, 0x33, 0xc2, 0x12, 0xd8, 0x97, 0x25 } }

class nsPIDOMWindow;

namespace mozilla {
namespace dom {
namespace quota {
class Client;
}
}
}

class nsIOfflineStorage : public nsISupports
{
public:
  typedef mozilla::dom::quota::Client Client;
  typedef mozilla::dom::quota::PersistenceType PersistenceType;

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_OFFLINESTORAGE_IID)

  NS_IMETHOD_(const nsACString&)
  Id() = 0;

  NS_IMETHOD_(Client*)
  GetClient() = 0;

  NS_IMETHOD_(bool)
  IsOwned(nsPIDOMWindow* aOwner) = 0;

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

  
  NS_IMETHOD_(bool)
  IsClosed() = 0;

  
  
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
  Id() MOZ_OVERRIDE;                                                           \
                                                                               \
  NS_IMETHOD_(Client*)                                                         \
  GetClient() MOZ_OVERRIDE;                                                    \
                                                                               \
  NS_IMETHOD_(bool)                                                            \
  IsOwned(nsPIDOMWindow* aOwner) MOZ_OVERRIDE;                                 \
                                                                               \
  NS_IMETHOD_(const nsACString&)                                               \
  Origin() MOZ_OVERRIDE;                                                       \
                                                                               \
  NS_IMETHOD_(nsresult)                                                        \
  Close() MOZ_OVERRIDE;                                                        \
                                                                               \
  NS_IMETHOD_(bool)                                                            \
  IsClosed() MOZ_OVERRIDE;                                                     \
                                                                               \
  NS_IMETHOD_(void)                                                            \
  Invalidate() MOZ_OVERRIDE;

#endif 
