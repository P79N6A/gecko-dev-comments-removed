



















































#ifndef rlogringbuffer_h__
#define rlogringbuffer_h__

#include <stdint.h>

#include <deque>
#include <string>
#include <vector>

#include "m_cpp_utils.h"

namespace mozilla {

class RLogRingBuffer {
  public:
    



    static RLogRingBuffer* CreateInstance();
    static RLogRingBuffer* GetInstance();
    static void DestroyInstance();

    





    void Filter(const std::string& substring,
                uint32_t limit,
                std::deque<std::string>* matching_logs);

    void FilterAny(const std::vector<std::string>& substrings,
                   uint32_t limit,
                   std::deque<std::string>* matching_logs);

    inline void GetAny(uint32_t limit,
                       std::deque<std::string>* matching_logs) {
      Filter("", limit, matching_logs);
    }

    void SetLogLimit(uint32_t new_limit);

    void Log(std::string&& log);

  private:
    RLogRingBuffer();
    ~RLogRingBuffer();
    void RemoveOld();
    static RLogRingBuffer* instance;

    




    std::deque<std::string> log_messages_;
    
    uint32_t log_limit_;

    DISALLOW_COPY_ASSIGN(RLogRingBuffer);
}; 

} 

#endif 


