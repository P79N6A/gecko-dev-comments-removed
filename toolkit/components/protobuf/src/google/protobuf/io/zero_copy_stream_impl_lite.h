










































#ifndef GOOGLE_PROTOBUF_IO_ZERO_COPY_STREAM_IMPL_LITE_H__
#define GOOGLE_PROTOBUF_IO_ZERO_COPY_STREAM_IMPL_LITE_H__

#include <string>
#include <iosfwd>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/stl_util.h>


namespace google {
namespace protobuf {
namespace io {




class LIBPROTOBUF_EXPORT ArrayInputStream : public ZeroCopyInputStream {
 public:
  
  
  
  
  
  
  
  ArrayInputStream(const void* data, int size, int block_size = -1);
  ~ArrayInputStream();

  
  bool Next(const void** data, int* size);
  void BackUp(int count);
  bool Skip(int count);
  int64 ByteCount() const;


 private:
  const uint8* const data_;  
  const int size_;           
  const int block_size_;     

  int position_;
  int last_returned_size_;   
                             

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ArrayInputStream);
};




class LIBPROTOBUF_EXPORT ArrayOutputStream : public ZeroCopyOutputStream {
 public:
  
  
  
  
  
  
  
  ArrayOutputStream(void* data, int size, int block_size = -1);
  ~ArrayOutputStream();

  
  bool Next(void** data, int* size);
  void BackUp(int count);
  int64 ByteCount() const;

 private:
  uint8* const data_;        
  const int size_;           
  const int block_size_;     

  int position_;
  int last_returned_size_;   
                             

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ArrayOutputStream);
};




class LIBPROTOBUF_EXPORT StringOutputStream : public ZeroCopyOutputStream {
 public:
  
  
  
  
  
  
  
  explicit StringOutputStream(string* target);
  ~StringOutputStream();

  
  bool Next(void** data, int* size);
  void BackUp(int count);
  int64 ByteCount() const;

 private:
  static const int kMinimumSize = 16;

  string* target_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(StringOutputStream);
};


















class LIBPROTOBUF_EXPORT CopyingInputStream {
 public:
  virtual ~CopyingInputStream();

  
  
  
  
  virtual int Read(void* buffer, int size) = 0;

  
  
  
  
  
  
  virtual int Skip(int count);
};








class LIBPROTOBUF_EXPORT CopyingInputStreamAdaptor : public ZeroCopyInputStream {
 public:
  
  
  
  
  
  explicit CopyingInputStreamAdaptor(CopyingInputStream* copying_stream,
                                     int block_size = -1);
  ~CopyingInputStreamAdaptor();

  
  
  void SetOwnsCopyingStream(bool value) { owns_copying_stream_ = value; }

  
  bool Next(const void** data, int* size);
  void BackUp(int count);
  bool Skip(int count);
  int64 ByteCount() const;

 private:
  
  void AllocateBufferIfNeeded();
  
  void FreeBuffer();

  
  CopyingInputStream* copying_stream_;
  bool owns_copying_stream_;

  
  bool failed_;

  
  
  int64 position_;

  
  
  scoped_array<uint8> buffer_;
  const int buffer_size_;

  
  
  int buffer_used_;

  
  
  
  int backup_bytes_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CopyingInputStreamAdaptor);
};














class LIBPROTOBUF_EXPORT CopyingOutputStream {
 public:
  virtual ~CopyingOutputStream();

  
  
  virtual bool Write(const void* buffer, int size) = 0;
};








class LIBPROTOBUF_EXPORT CopyingOutputStreamAdaptor : public ZeroCopyOutputStream {
 public:
  
  
  
  
  explicit CopyingOutputStreamAdaptor(CopyingOutputStream* copying_stream,
                                      int block_size = -1);
  ~CopyingOutputStreamAdaptor();

  
  
  
  bool Flush();

  
  
  void SetOwnsCopyingStream(bool value) { owns_copying_stream_ = value; }

  
  bool Next(void** data, int* size);
  void BackUp(int count);
  int64 ByteCount() const;

 private:
  
  bool WriteBuffer();
  
  void AllocateBufferIfNeeded();
  
  void FreeBuffer();

  
  CopyingOutputStream* copying_stream_;
  bool owns_copying_stream_;

  
  bool failed_;

  
  
  int64 position_;

  
  
  scoped_array<uint8> buffer_;
  const int buffer_size_;

  
  
  
  int buffer_used_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CopyingOutputStreamAdaptor);
};






inline char* mutable_string_data(string* s) {
#ifdef LANG_CXX11
  
  
  return &(*s)[0];
#else
  return string_as_array(s);
#endif
}

}  
}  

}  
#endif  
