






























#include <stdlib.h>

#include "common/linux/file_id.h"
#include "breakpad_googletest_includes.h"

using namespace google_breakpad;


namespace {
typedef testing::Test FileIDTest;
}

TEST(FileIDTest, FileIDStrip) {
  
  
  
  char exe_name[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", exe_name, PATH_MAX - 1);
  ASSERT_NE(len, -1);
  exe_name[len] = '\0';

  
  char templ[] = "/tmp/file-id-unittest-XXXXXX";
  mktemp(templ);
  char cmdline[4096];
  sprintf(cmdline, "cp \"%s\" \"%s\"", exe_name, templ);
  ASSERT_EQ(system(cmdline), 0);
  sprintf(cmdline, "strip \"%s\"", templ);
  ASSERT_EQ(system(cmdline), 0);

  uint8_t identifier1[sizeof(MDGUID)];
  uint8_t identifier2[sizeof(MDGUID)];
  FileID fileid1(exe_name);
  EXPECT_TRUE(fileid1.ElfFileIdentifier(identifier1));
  FileID fileid2(templ);
  EXPECT_TRUE(fileid2.ElfFileIdentifier(identifier2));
  char identifier_string1[37];
  char identifier_string2[37];
  FileID::ConvertIdentifierToString(identifier1, identifier_string1,
                                    37);
  FileID::ConvertIdentifierToString(identifier2, identifier_string2,
                                    37);
  EXPECT_STREQ(identifier_string1, identifier_string2);
  unlink(templ);
}
