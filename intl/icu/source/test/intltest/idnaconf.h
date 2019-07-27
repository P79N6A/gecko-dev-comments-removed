











#ifndef IDNA_CONF_TEST_H
#define IDNA_CONF_TEST_H

#include "intltest.h"
#include "unicode/ustring.h"


class IdnaConfTest: public IntlTest {
public:
    void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par=NULL);
    IdnaConfTest();
    virtual ~IdnaConfTest();
private:
    void Test(void);

    
    UChar* base;
    int len ;
    int curOffset;

    UBool  ReadAndConvertFile();
    int isNewlineMark();
    UBool ReadOneLine(UnicodeString&);

    
    UnicodeString id;   
    UnicodeString namebase;
    UnicodeString namezone;
    int type;     
    int option;   
    int passfail; 

    void ExplainCodePointTag(UnicodeString& buf);
    void Call();
};

#endif 
