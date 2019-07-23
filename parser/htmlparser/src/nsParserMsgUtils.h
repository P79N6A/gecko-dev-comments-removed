




































#ifndef nsParserMsgUtils_h
#define nsParserMsgUtils_h

#include "nsString.h"

#define XMLPARSER_PROPERTIES "chrome://global/locale/layout/xmlparser.properties"

class nsParserMsgUtils {
  nsParserMsgUtils();  
  ~nsParserMsgUtils(); 
public:
  static nsresult GetLocalizedStringByName(const char * aPropFileName, const char* aKey, nsString& aVal);
  static nsresult GetLocalizedStringByID(const char * aPropFileName, PRUint32 aID, nsString& aVal);
};

#endif
