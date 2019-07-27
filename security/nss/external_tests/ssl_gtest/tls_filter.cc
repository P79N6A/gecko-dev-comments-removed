





#include "tls_filter.h"

#include <iostream>

namespace nss_test {

bool TlsRecordFilter::Filter(const DataBuffer& input, DataBuffer* output) {
  bool changed = false;
  size_t output_offset = 0U;
  output->Allocate(input.len());

  TlsParser parser(input);
  while (parser.remaining()) {
    size_t start = parser.consumed();
    uint8_t content_type;
    if (!parser.Read(&content_type)) {
      return false;
    }
    uint32_t version;
    if (!parser.Read(&version, 2)) {
      return false;
    }

    if (IsDtls(version)) {
      if (!parser.Skip(8)) {
        return false;
      }
    }
    size_t header_len = parser.consumed() - start;
    output->Write(output_offset, input.data() + start, header_len);

    DataBuffer record;
    if (!parser.ReadVariable(&record, 2)) {
      return false;
    }

    
    
    
    output_offset = ApplyFilter(content_type, version, record, output,
                                output_offset + header_len,
                                &changed);
  }
  output->Truncate(output_offset);

  
  if (changed) {
    ++count_;
  }

  return changed;
}

size_t TlsRecordFilter::ApplyFilter(uint8_t content_type, uint16_t version,
                                    const DataBuffer& record,
                                    DataBuffer* output,
                                    size_t offset, bool* changed) {
  const DataBuffer* source = &record;
  DataBuffer filtered;
  if (FilterRecord(content_type, version, record, &filtered) &&
      filtered.len() < 0x10000) {
    *changed = true;
    std::cerr << "record old: " << record << std::endl;
    std::cerr << "record new: " << filtered << std::endl;
    source = &filtered;
  }

  output->Write(offset, source->len(), 2);
  output->Write(offset + 2, *source);
  return offset + 2 + source->len();
}

bool TlsHandshakeFilter::FilterRecord(uint8_t content_type, uint16_t version,
                                      const DataBuffer& input,
                                      DataBuffer* output) {
  
  if (content_type != kTlsHandshakeType) {
    return false;
  }

  bool changed = false;
  size_t output_offset = 0U;
  output->Allocate(input.len()); 

  TlsParser parser(input);
  while (parser.remaining()) {
    size_t start = parser.consumed();
    uint8_t handshake_type;
    if (!parser.Read(&handshake_type)) {
      return false; 
    }
    uint32_t length;
    if (!ReadLength(&parser, version, &length)) {
      return false;
    }

    size_t header_len = parser.consumed() - start;
    output->Write(output_offset, input.data() + start, header_len);

    DataBuffer handshake;
    if (!parser.Read(&handshake, length)) {
      return false;
    }

    
    
    
    
    output_offset = ApplyFilter(version, handshake_type, handshake,
                                output, output_offset + 1,
                                output_offset + header_len,
                                &changed);
  }
  output->Truncate(output_offset);
  return changed;
}

bool TlsHandshakeFilter::ReadLength(TlsParser* parser, uint16_t version, uint32_t *length) {
  if (!parser->Read(length, 3)) {
    return false; 
  }

  if (!IsDtls(version)) {
    return true; 
  }

  
  if (!parser->Skip(2)) { 
    return false;
  }

  uint32_t fragment_offset;
  if (!parser->Read(&fragment_offset, 3)) {
    return false;
  }

  uint32_t fragment_length;
  if (!parser->Read(&fragment_length, 3)) {
    return false;
  }

  
  return (fragment_offset == 0 && fragment_length == *length);
}

size_t TlsHandshakeFilter::ApplyFilter(
    uint16_t version, uint8_t handshake_type, const DataBuffer& handshake,
    DataBuffer* output, size_t length_offset, size_t value_offset,
    bool* changed) {
  const DataBuffer* source = &handshake;
  DataBuffer filtered;
  if (FilterHandshake(version, handshake_type, handshake, &filtered) &&
      filtered.len() < 0x1000000) {
    *changed = true;
    std::cerr << "handshake old: " << handshake << std::endl;
    std::cerr << "handshake new: " << filtered << std::endl;
    source = &filtered;
  }

  
  
  output->Write(length_offset, source->len(), 3);
  if (IsDtls(version)) {
    output->Write(length_offset + 8, source->len(), 3);
  }
  output->Write(value_offset, *source);
  return value_offset + source->len();
}

bool TlsInspectorRecordHandshakeMessage::FilterHandshake(
    uint16_t version, uint8_t handshake_type,
    const DataBuffer& input, DataBuffer* output) {
  
  if (buffer_.len()) {
    return false;
  }

  if (handshake_type == handshake_type_) {
    buffer_ = input;
  }
  return false;
}

bool TlsAlertRecorder::FilterRecord(uint8_t content_type, uint16_t version,
                                    const DataBuffer& input, DataBuffer* output) {
  if (level_ == kTlsAlertFatal) { 
    return false;
  }
  if (content_type != kTlsAlertType) {
    return false;
  }

  std::cerr << "Alert: " << input << std::endl;

  TlsParser parser(input);
  uint8_t lvl;
  if (!parser.Read(&lvl)) {
    return false;
  }
  if (lvl == kTlsAlertWarning) { 
    return false;
  }
  level_ = lvl;
  (void)parser.Read(&description_);
  return false;
}

ChainedPacketFilter::~ChainedPacketFilter() {
  for (auto it = filters_.begin(); it != filters_.end(); ++it) {
    delete *it;
  }
}

bool ChainedPacketFilter::Filter(const DataBuffer& input, DataBuffer* output) {
  DataBuffer in(input);
  bool changed = false;
  for (auto it = filters_.begin(); it != filters_.end(); ++it) {
    if ((*it)->Filter(in, output)) {
      in = *output;
      changed = true;
    }
  }
  return changed;
}

}  
