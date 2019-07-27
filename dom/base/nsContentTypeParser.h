




#ifndef nsContentTypeParser_h
#define nsContentTypeParser_h

#include "nsAString.h"

class nsIMIMEHeaderParam;

class nsContentTypeParser {
public:
  explicit nsContentTypeParser(const nsAString& aString);
  ~nsContentTypeParser();

  nsresult GetParameter(const char* aParameterName, nsAString& aResult);
  nsresult GetType(nsAString& aResult)
  {
    return GetParameter(nullptr, aResult);
  }

private:
  NS_ConvertUTF16toUTF8 mString;
  nsIMIMEHeaderParam*   mService;
};

#endif

