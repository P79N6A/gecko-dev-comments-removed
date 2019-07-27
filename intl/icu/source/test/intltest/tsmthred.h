






#ifndef MULTITHREADTEST_H
#define MULTITHREADTEST_H

#include "intltest.h"
#include "mutex.h"






class MultithreadTest : public IntlTest
{
public:
    MultithreadTest();
    virtual ~MultithreadTest();
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    


    void TestThreads(void);

	


    void TestArabicShapingThreads(void);
	
    


    void TestMutex(void);
#if !UCONFIG_NO_FORMATTING
    


    void TestThreadedIntl(void);
#endif
    void TestCollators(void);
    void TestString();
    void TestAnyTranslit();
    void TestConditionVariables();
    void TestUnifiedCache();

};

#endif

