









#ifndef WEBRTC_TOOLS_SIMPLE_COMMAND_LINE_PARSER_H_
#define WEBRTC_TOOLS_SIMPLE_COMMAND_LINE_PARSER_H_

#include <string>
#include <map>
#include <vector>





namespace webrtc {
namespace test {

class CommandLineParser {
 public:
  CommandLineParser() {}
  ~CommandLineParser() {}

  void Init(int argc, char** argv);

  
  void PrintEnteredFlags();

  
  
  
  void ProcessFlags();

  
  void SetUsageMessage(std::string usage_message);

  
  void PrintUsageMessage();

  
  void SetFlag(std::string flag_name, std::string flag_value);

  
  std::string GetFlag(std::string flag_name);

 private:
  
  std::vector<std::string> args_;
  
  std::map<std::string, std::string> flags_;
  
  std::string usage_message_;

  
  
  bool IsStandaloneFlag(std::string flag);

  
  bool IsFlagWellFormed(std::string flag);

  
  std::string GetCommandLineFlagName(std::string flag);

  
  std::string GetCommandLineFlagValue(std::string flag);
};

}  
}  

#endif  
