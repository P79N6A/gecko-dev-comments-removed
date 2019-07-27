









#ifndef TRANSAPI_H
#define TRANSAPI_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "intltest.h"





class TransliteratorAPITest : public IntlTest {
public:
    void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par=NULL);

    
    void TestGetDisplayName(void);

    void TestgetID(void);

    void TestgetInverse(void);

    void TestClone(void);

    void TestTransliterate1(void);

    void TestTransliterate2(void);

    void TestTransliterate3(void);

    void TestSimpleKeyboardTransliterator(void);

    void TestKeyboardTransliterator1(void);

    void TestKeyboardTransliterator2(void);

    void TestKeyboardTransliterator3(void);

    void TestGetAdoptFilter(void);

    void TestNullTransliterator(void);

    void TestRegisterUnregister(void);

    void TestLatinDevanagari(void);
    
    void TestDevanagariLatinRT(void);

    void TestUnicodeFunctor(void);

    
    void doTest(const UnicodeString& , const UnicodeString& , const UnicodeString& );

    void keyboardAux(Transliterator*, UnicodeString[] , UnicodeString&, int32_t, int32_t);

    void displayOutput(const UnicodeString&, const UnicodeString&, UnicodeString&,
                       UTransPosition&);

    void callEverything(const Transliterator *t, int line);

};

#endif 

#endif
