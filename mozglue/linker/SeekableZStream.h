



#ifndef SeekableZStream_h
#define SeekableZStream_h

#include "Zip.h"











#pragma pack(1)
struct SeekableZStreamHeader: public Zip::SignedEntity<SeekableZStreamHeader>
{
  SeekableZStreamHeader()
  : Zip::SignedEntity<SeekableZStreamHeader>(magic)
  , totalSize(0), chunkSize(0), nChunks(0), lastChunkSize(0) { }

  

  static const uint32_t magic = 0x7a5a6553;

  
  le_uint32 totalSize;

  
  le_uint32 chunkSize;

  
  le_uint32 nChunks;

  
  le_uint32 lastChunkSize;
};
#pragma pack()

MOZ_STATIC_ASSERT(sizeof(SeekableZStreamHeader) == 5 * 4,
                  "SeekableZStreamHeader should be 5 32-bits words");

#endif 
