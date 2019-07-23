




























#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "client/linux/minidump_writer/line_reader.h"
#include "breakpad_googletest_includes.h"

using namespace google_breakpad;

static int TemporaryFile() {
  static const char templ[] = "/tmp/line-reader-unittest-XXXXXX";
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
  write(fd, "a\n", 2);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ(len, 1);
  ASSERT_EQ(line[0], 'a');
  ASSERT_EQ(line[1], 0);
  reader.PopLine(len);

  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}

TEST(LineReaderTest, OneLine) {
  const int fd = TemporaryFile();
  write(fd, "a", 1);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ(len, 1);
  ASSERT_EQ(line[0], 'a');
  ASSERT_EQ(line[1], 0);
  reader.PopLine(len);

  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}

TEST(LineReaderTest, TwoLinesTerminated) {
  const int fd = TemporaryFile();
  write(fd, "a\nb\n", 4);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ(len, 1);
  ASSERT_EQ(line[0], 'a');
  ASSERT_EQ(line[1], 0);
  reader.PopLine(len);

  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ(len, 1);
  ASSERT_EQ(line[0], 'b');
  ASSERT_EQ(line[1], 0);
  reader.PopLine(len);

  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}

TEST(LineReaderTest, TwoLines) {
  const int fd = TemporaryFile();
  write(fd, "a\nb", 3);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ(len, 1);
  ASSERT_EQ(line[0], 'a');
  ASSERT_EQ(line[1], 0);
  reader.PopLine(len);

  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ(len, 1);
  ASSERT_EQ(line[0], 'b');
  ASSERT_EQ(line[1], 0);
  reader.PopLine(len);

  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}

TEST(LineReaderTest, MaxLength) {
  const int fd = TemporaryFile();
  char l[LineReader::kMaxLineLen - 1];
  memset(l, 'a', sizeof(l));
  write(fd, l, sizeof(l));
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ(len, sizeof(l));
  ASSERT_TRUE(memcmp(l, line, sizeof(l)) == 0);
  ASSERT_EQ(line[len], 0);

  close(fd);
}

TEST(LineReaderTest, TooLong) {
  const int fd = TemporaryFile();
  char l[LineReader::kMaxLineLen];
  memset(l, 'a', sizeof(l));
  write(fd, l, sizeof(l));
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}
