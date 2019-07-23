




























#include <unistd.h>
#include <sys/syscall.h>

#include "client/linux/handler/exception_handler.h"
#include "client/linux/minidump_writer/minidump_writer.h"
#include "breakpad_googletest_includes.h"

using namespace google_breakpad;



#define HANDLE_EINTR(x) ({ \
  typeof(x) __eintr_result__; \
  do { \
    __eintr_result__ = x; \
  } while (__eintr_result__ == -1 && errno == EINTR); \
  __eintr_result__;\
})

namespace {
typedef testing::Test MinidumpWriterTest;
}

TEST(MinidumpWriterTest, Setup) {
  int fds[2];
  ASSERT_NE(-1, pipe(fds));

  const pid_t child = fork();
  if (child == 0) {
    close(fds[1]);
    char b;
    HANDLE_EINTR(read(fds[0], &b, sizeof(b)));
    close(fds[0]);
    syscall(__NR_exit);
  }
  close(fds[0]);

  ExceptionHandler::CrashContext context;
  memset(&context, 0, sizeof(context));

  char templ[] = "/tmp/minidump-writer-unittest-XXXXXX";
  mktemp(templ);
  ASSERT_TRUE(WriteMinidump(templ, child, &context, sizeof(context)));
  struct stat st;
  ASSERT_EQ(stat(templ, &st), 0);
  ASSERT_GT(st.st_size, 0u);
  unlink(templ);

  close(fds[1]);
}
