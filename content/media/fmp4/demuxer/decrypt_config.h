



#ifndef MEDIA_BASE_DECRYPT_CONFIG_H_
#define MEDIA_BASE_DECRYPT_CONFIG_H_

#include <string>
#include <vector>

#include "mp4_demuxer/basictypes.h"

namespace mp4_demuxer {










struct SubsampleEntry {
  uint32_t clear_bytes;
  uint32_t cypher_bytes;
};


class DecryptConfig {
 public:
  
  static const int kDecryptionKeySize = 16;

  
  
  
  
  
  
  
  
  
  
  
  
  DecryptConfig(const std::string& key_id,
                const std::string& iv,
                const int data_offset,
                const std::vector<SubsampleEntry>& subsamples);
  ~DecryptConfig();

  const std::string& key_id() const { return key_id_; }
  const std::string& iv() const { return iv_; }
  int data_offset() const { return data_offset_; }
  const std::vector<SubsampleEntry>& subsamples() const { return subsamples_; }

 private:
  const std::string key_id_;

  
  const std::string iv_;

  
  
  
  const int data_offset_;

  
  
  const std::vector<SubsampleEntry> subsamples_;

  DISALLOW_COPY_AND_ASSIGN(DecryptConfig);
};

}  

#endif  
