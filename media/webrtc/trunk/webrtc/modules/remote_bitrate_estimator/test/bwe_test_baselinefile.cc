









#include "webrtc/modules/remote_bitrate_estimator/test/bwe_test_baselinefile.h"

#include <stdio.h>

#include <algorithm>
#include <vector>

#include "webrtc/base/constructormagic.h"
#include "webrtc/modules/remote_bitrate_estimator/test/bwe_test_fileutils.h"
#include "webrtc/modules/remote_bitrate_estimator/test/bwe_test_logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/test/testsupport/fileutils.h"

namespace webrtc {
namespace testing {
namespace bwe {








const uint32_t kMagicMarker = 0x42574521;
const uint32_t kFileVersion1 = 0x00000001;
const char kResourceSubDir[] = "remote_bitrate_estimator";

class BaseLineFileVerify : public BaseLineFileInterface {
 public:
  
  
  
  BaseLineFileVerify(const std::string& filepath, bool allow_missing_file)
      : reader_(),
        fail_to_read_response_(false) {
    scoped_ptr<ResourceFileReader> reader;
    reader.reset(ResourceFileReader::Create(filepath, "bin"));
    if (!reader.get()) {
      printf("WARNING: Missing baseline file for BWE test: %s.bin\n",
             filepath.c_str());
      fail_to_read_response_ = allow_missing_file;
    } else {
      uint32_t magic_marker = 0;
      uint32_t file_version = 0;
      if (reader->Read(&magic_marker) && magic_marker == kMagicMarker &&
          reader->Read(&file_version) && file_version == kFileVersion1) {
        reader_.swap(reader);
      } else {
        printf("WARNING: Bad baseline file header for BWE test: %s.bin\n",
               filepath.c_str());
      }
    }
  }
  virtual ~BaseLineFileVerify() {}

  virtual void Estimate(int64_t time_ms, uint32_t estimate_bps) {
    if (reader_.get()) {
      uint32_t read_ms = 0;
      uint32_t read_bps = 0;
      if (reader_->Read(&read_ms) && read_ms == time_ms &&
          reader_->Read(&read_bps) && read_bps == estimate_bps) {
      } else {
        printf("ERROR: Baseline differs starting at: %d ms (%d vs %d)!\n",
            static_cast<uint32_t>(time_ms), estimate_bps, read_bps);
        reader_.reset(NULL);
      }
    }
  }

  virtual bool VerifyOrWrite() {
    if (reader_.get()) {
      if (reader_->IsAtEnd()) {
        return true;
      } else {
        printf("ERROR: Baseline file contains more data!\n");
        return false;
      }
    }
    return fail_to_read_response_;
  }

 private:
  scoped_ptr<ResourceFileReader> reader_;
  bool fail_to_read_response_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(BaseLineFileVerify);
};

class BaseLineFileUpdate : public BaseLineFileInterface {
 public:
  BaseLineFileUpdate(const std::string& filepath,
                     BaseLineFileInterface* verifier)
      : verifier_(verifier),
        output_content_(),
        filepath_(filepath) {
    output_content_.push_back(kMagicMarker);
    output_content_.push_back(kFileVersion1);
  }
  virtual ~BaseLineFileUpdate() {}

  virtual void Estimate(int64_t time_ms, uint32_t estimate_bps) {
    verifier_->Estimate(time_ms, estimate_bps);
    output_content_.push_back(static_cast<uint32_t>(time_ms));
    output_content_.push_back(estimate_bps);
  }

  virtual bool VerifyOrWrite() {
    if (!verifier_->VerifyOrWrite()) {
      std::string dir_path = webrtc::test::OutputPath() + kResourceSubDir;
      if (!webrtc::test::CreateDir(dir_path)) {
        printf("WARNING: Cannot create output dir: %s\n", dir_path.c_str());
        return false;
      }
      scoped_ptr<OutputFileWriter> writer;
      writer.reset(OutputFileWriter::Create(filepath_, "bin"));
      if (!writer.get()) {
        printf("WARNING: Cannot create output file: %s.bin\n",
            filepath_.c_str());
        return false;
      }
      printf("NOTE: Writing baseline file for BWE test: %s.bin\n",
             filepath_.c_str());
      for (std::vector<uint32_t>::iterator it = output_content_.begin();
          it != output_content_.end(); ++it) {
        writer->Write(*it);
      }
      return true;
    }
    printf("NOTE: No change, not writing: %s\n", filepath_.c_str());
    return true;
  }

 private:
  scoped_ptr<BaseLineFileInterface> verifier_;
  std::vector<uint32_t> output_content_;
  std::string filepath_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(BaseLineFileUpdate);
};

BaseLineFileInterface* BaseLineFileInterface::Create(
    const std::string& filename, bool write_output_file) {
  std::string filepath = filename;
  std::replace(filepath.begin(), filepath.end(), '/', '_');
  filepath = std::string(kResourceSubDir) + "/" + filepath;

  scoped_ptr<BaseLineFileInterface> result;
  result.reset(new BaseLineFileVerify(filepath, !write_output_file));
  if (write_output_file) {
    
    result.reset(new BaseLineFileUpdate(filepath, result.release()));
  }
  return result.release();
}
}  
}  
}  
