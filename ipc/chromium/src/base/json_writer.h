



#ifndef BASE_JSON_WRITER_H_
#define BASE_JSON_WRITER_H_

#include <string>

#include "base/basictypes.h"

class Value;

class JSONWriter {
 public:
  
  
  
  
  
  
  
  static void Write(const Value* const node, bool pretty_print,
                    std::string* json);

 private:
  JSONWriter(bool pretty_print, std::string* json);

  
  
  void BuildJSONString(const Value* const node, int depth);

  
  void AppendQuotedString(const std::wstring& str);

  
  void IndentLine(int depth);

  
  std::string* json_string_;

  bool pretty_print_;

  DISALLOW_COPY_AND_ASSIGN(JSONWriter);
};

#endif  
