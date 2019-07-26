




#ifndef GFX_TRANSACTION_ID_ALLOCATOR_H
#define GFX_TRANSACTION_ID_ALLOCATOR_H

#include "nsISupportsImpl.h"

namespace mozilla {
namespace layers {

class TransactionIdAllocator {
public:
  NS_INLINE_DECL_REFCOUNTING(TransactionIdAllocator)

  virtual ~TransactionIdAllocator() {}

  






  virtual uint64_t GetTransactionId() = 0;

  







  virtual void NotifyTransactionCompleted(uint64_t aTransactionId) = 0;

  





  virtual void RevokeTransactionId(uint64_t aTransactionId) = 0;
};

}
}


#endif 
