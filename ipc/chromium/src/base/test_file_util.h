



#ifndef BASE_TEST_FILE_UTIL_H_
#define BASE_TEST_FILE_UTIL_H_



#include <string>

class FilePath;

namespace file_util {



bool EvictFileFromSystemCache(const FilePath& file);







bool CopyRecursiveDirNoCache(const std::wstring& source_dir,
                             const std::wstring& dest_dir);

}  

#endif  
