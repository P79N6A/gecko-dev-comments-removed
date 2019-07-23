



































#ifndef ojiapitests_h___
#define ojiapitests_h___


#include "nspr.h"
#include "nsString.h"

#define OJI_TEST_RESULTS "OJITestResults.txt"
#define OJI_TESTS_LIST   "OJITests.lst"
#ifdef XP_UNIX
#define OJI_JNI_TESTS   "libojiapijnitests.so"
#define OJI_JM_TESTS    "libojiapijmtests.so"
#define OJI_TM_TESTS    "libojiapitmtests.so"
#define OJI_LCM_TESTS   "libojiapilcmtests.so"
#define OJI_JMPTI_TESTS "libojiapijmptitests.so"
#else
#define OJI_JNI_TESTS   "ojiapijnitests.dll"
#define OJI_JM_TESTS    "ojiapijmtests.dll"
#define OJI_TM_TESTS    "ojiapitmtests.dll"
#define OJI_LCM_TESTS   "ojiapilcmtests.dll"
#define OJI_JMPTI_TESTS "ojiapijmptitests.dll"
#endif


class TestResult;

typedef TestResult* (*TYPEOF_OJIAPITest)();


typedef TestResult* (*TYPEOF_JNI_OJIAPITest)();
typedef TestResult* (*TYPEOF_JM_OJIAPITest)();
typedef TestResult* (*TYPEOF_LCM_OJIAPITest)();
typedef TestResult* (*TYPEOF_TM_OJIAPITest)();


#define JNI_OJIAPITest(TestName)             \
extern "C" NS_EXPORT TestResult*             \
TestName()

#define TM_OJIAPITest(TestName)             \
extern "C" NS_EXPORT TestResult*             \
TestName()

#define JM_OJIAPITest(TestName)             \
extern "C" NS_EXPORT TestResult*             \
TestName()

#define LCM_OJIAPITest(TestName)             \
extern "C" NS_EXPORT TestResult*             \
TestName()

#define GET_JNI_FOR_TEST     \
JNIEnv* env;                 \
if (NS_FAILED(GetJNI(&env)) || !env) \
	return TestResult::FAIL("Can't get JNIEnv !");

#define GET_TM_FOR_TEST                      \
nsIThreadManager* threadMgr;                 \
if (NS_FAILED(GetThreadManager(&threadMgr)) || !threadMgr) \
	return TestResult::FAIL("Can't get TM !");


#define GET_JM_FOR_TEST                      \
nsIJVMManager* jvmMgr;                       \
if (NS_FAILED(GetJVMManager(&jvmMgr)) || !jvmMgr)       \
	return TestResult::FAIL("Can't get JNIEnv !");

#define GET_LCM_FOR_TEST                      \
nsILiveConnectManager* lcMgr;                 \
if (NS_FAILED(GetLiveConnectManager(&lcMgr)) || !lcMgr) \
	return TestResult::FAIL("Can't get JNIEnv !");


enum StatusValue {
  FAIL_value = 0,
  PASS_value = 1
};

class TestResult {
public:
  







  static TestResult* PASS(char *comment) {
    char *msg = (char*)calloc(1, PL_strlen(comment)+1024);
    sprintf(msg, "Method %s", comment);
    CBufDescriptor *bufDescr = new CBufDescriptor((const char*)msg, PR_FALSE, PL_strlen(msg)+1, PL_strlen(msg));
    nsString *str = new nsAutoString(*bufDescr);
    return new TestResult(PASS_value, *str); 
  }

  static TestResult* FAIL(nsString comment) {
    return new TestResult(FAIL_value, comment); 
  }

  static TestResult* FAIL(char *method, nsresult rc) {
    char *comment = (char*)calloc(1, PL_strlen(method)+1024);
    sprintf(comment, "Method %s returned %X", method, rc);
    CBufDescriptor *bufDescr = new CBufDescriptor((const char*)comment, PR_FALSE, PL_strlen(comment)+1, PL_strlen(comment));
    nsString *str = new nsAutoString(*bufDescr);
    return new TestResult(FAIL_value, *str); 
  }

  static TestResult* FAIL(char *method, char* comment, nsresult rc) {
    char *outComment = (char*)calloc(1, PL_strlen(method)+1024);
    sprintf(outComment, "Method %s returned %X: %s", method, rc, comment);
    CBufDescriptor *bufDescr = new CBufDescriptor((const char*)comment, PR_FALSE, PL_strlen(comment)+1, PL_strlen(comment));
    nsString *str = new nsAutoString(*bufDescr);
    return new TestResult(FAIL_value, *str); 
  }

  static TestResult* FAIL(char *comment) {
      CBufDescriptor *bufDescr = new CBufDescriptor((const char*)comment, PR_FALSE, PL_strlen(comment)+1, PL_strlen(comment));
      nsString *str = new nsAutoString(*bufDescr);
      return new TestResult(FAIL_value, *str); 
  }

  TestResult ( StatusValue status, nsString comment) {
    this->status = status;
    this->comment = comment;
  }

  StatusValue status;
  nsString comment;

};

#endif 
