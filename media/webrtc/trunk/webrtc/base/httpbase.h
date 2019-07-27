










#ifndef WEBRTC_BASE_HTTPBASE_H__
#define WEBRTC_BASE_HTTPBASE_H__

#include "webrtc/base/httpcommon.h"

namespace rtc {

class StreamInterface;








class HttpParser {
public:
  enum ProcessResult { PR_CONTINUE, PR_BLOCK, PR_COMPLETE };
  HttpParser();
  virtual ~HttpParser();
  
  void reset();
  ProcessResult Process(const char* buffer, size_t len, size_t* processed,
                        HttpError* error);
  bool is_valid_end_of_input() const;
  void complete(HttpError err);
  
  size_t GetDataRemaining() const { return data_size_; }

protected:
  ProcessResult ProcessLine(const char* line, size_t len, HttpError* error);

  
  virtual ProcessResult ProcessLeader(const char* line, size_t len,
                                      HttpError* error) = 0;
  virtual ProcessResult ProcessHeader(const char* name, size_t nlen,
                                      const char* value, size_t vlen,
                                      HttpError* error) = 0;
  virtual ProcessResult ProcessHeaderComplete(bool chunked, size_t& data_size,
                                              HttpError* error) = 0;
  virtual ProcessResult ProcessData(const char* data, size_t len, size_t& read,
                                    HttpError* error) = 0;
  virtual void OnComplete(HttpError err) = 0;
  
private:
  enum State {
    ST_LEADER, ST_HEADERS,
    ST_CHUNKSIZE, ST_CHUNKTERM, ST_TRAILERS,
    ST_DATA, ST_COMPLETE
  } state_;
  bool chunked_;
  size_t data_size_;
};





enum HttpMode { HM_NONE, HM_CONNECT, HM_RECV, HM_SEND };

class IHttpNotify {
public:
  virtual ~IHttpNotify() {}
  virtual HttpError onHttpHeaderComplete(bool chunked, size_t& data_size) = 0;
  virtual void onHttpComplete(HttpMode mode, HttpError err) = 0;
  virtual void onHttpClosed(HttpError err) = 0;
};











class HttpBase
: private HttpParser,
  public sigslot::has_slots<>
{
public:
  HttpBase();
  virtual ~HttpBase();

  void notify(IHttpNotify* notify) { notify_ = notify; }
  bool attach(StreamInterface* stream);
  StreamInterface* stream() { return http_stream_; }
  StreamInterface* detach();
  bool isConnected() const;

  void send(HttpData* data);
  void recv(HttpData* data);
  void abort(HttpError err);

  HttpMode mode() const { return mode_; }

  void set_ignore_data(bool ignore) { ignore_data_ = ignore; }
  bool ignore_data() const { return ignore_data_; }

  
  
  
  StreamInterface* GetDocumentStream();

protected:
  
  
  HttpError HandleStreamClose(int error);

  
  
  
  
  
  
  
  
  
  
  
  
  
  bool DoReceiveLoop(HttpError* err);

  void read_and_process_data();
  void flush_data();
  bool queue_headers();
  void do_complete(HttpError err = HE_NONE);

  void OnHttpStreamEvent(StreamInterface* stream, int events, int error);
  void OnDocumentEvent(StreamInterface* stream, int events, int error);

  
  virtual ProcessResult ProcessLeader(const char* line, size_t len,
                                      HttpError* error);
  virtual ProcessResult ProcessHeader(const char* name, size_t nlen,
                                      const char* value, size_t vlen,
                                      HttpError* error);
  virtual ProcessResult ProcessHeaderComplete(bool chunked, size_t& data_size,
                                              HttpError* error);
  virtual ProcessResult ProcessData(const char* data, size_t len, size_t& read,
                                    HttpError* error);
  virtual void OnComplete(HttpError err);

private:
  class DocumentStream;
  friend class DocumentStream;

  enum { kBufferSize = 32 * 1024 };

  HttpMode mode_;
  HttpData* data_;
  IHttpNotify* notify_;
  StreamInterface* http_stream_;
  DocumentStream* doc_stream_;
  char buffer_[kBufferSize];
  size_t len_;

  bool ignore_data_, chunk_data_;
  HttpData::const_iterator header_;
};



} 

#endif
