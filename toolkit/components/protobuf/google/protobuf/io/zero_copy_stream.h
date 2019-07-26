









































































































#ifndef GOOGLE_PROTOBUF_IO_ZERO_COPY_STREAM_H__
#define GOOGLE_PROTOBUF_IO_ZERO_COPY_STREAM_H__

#include <string>
#include <google/protobuf/stubs/common.h>

namespace google {

namespace protobuf {
namespace io {


class ZeroCopyInputStream;
class ZeroCopyOutputStream;



class LIBPROTOBUF_EXPORT ZeroCopyInputStream {
 public:
  inline ZeroCopyInputStream() {}
  virtual ~ZeroCopyInputStream();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual bool Next(const void** data, int* size) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void BackUp(int count) = 0;

  
  
  
  
  virtual bool Skip(int count) = 0;

  
  virtual int64 ByteCount() const = 0;


 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ZeroCopyInputStream);
};



class LIBPROTOBUF_EXPORT ZeroCopyOutputStream {
 public:
  inline ZeroCopyOutputStream() {}
  virtual ~ZeroCopyOutputStream();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual bool Next(void** data, int* size) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void BackUp(int count) = 0;

  
  virtual int64 ByteCount() const = 0;


 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ZeroCopyOutputStream);
};

}  
}  

}  
#endif  
