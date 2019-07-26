




























#ifndef CLIENT_LINUX_HANDLER_EXCEPTION_HANDLER_H_
#define CLIENT_LINUX_HANDLER_EXCEPTION_HANDLER_H_

#include <string>
#include <vector>

#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ucontext.h>

#include "client/linux/crash_generation/crash_generation_client.h"
#include "client/linux/handler/minidump_descriptor.h"
#include "client/linux/minidump_writer/minidump_writer.h"
#include "common/scoped_ptr.h"
#include "common/using_std_string.h"
#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {


























class ExceptionHandler {
 public:
  
  
  
  
  
  
  
  
  
  typedef bool (*FilterCallback)(void *context);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef bool (*MinidumpCallback)(const MinidumpDescriptor& descriptor,
                                   void* context,
                                   bool succeeded);

  
  
  
  
  
  typedef bool (*HandlerCallback)(const void* crash_context,
                                  size_t crash_context_size,
                                  void* context);

  
  
  
  
  
  
  
  
  
  
  
  ExceptionHandler(const MinidumpDescriptor& descriptor,
                   FilterCallback filter,
                   MinidumpCallback callback,
                   void *callback_context,
                   bool install_handler,
                   const int server_fd);
  ~ExceptionHandler();

  const MinidumpDescriptor& minidump_descriptor() const {
    return minidump_descriptor_;
  }

  void set_minidump_descriptor(const MinidumpDescriptor& descriptor) {
    minidump_descriptor_ = descriptor;
  }

  void set_crash_handler(HandlerCallback callback) {
    crash_handler_ = callback;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  bool WriteMinidump();

  
  
  static bool WriteMinidump(const string& dump_path,
                            MinidumpCallback callback,
                            void* callback_context);

  
  
  
  
  
  
  
  
  
  
  
  static bool WriteMinidumpForChild(pid_t child,
                                    pid_t child_blamed_thread,
                                    const string& dump_path,
                                    MinidumpCallback callback,
                                    void* callback_context);

  
  
  struct CrashContext {
    siginfo_t siginfo;
    pid_t tid;  
    struct ucontext context;
#if !defined(__ARM_EABI__)
    
    struct _libc_fpstate float_state;
#endif
  };

  
  bool IsOutOfProcess() const {
      return crash_generation_client_.get() != NULL;
  }

  
  
  
  void AddMappingInfo(const string& name,
                      const uint8_t identifier[sizeof(MDGUID)],
                      uintptr_t start_address,
                      size_t mapping_size,
                      size_t file_offset);

  
  
  void RegisterAppMemory(void* ptr, size_t length);

  
  void UnregisterAppMemory(void* ptr);

  
  bool SimulateSignalDelivery(int sig);

  
  bool HandleSignal(int sig, siginfo_t* info, void* uc);
 private:
  
  static bool InstallHandlersLocked();
  
  static void RestoreHandlersLocked();

  void PreresolveSymbols();
  bool GenerateDump(CrashContext *context);
  void SendContinueSignalToChild();
  void WaitForContinueSignal();

  static void SignalHandler(int sig, siginfo_t* info, void* uc);
  static int ThreadEntry(void* arg);
  bool DoDump(pid_t crashing_process, const void* context,
              size_t context_size);

  const FilterCallback filter_;
  const MinidumpCallback callback_;
  void* const callback_context_;

  scoped_ptr<CrashGenerationClient> crash_generation_client_;

  MinidumpDescriptor minidump_descriptor_;

  HandlerCallback crash_handler_;

  
  
  
  static std::vector<ExceptionHandler*> *handler_stack_;
  static pthread_mutex_t handler_stack_mutex_;

  
  
  
  
  
  int fdes[2];

  
  
  MappingList mapping_list_;

  
  
  AppMemoryList app_memory_list_;
};

}  

#endif  
