



#ifndef Mappable_h
#define Mappable_h

#include <sys/types.h>
#include <pthread.h>
#include "Zip.h"
#include "SeekableZStream.h"
#include "mozilla/RefPtr.h"
#include "mozilla/UniquePtr.h"
#include "zlib.h"







class Mappable: public mozilla::RefCounted<Mappable>
{
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(Mappable)
  virtual ~Mappable() { }

  virtual MemoryRange mmap(const void *addr, size_t length, int prot, int flags,
                           off_t offset) = 0;

  enum Kind {
    MAPPABLE_FILE,
    MAPPABLE_EXTRACT_FILE,
    MAPPABLE_DEFLATE,
    MAPPABLE_SEEKABLE_ZSTREAM
  };

  virtual Kind GetKind() const = 0;

private:
  virtual void munmap(void *addr, size_t length) {
    ::munmap(addr, length);
  }
  

  friend class Mappable1stPagePtr;
  friend class LibHandle;

public:
  




  virtual bool ensure(const void *addr) { return true; }

  


  virtual void finalize() = 0;

  







  virtual void stats(const char *when, const char *name) const { }

  



  virtual size_t GetLength() const = 0;
};




class MappableFile: public Mappable
{
public:
  ~MappableFile() { }

  


  static Mappable *Create(const char *path);

  
  virtual MemoryRange mmap(const void *addr, size_t length, int prot, int flags, off_t offset);
  virtual void finalize();
  virtual size_t GetLength() const;

  virtual Kind GetKind() const { return MAPPABLE_FILE; };
protected:
  MappableFile(int fd): fd(fd) { }

private:
  
  AutoCloseFD fd;
};





class MappableExtractFile: public MappableFile
{
public:
  ~MappableExtractFile();

  



  static Mappable *Create(const char *name, Zip *zip, Zip::Stream *stream);

  
  virtual void finalize() {}

  virtual Kind GetKind() const { return MAPPABLE_EXTRACT_FILE; };
private:
  



  struct UnlinkFile
  {
    void operator()(char *value) {
      unlink(value);
      delete [] value;
    }
  };
  typedef mozilla::UniquePtr<char[], UnlinkFile> AutoUnlinkFile;

  MappableExtractFile(int fd, AutoUnlinkFile path)
  : MappableFile(fd), path(Move(path)), pid(getpid()) { }

  
  AutoUnlinkFile path;

  
  pid_t pid;
};

class _MappableBuffer;





class MappableDeflate: public Mappable
{
public:
  ~MappableDeflate();

  




  static Mappable *Create(const char *name, Zip *zip, Zip::Stream *stream);

  
  virtual MemoryRange mmap(const void *addr, size_t length, int prot, int flags, off_t offset);
  virtual void finalize();
  virtual size_t GetLength() const;

  virtual Kind GetKind() const { return MAPPABLE_DEFLATE; };
private:
  MappableDeflate(_MappableBuffer *buf, Zip *zip, Zip::Stream *stream);

  
  mozilla::RefPtr<Zip> zip;

  
  mozilla::UniquePtr<_MappableBuffer> buffer;

  
  zxx_stream zStream;
};





class MappableSeekableZStream: public Mappable
{
public:
  ~MappableSeekableZStream();

  





  static Mappable *Create(const char *name, Zip *zip,
                                         Zip::Stream *stream);

  
  virtual MemoryRange mmap(const void *addr, size_t length, int prot, int flags, off_t offset);
  virtual void munmap(void *addr, size_t length);
  virtual void finalize();
  virtual bool ensure(const void *addr);
  virtual void stats(const char *when, const char *name) const;
  virtual size_t GetLength() const;

  virtual Kind GetKind() const { return MAPPABLE_SEEKABLE_ZSTREAM; };
private:
  MappableSeekableZStream(Zip *zip);

  
  mozilla::RefPtr<Zip> zip;

  
  mozilla::UniquePtr<_MappableBuffer> buffer;

  
  SeekableZStream zStream;

  


  struct LazyMap
  {
    const void *addr;
    size_t length;
    int prot;
    off_t offset;

    
    const void *end() const {
      return reinterpret_cast<const void *>
             (reinterpret_cast<const unsigned char *>(addr) + length);
    }

    
    const off_t endOffset() const {
      return offset + length;
    }

    
    const off_t offsetOf(const void *ptr) const {
      return reinterpret_cast<uintptr_t>(ptr)
             - reinterpret_cast<uintptr_t>(addr) + offset;
    }

    
    const bool Contains(const void *ptr) const {
      return (ptr >= addr) && (ptr < end());
    }
  };

  
  std::vector<LazyMap> lazyMaps;

  

  mozilla::UniquePtr<unsigned char[]> chunkAvail;

  
  mozilla::Atomic<size_t> chunkAvailNum;

  
  pthread_mutex_t mutex;
};

#endif 
