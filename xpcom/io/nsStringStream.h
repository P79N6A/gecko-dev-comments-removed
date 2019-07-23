





































#ifndef nsStringStream_h__
#define nsStringStream_h__

#include "nsIStringStream.h"
#include "nsStringGlue.h"
#include "nsMemory.h"








#define NS_STRINGINPUTSTREAM_CLASSNAME  "nsStringInputStream"
#define NS_STRINGINPUTSTREAM_CONTRACTID "@mozilla.org/io/string-input-stream;1"
#define NS_STRINGINPUTSTREAM_CID                     \
{ /* 0abb0835-5000-4790-af28-61b3ba17c295 */         \
    0x0abb0835,                                      \
    0x5000,                                          \
    0x4790,                                          \
    {0xaf, 0x28, 0x61, 0xb3, 0xba, 0x17, 0xc2, 0x95} \
}





















extern NS_COM nsresult
NS_NewByteInputStream(nsIInputStream** aStreamResult,
                      const char* aStringToRead, PRInt32 aLength = -1,
                      nsAssignmentType aAssignment = NS_ASSIGNMENT_DEPEND);









extern NS_COM nsresult
NS_NewStringInputStream(nsIInputStream** aStreamResult,
                        const nsAString& aStringToRead);





extern NS_COM nsresult
NS_NewCStringInputStream(nsIInputStream** aStreamResult,
                         const nsACString& aStringToRead);

#endif 
