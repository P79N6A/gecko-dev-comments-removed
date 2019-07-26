






#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <windows.h>


#include "mozilla/FileUtils.h"  
#include "nsAutoPtr.h"          

























#pragma pack(push, 2)
typedef struct
{
  WORD Reserved;
  WORD ResourceType;
  WORD ImageCount;
} IconHeader;

typedef struct
{
  BYTE Width;
  BYTE Height;
  BYTE Colors;
  BYTE Reserved;
  WORD Planes;
  WORD BitsPerPixel;
  DWORD ImageSize;
  DWORD ImageOffset;
} IconDirEntry;

typedef struct
{
  BYTE Width;
  BYTE Height;
  BYTE Colors;
  BYTE Reserved;
  WORD Planes;
  WORD BitsPerPixel;
  DWORD ImageSize;
  WORD ResourceID;    
} IconResEntry;
#pragma pack(pop)

namespace {
  






  struct ScopedResourceUpdateTraits
  {
    typedef HANDLE type;
    static type empty() { return nullptr; }
    static void release(type handle) {
      if(nullptr != handle) {
        EndUpdateResourceW(handle, TRUE); 
      }
    }
  };

  typedef mozilla::Scoped<ScopedResourceUpdateTraits> ScopedResourceUpdate;
};

#ifdef __MINGW32__
extern "C"
#endif
int
wmain(int argc, wchar_t** argv)
{
  if (argc != 3) {
    printf("Usage: redit <exe file> <icon file>\n");
    return 1;
  }

  mozilla::ScopedClose file;
  if (0 != _wsopen_s(&file.rwget(),
                     argv[2],
                     _O_BINARY | _O_RDONLY,
                     _SH_DENYWR,
                     _S_IREAD)
  || (-1 == file)) {
    fprintf(stderr, "Unable to open icon file.\n");
    return 1;
  }

  
  long filesize = _filelength(file);
  nsAutoArrayPtr<BYTE> data(new BYTE[filesize]);
  if(!data) {
    fprintf(stderr, "Failed to allocate memory for icon file.\n");
    return 1;
  }
  _read(file, data, filesize);

  IconHeader* header = reinterpret_cast<IconHeader*>(data.get());

  
  ScopedResourceUpdate updateRes(BeginUpdateResourceW(argv[1], FALSE));
  if (nullptr == updateRes) {
    fprintf(stderr, "Unable to open library for modification.\n");
    return 1;
  }

  
  long groupSize = sizeof(IconHeader)
                 + header->ImageCount * sizeof(IconResEntry);
  nsAutoArrayPtr<BYTE> group(new BYTE[groupSize]);
  if(!group) {
    fprintf(stderr, "Failed to allocate memory for new images.\n");
    return 1;
  }
  memcpy(group, data, sizeof(IconHeader));

  IconDirEntry* sourceIcon =
                    reinterpret_cast<IconDirEntry*>(data
                                                  + sizeof(IconHeader));
  IconResEntry* targetIcon =
                    reinterpret_cast<IconResEntry*>(group
                                                  + sizeof(IconHeader));

  for (int id = 1; id <= header->ImageCount; id++) {
    
    if (!UpdateResourceW(updateRes, RT_ICON, MAKEINTRESOURCE(id),
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                         data + sourceIcon->ImageOffset,
                         sourceIcon->ImageSize)) {
      fprintf(stderr, "Unable to update resource (RT_ICON).\n");
      return 1;
    }
    
    
    memcpy(targetIcon, sourceIcon, sizeof(IconResEntry));
    targetIcon->ResourceID = id;
    sourceIcon++;
    targetIcon++;
  }

  if (!UpdateResourceW(updateRes, RT_GROUP_ICON, L"MAINICON",
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                       group, groupSize)) {
    fprintf(stderr, "Unable to update resource (RT_GROUP_ICON).\n");
    return 1;
  }

  
  if(!EndUpdateResourceW(updateRes.forget(), FALSE)) {
    fprintf(stderr, "Unable to write changes to library.\n");
    return 1;
  }

  return 0;
}
