






























#ifndef CLIENT_MAC_GENERATOR_MINIDUMP_GENERATOR_H__
#define CLIENT_MAC_GENERATOR_MINIDUMP_GENERATOR_H__

#include <mach/mach.h>

#include <string>

#include "client/minidump_file_writer.h"
#include "common/memory.h"
#include "common/mac/macho_utilities.h"
#include "google_breakpad/common/minidump_format.h"

#include "dynamic_images.h"

namespace google_breakpad {

using std::string;

const u_int64_t TOP_OF_THREAD0_STACK_64BIT = 0x00007fff5fbff000LL;
const u_int32_t TOP_OF_THREAD0_STACK_32BIT = 0xbffff000;



#if __DARWIN_UNIX03 || TARGET_CPU_X86_64 || TARGET_CPU_PPC64




#define REGISTER_FROM_THREADSTATE(a, b) ((a)->__ ## b)
#else
#define REGISTER_FROM_THREADSTATE(a, b) (a->b)
#endif








class MinidumpGenerator {
 public:
  MinidumpGenerator();
  MinidumpGenerator(mach_port_t crashing_task, mach_port_t handler_thread);

  ~MinidumpGenerator();

  
  
  static string UniqueNameInDirectory(const string &dir, string *unique_name);

  
  
  
  bool Write(const char *path);

  
  void SetExceptionInformation(int type, int code, int subcode,
                               mach_port_t thread_name) {
    exception_type_ = type;
    exception_code_ = code;
    exception_subcode_ = subcode;
    exception_thread_ = thread_name;
  }

  
  
  static void GatherSystemInformation();

 private:
    typedef bool (MinidumpGenerator::*WriteStreamFN)(MDRawDirectory *);

  
  bool WriteThreadListStream(MDRawDirectory *thread_list_stream);
  bool WriteMemoryListStream(MDRawDirectory *memory_list_stream);
  bool WriteExceptionStream(MDRawDirectory *exception_stream);
  bool WriteSystemInfoStream(MDRawDirectory *system_info_stream);
  bool WriteModuleListStream(MDRawDirectory *module_list_stream);
  bool WriteMiscInfoStream(MDRawDirectory *misc_info_stream);
  bool WriteBreakpadInfoStream(MDRawDirectory *breakpad_info_stream);

  
  u_int64_t CurrentPCForStack(breakpad_thread_state_data_t state);
  bool GetThreadState(thread_act_t target_thread, thread_state_t state,
                      mach_msg_type_number_t *count);
  bool WriteStackFromStartAddress(mach_vm_address_t start_addr,
                                  MDMemoryDescriptor *stack_location);
  bool WriteStack(breakpad_thread_state_data_t state,
                  MDMemoryDescriptor *stack_location);
  bool WriteContext(breakpad_thread_state_data_t state,
                    MDLocationDescriptor *register_location);
  bool WriteThreadStream(mach_port_t thread_id, MDRawThread *thread);
  bool WriteCVRecord(MDRawModule *module, int cpu_type, 
                     const char *module_path);
  bool WriteModuleStream(unsigned int index, MDRawModule *module);
  size_t CalculateStackSize(mach_vm_address_t start_addr);
  int  FindExecutableModule();

  
  bool WriteStackPPC(breakpad_thread_state_data_t state,
                     MDMemoryDescriptor *stack_location);
  bool WriteContextPPC(breakpad_thread_state_data_t state,
                       MDLocationDescriptor *register_location);
  u_int64_t CurrentPCForStackPPC(breakpad_thread_state_data_t state);
  bool WriteStackPPC64(breakpad_thread_state_data_t state,
                       MDMemoryDescriptor *stack_location);
  bool WriteContextPPC64(breakpad_thread_state_data_t state,
                       MDLocationDescriptor *register_location);
  u_int64_t CurrentPCForStackPPC64(breakpad_thread_state_data_t state);
  bool WriteStackX86(breakpad_thread_state_data_t state,
                       MDMemoryDescriptor *stack_location);
  bool WriteContextX86(breakpad_thread_state_data_t state,
                       MDLocationDescriptor *register_location);
  u_int64_t CurrentPCForStackX86(breakpad_thread_state_data_t state);
  bool WriteStackX86_64(breakpad_thread_state_data_t state,
                        MDMemoryDescriptor *stack_location);
  bool WriteContextX86_64(breakpad_thread_state_data_t state,
                          MDLocationDescriptor *register_location);
  u_int64_t CurrentPCForStackX86_64(breakpad_thread_state_data_t state);

  
  explicit MinidumpGenerator(const MinidumpGenerator &);
  void operator=(const MinidumpGenerator &);

  
  MinidumpFileWriter writer_;

  
  int exception_type_;
  int exception_code_;
  int exception_subcode_;
  mach_port_t exception_thread_;
  mach_port_t crashing_task_;
  mach_port_t handler_thread_;

  
  cpu_type_t cpu_type_;
  
  
  static char build_string_[16];
  static int os_major_version_;
  static int os_minor_version_;
  static int os_build_number_;
  
  
  DynamicImages *dynamic_images_;

  
  
  mutable PageAllocator allocator_;

  
  
  
  wasteful_vector<MDMemoryDescriptor> memory_blocks_;
};

}  

#endif  
