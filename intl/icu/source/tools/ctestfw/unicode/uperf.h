





#ifndef _UPERF_H
#define _UPERF_H

#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/ustring.h"

#include "unicode/testtype.h"
#include "unicode/utimer.h"
#include "ucbuf.h"


struct UOption;
typedef struct UOption UOption;

#if !UCONFIG_NO_CONVERSION

U_NAMESPACE_USE














#if 0
#define TESTCASE(id,test)                       \
    case id:                                    \
        name = #test;                           \
        if (exec) {                             \
            fprintf(stdout,#test "---");        \
            fprintf(stdout,"\n");               \
            return test();                      \
        }                                       \
        break

#endif
#define TESTCASE(id,test)                       \
    case id:                                    \
        name = #test;                           \
        if (exec) {                             \
            return test();                      \
        }                                       \
        break







class T_CTEST_EXPORT_API UPerfFunction {
public:
    


    virtual ~UPerfFunction();

    



    virtual void call(UErrorCode* status)=0;

    




    virtual long getOperationsPerIteration()=0;
    





    virtual long getEventsPerIteration(){
        return -1;
    }
    





     virtual double time(int32_t n, UErrorCode* status) {
        UTimer start, stop;
        utimer_getTime(&start); 
        while (n-- > 0) {
            call(status);
        }
        utimer_getTime(&stop);
        return utimer_getDeltaSeconds(&start,&stop); 
    }

};


class T_CTEST_EXPORT_API UPerfTest {
public:
    UBool run();
    UBool runTest( char* name = NULL, char* par = NULL ); 
        
    virtual void usage( void ) ;
    
    virtual ~UPerfTest();

    void setCaller( UPerfTest* callingTest ); 
    
    void setPath( char* path ); 
    
    ULine* getLines(UErrorCode& status);

    const UChar* getBuffer(int32_t& len,UErrorCode& status);

protected:
    UPerfTest(int32_t argc, const char* argv[], UErrorCode& status);

    UPerfTest(int32_t argc, const char* argv[],
              UOption addOptions[], int32_t addOptionsCount,
              const char *addUsage,
              UErrorCode& status);

    void init(UOption addOptions[], int32_t addOptionsCount,
              UErrorCode& status);

    virtual UPerfFunction* runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL ); 

    virtual UBool runTestLoop( char* testname, char* par );

    virtual UBool callTest( UPerfTest& testToBeCalled, char* par );

    int32_t      _argc;
    const char** _argv;
    const char * _addUsage;
    char*        resolvedFileName;
    UCHARBUF*    ucharBuf;
    const char*  encoding;
    UBool        uselen;
    const char*  fileName;
    const char*  sourceDir;
    int32_t      _remainingArgc;
    ULine*       lines;
    int32_t      numLines;
    UBool        line_mode;
    UChar* buffer;
    int32_t      bufferLen;
    UBool        verbose;
    UBool        bulk_mode;
    int32_t      passes;
    int32_t      iterations;
    int32_t      time;
    const char*  locale;
private:
    UPerfTest*   caller;
    char*        path;           


public:
    static UPerfTest* gTest;
    static const char gUsageString[];
};

#endif
#endif

