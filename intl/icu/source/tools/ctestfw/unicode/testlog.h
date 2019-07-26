









#ifndef U_TESTFW_TESTLOG
#define U_TESTFW_TESTLOG

#include "unicode/errorcode.h"
#include "unicode/unistr.h"
#include "unicode/testtype.h"





class T_CTEST_EXPORT_API TestLog {
public:
    virtual ~TestLog();
    virtual void errln( const UnicodeString &message ) = 0;
    virtual void logln( const UnicodeString &message ) = 0;
    virtual void dataerrln( const UnicodeString &message ) = 0;
    virtual const char* getTestDataPath(UErrorCode& err) = 0;
};

class T_CTEST_EXPORT_API IcuTestErrorCode : public ErrorCode {
public:
    IcuTestErrorCode(TestLog &callingTestClass, const char *callingTestName) :
        testClass(callingTestClass), testName(callingTestName) {}
    virtual ~IcuTestErrorCode();
    
    UBool logIfFailureAndReset(const char *fmt, ...);
    UBool logDataIfFailureAndReset(const char *fmt, ...);
protected:
    virtual void handleFailure() const;
private:
    TestLog &testClass;
    const char *const testName;
};

#endif
