



#ifndef Mappable_h
#define Mappable_h

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

private:
  MappableFile(int fd): fd(fd) { }

  
  AutoCloseFD fd;
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

  
  _MappableBuffer *buffer;

  
  z_stream zStream;
};

#endif 
