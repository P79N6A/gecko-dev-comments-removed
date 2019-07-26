



#ifndef Zip_h
#define Zip_h

#include <cstring>
#include <stdint.h>
#include <vector>
#include <zlib.h>
#include "Utils.h"
#include "mozilla/RefPtr.h"




class ZipCollection;










class Zip: public mozilla::RefCounted<Zip>
{
public:
  



  static mozilla::TemporaryRef<Zip> Create(const char *filename);

  


  static mozilla::TemporaryRef<Zip> Create(void *buffer, size_t size) {
    return Create(NULL, buffer, size);
  }

private:
  static mozilla::TemporaryRef<Zip> Create(const char *filename,
                                           void *buffer, size_t size);

  


  Zip(const char *filename, void *buffer, size_t size);

public:
  


  ~Zip();

  


  class Stream
  {
  public:
    


    enum Type {
      STORE = 0,
      DEFLATE = 8
    };

    


    Stream(): compressedBuf(NULL), compressedSize(0), uncompressedSize(0)
            , type(STORE) { }

    


    const void *GetBuffer() { return compressedBuf; }
    size_t GetSize() { return compressedSize; }
    size_t GetUncompressedSize() { return uncompressedSize; }
    Type GetType() { return type; }

    




    z_stream GetZStream(void *buf)
    {
      z_stream zStream;
      memset(&zStream, 0, sizeof(zStream));
      zStream.avail_in = compressedSize;
      zStream.next_in = reinterpret_cast<Bytef *>(
                        const_cast<void *>(compressedBuf));
      zStream.avail_out = uncompressedSize;
      zStream.next_out = static_cast<Bytef *>(buf);
      return zStream;
    }

  protected:
    friend class Zip;
    const void *compressedBuf;
    size_t compressedSize;
    size_t uncompressedSize;
    Type type;
  };

  


  bool GetStream(const char *path, Stream *out) const;

  


  const char *GetName() const
  {
    return name;
  }

private:
  
  char *name;
  
  void *mapped;
  
  size_t size;

  



  class StringBuf
  {
  public:
    


    StringBuf(const char *buf, size_t length): buf(buf), length(length) { }

    



    bool Equals(const char *str) const
    {
      return strncmp(str, buf, length) == 0;
    }

  private:
    const char *buf;
    size_t length;
  };


#pragma pack(1)
public:
  




  template <typename T>
  class SignedEntity
  {
  public:
    



    static const T *validate(const void *buf)
    {
      const T *ret = static_cast<const T *>(buf);
      if (ret->signature == T::magic)
        return ret;
      return NULL;
    }

    SignedEntity(uint32_t magic): signature(magic) { }
  private:
    le_uint32 signature;
  };

private:
  



  struct LocalFile: public SignedEntity<LocalFile>
  {
    
    static const uint32_t magic = 0x04034b50;

    


    StringBuf GetName() const
    {
      return StringBuf(reinterpret_cast<const char *>(this) + sizeof(*this),
                       filenameSize);
    }

    


    const void *GetData() const
    {
      return reinterpret_cast<const char *>(this) + sizeof(*this)
             + filenameSize + extraFieldSize;
    }
    
    le_uint16 minVersion;
    le_uint16 generalFlag;
    le_uint16 compression;
    le_uint16 lastModifiedTime;
    le_uint16 lastModifiedDate;
    le_uint32 CRC32;
    le_uint32 compressedSize;
    le_uint32 uncompressedSize;
    le_uint16 filenameSize;
    le_uint16 extraFieldSize;
  };

  





  struct DataDescriptor: public SignedEntity<DataDescriptor>
  {
    
    static const uint32_t magic = 0x08074b50;

    le_uint32 CRC32;
    le_uint32 compressedSize;
    le_uint32 uncompressedSize;
  };

  



  struct DirectoryEntry: public SignedEntity<DirectoryEntry>
  {
    
    static const uint32_t magic = 0x02014b50;

    


    StringBuf GetName() const
    {
      return StringBuf(reinterpret_cast<const char *>(this) + sizeof(*this),
                       filenameSize);
    }

    


    const DirectoryEntry *GetNext() const
    {
      return validate(reinterpret_cast<const char *>(this) + sizeof(*this)
                      + filenameSize + extraFieldSize + fileCommentSize);
    }

    le_uint16 creatorVersion;
    le_uint16 minVersion;
    le_uint16 generalFlag;
    le_uint16 compression;
    le_uint16 lastModifiedTime;
    le_uint16 lastModifiedDate;
    le_uint32 CRC32;
    le_uint32 compressedSize;
    le_uint32 uncompressedSize;
    le_uint16 filenameSize;
    le_uint16 extraFieldSize;
    le_uint16 fileCommentSize;
    le_uint16 diskNum;
    le_uint16 internalAttributes;
    le_uint32 externalAttributes;
    le_uint32 offset;
  };

  


  struct CentralDirectoryEnd: public SignedEntity<CentralDirectoryEnd>
  {
    
    static const uint32_t magic = 0x06054b50;

    le_uint16 diskNum;
    le_uint16 startDisk;
    le_uint16 recordsOnDisk;
    le_uint16 records;
    le_uint32 size;
    le_uint32 offset;
    le_uint16 commentSize;
  };
#pragma pack()

  


  const DirectoryEntry *GetFirstEntry() const;

  


  mutable const LocalFile *nextFile;

  
  mutable const DirectoryEntry *nextDir;

  
  mutable const DirectoryEntry *entries;
};




class ZipCollection
{
public:
  static ZipCollection Singleton;

  



  static mozilla::TemporaryRef<Zip> GetZip(const char *path);

protected:
  friend class Zip;
  



  static void Register(Zip *zip);

  



  static void Forget(Zip *zip);

private:
  
  std::vector<Zip *> zips;
};

#endif
