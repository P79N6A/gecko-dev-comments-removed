





#ifndef tls_filter_h_
#define tls_filter_h_

#include <memory>
#include <vector>

#include "test_io.h"
#include "tls_parser.h"

namespace nss_test {


class TlsRecordFilter : public PacketFilter {
 public:
  TlsRecordFilter() : count_(0) {}

  virtual bool Filter(const DataBuffer& input, DataBuffer* output);

  
  size_t filtered_packets() const { return count_; }

 protected:
  virtual bool FilterRecord(uint8_t content_type, uint16_t version,
                            const DataBuffer& data, DataBuffer* changed) = 0;
 private:
  size_t ApplyFilter(uint8_t content_type, uint16_t version,
                     const DataBuffer& record, DataBuffer* output,
                     size_t offset, bool* changed);

  size_t count_;
};




class TlsHandshakeFilter : public TlsRecordFilter {
 public:
  TlsHandshakeFilter() {}

  
  
  static bool ReadLength(TlsParser* parser, uint16_t version, uint32_t *length);

 protected:
  virtual bool FilterRecord(uint8_t content_type, uint16_t version,
                            const DataBuffer& input, DataBuffer* output);
  virtual bool FilterHandshake(uint16_t version, uint8_t handshake_type,
                               const DataBuffer& input, DataBuffer* output) = 0;

 private:
  size_t ApplyFilter(uint16_t version, uint8_t handshake_type,
                     const DataBuffer& record, DataBuffer* output,
                     size_t length_offset, size_t value_offset, bool* changed);
};


class TlsInspectorRecordHandshakeMessage : public TlsHandshakeFilter {
 public:
  TlsInspectorRecordHandshakeMessage(uint8_t handshake_type)
      : handshake_type_(handshake_type), buffer_() {}

  virtual bool FilterHandshake(uint16_t version, uint8_t handshake_type,
                               const DataBuffer& input, DataBuffer* output);

  const DataBuffer& buffer() const { return buffer_; }

 private:
  uint8_t handshake_type_;
  DataBuffer buffer_;
};



class TlsAlertRecorder : public TlsRecordFilter {
 public:
  TlsAlertRecorder() : level_(255), description_(255) {}

  virtual bool FilterRecord(uint8_t content_type, uint16_t version,
                            const DataBuffer& input, DataBuffer* output);

  uint8_t level() const { return level_; }
  uint8_t description() const { return description_; }

 private:
  uint8_t level_;
  uint8_t description_;
};


class ChainedPacketFilter : public PacketFilter {
 public:
  ChainedPacketFilter() {}
  ChainedPacketFilter(const std::vector<PacketFilter*> filters)
      : filters_(filters.begin(), filters.end()) {}
  virtual ~ChainedPacketFilter();

  virtual bool Filter(const DataBuffer& input, DataBuffer* output);

  
  void Add(PacketFilter* filter) {
    filters_.push_back(filter);
  }

 private:
  std::vector<PacketFilter*> filters_;
};

}  

#endif
