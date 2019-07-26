






#ifndef __INPUTEXT_H
#define __INPUTEXT_H










#include "unicode/uobject.h"

#if !UCONFIG_NO_CONVERSION

U_NAMESPACE_BEGIN 

class InputText : public UMemory
{
    
    InputText(const InputText &);
public:
    InputText(UErrorCode &status);
    ~InputText();

    void setText(const char *in, int32_t len);
    void setDeclaredEncoding(const char *encoding, int32_t len);
    UBool isSet() const; 
    void MungeInput(UBool fStripTags);

    
    
    uint8_t    *fInputBytes;
    int32_t     fInputLen;          
    
    
    
    int16_t  *fByteStats;
    UBool     fC1Bytes;          
    char     *fDeclaredEncoding;

    const uint8_t           *fRawInput;     
    
    
    
    int32_t                  fRawLength;    

};

U_NAMESPACE_END

#endif
#endif 
