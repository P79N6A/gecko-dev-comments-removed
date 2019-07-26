




#include "nsString.h"
#include "nsTArray.h"

#ifndef MuxerOperation_h_
#define MuxerOperation_h_

namespace mozilla {






















class MuxerOperation {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MuxerOperation)

  
  virtual nsresult Generate(uint32_t* aBoxSize) = 0;

  
  virtual nsresult Write() = 0;

  
  
  
  
  virtual nsresult Find(const nsACString& aType,
                        nsTArray<nsRefPtr<MuxerOperation>>& aOperations) = 0;

  virtual ~MuxerOperation() {}
};

}
#endif
