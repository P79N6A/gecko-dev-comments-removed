











#ifndef CHROME_COMMON_JSTEMPLATE_BUILDER_H_
#define CHROME_COMMON_JSTEMPLATE_BUILDER_H_

#include <string>

#include "base/values.h"

class StringPiece;

namespace jstemplate_builder {
  
  
  
  std::string GetTemplateHtml(const StringPiece& html_template,
                              const DictionaryValue* json,
                              const StringPiece& template_id);
}  
#endif  
