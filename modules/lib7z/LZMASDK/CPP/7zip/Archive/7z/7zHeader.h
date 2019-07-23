

#ifndef __7Z_HEADER_H
#define __7Z_HEADER_H

#include "../../../Common/Types.h"

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
    kUnpackInfo,
    kSubStreamsInfo,

    kSize,
    kCRC,

    kFolder,

    kCodersUnpackSize,
    kNumUnpackStream,

    kEmptyStream,
    kEmptyFile,
    kAnti,

    kName,
    kCTime,
    kATime,
    kMTime,
    kWinAttributes,
    kComment,

    kEncodedHeader,

    kStartPos,
    kDummy
  };
}

}}

#endif
