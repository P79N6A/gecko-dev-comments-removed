

































#include <string>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "breakpad_googletest_includes.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/process_state.h"
#include "google_breakpad/processor/network_source_line_resolver.h"
#include "processor/simple_symbol_supplier.h"
#include "processor/network_source_line_server.h"
#include "processor/simple_symbol_supplier.h"
#include "processor/udp_network.h"

namespace {

using std::string;
using google_breakpad::BasicSourceLineResolver;
using google_breakpad::CallStack;
using google_breakpad::MinidumpProcessor;
using google_breakpad::NetworkSourceLineResolver;
using google_breakpad::NetworkSourceLineServer;
using google_breakpad::ProcessState;
using google_breakpad::SimpleSymbolSupplier;
using google_breakpad::UDPNetwork;

static const char *kSystemInfoOS = "Windows NT";
static const char *kSystemInfoOSShort = "windows";
static const char *kSystemInfoOSVersion = "5.1.2600 Service Pack 2";
static const char *kSystemInfoCPU = "x86";
static const char *kSystemInfoCPUInfo =
    "GenuineIntel family 6 model 13 stepping 8";

bool exitProcess = false;

void signal_handler(int signal) {
  if (signal == SIGINT)
    exitProcess = true;
}

void RunSourceLineServer(int fd) {
  
  
  signal(SIGINT, signal_handler);

  BasicSourceLineResolver resolver;
  SimpleSymbolSupplier supplier(string(getenv("srcdir") ?
                                       getenv("srcdir") : ".") +
                                "/src/processor/testdata/symbols/");
  UDPNetwork net("localhost",
                 0,    
                 true); 

  NetworkSourceLineServer server(&supplier, &resolver, &net,
                                 0);   
  unsigned short port = -1;
  bool initialized = server.Initialize();
  if (initialized)
    port = net.port();

  
  ssize_t written = write(fd, &port, sizeof(port));
  close(fd);

  if (!initialized || written != sizeof(port))
    return;

  while (!exitProcess) {
    server.RunOnce(100);
  }
}

TEST(NetworkSourceLineResolverServer, SystemTest) {
  int fds[2];
  ASSERT_EQ(0, pipe(fds));
  
  pid_t pid = fork();
  if (pid == 0) {
    close(fds[0]);
    RunSourceLineServer(fds[1]);
    exit(0);
  }
  ASSERT_NE(-1, pid);
  
  close(fds[1]);
  unsigned short port;
  ssize_t nbytes = read(fds[0], &port, sizeof(port));
  ASSERT_EQ(sizeof(port), nbytes);
  ASSERT_NE(-1, port);

  NetworkSourceLineResolver resolver("localhost", port,
                                     5000); 
  MinidumpProcessor processor(&resolver, &resolver);
  
  string minidump_file = string(getenv("srcdir") ? getenv("srcdir") : ".") +
                         "/src/processor/testdata/minidump2.dmp";

  ProcessState state;
  ASSERT_EQ(processor.Process(minidump_file, &state),
            google_breakpad::PROCESS_OK);
  ASSERT_EQ(state.system_info()->os, kSystemInfoOS);
  ASSERT_EQ(state.system_info()->os_short, kSystemInfoOSShort);
  ASSERT_EQ(state.system_info()->os_version, kSystemInfoOSVersion);
  ASSERT_EQ(state.system_info()->cpu, kSystemInfoCPU);
  ASSERT_EQ(state.system_info()->cpu_info, kSystemInfoCPUInfo);
  ASSERT_TRUE(state.crashed());
  ASSERT_EQ(state.crash_reason(), "EXCEPTION_ACCESS_VIOLATION_WRITE");
  ASSERT_EQ(state.crash_address(), 0x45U);
  ASSERT_EQ(state.threads()->size(), size_t(1));
  ASSERT_EQ(state.requesting_thread(), 0);

  CallStack *stack = state.threads()->at(0);
  ASSERT_TRUE(stack);
  ASSERT_EQ(stack->frames()->size(), 4U);

  ASSERT_TRUE(stack->frames()->at(0)->module);
  ASSERT_EQ(stack->frames()->at(0)->module->base_address(), 0x400000U);
  ASSERT_EQ(stack->frames()->at(0)->module->code_file(), "c:\\test_app.exe");
  ASSERT_EQ(stack->frames()->at(0)->function_name,
            "`anonymous namespace'::CrashFunction");
  ASSERT_EQ(stack->frames()->at(0)->source_file_name, "c:\\test_app.cc");
  ASSERT_EQ(stack->frames()->at(0)->source_line, 58);

  ASSERT_TRUE(stack->frames()->at(1)->module);
  ASSERT_EQ(stack->frames()->at(1)->module->base_address(), 0x400000U);
  ASSERT_EQ(stack->frames()->at(1)->module->code_file(), "c:\\test_app.exe");
  ASSERT_EQ(stack->frames()->at(1)->function_name, "main");
  ASSERT_EQ(stack->frames()->at(1)->source_file_name, "c:\\test_app.cc");
  ASSERT_EQ(stack->frames()->at(1)->source_line, 65);

  
  ASSERT_TRUE(stack->frames()->at(2)->module);
  ASSERT_EQ(stack->frames()->at(2)->module->base_address(), 0x400000U);
  ASSERT_EQ(stack->frames()->at(2)->module->code_file(), "c:\\test_app.exe");
  ASSERT_EQ(stack->frames()->at(2)->function_name, "__tmainCRTStartup");
  ASSERT_EQ(stack->frames()->at(2)->source_file_name,
            "f:\\sp\\vctools\\crt_bld\\self_x86\\crt\\src\\crt0.c");
  ASSERT_EQ(stack->frames()->at(2)->source_line, 327);

  
  ASSERT_TRUE(stack->frames()->at(3)->module);
  ASSERT_EQ(stack->frames()->at(3)->module->base_address(), 0x7c800000U);
  ASSERT_EQ(stack->frames()->at(3)->module->code_file(),
            "C:\\WINDOWS\\system32\\kernel32.dll");
  ASSERT_EQ(stack->frames()->at(3)->function_name, "BaseProcessStart");
  ASSERT_TRUE(stack->frames()->at(3)->source_file_name.empty());
  ASSERT_EQ(stack->frames()->at(3)->source_line, 0);

  ASSERT_EQ(state.modules()->module_count(), 13U);
  ASSERT_TRUE(state.modules()->GetMainModule());
  ASSERT_EQ(state.modules()->GetMainModule()->code_file(), "c:\\test_app.exe");
  ASSERT_FALSE(state.modules()->GetModuleForAddress(0));
  ASSERT_EQ(state.modules()->GetMainModule(),
            state.modules()->GetModuleForAddress(0x400000));
  ASSERT_EQ(state.modules()->GetModuleForAddress(0x7c801234)->debug_file(),
            "kernel32.pdb");
  ASSERT_EQ(state.modules()->GetModuleForAddress(0x77d43210)->version(),
            "5.1.2600.2622");

  
  kill(pid, SIGINT);
  ASSERT_EQ(pid, waitpid(pid, NULL, 0));
}

}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
