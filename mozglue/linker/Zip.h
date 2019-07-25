



#ifndef Zip_h
#define Zip_h

#include <cstring>
#include <stdint.h>
#include <vector>
#include "Utils.h"
#include "mozilla/RefPtr.h"




class ZipCollection;










class Zip: public mozilla::RefCounted<Zip>
{
public:
  



  Zip(const char *filename, ZipCollection *collection = NULL);

  


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
  private:
    le_uint32 signature;
  };

  



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

  
  mutable ZipCollection *parent;
};




class ZipCollection
{
public:
  



  mozilla::TemporaryRef<Zip> GetZip(const char *path);

protected:
  



  friend Zip::~Zip();
  void Forget(Zip *zip);

private:
  
  std::vector<Zip *> zips;
};

#endif
