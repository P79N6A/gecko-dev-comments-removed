









#ifndef ALPHAINDEXTST_H
#define ALPHAINDEXTST_H

#include "intltest.h"

class AlphabeticIndexTest: public IntlTest {
public:
    AlphabeticIndexTest();
    virtual ~AlphabeticIndexTest();

    virtual void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL );

    virtual void APITest();
    virtual void ManyLocalesTest();
    virtual void HackPinyinTest();
    virtual void TestBug9009();
    void TestIndexCharactersList();
    


    void TestHaniFirst();
    


    void TestPinyinFirst();
    


    void TestSchSt();
    


    void TestNoLabels();
    


    void TestChineseZhuyin();
    void TestJapaneseKanji();
    void TestChineseUnihan();
};

#endif
