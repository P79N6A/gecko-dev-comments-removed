









#ifndef WEBRTC_BASE_TRANSFORMADAPTER_H__
#define WEBRTC_BASE_TRANSFORMADAPTER_H__

#include "webrtc/base/stream.h"

namespace rtc {


class TransformInterface {
public:
  virtual ~TransformInterface() { }

  
  
  
  
  
  
  
  virtual StreamResult Transform(const void * input, size_t * in_len,
                                 void * output, size_t * out_len,
                                 bool flush) = 0;
};







class TransformAdapter : public StreamAdapterInterface {
public:
  
  
  TransformAdapter(StreamInterface * stream,
                   TransformInterface * transform,
                   bool direction_read);
  virtual ~TransformAdapter();
  
  virtual StreamResult Read(void * buffer, size_t buffer_len,
                            size_t * read, int * error);
  virtual StreamResult Write(const void * data, size_t data_len,
                             size_t * written, int * error);
  virtual void Close();

  
  virtual bool GetAvailable(size_t* size) const { return false; }
  virtual bool ReserveSize(size_t size) { return true; }

  
  virtual bool Rewind() { return false; }

private:
  enum State { ST_PROCESSING, ST_FLUSHING, ST_COMPLETE, ST_ERROR };
  enum { BUFFER_SIZE = 1024 };

  TransformInterface * transform_;
  bool direction_read_;
  State state_;
  int error_;

  char buffer_[BUFFER_SIZE];
  size_t len_;
};



} 

#endif
