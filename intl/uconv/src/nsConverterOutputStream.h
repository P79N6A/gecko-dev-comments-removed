




































#ifndef NSCONVERTEROUTPUTSTREAM_H_
#define NSCONVERTEROUTPUTSTREAM_H_

#include "nsIOutputStream.h"
#include "nsIConverterOutputStream.h"
#include "nsCOMPtr.h"

class nsIUnicodeEncoder;
class nsIOutputStream;


#define NS_CONVERTEROUTPUTSTREAM_CID \
{ 0xff8780a5, 0xbbb1, 0x4bc5, \
  { 0x8e, 0xe7, 0x05, 0x7e, 0x7b, 0xc5, 0xc9, 0x25 } }

class nsConverterOutputStream : public nsIConverterOutputStream {
    public:
        nsConverterOutputStream() {}

        NS_DECL_ISUPPORTS
        NS_DECL_NSIUNICHAROUTPUTSTREAM
        NS_DECL_NSICONVERTEROUTPUTSTREAM

    private:
        ~nsConverterOutputStream();

        nsCOMPtr<nsIUnicodeEncoder> mConverter;
        nsCOMPtr<nsIOutputStream>   mOutStream;
};

#endif
