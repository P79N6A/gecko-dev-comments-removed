



#ifndef Mappable_h
#define Mappable_h

#include <sys/types.h>
#include "Zip.h"
#include "mozilla/RefPtr.h"
#include "zlib.h"










class Mappable
{
public:
  virtual ~Mappable() { }

  virtual void *mmap(const void *addr, size_t length, int prot, int flags,
                     off_t offset) = 0;
  virtual void munmap(void *addr, size_t length) {
    ::munmap(addr, length);
  }

  


  virtual void finalize() = 0;
};




class MappableFile: public Mappable
{
public:
  ~MappableFile() { }

  


  static MappableFile *Create(const char *path);

  
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

  



  static MappableExtractFile *Create(const char *name, Zip::Stream *stream);

  


  char *GetPath() {
    return path;
  }
private:
  MappableExtractFile(int fd, char *path)
  : MappableFile(fd), path(path), pid(getpid()) { }

  



  struct AutoUnlinkFileTraits: public AutoDeleteArrayTraits<char>
  {
    static void clean(char *value)
    {
      unlink(value);
      AutoDeleteArrayTraits<char>::clean(value);
    }
  };
  typedef AutoClean<AutoUnlinkFileTraits> AutoUnlinkFile;

  
  AutoUnlinkFile path;

  
  pid_t pid;
};

class _MappableBuffer;





class MappableDeflate: public Mappable
{
public:
  ~MappableDeflate();

  




  static MappableDeflate *Create(const char *name, Zip *zip, Zip::Stream *stream);

  
  virtual void *mmap(const void *addr, size_t length, int prot, int flags, off_t offset);
  virtual void finalize();

private:
  MappableDeflate(_MappableBuffer *buf, Zip *zip, Zip::Stream *stream);

  
  mozilla::RefPtr<Zip> zip;

  
  AutoDeletePtr<_MappableBuffer> buffer;

  
  z_stream zStream;
};

#endif 
