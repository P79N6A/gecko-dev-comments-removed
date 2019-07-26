









#ifndef WEBRTC_TOOLS_SIMPLE_COMMAND_LINE_PARSER_H_
#define WEBRTC_TOOLS_SIMPLE_COMMAND_LINE_PARSER_H_

#include <map>
#include <string>
#include <vector>

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/test/testsupport/gtest_prod_util.h"












namespace webrtc {
namespace test {

class CommandLineParser {
 public:
  CommandLineParser();
  ~CommandLineParser();

  void Init(int argc, char** argv);

  
  void PrintEnteredFlags();

  
  
  
  void ProcessFlags();

  
  void SetUsageMessage(std::string usage_message);

  
  void PrintUsageMessage();

  
  
  
  void SetFlag(std::string flag_name, std::string default_flag_value);

  
  
  
  std::string GetFlag(std::string flag_name);

 private:
  
  std::vector<std::string> args_;
  
  std::map<std::string, std::string> flags_;
  
  std::string usage_message_;

  
  
  bool IsStandaloneFlag(std::string flag);

  
  
  bool IsFlagWellFormed(std::string flag);

  
  std::string GetCommandLineFlagName(std::string flag);

  
  
  std::string GetCommandLineFlagValue(std::string flag);

  FRIEND_TEST_ALL_PREFIXES(CommandLineParserTest, IsStandaloneFlag);
  FRIEND_TEST_ALL_PREFIXES(CommandLineParserTest, IsFlagWellFormed);
  FRIEND_TEST_ALL_PREFIXES(CommandLineParserTest, GetCommandLineFlagName);
  FRIEND_TEST_ALL_PREFIXES(CommandLineParserTest, GetCommandLineFlagValue);

  DISALLOW_COPY_AND_ASSIGN(CommandLineParser);
};

}  
}  

#endif  
