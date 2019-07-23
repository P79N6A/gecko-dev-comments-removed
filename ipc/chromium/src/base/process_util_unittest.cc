



#define _CRT_SECURE_NO_WARNINGS

#include "base/command_line.h"
#include "base/eintr_wrapper.h"
#include "base/file_path.h"
#include "base/multiprocess_test.h"
#include "base/path_service.h"
#include "base/platform_thread.h"
#include "base/process_util.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_LINUX)
#include <dlfcn.h>
#endif
#if defined(OS_POSIX)
#include <fcntl.h>
#include <sys/socket.h>
#endif
#if defined(OS_WIN)
#include <windows.h>
#endif

namespace base {

class ProcessUtilTest : public MultiProcessTest {
};

MULTIPROCESS_TEST_MAIN(SimpleChildProcess) {
  return 0;
}

TEST_F(ProcessUtilTest, SpawnChild) {
  ProcessHandle handle = this->SpawnChild(L"SimpleChildProcess");

  ASSERT_NE(static_cast<ProcessHandle>(NULL), handle);
  EXPECT_TRUE(WaitForSingleProcess(handle, 5000));
  base::CloseProcessHandle(handle);
}

MULTIPROCESS_TEST_MAIN(SlowChildProcess) {
  
  FILE *fp;
  do {
    PlatformThread::Sleep(100);
    fp = fopen("SlowChildProcess.die", "r");
  } while (!fp);
  fclose(fp);
  remove("SlowChildProcess.die");
  exit(0);
  return 0;
}

TEST_F(ProcessUtilTest, KillSlowChild) {
  remove("SlowChildProcess.die");
  ProcessHandle handle = this->SpawnChild(L"SlowChildProcess");
  ASSERT_NE(static_cast<ProcessHandle>(NULL), handle);
  FILE *fp = fopen("SlowChildProcess.die", "w");
  fclose(fp);
  EXPECT_TRUE(base::WaitForSingleProcess(handle, 5000));
  base::CloseProcessHandle(handle);
}


#if defined(OS_WIN)
TEST_F(ProcessUtilTest, EnableLFH) {
  ASSERT_TRUE(EnableLowFragmentationHeap());
  if (IsDebuggerPresent()) {
    
    
    const char* no_debug_env = getenv("_NO_DEBUG_HEAP");
    if (!no_debug_env || strcmp(no_debug_env, "1"))
      return;
  }
  HANDLE heaps[1024] = { 0 };
  unsigned number_heaps = GetProcessHeaps(1024, heaps);
  EXPECT_GT(number_heaps, 0u);
  for (unsigned i = 0; i < number_heaps; ++i) {
    ULONG flag = 0;
    SIZE_T length;
    ASSERT_NE(0, HeapQueryInformation(heaps[i],
                                      HeapCompatibilityInformation,
                                      &flag,
                                      sizeof(flag),
                                      &length));
    
    
    
    

    
    EXPECT_LE(flag, 2u);
    EXPECT_NE(flag, 1u);
  }
}

TEST_F(ProcessUtilTest, CalcFreeMemory) {
  ProcessMetrics* metrics =
      ProcessMetrics::CreateProcessMetrics(::GetCurrentProcess());
  ASSERT_TRUE(NULL != metrics);

  
  
  FreeMBytes free_mem1 = {0};
  EXPECT_TRUE(metrics->CalculateFreeMemory(&free_mem1));
  EXPECT_LT(10u, free_mem1.total);
  EXPECT_LT(10u, free_mem1.largest);
  EXPECT_GT(2048u, free_mem1.total);
  EXPECT_GT(2048u, free_mem1.largest);
  EXPECT_GE(free_mem1.total, free_mem1.largest);
  EXPECT_TRUE(NULL != free_mem1.largest_ptr);

  
  const int kAllocMB = 20;
  char* alloc = new char[kAllocMB * 1024 * 1024];
  EXPECT_TRUE(NULL != alloc);

  size_t expected_total = free_mem1.total - kAllocMB;
  size_t expected_largest = free_mem1.largest;

  FreeMBytes free_mem2 = {0};
  EXPECT_TRUE(metrics->CalculateFreeMemory(&free_mem2));
  EXPECT_GE(free_mem2.total, free_mem2.largest);
  EXPECT_GE(expected_total, free_mem2.total);
  EXPECT_GE(expected_largest, free_mem2.largest);
  EXPECT_TRUE(NULL != free_mem2.largest_ptr);

  delete[] alloc;
  delete metrics;
}

TEST_F(ProcessUtilTest, GetAppOutput) {
  
  std::string message;
  for (int i = 0; i < 1025; i++) {  
                                    
    message += "Hello!";
  }

  FilePath python_runtime;
  ASSERT_TRUE(PathService::Get(base::DIR_SOURCE_ROOT, &python_runtime));
  python_runtime = python_runtime.Append(FILE_PATH_LITERAL("third_party"))
                                 .Append(FILE_PATH_LITERAL("python_24"))
                                 .Append(FILE_PATH_LITERAL("python.exe"));

  
  
  std::wstring cmd_line = L"\"" + python_runtime.value() + L"\" " +
      L"\"-c\" \"import sys; sys.stdout.write('" + ASCIIToWide(message) +
      L"');\"";
  std::string output;
  ASSERT_TRUE(base::GetAppOutput(cmd_line, &output));
  EXPECT_EQ(message, output);

  
  cmd_line = L"\"" + python_runtime.value() + L"\" " +
      L"\"-c\" \"import sys; sys.stderr.write('Hello!');\"";
  output.clear();
  ASSERT_TRUE(base::GetAppOutput(cmd_line, &output));
  EXPECT_EQ("", output);
}
#endif  

#if defined(OS_POSIX)


int GetMaxFilesOpenInProcess() {
  struct rlimit rlim;
  if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
    return 0;
  }

  
  
  rlim_t max_int = static_cast<rlim_t>(std::numeric_limits<int32>::max());
  if (rlim.rlim_cur > max_int) {
    return max_int;
  }

  return rlim.rlim_cur;
}

const int kChildPipe = 20;  
MULTIPROCESS_TEST_MAIN(ProcessUtilsLeakFDChildProcess) {
  
  
  int num_open_files = 0;
  int write_pipe = kChildPipe;
  int max_files = GetMaxFilesOpenInProcess();
  for (int i = STDERR_FILENO + 1; i < max_files; i++) {
    if (i != kChildPipe) {
      if (HANDLE_EINTR(close(i)) != -1) {
        LOG(WARNING) << "Leaked FD " << i;
        num_open_files += 1;
      }
    }
  }

  
  int expected_num_open_fds = 1;
#if defined(OS_LINUX)
  
  expected_num_open_fds += 1;
#endif  
  num_open_files -= expected_num_open_fds;

  int written = HANDLE_EINTR(write(write_pipe, &num_open_files,
                                   sizeof(num_open_files)));
  DCHECK_EQ(static_cast<size_t>(written), sizeof(num_open_files));
  HANDLE_EINTR(close(write_pipe));

  return 0;
}

TEST_F(ProcessUtilTest, FDRemapping) {
  
  int fds[2];
  if (pipe(fds) < 0)
    NOTREACHED();
  int pipe_read_fd = fds[0];
  int pipe_write_fd = fds[1];

  
  
  int dev_null = open("/dev/null", O_RDONLY);
  int sockets[2];
  socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);

  file_handle_mapping_vector fd_mapping_vec;
  fd_mapping_vec.push_back(std::pair<int,int>(pipe_write_fd, kChildPipe));
  ProcessHandle handle = this->SpawnChild(L"ProcessUtilsLeakFDChildProcess",
                                          fd_mapping_vec,
                                          false);
  ASSERT_NE(static_cast<ProcessHandle>(NULL), handle);
  HANDLE_EINTR(close(pipe_write_fd));

  
  int num_open_files = -1;
  ssize_t bytes_read =
      HANDLE_EINTR(read(pipe_read_fd, &num_open_files, sizeof(num_open_files)));
  ASSERT_EQ(bytes_read, static_cast<ssize_t>(sizeof(num_open_files)));

  
  ASSERT_EQ(0, num_open_files);

  EXPECT_TRUE(WaitForSingleProcess(handle, 1000));
  base::CloseProcessHandle(handle);
  HANDLE_EINTR(close(fds[0]));
  HANDLE_EINTR(close(sockets[0]));
  HANDLE_EINTR(close(sockets[1]));
  HANDLE_EINTR(close(dev_null));
}

TEST_F(ProcessUtilTest, GetAppOutput) {
  std::string output;
  EXPECT_TRUE(GetAppOutput(CommandLine(L"true"), &output));
  EXPECT_STREQ("", output.c_str());

  EXPECT_FALSE(GetAppOutput(CommandLine(L"false"), &output));

  std::vector<std::string> argv;
  argv.push_back("/bin/echo");
  argv.push_back("-n");
  argv.push_back("foobar42");
  EXPECT_TRUE(GetAppOutput(CommandLine(argv), &output));
  EXPECT_STREQ("foobar42", output.c_str());
}

#endif  

}  
