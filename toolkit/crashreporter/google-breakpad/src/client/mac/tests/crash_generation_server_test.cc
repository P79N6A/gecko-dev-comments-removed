































#include <dirent.h>
#include <glob.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string>

#include "breakpad_googletest_includes.h"
#include "client/mac/crash_generation/client_info.h"
#include "client/mac/crash_generation/crash_generation_client.h"
#include "client/mac/crash_generation/crash_generation_server.h"
#include "client/mac/handler/exception_handler.h"
#include "client/mac/tests/spawn_child_process.h"
#include "common/tests/auto_tempdir.h"
#include "google_breakpad/processor/minidump.h"

namespace google_breakpad {



std::ostringstream info_log;
}

namespace {
using std::string;
using google_breakpad::AutoTempDir;
using google_breakpad::ClientInfo;
using google_breakpad::CrashGenerationClient;
using google_breakpad::CrashGenerationServer;
using google_breakpad::ExceptionHandler;
using google_breakpad::Minidump;
using google_breakpad::MinidumpContext;
using google_breakpad::MinidumpException;
using google_breakpad::MinidumpModule;
using google_breakpad::MinidumpModuleList;
using google_breakpad::MinidumpSystemInfo;
using google_breakpad::MinidumpThread;
using google_breakpad::MinidumpThreadList;
using testing::Test;
using namespace google_breakpad_test;

class CrashGenerationServerTest : public Test {
public:
  
  char mach_port_name[128];
  
  string last_dump_name;
  
  pid_t child_pid;
  
  AutoTempDir temp_dir;
  
  static int i;
  bool filter_callback_called;

  void SetUp() {
    sprintf(mach_port_name,
            "com.google.breakpad.ServerTest.%d.%d", getpid(),
            CrashGenerationServerTest::i++);
    child_pid = (pid_t)-1;
    filter_callback_called = false;
  }
};
int CrashGenerationServerTest::i = 0;


TEST_F(CrashGenerationServerTest, testStartStopServer) {
  CrashGenerationServer server(mach_port_name,
                               NULL,  
                               NULL,  
                               NULL,  
                               NULL,  
                               NULL,  
                               NULL,  
                               false, 
                               ""); 
  ASSERT_TRUE(server.Start());
  ASSERT_TRUE(server.Stop());
}



TEST_F(CrashGenerationServerTest, testRequestDumpNoDump) {
  CrashGenerationServer server(mach_port_name,
                               NULL,  
                               NULL,  
                               NULL,  
                               NULL,  
                               NULL,  
                               NULL,  
                               false, 
                               temp_dir.path()); 
  ASSERT_TRUE(server.Start());

  pid_t pid = fork();
  ASSERT_NE(-1, pid);
  if (pid == 0) {
    CrashGenerationClient client(mach_port_name);
    bool result = client.RequestDump();
    exit(result ? 0 : 1);
  }

  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_TRUE(WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));
  EXPECT_TRUE(server.Stop());
  
  string pattern = temp_dir.path() + "/*";
  glob_t dirContents;
  ret = glob(pattern.c_str(), GLOB_NOSORT, NULL, &dirContents);
  EXPECT_EQ(GLOB_NOMATCH, ret);
  if (ret != GLOB_NOMATCH)
    globfree(&dirContents);
}

void dumpCallback(void *context, const ClientInfo &client_info,
                  const std::string &file_path) {
  if (context) {
    CrashGenerationServerTest* self =
        reinterpret_cast<CrashGenerationServerTest*>(context);
    if (!file_path.empty())
      self->last_dump_name = file_path;
    self->child_pid = client_info.pid();
  }
}

void *RequestDump(void *context) {
  CrashGenerationClient client((const char*)context);
  bool result = client.RequestDump();
  return (void*)(result ? 0 : 1);
}


TEST_F(CrashGenerationServerTest, testRequestDump) {
  CrashGenerationServer server(mach_port_name,
                               NULL,  
                               NULL,  
                               dumpCallback,  
                               this,  
                               NULL,  
                               NULL,  
                               true, 
                               temp_dir.path()); 
  ASSERT_TRUE(server.Start());

  pid_t pid = fork();
  ASSERT_NE(-1, pid);
  if (pid == 0) {
    
    
    
    pthread_t thread;
    if (pthread_create(&thread, NULL, RequestDump, (void*)mach_port_name) != 0)
      exit(1);
    void* result;
    pthread_join(thread, &result);
    exit(reinterpret_cast<intptr_t>(result));
  }

  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_TRUE(WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));
  EXPECT_TRUE(server.Stop());
  
  ASSERT_FALSE(last_dump_name.empty());
  struct stat st;
  EXPECT_EQ(0, stat(last_dump_name.c_str(), &st));
  EXPECT_LT(0, st.st_size);
  
  ASSERT_EQ(pid, child_pid);
}

static void Crasher() {
  int *a = (int*)0x42;

  fprintf(stdout, "Going to crash...\n");
  fprintf(stdout, "A = %d", *a);
}




TEST_F(CrashGenerationServerTest, testChildProcessCrash) {
  CrashGenerationServer server(mach_port_name,
                               NULL,  
                               NULL,  
                               dumpCallback,  
                               this,  
                               NULL,  
                               NULL,  
                               true, 
                               temp_dir.path()); 
  ASSERT_TRUE(server.Start());

  pid_t pid = fork();
  ASSERT_NE(-1, pid);
  if (pid == 0) {
    
    ExceptionHandler eh("", NULL, NULL, NULL, true, mach_port_name);
    Crasher();
    
    exit(0);
  }

  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_FALSE(WIFEXITED(ret));
  EXPECT_TRUE(server.Stop());
  
  ASSERT_FALSE(last_dump_name.empty());
  struct stat st;
  EXPECT_EQ(0, stat(last_dump_name.c_str(), &st));
  EXPECT_LT(0, st.st_size);

  
  Minidump minidump(last_dump_name.c_str());
  ASSERT_TRUE(minidump.Read());

  MinidumpSystemInfo* system_info = minidump.GetSystemInfo();
  ASSERT_TRUE(system_info);
  const MDRawSystemInfo* raw_info = system_info->system_info();
  ASSERT_TRUE(raw_info);
  EXPECT_EQ(kNativeArchitecture, raw_info->processor_architecture);

  MinidumpThreadList* thread_list = minidump.GetThreadList();
  ASSERT_TRUE(thread_list);
  ASSERT_EQ((unsigned int)1, thread_list->thread_count());

  MinidumpThread* main_thread = thread_list->GetThreadAtIndex(0);
  ASSERT_TRUE(main_thread);
  MinidumpContext* context = main_thread->GetContext();
  ASSERT_TRUE(context);
  EXPECT_EQ(kNativeContext, context->GetContextCPU());

  MinidumpModuleList* module_list = minidump.GetModuleList();
  ASSERT_TRUE(module_list);
  const MinidumpModule* main_module = module_list->GetMainModule();
  ASSERT_TRUE(main_module);
  EXPECT_EQ(GetExecutablePath(), main_module->code_file());
}

#if (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6) && \
  (defined(__x86_64__) || defined(__i386__))


TEST_F(CrashGenerationServerTest, testChildProcessCrashCrossArchitecture) {
  CrashGenerationServer server(mach_port_name,
                               NULL,  
                               NULL,  
                               dumpCallback,  
                               this,  
                               NULL,  
                               NULL,  
                               true, 
                               temp_dir.path()); 
  ASSERT_TRUE(server.Start());

  
  string helper_path = GetHelperPath();
  const char* argv[] = {
    helper_path.c_str(),
    "crash",
    mach_port_name,
    NULL
  };
  pid_t pid = spawn_child_process(argv);
  ASSERT_NE(-1, pid);

  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_FALSE(WIFEXITED(ret));
  EXPECT_TRUE(server.Stop());
  
  ASSERT_FALSE(last_dump_name.empty());
  struct stat st;
  EXPECT_EQ(0, stat(last_dump_name.c_str(), &st));
  EXPECT_LT(0, st.st_size);

const MDCPUArchitecture kExpectedArchitecture =
#if defined(__x86_64__)
  MD_CPU_ARCHITECTURE_X86
#elif defined(__i386__)
  MD_CPU_ARCHITECTURE_AMD64
#endif
  ;
const uint32_t kExpectedContext =
#if defined(__i386__)
  MD_CONTEXT_AMD64
#elif defined(__x86_64__)
  MD_CONTEXT_X86
#endif
  ;

  
  Minidump minidump(last_dump_name.c_str());
  ASSERT_TRUE(minidump.Read());

  MinidumpSystemInfo* system_info = minidump.GetSystemInfo();
  ASSERT_TRUE(system_info);
  const MDRawSystemInfo* raw_info = system_info->system_info();
  ASSERT_TRUE(raw_info);
  EXPECT_EQ(kExpectedArchitecture, raw_info->processor_architecture);

  MinidumpThreadList* thread_list = minidump.GetThreadList();
  ASSERT_TRUE(thread_list);
  ASSERT_EQ((unsigned int)1, thread_list->thread_count());

  MinidumpThread* main_thread = thread_list->GetThreadAtIndex(0);
  ASSERT_TRUE(main_thread);
  MinidumpContext* context = main_thread->GetContext();
  ASSERT_TRUE(context);
  EXPECT_EQ(kExpectedContext, context->GetContextCPU());

  MinidumpModuleList* module_list = minidump.GetModuleList();
  ASSERT_TRUE(module_list);
  const MinidumpModule* main_module = module_list->GetMainModule();
  ASSERT_TRUE(main_module);
  EXPECT_EQ(helper_path, main_module->code_file());
}
#endif

bool filter_callback(void* context) {
  CrashGenerationServerTest* self =
    reinterpret_cast<CrashGenerationServerTest*>(context);
  self->filter_callback_called = true;
  
  return false;
}


TEST_F(CrashGenerationServerTest, testFilter) {
  CrashGenerationServer server(mach_port_name,
                               filter_callback,  
                               this,            
                               dumpCallback,  
                               this,  
                               NULL,  
                               NULL,  
                               true, 
                               temp_dir.path()); 
  ASSERT_TRUE(server.Start());

  pid_t pid = fork();
  ASSERT_NE(-1, pid);
  if (pid == 0) {
    
    ExceptionHandler eh("", NULL, NULL, NULL, true, mach_port_name);
    Crasher();
    
    exit(0);
  }

  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_FALSE(WIFEXITED(ret));
  EXPECT_TRUE(server.Stop());

  
  EXPECT_TRUE(last_dump_name.empty());
  EXPECT_TRUE(filter_callback_called);
}

}  
