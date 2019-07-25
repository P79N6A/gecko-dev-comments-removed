






#ifndef _nsDiskCache_h_
#define _nsDiskCache_h_

#include "nsCacheEntry.h"

#ifdef XP_WIN
#include <winsock.h>  
#endif


class nsDiskCache {
public:
    enum {
            kCurrentVersion = 0x00010013      
    };

    enum { kData, kMetaData };

    
    
    
    enum CorruptCacheInfo {
      kNotCorrupt = 0,
      kInvalidArgPointer = 1,
      kUnexpectedError = 2,
      kOpenCacheMapError = 3,
      kBlockFilesShouldNotExist = 4,
      kOutOfMemory = 5,
      kCreateCacheSubdirectories = 6,
      kBlockFilesShouldExist = 7,
      kHeaderSizeNotRead = 8,
      kHeaderIsDirty = 9,
      kVersionMismatch = 10,
      kRecordsIncomplete = 11,
      kHeaderIncomplete = 12,
      kNotEnoughToRead = 13,
      kEntryCountIncorrect = 14,
      kCouldNotGetBlockFileForIndex = 15,
      kCouldNotCreateBlockFile = 16,
      kBlockFileSizeError = 17,
      kBlockFileBitMapWriteError = 18,
      kBlockFileSizeLessThanBitMap = 19,
      kBlockFileBitMapReadError = 20,
      kBlockFileEstimatedSizeError = 21,
      kFlushHeaderError = 22,
      kCacheCleanFilePathError = 23,
      kCacheCleanOpenFileError = 24,
      kCacheCleanTimerError = 25
    };

    
    
    
    
    
    
    
    
    
    
    static PLDHashNumber    Hash(const char* key, PLDHashNumber initval=0);
    static nsresult         Truncate(PRFileDesc *  fd, PRUint32  newEOF);
};

#endif
