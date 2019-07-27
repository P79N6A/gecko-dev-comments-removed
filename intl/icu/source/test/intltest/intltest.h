









#ifndef _INTLTEST
#define _INTLTEST


#include "unicode/fmtable.h"
#include "unicode/testlog.h"


#if U_NO_DEFAULT_INCLUDE_UTF_HEADERS

#include "unicode/utf_old.h" 
#endif









#ifdef ICU_USE_THREADS
    
#elif defined(APP_NO_THREADS)
    
#   define ICU_USE_THREADS 0
#else
#   define ICU_USE_THREADS 1
#endif

U_NAMESPACE_USE

#if U_PLATFORM == U_PF_OS390


#pragma map(IntlTest::log( const UnicodeString &message ),"logos390")
#endif




UnicodeString UCharToUnicodeString(UChar c);
UnicodeString Int64ToUnicodeString(int64_t num);

UnicodeString operator+(const UnicodeString& left, long num);
UnicodeString operator+(const UnicodeString& left, unsigned long num);
UnicodeString operator+(const UnicodeString& left, double num);
UnicodeString operator+(const UnicodeString& left, char num);
UnicodeString operator+(const UnicodeString& left, short num);
UnicodeString operator+(const UnicodeString& left, int num);
UnicodeString operator+(const UnicodeString& left, unsigned char num);
UnicodeString operator+(const UnicodeString& left, unsigned short num);
UnicodeString operator+(const UnicodeString& left, unsigned int num);
UnicodeString operator+(const UnicodeString& left, float num);
#if !UCONFIG_NO_FORMATTING
UnicodeString toString(const Formattable& f); 
UnicodeString toString(int32_t n);
#endif
UnicodeString toString(UBool b);















#define TESTCASE(id,test)             \
    case id:                          \
        name = #test;                 \
        if (exec) {                   \
            logln(#test "---");       \
            logln();                  \
            test();                   \
        }                             \
        break











#define TESTCASE_AUTO_BEGIN \
    for(;;) { \
        int32_t testCaseAutoNumber = 0

#define TESTCASE_AUTO(test) \
        if (index == testCaseAutoNumber++) { \
            name = #test; \
            if (exec) { \
                logln(#test "---"); \
                logln(); \
                test(); \
            } \
            break; \
        }

#define TESTCASE_AUTO_CLASS(TestClass) \
        if (index == testCaseAutoNumber++) { \
            name = #TestClass; \
            if (exec) { \
                logln(#TestClass "---"); \
                logln(); \
                TestClass test; \
                callTest(test, par); \
            } \
            break; \
        }

#define TESTCASE_AUTO_CREATE_CLASS(TestClass) \
        if (index == testCaseAutoNumber++) { \
            name = #TestClass; \
            if (exec) { \
                logln(#TestClass "---"); \
                logln(); \
                LocalPointer<IntlTest> test(create##TestClass()); \
                callTest(*test, par); \
            } \
            break; \
        }

#define TESTCASE_AUTO_END \
        name = ""; \
        break; \
    }

#define TEST_ASSERT_TRUE(x) \
  assertTrue(#x, (x), FALSE, FALSE, __FILE__, __LINE__)

#define TEST_ASSERT_STATUS(x) \
  assertSuccess(#x, (x), FALSE, __FILE__, __LINE__)

class IntlTest : public TestLog {
public:

    IntlTest();
    

    virtual UBool runTest( char* name = NULL, char* par = NULL, char *baseName = NULL); 

    virtual UBool setVerbose( UBool verbose = TRUE );
    virtual UBool setNoErrMsg( UBool no_err_msg = TRUE );
    virtual UBool setQuick( UBool quick = TRUE );
    virtual UBool setLeaks( UBool leaks = TRUE );
    virtual UBool setNotime( UBool no_time = TRUE );
    virtual UBool setWarnOnMissingData( UBool warn_on_missing_data = TRUE );
    virtual int32_t setThreadCount( int32_t count = 1);

    virtual int32_t getErrors( void );
    virtual int32_t getDataErrors (void );

    virtual void setCaller( IntlTest* callingTest ); 
    virtual void setPath( char* path ); 

    virtual void log( const UnicodeString &message );

    virtual void logln( const UnicodeString &message );

    virtual void logln( void );

    








    UBool logKnownIssue( const char *ticket, const UnicodeString &message );
    







    UBool logKnownIssue( const char *ticket );
    








    UBool logKnownIssue( const char *ticket, const char *fmt, ...);

    virtual void info( const UnicodeString &message );

    virtual void infoln( const UnicodeString &message );

    virtual void infoln( void );

    virtual void err(void);

    virtual void err( const UnicodeString &message );

    virtual void errln( const UnicodeString &message );

    virtual void dataerr( const UnicodeString &message );

    virtual void dataerrln( const UnicodeString &message );

    void errcheckln(UErrorCode status, const UnicodeString &message );

    
    void log(const char *fmt, ...);
    void logln(const char *fmt, ...);
    void info(const char *fmt, ...);
    void infoln(const char *fmt, ...);
    void err(const char *fmt, ...);
    void errln(const char *fmt, ...);
    void dataerr(const char *fmt, ...);
    void dataerrln(const char *fmt, ...);

    





    void errcheckln(UErrorCode status, const char *fmt, ...);

    
    void printErrors(); 

    
    UBool printKnownIssues();
        
    virtual void usage( void ) ;

    








    static float random(int32_t* seedp);

    


    static float random();

    enum { kMaxProps = 16 };

    virtual void setProperty(const char* propline);
    virtual const char* getProperty(const char* prop);

protected:
    
    UBool assertTrue(const char* message, UBool condition, UBool quiet=FALSE, UBool possibleDataError=FALSE, const char *file=NULL, int line=0);
    UBool assertFalse(const char* message, UBool condition, UBool quiet=FALSE);
    



    UBool assertSuccess(const char* message, UErrorCode ec, UBool possibleDataError=FALSE, const char *file=NULL, int line=0);
    UBool assertEquals(const char* message, const UnicodeString& expected,
                       const UnicodeString& actual, UBool possibleDataError=FALSE);
    UBool assertEquals(const char* message, const char* expected,
                       const char* actual);
    UBool assertEquals(const char* message, UBool expected,
                       UBool actual);
    UBool assertEquals(const char* message, int32_t expected, int32_t actual);
    UBool assertEquals(const char* message, int64_t expected, int64_t actual);
#if !UCONFIG_NO_FORMATTING
    UBool assertEquals(const char* message, const Formattable& expected,
                       const Formattable& actual, UBool possibleDataError=FALSE);
    UBool assertEquals(const UnicodeString& message, const Formattable& expected,
                       const Formattable& actual);
#endif
    UBool assertTrue(const UnicodeString& message, UBool condition, UBool quiet=FALSE);
    UBool assertFalse(const UnicodeString& message, UBool condition, UBool quiet=FALSE);
    UBool assertSuccess(const UnicodeString& message, UErrorCode ec);
    UBool assertEquals(const UnicodeString& message, const UnicodeString& expected,
                       const UnicodeString& actual, UBool possibleDataError=FALSE);
    UBool assertEquals(const UnicodeString& message, const char* expected,
                       const char* actual);
    UBool assertEquals(const UnicodeString& message, UBool expected, UBool actual);
    UBool assertEquals(const UnicodeString& message, int32_t expected, int32_t actual);
    UBool assertEquals(const UnicodeString& message, int64_t expected, int64_t actual);

    virtual void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL ); 

    virtual UBool runTestLoop( char* testname, char* par, char *baseName );

    virtual int32_t IncErrorCount( void );

    virtual int32_t IncDataErrorCount( void );

    virtual UBool callTest( IntlTest& testToBeCalled, char* par );


    UBool       verbose;
    UBool       no_err_msg;
    UBool       quick;
    UBool       leaks;
    UBool       warn_on_missing_data;
    UBool       no_time;
    int32_t     threadCount;

private:
    UBool       LL_linestart;
    int32_t     LL_indentlevel;

    int32_t     errorCount;
    int32_t     dataErrorCount;
    IntlTest*   caller;
    char*       testPath;           
    
    char basePath[1024];
    char currName[1024]; 

    
    void *testoutfp;

    const char* proplines[kMaxProps];
    int32_t     numProps;

protected:

    virtual void LL_message( UnicodeString message, UBool newline );

    

    static UnicodeString &prettify(const UnicodeString &source, UnicodeString &target);
    static UnicodeString prettify(const UnicodeString &source, UBool parseBackslash=FALSE);
    
    static UnicodeString &appendHex(uint32_t number, int32_t digits, UnicodeString &target);
    static UnicodeString toHex(uint32_t number, int32_t digits=-1);
    static inline UnicodeString toHex(int32_t number, int32_t digits=-1) {
        return toHex((uint32_t)number, digits);
    }

public:
    static void setICU_DATA();       

    static const char* pathToDataDirectory();

public:
    UBool run_phase2( char* name, char* par ); 
    static const char* loadTestData(UErrorCode& err);
    virtual const char* getTestDataPath(UErrorCode& err);
    static const char* getSourceTestData(UErrorCode& err);
    static char *getUnidataPath(char path[]);


public:
    static IntlTest* gTest;
    static const char* fgDataDir;

};

void it_log( UnicodeString message );
void it_logln( UnicodeString message );
void it_logln( void );
void it_info( UnicodeString message );
void it_infoln( UnicodeString message );
void it_infoln( void );
void it_err(void);
void it_err( UnicodeString message );
void it_errln( UnicodeString message );
void it_dataerr( UnicodeString message );
void it_dataerrln( UnicodeString message );






extern UnicodeString CharsToUnicodeString(const char* chars);


extern UnicodeString ctou(const char* chars);

#endif 
