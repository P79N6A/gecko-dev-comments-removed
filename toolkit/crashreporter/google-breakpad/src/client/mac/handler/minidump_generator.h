






























#ifndef CLIENT_MAC_GENERATOR_MINIDUMP_GENERATOR_H__
#define CLIENT_MAC_GENERATOR_MINIDUMP_GENERATOR_H__

#include <mach/mach.h>
#include <TargetConditionals.h>

#include <string>

#include "client/minidump_file_writer.h"
#include "common/memory.h"
#include "common/mac/macho_utilities.h"
#include "google_breakpad/common/minidump_format.h"

#include "dynamic_images.h"
#include "mach_vm_compat.h"

#if !TARGET_OS_IPHONE && (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7)
  #define HAS_PPC_SUPPORT
#endif
#if defined(__arm__)
  #define HAS_ARM_SUPPORT
#elif defined(__i386__) || defined(__x86_64__)
  #define HAS_X86_SUPPORT
#endif

namespace google_breakpad {

using std::string;



#if __DARWIN_UNIX03 || TARGET_CPU_X86_64 || TARGET_CPU_PPC64 || TARGET_CPU_ARM




#define REGISTER_FROM_THREADSTATE(a, b) ((a)->__ ## b)
#else
#define REGISTER_FROM_THREADSTATE(a, b) (a->b)
#endif








class MinidumpGenerator {
 public:
  MinidumpGenerator();
  MinidumpGenerator(mach_port_t crashing_task, mach_port_t handler_thread);

  virtual ~MinidumpGenerator();

  
  
  static string UniqueNameInDirectory(const string &dir, string *unique_name);

  
  
  
  bool Write(const char *path);

  
  void SetExceptionInformation(int type, int code, int subcode,
                               mach_port_t thread_name) {
    exception_type_ = type;
    exception_code_ = code;
    exception_subcode_ = subcode;
    exception_thread_ = thread_name;
  }

  
  
  
  void SetTaskContext(ucontext_t *task_context);

  
  
  static void GatherSystemInformation();

 protected:
  
  virtual bool WriteExceptionStream(MDRawDirectory *exception_stream);

  
  virtual bool WriteThreadStream(mach_port_t thread_id, MDRawThread *thread);

 private:
  typedef bool (MinidumpGenerator::*WriteStreamFN)(MDRawDirectory *);

  
  bool WriteThreadListStream(MDRawDirectory *thread_list_stream);
  bool WriteMemoryListStream(MDRawDirectory *memory_list_stream);
  bool WriteSystemInfoStream(MDRawDirectory *system_info_stream);
  bool WriteModuleListStream(MDRawDirectory *module_list_stream);
  bool WriteMiscInfoStream(MDRawDirectory *misc_info_stream);
  bool WriteBreakpadInfoStream(MDRawDirectory *breakpad_info_stream);

  
  uint64_t CurrentPCForStack(breakpad_thread_state_data_t state);
  bool GetThreadState(thread_act_t target_thread, thread_state_t state,
                      mach_msg_type_number_t *count);
  bool WriteStackFromStartAddress(mach_vm_address_t start_addr,
                                  MDMemoryDescriptor *stack_location);
  bool WriteStack(breakpad_thread_state_data_t state,
                  MDMemoryDescriptor *stack_location);
  bool WriteContext(breakpad_thread_state_data_t state,
                    MDLocationDescriptor *register_location);
  bool WriteCVRecord(MDRawModule *module, int cpu_type,
                     const char *module_path, bool in_memory);
  bool WriteModuleStream(unsigned int index, MDRawModule *module);
  size_t CalculateStackSize(mach_vm_address_t start_addr);
  int  FindExecutableModule();

  
#ifdef HAS_ARM_SUPPORT
  bool WriteStackARM(breakpad_thread_state_data_t state,
                     MDMemoryDescriptor *stack_location);
  bool WriteContextARM(breakpad_thread_state_data_t state,
                       MDLocationDescriptor *register_location);
  uint64_t CurrentPCForStackARM(breakpad_thread_state_data_t state);
#endif
#ifdef HAS_PPC_SUPPORT
  bool WriteStackPPC(breakpad_thread_state_data_t state,
                     MDMemoryDescriptor *stack_location);
  bool WriteContextPPC(breakpad_thread_state_data_t state,
                       MDLocationDescriptor *register_location);
  uint64_t CurrentPCForStackPPC(breakpad_thread_state_data_t state);
  bool WriteStackPPC64(breakpad_thread_state_data_t state,
                       MDMemoryDescriptor *stack_location);
  bool WriteContextPPC64(breakpad_thread_state_data_t state,
                       MDLocationDescriptor *register_location);
  uint64_t CurrentPCForStackPPC64(breakpad_thread_state_data_t state);
#endif
#ifdef HAS_X86_SUPPORT
  bool WriteStackX86(breakpad_thread_state_data_t state,
                       MDMemoryDescriptor *stack_location);
  bool WriteContextX86(breakpad_thread_state_data_t state,
                       MDLocationDescriptor *register_location);
  uint64_t CurrentPCForStackX86(breakpad_thread_state_data_t state);
  bool WriteStackX86_64(breakpad_thread_state_data_t state,
                        MDMemoryDescriptor *stack_location);
  bool WriteContextX86_64(breakpad_thread_state_data_t state,
                          MDLocationDescriptor *register_location);
  uint64_t CurrentPCForStackX86_64(breakpad_thread_state_data_t state);
#endif

  
  explicit MinidumpGenerator(const MinidumpGenerator &);
  void operator=(const MinidumpGenerator &);

 protected:
  
  MinidumpFileWriter writer_;

 private:
  
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

  
  ucontext_t *task_context_;

  
  DynamicImages *dynamic_images_;

  
  
  mutable PageAllocator allocator_;

 protected:
  
  
  
  wasteful_vector<MDMemoryDescriptor> memory_blocks_;
};

}  

#endif  
