



#ifndef BASE_PATH_SERVICE_H__
#define BASE_PATH_SERVICE_H__

#include "build/build_config.h"
#ifdef OS_WIN





#include <windows.h>
#endif

#include <string>

#include "base/base_paths.h"

class FilePath;




class PathService {
 public:
  
  
  
  
  
  
  
  static bool Get(int key, FilePath* path);
  
  
  static bool Get(int key, std::wstring* path);

  
  
  
  
  
  
  
  
  
  
  static bool Override(int key, const std::wstring& path);

  
  static bool IsOverridden(int key);

  
  static bool SetCurrentDirectory(const std::wstring& current_directory);

  
  
  
  
  
  
  
  
  typedef bool (*ProviderFunc)(int, FilePath*);

  
  
  static void RegisterProvider(ProviderFunc provider,
                               int key_start,
                               int key_end);
 private:
  static bool GetFromCache(int key, FilePath* path);
  static void AddToCache(int key, const FilePath& path);

};

#endif 
