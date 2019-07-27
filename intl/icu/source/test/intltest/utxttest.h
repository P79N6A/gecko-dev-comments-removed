










#ifndef UTXTTEST_H
#define UTXTTEST_H

#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/utext.h"

#include "intltest.h"





class UTextTest : public IntlTest {
public:
    UTextTest();
    virtual ~UTextTest();

    void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par=NULL);
    void TextTest();
    void ErrorTest();
    void FreezeTest();
    void Ticket5560();
    void Ticket6847();
    void Ticket10562();
    void Ticket10983();

private:
    struct m {                              
        int         nativeIdx;
        UChar32     cp;
    };

    void TestString(const UnicodeString &s);
    void TestAccess(const UnicodeString &us, UText *ut, int cpCount, m *cpMap);
    void TestAccessNoClone(const UnicodeString &us, UText *ut, int cpCount, m *cpMap);
    void TestCMR   (const UnicodeString &us, UText *ut, int cpCount, m *nativeMap, m *utf16Map);
    void TestCopyMove(const UnicodeString &us, UText *ut, UBool move,
                      int32_t nativeStart, int32_t nativeLimit, int32_t nativeDest,
                      int32_t u16Start, int32_t u16Limit, int32_t u16Dest);
    void TestReplace(const UnicodeString &us,  
            UText         *ut,                 
            int32_t       nativeStart,         
            int32_t       nativeLimit,
            int32_t       u16Start,            
            int32_t       u16Limit,            
            const UnicodeString &repStr);      


};


#endif
