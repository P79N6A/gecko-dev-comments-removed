









#ifndef WEBRTC_BASE_STREAM_H_
#define WEBRTC_BASE_STREAM_H_

#include <stdio.h>

#include "webrtc/base/basictypes.h"
#include "webrtc/base/buffer.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/messagehandler.h"
#include "webrtc/base/messagequeue.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/sigslot.h"

namespace rtc {













enum StreamState { SS_CLOSED, SS_OPENING, SS_OPEN };



enum StreamResult { SR_ERROR, SR_SUCCESS, SR_BLOCK, SR_EOS };







enum StreamEvent { SE_OPEN = 1, SE_READ = 2, SE_WRITE = 4, SE_CLOSE = 8 };

class Thread;

struct StreamEventData : public MessageData {
  int events, error;
  StreamEventData(int ev, int er) : events(ev), error(er) { }
};

class StreamInterface : public MessageHandler {
 public:
  enum {
    MSG_POST_EVENT = 0xF1F1, MSG_MAX = MSG_POST_EVENT
  };

  virtual ~StreamInterface();

  virtual StreamState GetState() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual StreamResult Read(void* buffer, size_t buffer_len,
                            size_t* read, int* error) = 0;
  virtual StreamResult Write(const void* data, size_t data_len,
                             size_t* written, int* error) = 0;
  
  
  virtual void Close() = 0;

  
  
  
  
  
  
  
  
  sigslot::signal3<StreamInterface*, int, int> SignalEvent;

  
  
  
  void PostEvent(Thread* t, int events, int err);
  
  void PostEvent(int events, int err);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  

  
  
  
  
  
  
  
  virtual const void* GetReadData(size_t* data_len) { return NULL; }
  virtual void ConsumeReadData(size_t used) {}

  
  
  
  
  
  
  
  
  
  
  
  virtual void* GetWriteBuffer(size_t* buf_len) { return NULL; }
  virtual void ConsumeWriteBuffer(size_t used) {}

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  virtual bool SetPosition(size_t position) { return false; }

  
  
  virtual bool GetPosition(size_t* position) const { return false; }

  
  
  virtual bool GetSize(size_t* size) const { return false; }

  
  
  virtual bool GetAvailable(size_t* size) const { return false; }

  
  
  virtual bool GetWriteRemaining(size_t* size) const { return false; }

  
  virtual bool Flush() { return false; }

  
  
  
  
  
  virtual bool ReserveSize(size_t size) { return true; }

  
  
  
  
  

  
  inline bool Rewind() { return SetPosition(0); }

  
  
  
  
  
  StreamResult WriteAll(const void* data, size_t data_len,
                        size_t* written, int* error);

  
  
  StreamResult ReadAll(void* buffer, size_t buffer_len,
                       size_t* read, int* error);

  
  
  
  
  StreamResult ReadLine(std::string* line);

 protected:
  StreamInterface();

  
  virtual void OnMessage(Message* msg);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(StreamInterface);
};









class StreamAdapterInterface : public StreamInterface,
                               public sigslot::has_slots<> {
 public:
  explicit StreamAdapterInterface(StreamInterface* stream, bool owned = true);

  
  virtual StreamState GetState() const {
    return stream_->GetState();
  }
  virtual StreamResult Read(void* buffer, size_t buffer_len,
                            size_t* read, int* error) {
    return stream_->Read(buffer, buffer_len, read, error);
  }
  virtual StreamResult Write(const void* data, size_t data_len,
                             size_t* written, int* error) {
    return stream_->Write(data, data_len, written, error);
  }
  virtual void Close() {
    stream_->Close();
  }

  
  



















  





  virtual bool SetPosition(size_t position) {
    return stream_->SetPosition(position);
  }
  virtual bool GetPosition(size_t* position) const {
    return stream_->GetPosition(position);
  }
  virtual bool GetSize(size_t* size) const {
    return stream_->GetSize(size);
  }
  virtual bool GetAvailable(size_t* size) const {
    return stream_->GetAvailable(size);
  }
  virtual bool GetWriteRemaining(size_t* size) const {
    return stream_->GetWriteRemaining(size);
  }
  virtual bool ReserveSize(size_t size) {
    return stream_->ReserveSize(size);
  }
  virtual bool Flush() {
    return stream_->Flush();
  }

  void Attach(StreamInterface* stream, bool owned = true);
  StreamInterface* Detach();

 protected:
  virtual ~StreamAdapterInterface();

  
  
  virtual void OnEvent(StreamInterface* stream, int events, int err) {
    SignalEvent(this, events, err);
  }
  StreamInterface* stream() { return stream_; }

 private:
  StreamInterface* stream_;
  bool owned_;
  DISALLOW_EVIL_CONSTRUCTORS(StreamAdapterInterface);
};







class StreamTap : public StreamAdapterInterface {
 public:
  explicit StreamTap(StreamInterface* stream, StreamInterface* tap);

  void AttachTap(StreamInterface* tap);
  StreamInterface* DetachTap();
  StreamResult GetTapResult(int* error);

  
  virtual StreamResult Read(void* buffer, size_t buffer_len,
                            size_t* read, int* error);
  virtual StreamResult Write(const void* data, size_t data_len,
                             size_t* written, int* error);

 private:
  scoped_ptr<StreamInterface> tap_;
  StreamResult tap_result_;
  int tap_error_;
  DISALLOW_EVIL_CONSTRUCTORS(StreamTap);
};









class StreamSegment : public StreamAdapterInterface {
 public:
  
  
  explicit StreamSegment(StreamInterface* stream);
  explicit StreamSegment(StreamInterface* stream, size_t length);

  
  virtual StreamResult Read(void* buffer, size_t buffer_len,
                            size_t* read, int* error);
  virtual bool SetPosition(size_t position);
  virtual bool GetPosition(size_t* position) const;
  virtual bool GetSize(size_t* size) const;
  virtual bool GetAvailable(size_t* size) const;

 private:
  size_t start_, pos_, length_;
  DISALLOW_EVIL_CONSTRUCTORS(StreamSegment);
};





class NullStream : public StreamInterface {
 public:
  NullStream();
  virtual ~NullStream();

  
  virtual StreamState GetState() const;
  virtual StreamResult Read(void* buffer, size_t buffer_len,
                            size_t* read, int* error);
  virtual StreamResult Write(const void* data, size_t data_len,
                             size_t* written, int* error);
  virtual void Close();
};






class FileStream : public StreamInterface {
 public:
  FileStream();
  virtual ~FileStream();

  
  virtual bool Open(const std::string& filename, const char* mode, int* error);
  virtual bool OpenShare(const std::string& filename, const char* mode,
                         int shflag, int* error);

  
  
  virtual bool DisableBuffering();

  virtual StreamState GetState() const;
  virtual StreamResult Read(void* buffer, size_t buffer_len,
                            size_t* read, int* error);
  virtual StreamResult Write(const void* data, size_t data_len,
                             size_t* written, int* error);
  virtual void Close();
  virtual bool SetPosition(size_t position);
  virtual bool GetPosition(size_t* position) const;
  virtual bool GetSize(size_t* size) const;
  virtual bool GetAvailable(size_t* size) const;
  virtual bool ReserveSize(size_t size);

  virtual bool Flush();

#if defined(WEBRTC_POSIX) && !defined(__native_client__)
  
  
  bool TryLock();
  bool Unlock();
#endif

  
  static bool GetSize(const std::string& filename, size_t* size);

 protected:
  virtual void DoClose();

  FILE* file_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(FileStream);
};




class CircularFileStream : public FileStream {
 public:
  explicit CircularFileStream(size_t max_size);

  virtual bool Open(const std::string& filename, const char* mode, int* error);
  virtual StreamResult Read(void* buffer, size_t buffer_len,
                            size_t* read, int* error);
  virtual StreamResult Write(const void* data, size_t data_len,
                             size_t* written, int* error);

 private:
  enum ReadSegment {
    READ_MARKED,  
    READ_MIDDLE,  
    READ_LATEST,  
                  
  };

  size_t max_write_size_;
  size_t position_;
  size_t marked_position_;
  size_t last_write_position_;
  ReadSegment read_segment_;
  size_t read_segment_available_;
};



class AsyncWriteStream : public StreamInterface {
 public:
  
  AsyncWriteStream(StreamInterface* stream, rtc::Thread* write_thread)
      : stream_(stream),
        write_thread_(write_thread),
        state_(stream ? stream->GetState() : SS_CLOSED) {
  }

  virtual ~AsyncWriteStream();

  
  virtual StreamState GetState() const { return state_; }
  
  virtual bool GetPosition(size_t* position) const;
  virtual StreamResult Read(void* buffer, size_t buffer_len,
                            size_t* read, int* error);
  virtual StreamResult Write(const void* data, size_t data_len,
                             size_t* written, int* error);
  virtual void Close();
  virtual bool Flush();

 protected:
  
  virtual void OnMessage(rtc::Message* pmsg);
  virtual void ClearBufferAndWrite();

 private:
  rtc::scoped_ptr<StreamInterface> stream_;
  Thread* write_thread_;
  StreamState state_;
  Buffer buffer_;
  mutable CriticalSection crit_stream_;
  CriticalSection crit_buffer_;

  DISALLOW_EVIL_CONSTRUCTORS(AsyncWriteStream);
};


#if defined(WEBRTC_POSIX) && !defined(__native_client__)



class POpenStream : public FileStream {
 public:
  POpenStream() : wait_status_(-1) {}
  virtual ~POpenStream();

  virtual bool Open(const std::string& subcommand, const char* mode,
                    int* error);
  
  virtual bool OpenShare(const std::string& subcommand, const char* mode,
                         int shflag, int* error);

  
  
  
  int GetWaitStatus() const { return wait_status_; }

 protected:
  virtual void DoClose();

 private:
  int wait_status_;
};
#endif  








class MemoryStreamBase : public StreamInterface {
 public:
  virtual StreamState GetState() const;
  virtual StreamResult Read(void* buffer, size_t bytes, size_t* bytes_read,
                            int* error);
  virtual StreamResult Write(const void* buffer, size_t bytes,
                             size_t* bytes_written, int* error);
  virtual void Close();
  virtual bool SetPosition(size_t position);
  virtual bool GetPosition(size_t* position) const;
  virtual bool GetSize(size_t* size) const;
  virtual bool GetAvailable(size_t* size) const;
  virtual bool ReserveSize(size_t size);

  char* GetBuffer() { return buffer_; }
  const char* GetBuffer() const { return buffer_; }

 protected:
  MemoryStreamBase();

  virtual StreamResult DoReserve(size_t size, int* error);

  
  char* buffer_;
  size_t buffer_length_;
  size_t data_length_;
  size_t seek_position_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(MemoryStreamBase);
};



class MemoryStream : public MemoryStreamBase {
 public:
  MemoryStream();
  explicit MemoryStream(const char* data);  
  MemoryStream(const void* data, size_t length);  
  virtual ~MemoryStream();

  void SetData(const void* data, size_t length);

 protected:
  virtual StreamResult DoReserve(size_t size, int* error);
  
  static const int kAlignment = 16;
  char* buffer_alloc_;
};




class ExternalMemoryStream : public MemoryStreamBase {
 public:
  ExternalMemoryStream();
  ExternalMemoryStream(void* data, size_t length);
  virtual ~ExternalMemoryStream();

  void SetData(void* data, size_t length);
};





class FifoBuffer : public StreamInterface {
 public:
  
  explicit FifoBuffer(size_t length);
  
  FifoBuffer(size_t length, Thread* owner);
  virtual ~FifoBuffer();
  
  bool GetBuffered(size_t* data_len) const;
  
  bool SetCapacity(size_t length);

  
  
  
  
  StreamResult ReadOffset(void* buffer, size_t bytes, size_t offset,
                          size_t* bytes_read);

  
  
  
  
  StreamResult WriteOffset(const void* buffer, size_t bytes, size_t offset,
                           size_t* bytes_written);

  
  virtual StreamState GetState() const;
  virtual StreamResult Read(void* buffer, size_t bytes,
                            size_t* bytes_read, int* error);
  virtual StreamResult Write(const void* buffer, size_t bytes,
                             size_t* bytes_written, int* error);
  virtual void Close();
  virtual const void* GetReadData(size_t* data_len);
  virtual void ConsumeReadData(size_t used);
  virtual void* GetWriteBuffer(size_t* buf_len);
  virtual void ConsumeWriteBuffer(size_t used);
  virtual bool GetWriteRemaining(size_t* size) const;

 private:
  
  
  StreamResult ReadOffsetLocked(void* buffer, size_t bytes, size_t offset,
                                size_t* bytes_read);

  
  
  StreamResult WriteOffsetLocked(const void* buffer, size_t bytes,
                                 size_t offset, size_t* bytes_written);

  StreamState state_;  
  scoped_ptr<char[]> buffer_;  
  size_t buffer_length_;  
  size_t data_length_;  
  size_t read_position_;  
  Thread* owner_;  
  mutable CriticalSection crit_;  
  DISALLOW_EVIL_CONSTRUCTORS(FifoBuffer);
};



class LoggingAdapter : public StreamAdapterInterface {
 public:
  LoggingAdapter(StreamInterface* stream, LoggingSeverity level,
                 const std::string& label, bool hex_mode = false);

  void set_label(const std::string& label);

  virtual StreamResult Read(void* buffer, size_t buffer_len,
                            size_t* read, int* error);
  virtual StreamResult Write(const void* data, size_t data_len,
                             size_t* written, int* error);
  virtual void Close();

 protected:
  virtual void OnEvent(StreamInterface* stream, int events, int err);

 private:
  LoggingSeverity level_;
  std::string label_;
  bool hex_mode_;
  LogMultilineState lms_;

  DISALLOW_EVIL_CONSTRUCTORS(LoggingAdapter);
};





class StringStream : public StreamInterface {
 public:
  explicit StringStream(std::string& str);
  explicit StringStream(const std::string& str);

  virtual StreamState GetState() const;
  virtual StreamResult Read(void* buffer, size_t buffer_len,
                            size_t* read, int* error);
  virtual StreamResult Write(const void* data, size_t data_len,
                             size_t* written, int* error);
  virtual void Close();
  virtual bool SetPosition(size_t position);
  virtual bool GetPosition(size_t* position) const;
  virtual bool GetSize(size_t* size) const;
  virtual bool GetAvailable(size_t* size) const;
  virtual bool ReserveSize(size_t size);

 private:
  std::string& str_;
  size_t read_pos_;
  bool read_only_;
};














class StreamReference : public StreamAdapterInterface {
  class StreamRefCount;
 public:
  
  
  
  explicit StreamReference(StreamInterface* stream);
  StreamInterface* GetStream() { return stream(); }
  StreamInterface* NewReference();
  virtual ~StreamReference();

 private:
  class StreamRefCount {
   public:
    explicit StreamRefCount(StreamInterface* stream)
        : stream_(stream), ref_count_(1) {
    }
    void AddReference() {
      CritScope lock(&cs_);
      ++ref_count_;
    }
    void Release() {
      int ref_count;
      {  
        CritScope lock(&cs_);
        ref_count = --ref_count_;
      }
      if (ref_count == 0) {
        delete stream_;
        delete this;
      }
    }
   private:
    StreamInterface* stream_;
    int ref_count_;
    CriticalSection cs_;
    DISALLOW_EVIL_CONSTRUCTORS(StreamRefCount);
  };

  
  explicit StreamReference(StreamRefCount* stream_ref_count,
                           StreamInterface* stream);

  StreamRefCount* stream_ref_count_;
  DISALLOW_EVIL_CONSTRUCTORS(StreamReference);
};












StreamResult Flow(StreamInterface* source,
                  char* buffer, size_t buffer_len,
                  StreamInterface* sink, size_t* data_len = NULL);



}  

#endif
