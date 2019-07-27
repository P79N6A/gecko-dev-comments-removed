









#include "webrtc/base/optionsfile.h"

#include <ctype.h>

#include "webrtc/base/logging.h"
#include "webrtc/base/stream.h"
#include "webrtc/base/stringencode.h"

namespace rtc {

OptionsFile::OptionsFile(const std::string &path) : path_(path) {
}

bool OptionsFile::Load() {
  options_.clear();
  
  FileStream stream;
  int err;
  if (!stream.Open(path_, "r", &err)) {
    LOG_F(LS_WARNING) << "Could not open file, err=" << err;
    
    
    return true;
  }
  
  std::string line;
  StreamResult res;
  for (;;) {
    res = stream.ReadLine(&line);
    if (res != SR_SUCCESS) {
      break;
    }
    size_t equals_pos = line.find('=');
    if (equals_pos == std::string::npos) {
      
      
      LOG_F(LS_WARNING) << "Ignoring malformed line in " << path_;
      continue;
    }
    std::string key(line, 0, equals_pos);
    std::string value(line, equals_pos + 1, line.length() - (equals_pos + 1));
    options_[key] = value;
  }
  if (res != SR_EOS) {
    LOG_F(LS_ERROR) << "Error when reading from file";
    return false;
  } else {
    return true;
  }
}

bool OptionsFile::Save() {
  
  FileStream stream;
  int err;
  if (!stream.Open(path_, "w", &err)) {
    LOG_F(LS_ERROR) << "Could not open file, err=" << err;
    return false;
  }
  
  StreamResult res = SR_SUCCESS;
  size_t written;
  int error;
  for (OptionsMap::const_iterator i = options_.begin(); i != options_.end();
       ++i) {
    res = stream.WriteAll(i->first.c_str(), i->first.length(), &written,
        &error);
    if (res != SR_SUCCESS) {
      break;
    }
    res = stream.WriteAll("=", 1, &written, &error);
    if (res != SR_SUCCESS) {
      break;
    }
    res = stream.WriteAll(i->second.c_str(), i->second.length(), &written,
        &error);
    if (res != SR_SUCCESS) {
      break;
    }
    res = stream.WriteAll("\n", 1, &written, &error);
    if (res != SR_SUCCESS) {
      break;
    }
  }
  if (res != SR_SUCCESS) {
    LOG_F(LS_ERROR) << "Unable to write to file";
    return false;
  } else {
    return true;
  }
}

bool OptionsFile::IsLegalName(const std::string &name) {
  for (size_t pos = 0; pos < name.length(); ++pos) {
    if (name[pos] == '\n' || name[pos] == '\\' || name[pos] == '=') {
      
      LOG(LS_WARNING) << "Ignoring operation for illegal option " << name;
      return false;
    }
  }
  return true;
}

bool OptionsFile::IsLegalValue(const std::string &value) {
  for (size_t pos = 0; pos < value.length(); ++pos) {
    if (value[pos] == '\n' || value[pos] == '\\') {
      
      LOG(LS_WARNING) << "Ignoring operation for illegal value " << value;
      return false;
    }
  }
  return true;
}

bool OptionsFile::GetStringValue(const std::string& option,
                                 std::string *out_val) const {
  LOG(LS_VERBOSE) << "OptionsFile::GetStringValue "
                  << option;
  if (!IsLegalName(option)) {
    return false;
  }
  OptionsMap::const_iterator i = options_.find(option);
  if (i == options_.end()) {
    return false;
  }
  *out_val = i->second;
  return true;
}

bool OptionsFile::GetIntValue(const std::string& option,
                              int *out_val) const {
  LOG(LS_VERBOSE) << "OptionsFile::GetIntValue "
                  << option;
  if (!IsLegalName(option)) {
    return false;
  }
  OptionsMap::const_iterator i = options_.find(option);
  if (i == options_.end()) {
    return false;
  }
  return FromString(i->second, out_val);
}

bool OptionsFile::SetStringValue(const std::string& option,
                                 const std::string& value) {
  LOG(LS_VERBOSE) << "OptionsFile::SetStringValue "
                  << option << ":" << value;
  if (!IsLegalName(option) || !IsLegalValue(value)) {
    return false;
  }
  options_[option] = value;
  return true;
}

bool OptionsFile::SetIntValue(const std::string& option,
                              int value) {
  LOG(LS_VERBOSE) << "OptionsFile::SetIntValue "
                  << option << ":" << value;
  if (!IsLegalName(option)) {
    return false;
  }
  return ToString(value, &options_[option]);
}

bool OptionsFile::RemoveValue(const std::string& option) {
  LOG(LS_VERBOSE) << "OptionsFile::RemoveValue " << option;
  if (!IsLegalName(option)) {
    return false;
  }
  options_.erase(option);
  return true;
}

}  
