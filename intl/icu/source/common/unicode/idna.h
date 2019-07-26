













#ifndef __IDNA_H__
#define __IDNA_H__






#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

#include "unicode/bytestream.h"
#include "unicode/stringpiece.h"
#include "unicode/uidna.h"
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

class IDNAInfo;













class U_COMMON_API IDNA : public UObject {
public:
    



    ~IDNA();

    
































    static IDNA *
    createUTS46Instance(uint32_t options, UErrorCode &errorCode);

    



















    virtual UnicodeString &
    labelToASCII(const UnicodeString &label, UnicodeString &dest,
                 IDNAInfo &info, UErrorCode &errorCode) const = 0;

    

















    virtual UnicodeString &
    labelToUnicode(const UnicodeString &label, UnicodeString &dest,
                   IDNAInfo &info, UErrorCode &errorCode) const = 0;

    



















    virtual UnicodeString &
    nameToASCII(const UnicodeString &name, UnicodeString &dest,
                IDNAInfo &info, UErrorCode &errorCode) const = 0;

    

















    virtual UnicodeString &
    nameToUnicode(const UnicodeString &name, UnicodeString &dest,
                  IDNAInfo &info, UErrorCode &errorCode) const = 0;

    

    













    virtual void
    labelToASCII_UTF8(const StringPiece &label, ByteSink &dest,
                      IDNAInfo &info, UErrorCode &errorCode) const;

    













    virtual void
    labelToUnicodeUTF8(const StringPiece &label, ByteSink &dest,
                       IDNAInfo &info, UErrorCode &errorCode) const;

    













    virtual void
    nameToASCII_UTF8(const StringPiece &name, ByteSink &dest,
                     IDNAInfo &info, UErrorCode &errorCode) const;

    













    virtual void
    nameToUnicodeUTF8(const StringPiece &name, ByteSink &dest,
                      IDNAInfo &info, UErrorCode &errorCode) const;

private:
    
    virtual UClassID getDynamicClassID() const;
};

class UTS46;






class U_COMMON_API IDNAInfo : public UMemory {
public:
    



    IDNAInfo() : errors(0), labelErrors(0), isTransDiff(FALSE), isBiDi(FALSE), isOkBiDi(TRUE) {}
    




    UBool hasErrors() const { return errors!=0; }
    





    uint32_t getErrors() const { return errors; }
    












    UBool isTransitionalDifferent() const { return isTransDiff; }

private:
    friend class UTS46;

    IDNAInfo(const IDNAInfo &other);  
    IDNAInfo &operator=(const IDNAInfo &other);  

    void reset() {
        errors=labelErrors=0;
        isTransDiff=FALSE;
        isBiDi=FALSE;
        isOkBiDi=TRUE;
    }

    uint32_t errors, labelErrors;
    UBool isTransDiff;
    UBool isBiDi;
    UBool isOkBiDi;
};

U_NAMESPACE_END

#endif  
#endif  
