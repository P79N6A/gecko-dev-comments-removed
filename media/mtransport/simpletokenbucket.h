











#ifndef simpletokenbucket_h__
#define simpletokenbucket_h__

#include <stdint.h>

#include "prinrval.h"

#include "m_cpp_utils.h"

namespace mozilla {

class SimpleTokenBucket {
  public:
    





    SimpleTokenBucket(size_t bucket_size, size_t tokens_per_second);

    






    size_t getTokens(size_t num_tokens);

  protected: 
    uint64_t max_tokens_;
    uint64_t num_tokens_;
    size_t tokens_per_second_;
    PRIntervalTime last_time_tokens_added_;

    DISALLOW_COPY_ASSIGN(SimpleTokenBucket);
};

} 

#endif 

