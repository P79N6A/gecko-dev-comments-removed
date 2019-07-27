









#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_TEST_BWE_TEST_FILEUTILS_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_TEST_BWE_TEST_FILEUTILS_H_

#include <stdio.h>

#include <string>

#include "webrtc/base/constructormagic.h"
#include "webrtc/modules/interface/module_common_types.h"

namespace webrtc {
namespace testing {
namespace bwe {

class ResourceFileReader {
 public:
  ~ResourceFileReader();

  bool IsAtEnd();
  bool Read(uint32_t* out);

  static ResourceFileReader* Create(const std::string& filename,
                                    const std::string& extension);

 private:
  explicit ResourceFileReader(FILE* file) : file_(file) {}
  FILE* file_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(ResourceFileReader);
};

class OutputFileWriter {
 public:
  ~OutputFileWriter();

  bool Write(uint32_t value);

  static OutputFileWriter* Create(const std::string& filename,
                                  const std::string& extension);

 private:
  explicit OutputFileWriter(FILE* file) : file_(file) {}
  FILE* file_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(OutputFileWriter);
};
}  
}  
}  

#endif  
