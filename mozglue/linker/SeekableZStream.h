



#ifndef SeekableZStream_h
#define SeekableZStream_h

#include "Zip.h"











#pragma pack(1)
struct SeekableZStreamHeader: public Zip::SignedEntity<SeekableZStreamHeader>
{
  SeekableZStreamHeader()
  : Zip::SignedEntity<SeekableZStreamHeader>(magic)
  , totalSize(0), chunkSize(0), dictSize(0), nChunks(0), lastChunkSize(0)
  , windowBits(0), filter(0) { }

  

  static const uint32_t magic = 0x7a5a6553;

  
  le_uint32 totalSize;

  
  le_uint16 chunkSize;

  
  le_uint16 dictSize;

  
  le_uint32 nChunks;

  
  le_uint16 lastChunkSize;

  
  signed char windowBits;

  
  unsigned char filter;
};
#pragma pack()

static_assert(sizeof(SeekableZStreamHeader) == 5 * 4,
              "SeekableZStreamHeader should be 5 32-bits words");




class SeekableZStream {
public:
  

  bool Init(const void *buf, size_t length);

  




  bool Decompress(void *where, size_t chunk, size_t length = 0);

  


  bool DecompressChunk(void *where, size_t chunk, size_t length = 0);
 
  
  const size_t GetUncompressedSize() const
  {
    return (offsetTable.numElements() - 1) * chunkSize + lastChunkSize;
  }

  
  const size_t GetChunkSize(size_t chunk = 0) const {
    return (chunk == offsetTable.numElements() - 1) ? lastChunkSize : chunkSize;
  }

  
  const size_t GetChunksNum() const {
    return offsetTable.numElements();
  }

  


  enum FilterDirection {
    FILTER,
    UNFILTER
  };
  typedef void (*ZStreamFilter)(off_t, FilterDirection,
                                  unsigned char *, size_t);

  enum FilterId {
    NONE,
    BCJ_THUMB,
    BCJ_ARM,
    BCJ_X86,
    FILTER_MAX
  };
  static ZStreamFilter GetFilter(FilterId id);

  static ZStreamFilter GetFilter(uint16_t id) {
    return GetFilter(static_cast<FilterId>(id));
  }

private:
  
  const unsigned char *buffer;

  
  uint32_t totalSize;

  
  uint32_t chunkSize;

  
  uint32_t lastChunkSize;

  
  int windowBits;

  
  Array<le_uint32> offsetTable;

  
  ZStreamFilter filter;

  
  Array<unsigned char> dictionary;

  
  zxx_stream::StaticAllocator allocator;
};

inline void
operator++(SeekableZStream::FilterId &other)
{
  const int orig = static_cast<int>(other);
  other = static_cast<SeekableZStream::FilterId>(orig + 1);
}

#endif 
