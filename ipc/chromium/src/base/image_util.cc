


#include <windows.h>
#include <ImageHlp.h>
#include <psapi.h>

#include "base/image_util.h"
#include "base/process_util.h"




#pragma comment(lib, "imagehlp.lib")

namespace image_util {


ImageMetrics::ImageMetrics(HANDLE process) : process_(process) {
}

ImageMetrics::~ImageMetrics() {
}

bool ImageMetrics::GetDllImageSectionData(const std::string& loaded_dll_name,
                                          ImageSectionsData* section_sizes) {
  
  HMODULE the_dll = GetModuleHandleA(loaded_dll_name.c_str());
  char full_filename[MAX_PATH];
  
  if (GetModuleFileNameExA(process_, the_dll, full_filename, MAX_PATH)) {
    return GetImageSectionSizes(full_filename, section_sizes);
  }
  return false;
}

bool ImageMetrics::GetProcessImageSectionData(ImageSectionsData*
                                              section_sizes) {
  char exe_path[MAX_PATH];
  
  if (GetModuleFileNameExA(process_, NULL, exe_path, MAX_PATH)) {
    return GetImageSectionSizes(exe_path, section_sizes);
  }
  return false;
}


bool ImageMetrics::GetImageSectionSizes(char* qualified_path,
                                        ImageSectionsData* result) {
  LOADED_IMAGE li;
  
  
  
  if (MapAndLoad(qualified_path, 0, &li, FALSE, TRUE)) {
    IMAGE_SECTION_HEADER* section_header = li.Sections;
    for (unsigned i = 0; i < li.NumberOfSections; i++, section_header++) {
      std::string name(reinterpret_cast<char*>(section_header->Name));
      ImageSectionData data(name, section_header->Misc.VirtualSize ?
          section_header->Misc.VirtualSize :
          section_header->SizeOfRawData);
      
      result->push_back(data);
    }
  } else {
    
    return false;
  }
  UnMapAndLoad(&li);
  return true;
}

} 
