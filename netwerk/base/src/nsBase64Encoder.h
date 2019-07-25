



#ifndef NSBASE64ENCODER_H_
#define NSBASE64ENCODER_H_

#include "nsIOutputStream.h"
#include "nsString.h"
#include "mozilla/Attributes.h"





class nsBase64Encoder MOZ_FINAL : public nsIOutputStream {
  public:
    nsBase64Encoder() {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOUTPUTSTREAM

    nsresult Finish(nsCSubstring& _result);
  private:
    ~nsBase64Encoder() {}

    
    
    nsCString mData;
};

#endif
