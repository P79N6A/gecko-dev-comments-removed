

#ifndef __7Z_HEADER_H
#define __7Z_HEADER_H

#include "7zMethodID.h"

namespace NArchive {
namespace N7z {

const int kSignatureSize = 6;
extern Byte kSignature[kSignatureSize];







#ifdef _7Z_VOL
extern Byte kFinishSignature[kSignatureSize];
#endif

struct CArchiveVersion
{
  Byte Major;
  Byte Minor;
};

const Byte kMajorVersion = 0;

struct CStartHeader
{
  UInt64 NextHeaderOffset;
  UInt64 NextHeaderSize;
  UInt32 NextHeaderCRC;
};

const UInt32 kStartHeaderSize = 20;

#ifdef _7Z_VOL
struct CFinishHeader: public CStartHeader
{
  UInt64 ArchiveStartOffset;  
  UInt64 AdditionalStartBlockSize; 
};

const UInt32 kFinishHeaderSize = kStartHeaderSize + 16;
#endif

namespace NID
{
  enum EEnum
  {
    kEnd,

    kHeader,

    kArchiveProperties,
    
    kAdditionalStreamsInfo,
    kMainStreamsInfo,
    kFilesInfo,
    
    kPackInfo,
    kUnPackInfo,
    kSubStreamsInfo,

    kSize,
    kCRC,

    kFolder,

    kCodersUnPackSize,
    kNumUnPackStream,

    kEmptyStream,
    kEmptyFile,
    kAnti,

    kName,
    kCreationTime,
    kLastAccessTime,
    kLastWriteTime,
    kWinAttributes,
    kComment,

    kEncodedHeader,

    kStartPos
  };
}

}}

#endif
