




#ifndef GFX_TRANSACTION_ID_ALLOCATOR_H
#define GFX_TRANSACTION_ID_ALLOCATOR_H

#include "nsISupportsImpl.h"
#include "mozilla/TimeStamp.h"

namespace mozilla {
namespace layers {

class TransactionIdAllocator {
protected:
  virtual ~TransactionIdAllocator() {}

public:
  NS_INLINE_DECL_REFCOUNTING(TransactionIdAllocator)

  






  virtual uint64_t GetTransactionId() = 0;

  







  virtual void NotifyTransactionCompleted(uint64_t aTransactionId) = 0;

  





  virtual void RevokeTransactionId(uint64_t aTransactionId) = 0;

  


  virtual mozilla::TimeStamp GetTransactionStart() = 0;
};

}
}


#endif 
