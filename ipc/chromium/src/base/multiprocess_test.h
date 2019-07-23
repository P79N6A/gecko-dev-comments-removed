



#ifndef BASE_MULTIPROCESS_TEST_H__
#define BASE_MULTIPROCESS_TEST_H__

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/process_util.h"
#include "base/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"
#include "testing/platform_test.h"

#if defined(OS_POSIX)
#include <sys/types.h>
#include <unistd.h>
#endif



static const wchar_t kRunClientProcess[] = L"client";























class MultiProcessTest : public PlatformTest {
 protected:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  base::ProcessHandle SpawnChild(const std::wstring& procname) {
    return SpawnChild(procname, false);
  }

  base::ProcessHandle SpawnChild(const std::wstring& procname,
                                 bool debug_on_start) {
#if defined(OS_WIN)
    return SpawnChildImpl(procname, debug_on_start);
#elif defined(OS_POSIX)
    base::file_handle_mapping_vector empty_file_list;
    return SpawnChildImpl(procname, empty_file_list, debug_on_start);
#endif
  }

#if defined(OS_POSIX)
  base::ProcessHandle SpawnChild(
      const std::wstring& procname,
      const base::file_handle_mapping_vector& fds_to_map,
      bool debug_on_start) {
    return SpawnChildImpl(procname, fds_to_map, debug_on_start);
  }
#endif

 private:
#if defined(OS_WIN)
  base::ProcessHandle SpawnChildImpl(
      const std::wstring& procname,
      bool debug_on_start) {
    CommandLine cl(*CommandLine::ForCurrentProcess());
    base::ProcessHandle handle = static_cast<base::ProcessHandle>(NULL);
    cl.AppendSwitchWithValue(kRunClientProcess, procname);

    if (debug_on_start)
      cl.AppendSwitch(switches::kDebugOnStart);

    base::LaunchApp(cl, false, true, &handle);
    return handle;
  }
#elif defined(OS_POSIX)
  
  
  base::ProcessHandle SpawnChildImpl(
      const std::wstring& procname,
      const base::file_handle_mapping_vector& fds_to_map,
      bool debug_on_start) {
    CommandLine cl(*CommandLine::ForCurrentProcess());
    base::ProcessHandle handle = static_cast<base::ProcessHandle>(NULL);
    cl.AppendSwitchWithValue(kRunClientProcess, procname);

    if (debug_on_start)
      cl.AppendSwitch(switches::kDebugOnStart);

    base::LaunchApp(cl.argv(), fds_to_map, false, &handle);
    return handle;
  }
#endif
};

#endif  
