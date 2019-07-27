



#include "chrome/common/ipc_channel.h"

#include <limits>

#include "base/atomic_sequence_num.h"
#include "base/process_util.h"
#include "base/rand_util.h"
#include "base/string_util.h"

namespace {


base::StaticAtomicSequenceNumber g_last_id;

}  

namespace IPC {


std::wstring Channel::GenerateUniqueRandomChannelID() {
  
  
  
  
  
  
  

  return StringPrintf(L"%d.%u.%d",
      base::GetCurrentProcId(),
      g_last_id.GetNext(),
      base::RandInt(0, std::numeric_limits<int32_t>::max()));
}

}  