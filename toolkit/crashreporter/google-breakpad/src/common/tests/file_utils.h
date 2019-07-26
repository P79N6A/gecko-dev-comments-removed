































#ifndef COMMON_TESTS_FILE_UTILS_H_
#define COMMON_TESTS_FILE_UTILS_H_

namespace google_breakpad {


bool CopyFile(const char* from_path, const char* to_path);




bool ReadFile(const char* path, void* buffer, ssize_t* buffer_size);



bool WriteFile(const char* path, const void* buffer, size_t buffer_size);

}  

#endif  
