









#include <set>
#include <sstream>

#include "webrtc/base/gunit.h"
#include "webrtc/base/linuxfdwalk.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static const int kArbitraryLargeFdNumber = 424;

static void FdCheckVisitor(void *data, int fd) {
  std::set<int> *fds = static_cast<std::set<int> *>(data);
  EXPECT_EQ(1U, fds->erase(fd));
}

static void FdEnumVisitor(void *data, int fd) {
  std::set<int> *fds = static_cast<std::set<int> *>(data);
  EXPECT_TRUE(fds->insert(fd).second);
}


static void CheckOpenFdList(std::set<int> fds) {
  EXPECT_EQ(0, fdwalk(&FdCheckVisitor, &fds));
  EXPECT_EQ(0U, fds.size());
}

static void GetOpenFdList(std::set<int> *fds) {
  fds->clear();
  EXPECT_EQ(0, fdwalk(&FdEnumVisitor, fds));
}

TEST(LinuxFdWalk, TestFdWalk) {
  std::set<int> fds;
  GetOpenFdList(&fds);
  std::ostringstream str;
  
  
  
  str << "File descriptors open in test executable:";
  for (std::set<int>::const_iterator i = fds.begin(); i != fds.end(); ++i) {
    str << " " << *i;
  }
  LOG(LS_INFO) << str.str();
  
  int fd1 = open("/dev/null", O_RDONLY);
  EXPECT_LE(0, fd1);
  int fd2 = open("/dev/null", O_WRONLY);
  EXPECT_LE(0, fd2);
  int fd3 = open("/dev/null", O_RDWR);
  EXPECT_LE(0, fd3);
  int fd4 = dup2(fd3, kArbitraryLargeFdNumber);
  EXPECT_LE(0, fd4);
  EXPECT_TRUE(fds.insert(fd1).second);
  EXPECT_TRUE(fds.insert(fd2).second);
  EXPECT_TRUE(fds.insert(fd3).second);
  EXPECT_TRUE(fds.insert(fd4).second);
  CheckOpenFdList(fds);
  EXPECT_EQ(0, close(fd1));
  EXPECT_EQ(0, close(fd2));
  EXPECT_EQ(0, close(fd3));
  EXPECT_EQ(0, close(fd4));
}
