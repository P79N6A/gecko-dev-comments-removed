






























#ifndef CLIENT_MAC_GENERATOR_MINIDUMP_GENERATOR_H__
#define CLIENT_MAC_GENERATOR_MINIDUMP_GENERATOR_H__

#include <mach/mach.h>

#include <string>

#include "client/minidump_file_writer.h"
#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {

using std::string;








class MinidumpGenerator {
 public:
  MinidumpGenerator();
  ~MinidumpGenerator();

  
  
  static string UniqueNameInDirectory(const string &dir, string *unique_name);

  
  
  
  bool Write(const char *path);

  
  void SetExceptionInformation(int type, int code, mach_port_t thread_name) {
    exception_type_ = type;
    exception_code_ = code;
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

  
  u_int64_t CurrentPCForStack(thread_state_data_t state);
  bool WriteStackFromStartAddress(vm_address_t start_addr,
                                  MDMemoryDescriptor *stack_location);
  bool WriteStack(thread_state_data_t state,
                  MDMemoryDescriptor *stack_location);
  bool WriteContext(thread_state_data_t state,
                    MDLocationDescriptor *register_location);
  bool WriteThreadStream(mach_port_t thread_id, MDRawThread *thread);
  bool WriteCVRecord(MDRawModule *module, int cpu_type, 
                     const char *module_path);
  bool WriteModuleStream(unsigned int index, MDRawModule *module);

  
  explicit MinidumpGenerator(const MinidumpGenerator &);
  void operator=(const MinidumpGenerator &);

  
  MinidumpFileWriter writer_;

  
  int exception_type_;
  int exception_code_;
  mach_port_t exception_thread_;

  
  static char build_string_[16];
  static int os_major_version_;
  static int os_minor_version_;
  static int os_build_number_;
};

}  

#endif  
