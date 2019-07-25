



#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstring>
#include <cstdlib>
#include "Mappable.h"
#ifdef ANDROID
#include <linux/ashmem.h>
#endif
#include "Logging.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#ifndef PAGE_MASK
#define PAGE_MASK (~ (PAGE_SIZE - 1))
#endif

MappableFile *MappableFile::Create(const char *path)
{
  int fd = open(path, O_RDONLY);
  if (fd != -1)
    return new MappableFile(fd);
  return NULL;
}

void *
MappableFile::mmap(const void *addr, size_t length, int prot, int flags,
                   off_t offset)
{
  
  
  flags |= MAP_PRIVATE;

  void *mapped = ::mmap(const_cast<void *>(addr), length, prot, flags,
                        fd, offset);
  if (mapped == MAP_FAILED)
    return mapped;

  

  if ((mapped != MAP_FAILED) && (prot & PROT_WRITE) &&
      (length & (PAGE_SIZE - 1))) {
    memset(reinterpret_cast<char *>(mapped) + length, 0,
           PAGE_SIZE - (length & ~(PAGE_MASK)));
  }
  return mapped;
}

void
MappableFile::finalize()
{
  
  fd = -1;
}







class _MappableBuffer: public MappedPtr
{
public:
  



  static _MappableBuffer *Create(const char *name, size_t length)
  {
    AutoCloseFD fd;
#ifdef ANDROID
    
    fd = open("/" ASHMEM_NAME_DEF, O_RDWR, 0600);
    if (fd == -1)
      return NULL;
    char str[ASHMEM_NAME_LEN];
    strlcpy(str, name, sizeof(str));
    ioctl(fd, ASHMEM_SET_NAME, str);
    if (ioctl(fd, ASHMEM_SET_SIZE, length))
      return NULL;

    




    void *buf = ::mmap(NULL, length + PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buf != MAP_FAILED) {
      
      ::mmap(reinterpret_cast<char *>(buf) + ((length + PAGE_SIZE) & PAGE_MASK),
             PAGE_SIZE, PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS,
             -1, 0);
      debug("Decompression buffer of size %d in ashmem \"%s\", mapped @%p",
            length, str, buf);
      return new _MappableBuffer(fd.forget(), buf, length);
    }
#else
    

    
    char path[256];
    sprintf(path, "/dev/shm/%s.XXXXXX", name);
    fd = mkstemp(path);
    if (fd == -1)
      return NULL;
    unlink(path);
    ftruncate(fd, length);

    void *buf = ::mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buf != MAP_FAILED) {
      debug("Decompression buffer of size %ld in \"%s\", mapped @%p",
            length, path, buf);
      return new _MappableBuffer(fd.forget(), buf, length);
    }
#endif
    return NULL;
  }

  void *mmap(const void *addr, size_t length, int prot, int flags, off_t offset)
  {
    
#ifdef ANDROID
    

    if (flags & MAP_PRIVATE) {
      flags &= ~MAP_PRIVATE;
      flags |= MAP_SHARED;
    }
#endif
    return ::mmap(const_cast<void *>(addr), length, prot, flags, fd, offset);
  }

#ifdef ANDROID
  ~_MappableBuffer() {
    
    munmap(this + ((GetLength() + PAGE_SIZE) & ~(PAGE_SIZE - 1)), PAGE_SIZE);
  }
#endif

private:
  _MappableBuffer(int fd, void *buf, size_t length)
  : MappedPtr(buf, length), fd(fd) { }

  
  AutoCloseFD fd;
};


MappableDeflate *
MappableDeflate::Create(const char *name, Zip *zip, Zip::Stream *stream)
{
  
  _MappableBuffer *buf = _MappableBuffer::Create(name, stream->GetUncompressedSize());
  if (buf)
    return new MappableDeflate(buf, zip, stream);
  return NULL;
}

MappableDeflate::MappableDeflate(_MappableBuffer *buf, Zip *zip,
                                 Zip::Stream *stream)
: zip(zip), buffer(buf)
{
  
  memset(&zStream, 0, sizeof(zStream));
  zStream.avail_in = stream->GetSize();
  zStream.next_in = const_cast<Bytef *>(
                    reinterpret_cast<const Bytef *>(stream->GetBuffer()));
  zStream.total_in = 0;
  zStream.avail_out = stream->GetUncompressedSize();
  zStream.next_out = static_cast<Bytef*>(*buffer);
  zStream.total_out = 0;
}

MappableDeflate::~MappableDeflate()
{
  delete buffer;
}

void *
MappableDeflate::mmap(const void *addr, size_t length, int prot, int flags, off_t offset)
{
  
  
  flags |= MAP_PRIVATE;

  

  ssize_t missing = offset + length + zStream.avail_out - buffer->GetLength();
  if (missing > 0) {
    uInt avail_out = zStream.avail_out;
    zStream.avail_out = missing;
    if ((*buffer == zStream.next_out) &&
        (inflateInit2(&zStream, -MAX_WBITS) != Z_OK)) {
      log("inflateInit failed: %s", zStream.msg);
      return MAP_FAILED;
    }
    int ret = inflate(&zStream, Z_SYNC_FLUSH);
    if (ret < 0) {
      log("inflate failed: %s", zStream.msg);
      return MAP_FAILED;
    }
    if (ret == Z_NEED_DICT) {
      log("zstream requires a dictionary. %s", zStream.msg);
      return MAP_FAILED;
    }
    zStream.avail_out = avail_out - missing + zStream.avail_out;
    if (ret == Z_STREAM_END) {
      if (inflateEnd(&zStream) != Z_OK) {
        log("inflateEnd failed: %s", zStream.msg);
        return MAP_FAILED;
      }
      if (zStream.total_out != buffer->GetLength()) {
        log("File not fully uncompressed! %ld / %d", zStream.total_out,
            static_cast<unsigned int>(buffer->GetLength()));
        return MAP_FAILED;
      }
    }
  }
#if defined(ANDROID) && defined(__arm__)
  if (prot & PROT_EXEC) {
    

    debug("cacheflush(%p, %p)", *buffer + offset, *buffer + (offset + length));
    cacheflush(reinterpret_cast<uintptr_t>(*buffer + offset),
               reinterpret_cast<uintptr_t>(*buffer + (offset + length)), 0);
  }
#endif

  return buffer->mmap(addr, length, prot, flags, offset);
}

void
MappableDeflate::finalize()
{
  
  delete buffer;
  buffer = NULL;
  
  zip = NULL;
}
