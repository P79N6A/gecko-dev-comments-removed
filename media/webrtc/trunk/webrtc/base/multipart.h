









#ifndef WEBRTC_BASE_MULTIPART_H__
#define WEBRTC_BASE_MULTIPART_H__

#include <string>
#include <vector>

#include "webrtc/base/sigslot.h"
#include "webrtc/base/stream.h"

namespace rtc {






class MultipartStream : public StreamInterface, public sigslot::has_slots<> {
 public:
  MultipartStream(const std::string& type, const std::string& boundary);
  virtual ~MultipartStream();

  void GetContentType(std::string* content_type);

  
  
  bool AddPart(StreamInterface* data_stream,
               const std::string& content_disposition,
               const std::string& content_type);
  bool AddPart(const std::string& data,
               const std::string& content_disposition,
               const std::string& content_type);
  void EndParts();

  
  size_t GetPartSize(const std::string& data,
                     const std::string& content_disposition,
                     const std::string& content_type) const;
  size_t GetEndPartSize() const;

  
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

 private:
  typedef std::vector<StreamInterface*> PartList;

  
  void OnEvent(StreamInterface* stream, int events, int error);

  std::string type_, boundary_;
  PartList parts_;
  bool adding_;
  size_t current_;  
  size_t position_;  

  DISALLOW_COPY_AND_ASSIGN(MultipartStream);
};

}  

#endif  
