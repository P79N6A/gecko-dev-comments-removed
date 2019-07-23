






























#ifndef CLIENT_MAC_GENERATOR_MINIDUMP_GENERATOR_H__
#define CLIENT_MAC_GENERATOR_MINIDUMP_GENERATOR_H__

#include <mach/mach.h>

#include <string>

#include "client/minidump_file_writer.h"
#include "google_breakpad/common/minidump_format.h"
#include "common/mac/macho_utilities.h"

#include "dynamic_images.h"

namespace google_breakpad {

using std::string;

#if TARGET_CPU_X86_64 || TARGET_CPU_PPC64
#define TOP_OF_THREAD0_STACK 0x00007fff5fbff000
#else
#define TOP_OF_THREAD0_STACK 0xbffff000
#endif

#if TARGET_CPU_X86_64
typedef x86_thread_state64_t breakpad_thread_state_t;
typedef MDRawContextAMD64 MinidumpContext;
#elif TARGET_CPU_X86
typedef  i386_thread_state_t breakpad_thread_state_t;
typedef MDRawContextX86 MinidumpContext;
#elif TARGET_CPU_PPC64
typedef ppc_thread_state64_t breakpad_thread_state_t;
typedef MDRawContextPPC64 MinidumpContext;
#elif TARGET_CPU_PPC
typedef ppc_thread_state_t breakpad_thread_state_t;
typedef MDRawContextPPC MinidumpContext;
#endif



#if __DARWIN_UNIX03 || !TARGET_CPU_X86 || TARGET_CPU_X86_64




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
  bool WriteExceptionStream(MDRawDirectory *exception_stream);
  bool WriteSystemInfoStream(MDRawDirectory *system_info_stream);
  bool WriteModuleListStream(MDRawDirectory *module_list_stream);
  bool WriteMiscInfoStream(MDRawDirectory *misc_info_stream);
  bool WriteBreakpadInfoStream(MDRawDirectory *breakpad_info_stream);

  
  u_int64_t CurrentPCForStack(breakpad_thread_state_data_t state);
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

  
  explicit MinidumpGenerator(const MinidumpGenerator &);
  void operator=(const MinidumpGenerator &);

  
  MinidumpFileWriter writer_;

  
  int exception_type_;
  int exception_code_;
  int exception_subcode_;
  mach_port_t exception_thread_;
  mach_port_t crashing_task_;
  mach_port_t handler_thread_;
  
  
  static char build_string_[16];
  static int os_major_version_;
  static int os_minor_version_;
  static int os_build_number_;
  
  
  DynamicImages *dynamic_images_;
};

}  

#endif  
