





































#include "crashreporter.h"

#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <algorithm>

using namespace CrashReporter;
using std::string;
using std::vector;

struct FileData
{
  time_t timestamp;
  string path;
};

static bool CompareFDTime(const FileData& fd1, const FileData& fd2)
{
  return fd1.timestamp > fd2.timestamp;
}

void UIPruneSavedDumps(const std::string& directory)
{
  DIR *dirfd = opendir(directory.c_str());
  if (!dirfd)
    return;

  vector<FileData> dumpfiles;

  while (dirent *dir = readdir(dirfd)) {
    FileData fd;
    fd.path = directory + '/' + dir->d_name;
    if (fd.path.size() < 5)
      continue;

    if (fd.path.compare(fd.path.size() - 4, 4, ".dmp") != 0)
      continue;

    struct stat st;
    if (stat(fd.path.c_str(), &st)) {
      closedir(dirfd);
      return;
    }

    fd.timestamp = st.st_mtime;

    dumpfiles.push_back(fd);
  }

  sort(dumpfiles.begin(), dumpfiles.end(), CompareFDTime);

  while (dumpfiles.size() > kSaveCount) {
    
    string path = (--dumpfiles.end())->path;
    UIDeleteFile(path.c_str());

    
    path.replace(path.size() - 4, 4, ".extra");
    UIDeleteFile(path.c_str());

    dumpfiles.pop_back();
  }
}
