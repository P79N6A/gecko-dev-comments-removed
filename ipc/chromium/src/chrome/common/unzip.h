



#ifndef CHROME_COMMON_UNZIP_H_
#define CHROME_COMMON_UNZIP_H_

#include <vector>

#include "base/file_path.h"





bool Unzip(const FilePath& zip_file, const FilePath& dest_dir,
           std::vector<FilePath>* files);

#endif  
