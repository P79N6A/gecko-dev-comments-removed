





#ifndef nsIOfflineStorage_h__
#define nsIOfflineStorage_h__

#include "nsIFileStorage.h"

#define NS_OFFLINESTORAGE_IID \
  {0xe531b6e0, 0x55b8, 0x4f39, \
  { 0x95, 0xbb, 0x97, 0x21, 0x4c, 0xb0, 0xf6, 0x1a } }

namespace mozilla {
namespace dom {
namespace quota {
class Client;
}
}
}

class nsIOfflineStorage : public nsIFileStorage
{
public:
  typedef mozilla::dom::quota::Client Client;

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_OFFLINESTORAGE_IID)

  NS_IMETHOD_(Client*)
  GetClient() = 0;

  NS_IMETHOD_(bool)
  IsOwned(nsPIDOMWindow* aOwner) = 0;

  NS_IMETHOD_(const nsACString&)
  Origin() = 0;

  
  
  NS_IMETHOD_(nsresult)
  Close() = 0;

  
  NS_IMETHOD_(bool)
  IsClosed() = 0;

  
  
  NS_IMETHOD_(void)
  Invalidate() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIOfflineStorage, NS_OFFLINESTORAGE_IID)

#define NS_DECL_NSIOFFLINESTORAGE                                              \
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

#define NS_DECL_NSIOFFLINESTORAGE_NOCLOSE                                      \
  NS_IMETHOD_(Client*)                                                         \
  GetClient() MOZ_OVERRIDE;                                                    \
                                                                               \
  NS_IMETHOD_(bool)                                                            \
  IsOwned(nsPIDOMWindow* aOwner) MOZ_OVERRIDE;                                 \
                                                                               \
  NS_IMETHOD_(const nsACString&)                                               \
  Origin() MOZ_OVERRIDE;                                                       \
                                                                               \
  NS_IMETHOD_(bool)                                                            \
  IsClosed() MOZ_OVERRIDE;                                                     \
                                                                               \
  NS_IMETHOD_(void)                                                            \
  Invalidate() MOZ_OVERRIDE;

#endif 
