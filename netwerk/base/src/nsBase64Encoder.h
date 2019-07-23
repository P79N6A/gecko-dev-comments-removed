




































#ifndef NSBASE64ENCODER_H_
#define NSBASE64ENCODER_H_

#include "nsIOutputStream.h"
#include "nsString.h"





class nsBase64Encoder : public nsIOutputStream {
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
