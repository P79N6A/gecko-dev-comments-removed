






#include "chrome/common/jstemplate_builder.h"

#include "app/resource_bundle.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "chrome/common/json_value_serializer.h"

#include "grit/common_resources.h"

namespace jstemplate_builder {

std::string GetTemplateHtml(const StringPiece& html_template,
                            const DictionaryValue* json,
                            const StringPiece& template_id) {
  
  static const StringPiece jstemplate_src(
    ResourceBundle::GetSharedInstance().GetRawDataResource(IDR_JSTEMPLATE_JS));

  if (jstemplate_src.empty()) {
    NOTREACHED() << "Unable to get jstemplate src";
    return std::string();
  }

  
  DCHECK(json) << "must include json data structure";

  std::string jstext;
  JSONStringValueSerializer serializer(&jstext);
  serializer.Serialize(*json);
  
  
  ReplaceSubstringsAfterOffset(&jstext, 0, "</", "<\\/");

  std::string output(html_template.data(), html_template.size());
  output.append("<script>");
  output.append(jstemplate_src.data(), jstemplate_src.size());
  output.append("var tp = document.getElementById('");
  output.append(template_id.data(), template_id.size());
  output.append("'); var cx = new JsExprContext(");
  output.append(jstext);
  output.append("); jstProcess(cx, tp);</script>");

  return output;
}

}  
