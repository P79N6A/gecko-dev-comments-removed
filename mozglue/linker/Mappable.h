



#ifndef Mappable_h
#define Mappable_h

#include <sys/types.h>
#include <pthread.h>
#include "Zip.h"
#include "SeekableZStream.h"
#include "mozilla/RefPtr.h"
#include "mozilla/Scoped.h"
#include "zlib.h"










class Mappable
{
public:
  virtual ~Mappable() { }

  virtual void *mmap(const void *addr, size_t length, int prot, int flags,
                     off_t offset) = 0;

private:
  virtual void munmap(void *addr, size_t length) {
    ::munmap(addr, length);
  }
  

  friend class Mappable1stPagePtr;

public:
  




  virtual bool ensure(const void *addr) { return true; }

  


  virtual void finalize() = 0;

  







  virtual void stats(const char *when, const char *name) const { }
};




class MappableFile: public Mappable
{
public:
  ~MappableFile() { }

  


  static Mappable *Create(const char *path);

  
  virtual void *mmap(const void *addr, size_t length, int prot, int flags, off_t offset);
  virtual void finalize();

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

private:
  MappableExtractFile(int fd, char *path)
  : MappableFile(fd), path(path), pid(getpid()) { }

  



  struct AutoUnlinkFileTraits: public mozilla::ScopedDeleteArrayTraits<char>
  {
    static void release(char *value)
    {
      if (!value)
        return;
      unlink(value);
      mozilla::ScopedDeleteArrayTraits<char>::release(value);
    }
  };
  typedef mozilla::Scoped<AutoUnlinkFileTraits> AutoUnlinkFile;

  
  AutoUnlinkFile path;

  
  pid_t pid;
};

class _MappableBuffer;





class MappableDeflate: public Mappable
{
public:
  ~MappableDeflate();

  




  static Mappable *Create(const char *name, Zip *zip, Zip::Stream *stream);

  
  virtual void *mmap(const void *addr, size_t length, int prot, int flags, off_t offset);
  virtual void finalize();

private:
  MappableDeflate(_MappableBuffer *buf, Zip *zip, Zip::Stream *stream);

  
  mozilla::RefPtr<Zip> zip;

  
  mozilla::ScopedDeletePtr<_MappableBuffer> buffer;

  
  z_stream zStream;
};





class MappableSeekableZStream: public Mappable
{
public:
  ~MappableSeekableZStream();

  





  static Mappable *Create(const char *name, Zip *zip,
                                         Zip::Stream *stream);

  
  virtual void *mmap(const void *addr, size_t length, int prot, int flags, off_t offset);
  virtual void munmap(void *addr, size_t length);
  virtual void finalize();
  virtual bool ensure(const void *addr);
  virtual void stats(const char *when, const char *name) const;

private:
  MappableSeekableZStream(Zip *zip);

  
  mozilla::RefPtr<Zip> zip;

  
  mozilla::ScopedDeletePtr<_MappableBuffer> buffer;

  
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

  

  mozilla::ScopedDeleteArray<unsigned char> chunkAvail;

  
  size_t chunkAvailNum;

  
  pthread_mutex_t mutex;
};

#endif 
