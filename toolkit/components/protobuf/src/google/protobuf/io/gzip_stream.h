









































#ifndef GOOGLE_PROTOBUF_IO_GZIP_STREAM_H__
#define GOOGLE_PROTOBUF_IO_GZIP_STREAM_H__

#include <zlib.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/zero_copy_stream.h>

namespace google {
namespace protobuf {
namespace io {


class LIBPROTOBUF_EXPORT GzipInputStream : public ZeroCopyInputStream {
 public:
  
  enum Format {
    
    AUTO = 0,

    
    GZIP = 1,

    
    ZLIB = 2,
  };

  
  explicit GzipInputStream(
      ZeroCopyInputStream* sub_stream,
      Format format = AUTO,
      int buffer_size = -1);
  virtual ~GzipInputStream();

  
  inline const char* ZlibErrorMessage() const {
    return zcontext_.msg;
  }
  inline int ZlibErrorCode() const {
    return zerror_;
  }

  
  bool Next(const void** data, int* size);
  void BackUp(int count);
  bool Skip(int count);
  int64 ByteCount() const;

 private:
  Format format_;

  ZeroCopyInputStream* sub_stream_;

  z_stream zcontext_;
  int zerror_;

  void* output_buffer_;
  void* output_position_;
  size_t output_buffer_length_;

  int Inflate(int flush);
  void DoNextOutput(const void** data, int* size);

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(GzipInputStream);
};


class LIBPROTOBUF_EXPORT GzipOutputStream : public ZeroCopyOutputStream {
 public:
  
  enum Format {
    
    GZIP = 1,

    
    ZLIB = 2,
  };

  struct Options {
    
    Format format;

    
    int buffer_size;

    
    
    int compression_level;

    
    
    
    int compression_strategy;

    Options();  
  };

  
  explicit GzipOutputStream(ZeroCopyOutputStream* sub_stream);

  
  GzipOutputStream(
      ZeroCopyOutputStream* sub_stream,
      const Options& options);

  virtual ~GzipOutputStream();

  
  inline const char* ZlibErrorMessage() const {
    return zcontext_.msg;
  }
  inline int ZlibErrorCode() const {
    return zerror_;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  bool Flush();

  
  
  
  
  bool Close();

  
  bool Next(void** data, int* size);
  void BackUp(int count);
  int64 ByteCount() const;

 private:
  ZeroCopyOutputStream* sub_stream_;
  
  void* sub_data_;
  int sub_data_size_;

  z_stream zcontext_;
  int zerror_;
  void* input_buffer_;
  size_t input_buffer_length_;

  
  void Init(ZeroCopyOutputStream* sub_stream, const Options& options);

  
  
  
  int Deflate(int flush);

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(GzipOutputStream);
};

}  
}  

}  
#endif
