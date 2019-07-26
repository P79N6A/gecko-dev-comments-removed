




























#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "client/linux/minidump_writer/line_reader.h"
#include "breakpad_googletest_includes.h"
#include "common/linux/eintr_wrapper.h"

using namespace google_breakpad;

#if !defined(__ANDROID__)
#define TEMPDIR "/tmp"
#else
#define TEMPDIR "/data/local/tmp"
#endif

static int TemporaryFile() {
  static const char templ[] = TEMPDIR "/line-reader-unittest-XXXXXX";
  char templ_copy[sizeof(templ)];
  memcpy(templ_copy, templ, sizeof(templ));
  const int fd = mkstemp(templ_copy);
  if (fd >= 0)
    unlink(templ_copy);

  return fd;
}

namespace {
typedef testing::Test LineReaderTest;
}

TEST(LineReaderTest, EmptyFile) {
  const int fd = TemporaryFile();
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}

TEST(LineReaderTest, OneLineTerminated) {
  const int fd = TemporaryFile();
  const int r = HANDLE_EINTR(write(fd, "a\n", 2));
  ASSERT_EQ(2, r);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned int len;
  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ((unsigned int)1, len);
  ASSERT_EQ('a', line[0]);
  ASSERT_EQ('\0', line[1]);
  reader.PopLine(len);

  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}

TEST(LineReaderTest, OneLine) {
  const int fd = TemporaryFile();
  const int r = HANDLE_EINTR(write(fd, "a", 1));
  ASSERT_EQ(1, r);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ((unsigned)1, len);
  ASSERT_EQ('a', line[0]);
  ASSERT_EQ('\0', line[1]);
  reader.PopLine(len);

  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}

TEST(LineReaderTest, TwoLinesTerminated) {
  const int fd = TemporaryFile();
  const int r = HANDLE_EINTR(write(fd, "a\nb\n", 4));
  ASSERT_EQ(4, r);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ((unsigned)1, len);
  ASSERT_EQ('a', line[0]);
  ASSERT_EQ('\0', line[1]);
  reader.PopLine(len);

  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ((unsigned)1, len);
  ASSERT_EQ('b', line[0]);
  ASSERT_EQ('\0', line[1]);
  reader.PopLine(len);

  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}

TEST(LineReaderTest, TwoLines) {
  const int fd = TemporaryFile();
  const int r = HANDLE_EINTR(write(fd, "a\nb", 3));
  ASSERT_EQ(3, r);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ((unsigned)1, len);
  ASSERT_EQ('a', line[0]);
  ASSERT_EQ('\0', line[1]);
  reader.PopLine(len);

  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ((unsigned)1, len);
  ASSERT_EQ('b', line[0]);
  ASSERT_EQ('\0', line[1]);
  reader.PopLine(len);

  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}

TEST(LineReaderTest, MaxLength) {
  const int fd = TemporaryFile();
  char l[LineReader::kMaxLineLen - 1];
  memset(l, 'a', sizeof(l));
  const int r = HANDLE_EINTR(write(fd, l, sizeof(l)));
  ASSERT_EQ(static_cast<ssize_t>(sizeof(l)), r);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ(sizeof(l), len);
  ASSERT_TRUE(memcmp(l, line, sizeof(l)) == 0);
  ASSERT_EQ('\0', line[len]);

  close(fd);
}

TEST(LineReaderTest, TooLong) {
  const int fd = TemporaryFile();
  char l[LineReader::kMaxLineLen];
  memset(l, 'a', sizeof(l));
  const int r = HANDLE_EINTR(write(fd, l, sizeof(l)));
  ASSERT_EQ(static_cast<ssize_t>(sizeof(l)), r);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}
