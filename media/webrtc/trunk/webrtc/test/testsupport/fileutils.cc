









#include "webrtc/test/testsupport/fileutils.h"

#ifdef WIN32
#include <direct.h>
#include <algorithm>
#define GET_CURRENT_DIR _getcwd
#else
#include <unistd.h>
#define GET_CURRENT_DIR getcwd
#endif

#include <sys/stat.h>  
#ifndef S_ISDIR  
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

#include <stdio.h>
#include <string.h>

#include "webrtc/typedefs.h"  

namespace webrtc {
namespace test {

namespace {

#ifdef WIN32
const char* kPathDelimiter = "\\";
#else
const char* kPathDelimiter = "/";
#endif

#ifdef WEBRTC_ANDROID
const char* kResourcesDirName = "resources";
#else

const char* kProjectRootFileName = "DEPS";
const char* kResourcesDirName = "resources";
#endif

const char* kFallbackPath = "./";
const char* kOutputDirName = "out";
char relative_dir_path[FILENAME_MAX];
bool relative_dir_path_set = false;

}  

const char* kCannotFindProjectRootDir = "ERROR_CANNOT_FIND_PROJECT_ROOT_DIR";

std::string OutputPathAndroid();
std::string ProjectRootPathAndroid();

void SetExecutablePath(const std::string& path) {
  std::string working_dir = WorkingDir();
  std::string temp_path = path;

  
  if (path.find(working_dir) != std::string::npos) {
    temp_path = path.substr(working_dir.length() + 1);
  }
  
  
  
#ifdef WIN32
  std::replace(temp_path.begin(), temp_path.end(), '/', '\\');
#endif

  
  temp_path = temp_path.substr(0, temp_path.find_last_of(kPathDelimiter));
  strncpy(relative_dir_path, temp_path.c_str(), FILENAME_MAX);
  relative_dir_path_set = true;
}

bool FileExists(std::string& file_name) {
  struct stat file_info = {0};
  return stat(file_name.c_str(), &file_info) == 0;
}

std::string OutputPathImpl() {
  std::string path = ProjectRootPath();
  if (path == kCannotFindProjectRootDir) {
    return kFallbackPath;
  }
  path += kOutputDirName;
  if (!CreateDirectory(path)) {
    return kFallbackPath;
  }
  return path + kPathDelimiter;
}

#ifdef WEBRTC_ANDROID

std::string ProjectRootPath() {
  return ProjectRootPathAndroid();
}

std::string OutputPath() {
  return OutputPathAndroid();
}

std::string WorkingDir() {
  return ProjectRootPath();
}

#else 

std::string ProjectRootPath() {
  std::string path = WorkingDir();
  if (path == kFallbackPath) {
    return kCannotFindProjectRootDir;
  }
  if (relative_dir_path_set) {
    path = path + kPathDelimiter + relative_dir_path;
  }
  
  size_t path_delimiter_index = path.find_last_of(kPathDelimiter);
  while (path_delimiter_index != std::string::npos) {
    std::string root_filename = path + kPathDelimiter + kProjectRootFileName;
    if (FileExists(root_filename)) {
      return path + kPathDelimiter;
    }
    
    path = path.substr(0, path_delimiter_index);
    path_delimiter_index = path.find_last_of(kPathDelimiter);
  }
  
  fprintf(stderr, "Cannot find project root directory!\n");
  return kCannotFindProjectRootDir;
}

std::string OutputPath() {
  return OutputPathImpl();
}

std::string WorkingDir() {
  char path_buffer[FILENAME_MAX];
  if (!GET_CURRENT_DIR(path_buffer, sizeof(path_buffer))) {
    fprintf(stderr, "Cannot get current directory!\n");
    return kFallbackPath;
  } else {
    return std::string(path_buffer);
  }
}

#endif  

bool CreateDirectory(std::string directory_name) {
  struct stat path_info = {0};
  
  if (stat(directory_name.c_str(), &path_info) == 0) {
    if (!S_ISDIR(path_info.st_mode)) {
      fprintf(stderr, "Path %s exists but is not a directory! Remove this "
              "file and re-run to create the directory.\n",
              directory_name.c_str());
      return false;
    }
  } else {
#ifdef WIN32
    return _mkdir(directory_name.c_str()) == 0;
#else
    return mkdir(directory_name.c_str(),  S_IRWXU | S_IRWXG | S_IRWXO) == 0;
#endif
  }
  return true;
}

std::string ResourcePath(std::string name, std::string extension) {
  std::string platform = "win";
#ifdef WEBRTC_LINUX
  platform = "linux";
#endif  
#ifdef WEBRTC_MAC
  platform = "mac";
#endif  

#ifdef WEBRTC_ARCH_64_BITS
  std::string architecture = "64";
#else
  std::string architecture = "32";
#endif  

  std::string resources_path = ProjectRootPath() + kResourcesDirName +
      kPathDelimiter;
  std::string resource_file = resources_path + name + "_" + platform + "_" +
      architecture + "." + extension;
  if (FileExists(resource_file)) {
    return resource_file;
  }
  
  resource_file = resources_path + name + "_" + platform + "." + extension;
  if (FileExists(resource_file)) {
    return resource_file;
  }
  
  resource_file = resources_path + name + "_" + architecture + "." + extension;
  if (FileExists(resource_file)) {
    return resource_file;
  }

  
  return resources_path + name + "." + extension;
}

size_t GetFileSize(std::string filename) {
  FILE* f = fopen(filename.c_str(), "rb");
  size_t size = 0;
  if (f != NULL) {
    if (fseek(f, 0, SEEK_END) == 0) {
      size = ftell(f);
    }
    fclose(f);
  }
  return size;
}

}  
}  
