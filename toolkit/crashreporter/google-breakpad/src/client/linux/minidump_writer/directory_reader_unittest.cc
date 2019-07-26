




























#include <set>
#include <string>

#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>

#include "client/linux/minidump_writer/directory_reader.h"
#include "breakpad_googletest_includes.h"

using namespace google_breakpad;

namespace {
typedef testing::Test DirectoryReaderTest;
}

TEST(DirectoryReaderTest, CompareResults) {
  std::set<std::string> dent_set;

  DIR *const dir = opendir("/proc/self");
  ASSERT_TRUE(dir != NULL);

  struct dirent* dent;
  while ((dent = readdir(dir)))
    dent_set.insert(dent->d_name);

  closedir(dir);

  const int fd = open("/proc/self", O_DIRECTORY | O_RDONLY);
  ASSERT_GE(fd, 0);

  DirectoryReader dir_reader(fd);
  unsigned seen = 0;

  const char* name;
  while (dir_reader.GetNextEntry(&name)) {
    ASSERT_TRUE(dent_set.find(name) != dent_set.end());
    seen++;
    dir_reader.PopEntry();
  }

  ASSERT_TRUE(dent_set.find("status") != dent_set.end());
  ASSERT_TRUE(dent_set.find("stat") != dent_set.end());
  ASSERT_TRUE(dent_set.find("cmdline") != dent_set.end());

  ASSERT_EQ(dent_set.size(), seen);
  close(fd);
}
