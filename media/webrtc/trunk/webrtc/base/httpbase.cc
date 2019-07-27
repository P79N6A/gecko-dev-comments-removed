










#if defined(WEBRTC_WIN)
#include "webrtc/base/win32.h"
#else  
#define SEC_E_CERT_EXPIRED (-2146893016)
#endif  

#include "webrtc/base/common.h"
#include "webrtc/base/httpbase.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/socket.h"
#include "webrtc/base/stringutils.h"
#include "webrtc/base/thread.h"

namespace rtc {





bool MatchHeader(const char* str, size_t len, HttpHeader header) {
  const char* const header_str = ToString(header);
  const size_t header_len = strlen(header_str);
  return (len == header_len) && (_strnicmp(str, header_str, header_len) == 0);
}

enum {
  MSG_READ
};





HttpParser::HttpParser() {
  reset();
}

HttpParser::~HttpParser() {
}

void
HttpParser::reset() {
  state_ = ST_LEADER;
  chunked_ = false;
  data_size_ = SIZE_UNKNOWN;
}

HttpParser::ProcessResult
HttpParser::Process(const char* buffer, size_t len, size_t* processed,
                    HttpError* error) {
  *processed = 0;
  *error = HE_NONE;

  if (state_ >= ST_COMPLETE) {
    ASSERT(false);
    return PR_COMPLETE;
  }

  while (true) {
    if (state_ < ST_DATA) {
      size_t pos = *processed;
      while ((pos < len) && (buffer[pos] != '\n')) {
        pos += 1;
      }
      if (pos >= len) {
        break;  
      }
      const char* line = buffer + *processed;
      size_t len = (pos - *processed);
      *processed = pos + 1;
      while ((len > 0) && isspace(static_cast<unsigned char>(line[len-1]))) {
        len -= 1;
      }
      ProcessResult result = ProcessLine(line, len, error);
      LOG(LS_VERBOSE) << "Processed line, result=" << result;

      if (PR_CONTINUE != result) {
        return result;
      }
    } else if (data_size_ == 0) {
      if (chunked_) {
        state_ = ST_CHUNKTERM;
      } else {
        return PR_COMPLETE;
      }
    } else {
      size_t available = len - *processed;
      if (available <= 0) {
        break; 
      }
      if ((data_size_ != SIZE_UNKNOWN) && (available > data_size_)) {
        available = data_size_;
      }
      size_t read = 0;
      ProcessResult result = ProcessData(buffer + *processed, available, read,
                                         error);
      LOG(LS_VERBOSE) << "Processed data, result: " << result << " read: "
                      << read << " err: " << error;

      if (PR_CONTINUE != result) {
        return result;
      }
      *processed += read;
      if (data_size_ != SIZE_UNKNOWN) {
        data_size_ -= read;
      }
    }
  }

  return PR_CONTINUE;
}

HttpParser::ProcessResult
HttpParser::ProcessLine(const char* line, size_t len, HttpError* error) {
  LOG_F(LS_VERBOSE) << " state: " << state_ << " line: "
                    << std::string(line, len) << " len: " << len << " err: "
                    << error;

  switch (state_) {
  case ST_LEADER:
    state_ = ST_HEADERS;
    return ProcessLeader(line, len, error);

  case ST_HEADERS:
    if (len > 0) {
      const char* value = strchrn(line, len, ':');
      if (!value) {
        *error = HE_PROTOCOL;
        return PR_COMPLETE;
      }
      size_t nlen = (value - line);
      const char* eol = line + len;
      do {
        value += 1;
      } while ((value < eol) && isspace(static_cast<unsigned char>(*value)));
      size_t vlen = eol - value;
      if (MatchHeader(line, nlen, HH_CONTENT_LENGTH)) {
        
        
        
        std::string value_str(value, vlen);
        unsigned int temp_size;
        if (sscanf(value_str.c_str(), "%u", &temp_size) != 1) {
          *error = HE_PROTOCOL;
          return PR_COMPLETE;
        }
        data_size_ = static_cast<size_t>(temp_size);
      } else if (MatchHeader(line, nlen, HH_TRANSFER_ENCODING)) {
        if ((vlen == 7) && (_strnicmp(value, "chunked", 7) == 0)) {
          chunked_ = true;
        } else if ((vlen == 8) && (_strnicmp(value, "identity", 8) == 0)) {
          chunked_ = false;
        } else {
          *error = HE_PROTOCOL;
          return PR_COMPLETE;
        }
      }
      return ProcessHeader(line, nlen, value, vlen, error);
    } else {
      state_ = chunked_ ? ST_CHUNKSIZE : ST_DATA;
      return ProcessHeaderComplete(chunked_, data_size_, error);
    }
    break;

  case ST_CHUNKSIZE:
    if (len > 0) {
      char* ptr = NULL;
      data_size_ = strtoul(line, &ptr, 16);
      if (ptr != line + len) {
        *error = HE_PROTOCOL;
        return PR_COMPLETE;
      }
      state_ = (data_size_ == 0) ? ST_TRAILERS : ST_DATA;
    } else {
      *error = HE_PROTOCOL;
      return PR_COMPLETE;
    }
    break;

  case ST_CHUNKTERM:
    if (len > 0) {
      *error = HE_PROTOCOL;
      return PR_COMPLETE;
    } else {
      state_ = chunked_ ? ST_CHUNKSIZE : ST_DATA;
    }
    break;

  case ST_TRAILERS:
    if (len == 0) {
      return PR_COMPLETE;
    }
    
    break;

  default:
    ASSERT(false);
    break;
  }

  return PR_CONTINUE;
}

bool
HttpParser::is_valid_end_of_input() const {
  return (state_ == ST_DATA) && (data_size_ == SIZE_UNKNOWN);
}

void
HttpParser::complete(HttpError error) {
  if (state_ < ST_COMPLETE) {
    state_ = ST_COMPLETE;
    OnComplete(error);
  }
}





class BlockingMemoryStream : public ExternalMemoryStream {
public:
  BlockingMemoryStream(char* buffer, size_t size)
  : ExternalMemoryStream(buffer, size) { }

  virtual StreamResult DoReserve(size_t size, int* error) {
    return (buffer_length_ >= size) ? SR_SUCCESS : SR_BLOCK;
  }
};

class HttpBase::DocumentStream : public StreamInterface {
public:
  DocumentStream(HttpBase* base) : base_(base), error_(HE_DEFAULT) { }

  virtual StreamState GetState() const {
    if (NULL == base_)
      return SS_CLOSED;
    if (HM_RECV == base_->mode_)
      return SS_OPEN;
    return SS_OPENING;
  }

  virtual StreamResult Read(void* buffer, size_t buffer_len,
                            size_t* read, int* error) {
    if (!base_) {
      if (error) *error = error_;
      return (HE_NONE == error_) ? SR_EOS : SR_ERROR;
    }

    if (HM_RECV != base_->mode_) {
      return SR_BLOCK;
    }

    
    
    
    
    
    
    
    scoped_ptr<StreamInterface>
        stream(new BlockingMemoryStream(reinterpret_cast<char*>(buffer),
                                        buffer_len));

    
    base_->data_->document.swap(stream);

    
    
    
    
    
    HttpError http_error;
    bool complete = base_->DoReceiveLoop(&http_error);

    
    base_->data_->document.swap(stream);

    
    
    
    StreamResult result = SR_BLOCK;
    if (complete) {
      HttpBase* base = Disconnect(http_error);
      if (error) *error = error_;
      result = (HE_NONE == error_) ? SR_EOS : SR_ERROR;
      base->complete(http_error);
    }

    
    
    size_t position;
    stream->GetPosition(&position);
    if (position > 0) {
      if (read) *read = position;
      result = SR_SUCCESS;
    }
    return result;
  }

  virtual StreamResult Write(const void* data, size_t data_len,
                             size_t* written, int* error) {
    if (error) *error = -1;
    return SR_ERROR;
  }

  virtual void Close() {
    if (base_) {
      HttpBase* base = Disconnect(HE_NONE);
      if (HM_RECV == base->mode_ && base->http_stream_) {
        
        
        base->http_stream_->PostEvent(SE_READ, 0);
      }
    }
  }

  virtual bool GetAvailable(size_t* size) const {
    if (!base_ || HM_RECV != base_->mode_)
      return false;
    size_t data_size = base_->GetDataRemaining();
    if (SIZE_UNKNOWN == data_size)
      return false;
    if (size)
      *size = data_size;
    return true;
  }

  HttpBase* Disconnect(HttpError error) {
    ASSERT(NULL != base_);
    ASSERT(NULL != base_->doc_stream_);
    HttpBase* base = base_;
    base_->doc_stream_ = NULL;
    base_ = NULL;
    error_ = error;
    return base;
  }

private:
  HttpBase* base_;
  HttpError error_;
};





HttpBase::HttpBase() : mode_(HM_NONE), data_(NULL), notify_(NULL),
                       http_stream_(NULL), doc_stream_(NULL) {
}

HttpBase::~HttpBase() {
  ASSERT(HM_NONE == mode_);
}

bool
HttpBase::isConnected() const {
  return (http_stream_ != NULL) && (http_stream_->GetState() == SS_OPEN);
}

bool
HttpBase::attach(StreamInterface* stream) {
  if ((mode_ != HM_NONE) || (http_stream_ != NULL) || (stream == NULL)) {
    ASSERT(false);
    return false;
  }
  http_stream_ = stream;
  http_stream_->SignalEvent.connect(this, &HttpBase::OnHttpStreamEvent);
  mode_ = (http_stream_->GetState() == SS_OPENING) ? HM_CONNECT : HM_NONE;
  return true;
}

StreamInterface*
HttpBase::detach() {
  ASSERT(HM_NONE == mode_);
  if (mode_ != HM_NONE) {
    return NULL;
  }
  StreamInterface* stream = http_stream_;
  http_stream_ = NULL;
  if (stream) {
    stream->SignalEvent.disconnect(this);
  }
  return stream;
}

void
HttpBase::send(HttpData* data) {
  ASSERT(HM_NONE == mode_);
  if (mode_ != HM_NONE) {
    return;
  } else if (!isConnected()) {
    OnHttpStreamEvent(http_stream_, SE_CLOSE, HE_DISCONNECTED);
    return;
  }

  mode_ = HM_SEND;
  data_ = data;
  len_ = 0;
  ignore_data_ = chunk_data_ = false;

  if (data_->document) {
    data_->document->SignalEvent.connect(this, &HttpBase::OnDocumentEvent);
  }

  std::string encoding;
  if (data_->hasHeader(HH_TRANSFER_ENCODING, &encoding)
      && (encoding == "chunked")) {
    chunk_data_ = true;
  }

  len_ = data_->formatLeader(buffer_, sizeof(buffer_));
  len_ += strcpyn(buffer_ + len_, sizeof(buffer_) - len_, "\r\n");

  header_ = data_->begin();
  if (header_ == data_->end()) {
    
    queue_headers();
  }

  flush_data();
}

void
HttpBase::recv(HttpData* data) {
  ASSERT(HM_NONE == mode_);
  if (mode_ != HM_NONE) {
    return;
  } else if (!isConnected()) {
    OnHttpStreamEvent(http_stream_, SE_CLOSE, HE_DISCONNECTED);
    return;
  }

  mode_ = HM_RECV;
  data_ = data;
  len_ = 0;
  ignore_data_ = chunk_data_ = false;

  reset();
  if (doc_stream_) {
    doc_stream_->SignalEvent(doc_stream_, SE_OPEN | SE_READ, 0);
  } else {
    read_and_process_data();
  }
}

void
HttpBase::abort(HttpError err) {
  if (mode_ != HM_NONE) {
    if (http_stream_ != NULL) {
      http_stream_->Close();
    }
    do_complete(err);
  }
}

StreamInterface* HttpBase::GetDocumentStream() {
  if (doc_stream_)
    return NULL;
  doc_stream_ = new DocumentStream(this);
  return doc_stream_;
}

HttpError HttpBase::HandleStreamClose(int error) {
  if (http_stream_ != NULL) {
    http_stream_->Close();
  }
  if (error == 0) {
    if ((mode_ == HM_RECV) && is_valid_end_of_input()) {
      return HE_NONE;
    } else {
      return HE_DISCONNECTED;
    }
  } else if (error == SOCKET_EACCES) {
    return HE_AUTH;
  } else if (error == SEC_E_CERT_EXPIRED) {
    return HE_CERTIFICATE_EXPIRED;
  }
  LOG_F(LS_ERROR) << "(" << error << ")";
  return (HM_CONNECT == mode_) ? HE_CONNECT_FAILED : HE_SOCKET_ERROR;
}

bool HttpBase::DoReceiveLoop(HttpError* error) {
  ASSERT(HM_RECV == mode_);
  ASSERT(NULL != error);

  
  
  
  

  size_t loop_count = 0;
  const size_t kMaxReadCount = 20;
  bool process_requires_more_data = false;
  do {
    
    
    

    if (len_ < sizeof(buffer_)) {
      
      size_t read;
      int read_error;
      StreamResult read_result = http_stream_->Read(buffer_ + len_,
                                                    sizeof(buffer_) - len_,
                                                    &read, &read_error);
      switch (read_result) {
      case SR_SUCCESS:
        ASSERT(len_ + read <= sizeof(buffer_));
        len_ += read;
        break;
      case SR_BLOCK:
        if (process_requires_more_data) {
          
          return false;
        }
        
        break;
      case SR_EOS:
        
        read_error = 0;
      case SR_ERROR:
        *error = HandleStreamClose(read_error);
        return true;
      }
    } else if (process_requires_more_data) {
      
      
      
      *error = HE_OVERFLOW;
      return true;
    }

    
    
    
    
    
    
    size_t processed;
    ProcessResult process_result = Process(buffer_, len_, &processed,
                                            error);
    ASSERT(processed <= len_);
    len_ -= processed;
    memmove(buffer_, buffer_ + processed, len_);
    switch (process_result) {
    case PR_CONTINUE:
      
      process_requires_more_data = true;
      break;
    case PR_BLOCK:
      
      return false;
    case PR_COMPLETE:
      
      return true;
    }
  } while (++loop_count <= kMaxReadCount);

  LOG_F(LS_WARNING) << "danger of starvation";
  return false;
}

void
HttpBase::read_and_process_data() {
  HttpError error;
  if (DoReceiveLoop(&error)) {
    complete(error);
  }
}

void
HttpBase::flush_data() {
  ASSERT(HM_SEND == mode_);

  
  
  bool send_required = (len_ >= sizeof(buffer_));

  while (true) {
    ASSERT(len_ <= sizeof(buffer_));

    
    
    
    
    
    

    if (!send_required && (header_ != data_->end())) {
      
      send_required = queue_headers();
    }

    if (!send_required && data_->document) {
      

      const size_t kChunkDigits = 8;
      size_t offset, reserve;
      if (chunk_data_) {
        
        offset = len_ + kChunkDigits + 2;
        
        reserve = offset + 2;
      } else {
        offset = len_;
        reserve = offset;
      }

      if (reserve >= sizeof(buffer_)) {
        send_required = true;
      } else {
        size_t read;
        int error;
        StreamResult result = data_->document->Read(buffer_ + offset,
                                                    sizeof(buffer_) - reserve,
                                                    &read, &error);
        if (result == SR_SUCCESS) {
          ASSERT(reserve + read <= sizeof(buffer_));
          if (chunk_data_) {
            
            
            
            sprintfn(buffer_ + len_, kChunkDigits + 1, "%.*x",
                     kChunkDigits, read);
            
            memcpy(buffer_ + len_ + kChunkDigits, "\r\n", 2);
            
            memcpy(buffer_ + offset + read, "\r\n", 2);
          }
          len_ = reserve + read;
        } else if (result == SR_BLOCK) {
          
          send_required = true;
        } else if (result == SR_EOS) {
          if (chunk_data_) {
            
            
            ASSERT(len_ + 5 <= sizeof(buffer_));
            memcpy(buffer_ + len_, "0\r\n\r\n", 5);
            len_ += 5;
            chunk_data_ = false;
          } else if (0 == len_) {
            
            do_complete();
            return;
          }
          
          
          send_required = true;
        } else {
          LOG_F(LS_ERROR) << "Read error: " << error;
          do_complete(HE_STREAM);
          return;
        }
      }
    }

    if (0 == len_) {
      
      if (!data_->document) {
        
        do_complete();
      }
      return;
    }

    size_t written;
    int error;
    StreamResult result = http_stream_->Write(buffer_, len_, &written, &error);
    if (result == SR_SUCCESS) {
      ASSERT(written <= len_);
      len_ -= written;
      memmove(buffer_, buffer_ + written, len_);
      send_required = false;
    } else if (result == SR_BLOCK) {
      if (send_required) {
        
        return;
      }
    } else {
      ASSERT(result == SR_ERROR);
      LOG_F(LS_ERROR) << "error";
      OnHttpStreamEvent(http_stream_, SE_CLOSE, error);
      return;
    }
  }

  ASSERT(false);
}

bool
HttpBase::queue_headers() {
  ASSERT(HM_SEND == mode_);
  while (header_ != data_->end()) {
    size_t len = sprintfn(buffer_ + len_, sizeof(buffer_) - len_,
                          "%.*s: %.*s\r\n",
                          header_->first.size(), header_->first.data(),
                          header_->second.size(), header_->second.data());
    if (len_ + len < sizeof(buffer_) - 3) {
      len_ += len;
      ++header_;
    } else if (len_ == 0) {
      LOG(WARNING) << "discarding header that is too long: " << header_->first;
      ++header_;
    } else {
      
      return true;
    }
  }
  
  len_ += strcpyn(buffer_ + len_, sizeof(buffer_) - len_, "\r\n");
  return false;
}

void
HttpBase::do_complete(HttpError err) {
  ASSERT(mode_ != HM_NONE);
  HttpMode mode = mode_;
  mode_ = HM_NONE;
  if (data_ && data_->document) {
    data_->document->SignalEvent.disconnect(this);
  }
  data_ = NULL;
  if ((HM_RECV == mode) && doc_stream_) {
    ASSERT(HE_NONE != err);  
    DocumentStream* ds = doc_stream_;
    ds->Disconnect(err);
    ds->SignalEvent(ds, SE_CLOSE, err);
  }
  if (notify_) {
    notify_->onHttpComplete(mode, err);
  }
}





void
HttpBase::OnHttpStreamEvent(StreamInterface* stream, int events, int error) {
  ASSERT(stream == http_stream_);
  if ((events & SE_OPEN) && (mode_ == HM_CONNECT)) {
    do_complete();
    return;
  }

  if ((events & SE_WRITE) && (mode_ == HM_SEND)) {
    flush_data();
    return;
  }

  if ((events & SE_READ) && (mode_ == HM_RECV)) {
    if (doc_stream_) {
      doc_stream_->SignalEvent(doc_stream_, SE_READ, 0);
    } else {
      read_and_process_data();
    }
    return;
  }

  if ((events & SE_CLOSE) == 0)
    return;

  HttpError http_error = HandleStreamClose(error);
  if (mode_ == HM_RECV) {
    complete(http_error);
  } else if (mode_ != HM_NONE) {
    do_complete(http_error);
  } else if (notify_) {
    notify_->onHttpClosed(http_error);
  }
}

void
HttpBase::OnDocumentEvent(StreamInterface* stream, int events, int error) {
  ASSERT(stream == data_->document.get());
  if ((events & SE_WRITE) && (mode_ == HM_RECV)) {
    read_and_process_data();
    return;
  }

  if ((events & SE_READ) && (mode_ == HM_SEND)) {
    flush_data();
    return;
  }

  if (events & SE_CLOSE) {
    LOG_F(LS_ERROR) << "Read error: " << error;
    do_complete(HE_STREAM);
    return;
  }
}





HttpParser::ProcessResult
HttpBase::ProcessLeader(const char* line, size_t len, HttpError* error) {
  *error = data_->parseLeader(line, len);
  return (HE_NONE == *error) ? PR_CONTINUE : PR_COMPLETE;
}

HttpParser::ProcessResult
HttpBase::ProcessHeader(const char* name, size_t nlen, const char* value,
                        size_t vlen, HttpError* error) {
  std::string sname(name, nlen), svalue(value, vlen);
  data_->addHeader(sname, svalue);
  return PR_CONTINUE;
}

HttpParser::ProcessResult
HttpBase::ProcessHeaderComplete(bool chunked, size_t& data_size,
                                HttpError* error) {
  StreamInterface* old_docstream = doc_stream_;
  if (notify_) {
    *error = notify_->onHttpHeaderComplete(chunked, data_size);
    
    ASSERT(NULL != data_);
  }
  if ((HE_NONE == *error) && data_->document) {
    data_->document->SignalEvent.connect(this, &HttpBase::OnDocumentEvent);
  }
  if (HE_NONE != *error) {
    return PR_COMPLETE;
  }
  if (old_docstream != doc_stream_) {
    
    return PR_BLOCK;
  }
  return PR_CONTINUE;
}

HttpParser::ProcessResult
HttpBase::ProcessData(const char* data, size_t len, size_t& read,
                      HttpError* error) {
  if (ignore_data_ || !data_->document) {
    read = len;
    return PR_CONTINUE;
  }
  int write_error = 0;
  switch (data_->document->Write(data, len, &read, &write_error)) {
  case SR_SUCCESS:
    return PR_CONTINUE;
  case SR_BLOCK:
    return PR_BLOCK;
  case SR_EOS:
    LOG_F(LS_ERROR) << "Unexpected EOS";
    *error = HE_STREAM;
    return PR_COMPLETE;
  case SR_ERROR:
  default:
    LOG_F(LS_ERROR) << "Write error: " << write_error;
    *error = HE_STREAM;
    return PR_COMPLETE;
  }
}

void
HttpBase::OnComplete(HttpError err) {
  LOG_F(LS_VERBOSE);
  do_complete(err);
}

} 
