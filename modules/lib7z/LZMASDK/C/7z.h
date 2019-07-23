


#ifndef __7Z_H
#define __7Z_H

#include "7zBuf.h"

EXTERN_C_BEGIN

#define k7zStartHeaderSize 0x20
#define k7zSignatureSize 6
extern Byte k7zSignature[k7zSignatureSize];
#define k7zMajorVersion 0

enum EIdEnum
{
  k7zIdEnd,
  k7zIdHeader,
  k7zIdArchiveProperties,
  k7zIdAdditionalStreamsInfo,
  k7zIdMainStreamsInfo,
  k7zIdFilesInfo,
  k7zIdPackInfo,
  k7zIdUnpackInfo,
  k7zIdSubStreamsInfo,
  k7zIdSize,
  k7zIdCRC,
  k7zIdFolder,
  k7zIdCodersUnpackSize,
  k7zIdNumUnpackStream,
  k7zIdEmptyStream,
  k7zIdEmptyFile,
  k7zIdAnti,
  k7zIdName,
  k7zIdCTime,
  k7zIdATime,
  k7zIdMTime,
  k7zIdWinAttributes,
  k7zIdComment,
  k7zIdEncodedHeader,
  k7zIdStartPos,
  k7zIdDummy
};

typedef struct
{
  UInt32 NumInStreams;
  UInt32 NumOutStreams;
  UInt64 MethodID;
  CBuf Props;
} CSzCoderInfo;

void SzCoderInfo_Init(CSzCoderInfo *p);
void SzCoderInfo_Free(CSzCoderInfo *p, ISzAlloc *alloc);

typedef struct
{
  UInt32 InIndex;
  UInt32 OutIndex;
} CSzBindPair;

typedef struct
{
  CSzCoderInfo *Coders;
  CSzBindPair *BindPairs;
  UInt32 *PackStreams;
  UInt64 *UnpackSizes;
  UInt32 NumCoders;
  UInt32 NumBindPairs;
  UInt32 NumPackStreams;
  int UnpackCRCDefined;
  UInt32 UnpackCRC;

  UInt32 NumUnpackStreams;
} CSzFolder;

void SzFolder_Init(CSzFolder *p);
UInt64 SzFolder_GetUnpackSize(CSzFolder *p);
int SzFolder_FindBindPairForInStream(CSzFolder *p, UInt32 inStreamIndex);
UInt32 SzFolder_GetNumOutStreams(CSzFolder *p);
UInt64 SzFolder_GetUnpackSize(CSzFolder *p);

SRes SzFolder_Decode(const CSzFolder *folder, const UInt64 *packSizes,
    ILookInStream *stream, UInt64 startPos,
    Byte *outBuffer, size_t outSize, ISzAlloc *allocMain);

typedef struct
{
  UInt32 Low;
  UInt32 High;
} CNtfsFileTime;

typedef struct
{
  CNtfsFileTime MTime;
  UInt64 Size;
  UInt32 Crc;
  Byte HasStream;
  Byte IsDir;
  Byte IsAnti;
  Byte CrcDefined;
  Byte MTimeDefined;
} CSzFileItem;

void SzFile_Init(CSzFileItem *p);

typedef struct
{
  UInt64 *PackSizes;
  Byte *PackCRCsDefined;
  UInt32 *PackCRCs;
  CSzFolder *Folders;
  CSzFileItem *Files;
  UInt32 NumPackStreams;
  UInt32 NumFolders;
  UInt32 NumFiles;
} CSzAr;

void SzAr_Init(CSzAr *p);
void SzAr_Free(CSzAr *p, ISzAlloc *alloc);






















typedef struct
{
  CSzAr db;
  
  UInt64 startPosAfterHeader;
  UInt64 dataPos;

  UInt32 *FolderStartPackStreamIndex;
  UInt64 *PackStreamStartPositions;
  UInt32 *FolderStartFileIndex;
  UInt32 *FileIndexToFolderIndexMap;

  size_t *FileNameOffsets; 
  CBuf FileNames;  
} CSzArEx;

void SzArEx_Init(CSzArEx *p);
void SzArEx_Free(CSzArEx *p, ISzAlloc *alloc);
UInt64 SzArEx_GetFolderStreamPos(const CSzArEx *p, UInt32 folderIndex, UInt32 indexInFolder);
int SzArEx_GetFolderFullPackSize(const CSzArEx *p, UInt32 folderIndex, UInt64 *resSize);







size_t SzArEx_GetFileNameUtf16(const CSzArEx *p, size_t fileIndex, UInt16 *dest);

SRes SzArEx_Extract(
    const CSzArEx *db,
    ILookInStream *inStream,
    UInt32 fileIndex,         
    UInt32 *blockIndex,       
    Byte **outBuffer,         
    size_t *outBufferSize,    
    size_t *offset,           
    size_t *outSizeProcessed, 
    ISzAlloc *allocMain,
    ISzAlloc *allocTemp);













SRes SzArEx_Open(CSzArEx *p, ILookInStream *inStream, ISzAlloc *allocMain, ISzAlloc *allocTemp);

EXTERN_C_END

#endif
