







#include "dtfmtrtperf.h"
#include "uoptions.h"
#include <stdio.h>

#include <iostream>
using namespace std;

DateTimeRoundTripPerfTest::DateTimeRoundTripPerfTest(int32_t argc, const char* argv[], UErrorCode& status)
: UPerfTest(argc,argv,status) { }

DateTimeRoundTripPerfTest::~DateTimeRoundTripPerfTest() { }

UPerfFunction* DateTimeRoundTripPerfTest::runIndexedTest(int32_t index, UBool exec,const char* &name, char* par) {

    switch (index) 
    {
        TESTCASE(0,RoundTripLocale1);     
        TESTCASE(1,RoundTripLocale10);    
        TESTCASE(2,RoundTripLocale11);    
        TESTCASE(3,RoundTripLocale21);    
        default: 
            name = ""; 
            return NULL;
    }
    return NULL;

}

UPerfFunction* DateTimeRoundTripPerfTest::RoundTripLocale1(){
    DateTimeRoundTripFunction* func= new DateTimeRoundTripFunction(1);
    return func;
}

UPerfFunction* DateTimeRoundTripPerfTest::RoundTripLocale10(){
    DateTimeRoundTripFunction* func= new DateTimeRoundTripFunction(10);
    return func;
}

UPerfFunction* DateTimeRoundTripPerfTest::RoundTripLocale11(){
    DateTimeRoundTripFunction* func= new DateTimeRoundTripFunction(11);
    return func;
}

UPerfFunction* DateTimeRoundTripPerfTest::RoundTripLocale21(){
    DateTimeRoundTripFunction* func= new DateTimeRoundTripFunction(21);
    return func;
}

int main(int argc, const char* argv[]){

	cout << "ICU version - " << U_ICU_VERSION << endl;

    UErrorCode status = U_ZERO_ERROR;
    DateTimeRoundTripPerfTest test(argc, argv, status);
    if(U_FAILURE(status)){
		cout << "initialization failed! " << status << endl;
        return status;
    }

    if(test.run()==FALSE){
		cout << "run failed!" << endl;
        fprintf(stderr,"FAILED: Tests could not be run please check the arguments.\n");
        return -1;
    }

	cout << "done!" << endl;
    return 0;
}