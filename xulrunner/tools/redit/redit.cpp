




































#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

























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

int
main(int argc, char **argv)
{
  if (argc != 3) {
    printf("Usage: redit <exe file> <icon file>\n");
    return 1;
  }

  int file = _open(argv[2], _O_BINARY | _O_RDONLY);
  if (file == -1) {
    fprintf(stderr, "Unable to open icon file.\n");
    return 1;
  }

  
  long filesize = _filelength(file);
  char* data = (char*)malloc(filesize);
  _read(file, data, filesize);
  _close(file);
  IconHeader* header = (IconHeader*)data;

  
  HANDLE updateRes = BeginUpdateResource(argv[1], FALSE);
  if (updateRes == NULL) {
    fprintf(stderr, "Unable to open library for modification.\n");
    free(data);
    return 1;
  }

  
  long groupsize = sizeof(IconHeader) + header->ImageCount * sizeof(IconResEntry);
  char* group = (char*)malloc(groupsize);
  if (!group) {
    fprintf(stderr, "Failed to allocate memory for new images.\n");
    free(data);
    return 1;
  }
  memcpy(group, data, sizeof(IconHeader));

  IconDirEntry* sourceIcon = (IconDirEntry*)(data + sizeof(IconHeader));
  IconResEntry* targetIcon = (IconResEntry*)(group + sizeof(IconHeader));

  for (int id = 1; id <= header->ImageCount; id++) {
    
    if (!UpdateResource(updateRes, RT_ICON, MAKEINTRESOURCE(id),
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                        data + sourceIcon->ImageOffset, sourceIcon->ImageSize)) {
      fprintf(stderr, "Unable to update resource.\n");
      EndUpdateResource(updateRes, TRUE);  
      free(data);
      free(group);
      return 1;
    }
    
    memcpy(targetIcon, sourceIcon, sizeof(IconResEntry));
    targetIcon->ResourceID = id;
    sourceIcon++;
    targetIcon++;
  }
  free(data);

  if (!UpdateResource(updateRes, RT_GROUP_ICON, "MAINICON",
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                      group, groupsize)) {
    fprintf(stderr, "Unable to update resource.\n");
    EndUpdateResource(updateRes, TRUE);  
    free(group);
    return 1;
  }

  free(group);

  
  if (!EndUpdateResource(updateRes, FALSE)) {
    fprintf(stderr, "Unable to write changes to library.\n");
    return 1;
  }

  return 0;
}
