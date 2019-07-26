









#include <cstdio>




















































#ifndef WEBRTC_TEST_TESTSUPPORT_FILEUTILS_H_
#define WEBRTC_TEST_TESTSUPPORT_FILEUTILS_H_

#include <string>

namespace webrtc {
namespace test {



extern const char* kCannotFindProjectRootDir;

















std::string ProjectRootPath();











std::string OutputPath();




















std::string ResourcePath(std::string name, std::string extension);




std::string WorkingDir();




bool CreateDirectory(std::string directory_name);



size_t GetFileSize(std::string filename);







void SetExecutablePath(const std::string& path_to_executable);

}  
}  

#endif  
