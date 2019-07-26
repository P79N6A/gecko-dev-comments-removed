




























#include <stdio.h>

#include "processor/pathname_stripper.h"
#include "processor/logging.h"

#define ASSERT_TRUE(condition) \
  if (!(condition)) { \
    fprintf(stderr, "FAIL: %s @ %s:%d\n", #condition, __FILE__, __LINE__); \
    return false; \
  }

#define ASSERT_EQ(e1, e2) ASSERT_TRUE((e1) == (e2))

namespace {

using google_breakpad::PathnameStripper;

static bool RunTests() {
  ASSERT_EQ(PathnameStripper::File("/dir/file"), "file");
  ASSERT_EQ(PathnameStripper::File("\\dir\\file"), "file");
  ASSERT_EQ(PathnameStripper::File("/dir\\file"), "file");
  ASSERT_EQ(PathnameStripper::File("\\dir/file"), "file");
  ASSERT_EQ(PathnameStripper::File("dir/file"), "file");
  ASSERT_EQ(PathnameStripper::File("dir\\file"), "file");
  ASSERT_EQ(PathnameStripper::File("dir/\\file"), "file");
  ASSERT_EQ(PathnameStripper::File("dir\\/file"), "file");
  ASSERT_EQ(PathnameStripper::File("file"), "file");
  ASSERT_EQ(PathnameStripper::File("dir/"), "");
  ASSERT_EQ(PathnameStripper::File("dir\\"), "");
  ASSERT_EQ(PathnameStripper::File("dir/dir/"), "");
  ASSERT_EQ(PathnameStripper::File("dir\\dir\\"), "");
  ASSERT_EQ(PathnameStripper::File("dir1/dir2/file"), "file");
  ASSERT_EQ(PathnameStripper::File("dir1\\dir2\\file"), "file");
  ASSERT_EQ(PathnameStripper::File("dir1/dir2\\file"), "file");
  ASSERT_EQ(PathnameStripper::File("dir1\\dir2/file"), "file");
  ASSERT_EQ(PathnameStripper::File(""), "");
  ASSERT_EQ(PathnameStripper::File("1"), "1");
  ASSERT_EQ(PathnameStripper::File("1/2"), "2");
  ASSERT_EQ(PathnameStripper::File("1\\2"), "2");
  ASSERT_EQ(PathnameStripper::File("/1/2"), "2");
  ASSERT_EQ(PathnameStripper::File("\\1\\2"), "2");
  ASSERT_EQ(PathnameStripper::File("dir//file"), "file");
  ASSERT_EQ(PathnameStripper::File("dir\\\\file"), "file");
  ASSERT_EQ(PathnameStripper::File("/dir//file"), "file");
  ASSERT_EQ(PathnameStripper::File("\\dir\\\\file"), "file");
  ASSERT_EQ(PathnameStripper::File("c:\\dir\\file"), "file");
  ASSERT_EQ(PathnameStripper::File("c:\\dir\\file.ext"), "file.ext");

  return true;
}

}  

int main(int argc, char **argv) {
  BPLOG_INIT(&argc, &argv);

  return RunTests() ? 0 : 1;
}
