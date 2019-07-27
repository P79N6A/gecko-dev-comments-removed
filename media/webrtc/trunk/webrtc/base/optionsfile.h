









#ifndef WEBRTC_BASE_OPTIONSFILE_H_
#define WEBRTC_BASE_OPTIONSFILE_H_

#include <map>
#include <string>

namespace rtc {




class OptionsFile {
 public:
  OptionsFile(const std::string &path);

  
  bool Load();
  
  bool Save();

  bool GetStringValue(const std::string& option, std::string* out_val) const;
  bool GetIntValue(const std::string& option, int* out_val) const;
  bool SetStringValue(const std::string& option, const std::string& val);
  bool SetIntValue(const std::string& option, int val);
  bool RemoveValue(const std::string& option);

 private:
  typedef std::map<std::string, std::string> OptionsMap;

  static bool IsLegalName(const std::string &name);
  static bool IsLegalValue(const std::string &value);

  std::string path_;
  OptionsMap options_;
};

}  

#endif  
