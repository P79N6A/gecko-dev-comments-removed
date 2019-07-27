



#ifndef SANDBOX_SRC_HANDLE_CLOSER_H_
#define SANDBOX_SRC_HANDLE_CLOSER_H_

#include <map>
#include <set>

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/interception.h"
#include "sandbox/win/src/sandbox_types.h"
#include "sandbox/win/src/target_process.h"

namespace sandbox {




typedef std::map<const base::string16, std::set<const base::string16> >
    HandleMap;


struct HandleListEntry {
  size_t record_bytes;       
  size_t offset_to_names;    
  size_t name_count;
  base::char16 handle_type[1];
};


struct HandleCloserInfo {
  size_t record_bytes;       
  size_t num_handle_types;
  struct HandleListEntry handle_entries[1];
};

SANDBOX_INTERCEPT HandleCloserInfo* g_handle_closer_info;


class HandleCloser {
 public:
  HandleCloser();

  
  
  
  ResultCode AddHandle(const base::char16* handle_type,
                       const base::char16* handle_name);

  
  bool InitializeTargetHandles(TargetProcess* target);

 private:
  
  
  size_t GetBufferSize();

  
  bool SetupHandleList(void* buffer, size_t buffer_bytes);

  HandleMap handles_to_close_;

  DISALLOW_COPY_AND_ASSIGN(HandleCloser);
};


bool GetHandleName(HANDLE handle, base::string16* handle_name);

}  

#endif  
