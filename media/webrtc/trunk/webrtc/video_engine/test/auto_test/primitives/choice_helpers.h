









#ifndef WEBRTC_VIDEO_ENGINE_TEST_AUTO_TEST_PRIMITIVES_CHOICE_HELPERS_H_
#define WEBRTC_VIDEO_ENGINE_TEST_AUTO_TEST_PRIMITIVES_CHOICE_HELPERS_H_

#include <string>
#include <vector>

#include "video_engine/test/auto_test/primitives/input_helpers.h"

namespace webrtc {

typedef std::vector<std::string> Choices;

















class ChoiceBuilder {
 public:
  explicit ChoiceBuilder(const std::string& title, const Choices& choices);

  
  
  
  ChoiceBuilder& WithDefault(const std::string& default_choice);

  
  ChoiceBuilder& WithInputSource(FILE* input_source);

  
  
  int Choose();
 private:
  std::string MakeHumanReadableOptions();

  Choices choices_;
  InputBuilder input_helper_;
};



ChoiceBuilder FromChoices(const std::string& title,
                          const std::string& raw_choices);


Choices SplitChoices(const std::string& raw_choices);

}  

#endif  
