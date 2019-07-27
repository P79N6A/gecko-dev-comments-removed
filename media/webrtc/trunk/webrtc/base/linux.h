









#ifndef WEBRTC_BASE_LINUX_H_
#define WEBRTC_BASE_LINUX_H_

#if defined(WEBRTC_LINUX)
#include <string>
#include <map>
#include <vector>

#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/stream.h"

namespace rtc {
















class ConfigParser {
 public:
  typedef std::map<std::string, std::string> SimpleMap;
  typedef std::vector<SimpleMap> MapVector;

  ConfigParser();
  virtual ~ConfigParser();

  virtual bool Open(const std::string& filename);
  virtual void Attach(StreamInterface* stream);
  virtual bool Parse(MapVector* key_val_pairs);
  virtual bool ParseSection(SimpleMap* key_val_pair);
  virtual bool ParseLine(std::string* key, std::string* value);

 private:
  scoped_ptr<StreamInterface> instream_;
};















class ProcCpuInfo {
 public:
  ProcCpuInfo();
  virtual ~ProcCpuInfo();

  
  
  virtual bool LoadFromSystem();

  
  virtual bool GetNumCpus(int* num);

  
  virtual bool GetNumPhysicalCpus(int* num);

  
  virtual bool GetCpuFamily(int* id);

  
  
  virtual bool GetSectionCount(size_t* count);

  
  
  virtual bool GetSectionStringValue(size_t section_num, const std::string& key,
                                     std::string* result);

  
  
  virtual bool GetSectionIntValue(size_t section_num, const std::string& key,
                                  int* result);

 private:
  ConfigParser::MapVector sections_;
};

#if !defined(WEBRTC_CHROMIUM_BUILD)

std::string ReadLinuxLsbRelease();
#endif


std::string ReadLinuxUname();




int ReadCpuMaxFreq();

}  

#endif  
#endif  
