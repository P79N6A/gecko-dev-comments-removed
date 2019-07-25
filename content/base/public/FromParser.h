



































#ifndef mozilla_dom_FromParser_h
#define mozilla_dom_FromParser_h

namespace mozilla {
namespace dom {




enum FromParser {
  NOT_FROM_PARSER = 0,
  FROM_PARSER_NETWORK = 1,
  FROM_PARSER_DOCUMENT_WRITE = 1 << 1,
  FROM_PARSER_FRAGMENT = 1 << 2,
  FROM_PARSER_XSLT = 1 << 3
};

} 
} 

#endif 
