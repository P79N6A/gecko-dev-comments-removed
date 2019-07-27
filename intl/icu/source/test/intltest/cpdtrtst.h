











#ifndef CPDTRTST_H
#define CPDTRTST_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "intltest.h"





class CompoundTransliteratorTest : public IntlTest {
public:
    void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par=NULL);

    
    void TestConstruction(void);
    
    void TestCloneEqual(void);
    
    void TestGetCount(void);
    
    void TestGetSetAdoptTransliterator(void);
    
    void TestTransliterate(void);

    
    
    

    


    UnicodeString* split(const UnicodeString& str, UChar seperator, int32_t& count);

    void expect(const CompoundTransliterator& t,
                const UnicodeString& source,
                const UnicodeString& expectedResult);

    void expectAux(const UnicodeString& tag,
                   const UnicodeString& summary, UBool pass,
                   const UnicodeString& expectedResult);


};

#endif 

#endif
