







#ifndef BASE_IMAGE_UTIL_H_
#define BASE_IMAGE_UTIL_H_

#include <windows.h>
#include <vector>

#include "base/basictypes.h"

namespace image_util {


struct ImageSectionData {
  ImageSectionData (const std::string& section_name, size_t section_size)
      : name (section_name),
        size_in_bytes(section_size) {
  }

  std::string name;
  size_t size_in_bytes;
};

typedef std::vector<ImageSectionData> ImageSectionsData;





class ImageMetrics {
 public:
  
  
  explicit ImageMetrics(HANDLE process);
  ~ImageMetrics();

  
  
  
  
  bool GetDllImageSectionData(const std::string& loaded_dll_name,
                              ImageSectionsData* section_sizes);

  
  
  
  bool GetProcessImageSectionData(ImageSectionsData* section_sizes);

 private:
  
  bool GetImageSectionSizes(char* qualified_path, ImageSectionsData* result);

  HANDLE process_;

  DISALLOW_COPY_AND_ASSIGN(ImageMetrics);
};

}  

#endif  
