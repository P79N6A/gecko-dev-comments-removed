






























#include <sys/stat.h>
#include <unistd.h>

#include "breakpad_googletest_includes.h"
#include "client/mac/handler/exception_handler.h"
#include "client/mac/tests/auto_tempdir.h"
#include "common/mac/MachIPC.h"

namespace {
using std::string;
using google_breakpad::AutoTempDir;
using google_breakpad::ExceptionHandler;
using google_breakpad::MachPortSender;
using google_breakpad::MachReceiveMessage;
using google_breakpad::MachSendMessage;
using google_breakpad::ReceivePort;
using testing::Test;

class ExceptionHandlerTest : public Test {
 public:
  AutoTempDir tempDir;
  string lastDumpName;
};

static void Crasher() {
  int *a = (int*)0x42;

  fprintf(stdout, "Going to crash...\n");
  fprintf(stdout, "A = %d", *a);
}

static void SoonToCrash() {
  Crasher();
}

static bool MDCallback(const char *dump_dir, const char *file_name,
                       void *context, bool success) {
  string path(dump_dir);
  path.append("/");
  path.append(file_name);
  path.append(".dmp");

  int fd = *reinterpret_cast<int*>(context);
  (void)write(fd, path.c_str(), path.length() + 1);
  close(fd);
  exit(0);
  
  return true;
}

TEST_F(ExceptionHandlerTest, InProcess) {
  AutoTempDir tempDir;
  
  int fds[2];
  ASSERT_EQ(0, pipe(fds));
  
  pid_t pid = fork();
  if (pid == 0) {
    
    close(fds[0]);
    ExceptionHandler eh(tempDir.path, NULL, MDCallback, &fds[1], true, NULL);
    
    SoonToCrash();
    
    exit(1);
  }
  
  ASSERT_NE(-1, pid);
  
  close(fds[1]);
  char minidump_file[PATH_MAX];
  ssize_t nbytes = read(fds[0], minidump_file, sizeof(minidump_file));
  ASSERT_NE(0, nbytes);
  
  struct stat st;
  ASSERT_EQ(0, stat(minidump_file, &st));
  ASSERT_LT(0, st.st_size);

  
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));
}

static bool ChildMDCallback(const char *dump_dir, const char *file_name,
			    void *context, bool success) {
  ExceptionHandlerTest *self = reinterpret_cast<ExceptionHandlerTest*>(context);
  if (dump_dir && file_name) {
    self->lastDumpName = dump_dir;
    self->lastDumpName += "/";
    self->lastDumpName += file_name;
    self->lastDumpName += ".dmp";
  }
  return true;
}

TEST_F(ExceptionHandlerTest, DumpChildProcess) {
  const int kTimeoutMs = 2000;
  
  char machPortName[128];
  sprintf(machPortName, "ExceptionHandlerTest.%d", getpid());
  ReceivePort parent_recv_port(machPortName);

  
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  
  pid_t pid = fork();
  if (pid == 0) {
    
    close(fds[0]);

    
    MachSendMessage child_message(0);
    child_message.AddDescriptor(mach_task_self());
    child_message.AddDescriptor(mach_thread_self());

    MachPortSender child_sender(machPortName);
    if (child_sender.SendMessage(child_message, kTimeoutMs) != KERN_SUCCESS)
      exit(1);

    
    uint8_t data;
    read(fds[1], &data, 1);
    exit(0);
  }
  
  ASSERT_NE(-1, pid);
  close(fds[1]);

  
  MachReceiveMessage child_message;
  ASSERT_EQ(KERN_SUCCESS,
	    parent_recv_port.WaitForMessage(&child_message, kTimeoutMs));
  mach_port_t child_task = child_message.GetTranslatedPort(0);
  mach_port_t child_thread = child_message.GetTranslatedPort(1);
  ASSERT_NE(MACH_PORT_NULL, child_task);
  ASSERT_NE(MACH_PORT_NULL, child_thread);

  
  bool result = ExceptionHandler::WriteMinidumpForChild(child_task,
							child_thread,
							tempDir.path,
							ChildMDCallback,
							this);
  ASSERT_EQ(true, result);

  
  ASSERT_FALSE(lastDumpName.empty());
  struct stat st;
  ASSERT_EQ(0, stat(lastDumpName.c_str(), &st));
  ASSERT_LT(0, st.st_size);

  
  uint8_t data = 1;
  (void)write(fds[0], &data, 1);

  
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));
}

}
