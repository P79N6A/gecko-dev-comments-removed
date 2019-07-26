









#ifndef WEBRTC_VIDEO_ENGINE_TEST_AUTO_TEST_PRIMITIVES_
#define WEBRTC_VIDEO_ENGINE_TEST_AUTO_TEST_PRIMITIVES_

#include <cstdlib>

#include <map>
#include <string>
#include <vector>

#include "gflags/gflags.h"

namespace webrtc {

class InputValidator;
class OverrideRegistry;


class InputBuilder {
 public:
  
  
  InputBuilder(const std::string& title,
               const InputValidator* input_validator,
               const OverrideRegistry& override_registry);
  ~InputBuilder();

  
  
  
  
  
  
  
  
  std::string AskForInput() const;

  
  InputBuilder& WithInputSource(FILE* input_source);
  
  
  InputBuilder& WithInputValidator(const InputValidator* input_validator);
  
  
  InputBuilder& WithDefault(const std::string& default_value);
  
  InputBuilder& WithAdditionalInfo(const std::string& title);

 private:
  const std::string& GetOverride() const;
  std::string ActuallyAskUser() const;

  FILE* input_source_;
  const InputValidator* input_validator_;
  const OverrideRegistry& override_registry_;
  std::string default_value_;
  std::string title_;
  std::string additional_info_;
};



class OverrideRegistry {
 public:
  OverrideRegistry(const std::string& overrides);
  bool HasOverrideFor(const std::string& title) const;
  const std::string& GetOverrideFor(const std::string& title) const;
 private:
  typedef std::map<std::string, std::string> OverrideMap;
  OverrideMap overrides_;
};

class InputValidator {
 public:
  virtual ~InputValidator() {}

  virtual bool InputOk(const std::string& value) const = 0;
};


class IntegerWithinRangeValidator : public InputValidator {
 public:
  IntegerWithinRangeValidator(int low, int high)
      : low_(low), high_(high) {}

  bool InputOk(const std::string& input) const {
    int value = atoi(input.c_str());
    
    if (value == 0 && input.length() > 0 && input[0] != '0')
      return false;  
    return value >= low_ && value <= high_;
  }

 private:
  int low_;
  int high_;
};

std::vector<std::string> Split(const std::string& to_split,
                               const std::string& delimiter);


InputBuilder TypedInput(const std::string& title);

}  

#endif  
