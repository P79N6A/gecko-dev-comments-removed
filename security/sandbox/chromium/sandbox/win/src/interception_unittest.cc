







#include <windows.h>

#include "base/memory/scoped_ptr.h"
#include "sandbox/win/src/interception.h"
#include "sandbox/win/src/interceptors.h"
#include "sandbox/win/src/interception_internal.h"
#include "sandbox/win/src/target_process.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {









void WalkBuffer(void* buffer, size_t size, int* num_dlls, int* num_functions,
                int* num_names) {
  ASSERT_TRUE(NULL != buffer);
  ASSERT_TRUE(NULL != num_functions);
  ASSERT_TRUE(NULL != num_names);
  *num_dlls = *num_functions = *num_names = 0;
  SharedMemory *memory = reinterpret_cast<SharedMemory*>(buffer);

  ASSERT_GT(size, sizeof(SharedMemory));
  DllPatchInfo *dll = &memory->dll_list[0];

  for (int i = 0; i < memory->num_intercepted_dlls; i++) {
    ASSERT_NE(0u, wcslen(dll->dll_name));
    ASSERT_EQ(0u, dll->record_bytes % sizeof(size_t));
    ASSERT_EQ(0u, dll->offset_to_functions % sizeof(size_t));
    ASSERT_NE(0, dll->num_functions);

    FunctionInfo *function = reinterpret_cast<FunctionInfo*>(
      reinterpret_cast<char*>(dll) + dll->offset_to_functions);

    for (int j = 0; j < dll->num_functions; j++) {
      ASSERT_EQ(0u, function->record_bytes % sizeof(size_t));

      char* name = function->function;
      size_t length = strlen(name);
      ASSERT_NE(0u, length);
      name += length + 1;

      
      ASSERT_GT(reinterpret_cast<char*>(buffer) + size, name + strlen(name));

      
      if (strlen(name)) {
        (*num_names)++;
        EXPECT_TRUE(NULL == function->interceptor_address);
      } else {
        EXPECT_TRUE(NULL != function->interceptor_address);
      }

      (*num_functions)++;
      function = reinterpret_cast<FunctionInfo*>(
        reinterpret_cast<char*>(function) + function->record_bytes);
    }

    (*num_dlls)++;
    dll = reinterpret_cast<DllPatchInfo*>(reinterpret_cast<char*>(dll) +
                                          dll->record_bytes);
  }
}

TEST(InterceptionManagerTest, BufferLayout1) {
  wchar_t exe_name[MAX_PATH];
  ASSERT_NE(0u, GetModuleFileName(NULL, exe_name, MAX_PATH - 1));

  TargetProcess *target = MakeTestTargetProcess(::GetCurrentProcess(),
                                                ::GetModuleHandle(exe_name));

  InterceptionManager interceptions(target, true);

  
  void* function = &interceptions;

  
  interceptions.AddToPatchedFunctions(L"ntdll.dll", "NtCreateFile",
                                      INTERCEPTION_SERVICE_CALL, function,
                                      OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"kernel32.dll", "CreateFileEx",
                                      INTERCEPTION_EAT, function, OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"kernel32.dll", "SomeFileEx",
                                      INTERCEPTION_SMART_SIDESTEP, function,
                                      OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"user32.dll", "FindWindow",
                                      INTERCEPTION_EAT, function, OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"kernel32.dll", "CreateMutex",
                                      INTERCEPTION_EAT, function, OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"user32.dll", "PostMsg",
                                      INTERCEPTION_EAT, function, OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"user32.dll", "PostMsg",
                                      INTERCEPTION_EAT, "replacement",
                                      OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"comctl.dll", "SaveAsDlg",
                                      INTERCEPTION_EAT, function, OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"ntdll.dll", "NtClose",
                                      INTERCEPTION_SERVICE_CALL, function,
                                      OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"ntdll.dll", "NtOpenFile",
                                      INTERCEPTION_SIDESTEP, function,
                                      OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"some.dll", "Superfn",
                                      INTERCEPTION_EAT, function, OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"comctl.dll", "SaveAsDlg",
                                      INTERCEPTION_EAT, "a", OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"comctl.dll", "SaveAsDlg",
                                      INTERCEPTION_SIDESTEP, "ab", OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"comctl.dll", "SaveAsDlg",
                                      INTERCEPTION_EAT, "abc", OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"a.dll", "p",
                                      INTERCEPTION_EAT, function, OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"b.dll",
                                      "TheIncredibleCallToSaveTheWorld",
                                      INTERCEPTION_EAT, function, OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"a.dll", "BIsLame",
                                      INTERCEPTION_EAT, function, OPEN_KEY_ID);
  interceptions.AddToPatchedFunctions(L"a.dll", "ARules",
                                      INTERCEPTION_EAT, function, OPEN_KEY_ID);

  
  ASSERT_EQ(18, interceptions.interceptions_.size());

  size_t buffer_size = interceptions.GetBufferSize();
  scoped_ptr<BYTE[]> local_buffer(new BYTE[buffer_size]);

  ASSERT_TRUE(interceptions.SetupConfigBuffer(local_buffer.get(),
                                              buffer_size));

  
  
  
  
  
  
  
  
  EXPECT_EQ(3, interceptions.interceptions_.size());

  int num_dlls, num_functions, num_names;
  WalkBuffer(local_buffer.get(), buffer_size, &num_dlls, &num_functions,
             &num_names);

  
  
  
  EXPECT_EQ(6, num_dlls);
  EXPECT_EQ(15, num_functions);
  EXPECT_EQ(4, num_names);
}

TEST(InterceptionManagerTest, BufferLayout2) {
  wchar_t exe_name[MAX_PATH];
  ASSERT_NE(0u, GetModuleFileName(NULL, exe_name, MAX_PATH - 1));

  TargetProcess *target = MakeTestTargetProcess(::GetCurrentProcess(),
                                                ::GetModuleHandle(exe_name));

  InterceptionManager interceptions(target, true);

  
  void* function = &interceptions;
  interceptions.AddToUnloadModules(L"some01.dll");
  
  interceptions.AddToPatchedFunctions(L"ntdll.dll", "NtCreateFile",
                                      INTERCEPTION_SERVICE_CALL, function,
                                      OPEN_FILE_ID);
  interceptions.AddToPatchedFunctions(L"kernel32.dll", "CreateFileEx",
                                      INTERCEPTION_EAT, function, OPEN_FILE_ID);
  interceptions.AddToUnloadModules(L"some02.dll");
  interceptions.AddToPatchedFunctions(L"kernel32.dll", "SomeFileEx",
                                      INTERCEPTION_SMART_SIDESTEP, function,
                                      OPEN_FILE_ID);
  
  ASSERT_EQ(5, interceptions.interceptions_.size());

  size_t buffer_size = interceptions.GetBufferSize();
  scoped_ptr<BYTE[]> local_buffer(new BYTE[buffer_size]);

  ASSERT_TRUE(interceptions.SetupConfigBuffer(local_buffer.get(),
                                              buffer_size));

  
  
  
  
  
  EXPECT_EQ(1, interceptions.interceptions_.size());

  int num_dlls, num_functions, num_names;
  WalkBuffer(local_buffer.get(), buffer_size, &num_dlls, &num_functions,
             &num_names);

  EXPECT_EQ(3, num_dlls);
  EXPECT_EQ(4, num_functions);
  EXPECT_EQ(0, num_names);
}

}  
