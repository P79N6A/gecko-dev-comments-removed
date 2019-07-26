









#ifndef U_TESTFW_TESTMODULE
#define U_TESTFW_TESTMODULE

#include "unicode/unistr.h"
#include "unicode/ures.h"
#include "unicode/testtype.h"
#include "unicode/testdata.h"
#include "unicode/datamap.h"
#include "unicode/testlog.h"







class DataMap;
class TestData;






class T_CTEST_EXPORT_API TestDataModule {
  const char* testName;

protected:
  DataMap *fInfo;
  TestLog& fLog;

public:
  





  static TestDataModule *getTestDataModule(const char* name, TestLog& log, UErrorCode &status);
  virtual ~TestDataModule();

protected:
  TestDataModule(const char* name, TestLog& log, UErrorCode& status);

public:
  


  const char * getName() const;

  





  virtual UBool getInfo(const DataMap *& info, UErrorCode &status) const = 0;

  





  virtual TestData* createTestData(int32_t index, UErrorCode &status) const = 0;

  




  virtual TestData* createTestData(const char* name, UErrorCode &status) const = 0;
};

class T_CTEST_EXPORT_API RBTestDataModule : public TestDataModule {
public:
  virtual ~RBTestDataModule();

public:
  RBTestDataModule(const char* name, TestLog& log, UErrorCode& status);

public:
  virtual UBool getInfo(const DataMap *& info, UErrorCode &status) const;

  virtual TestData* createTestData(int32_t index, UErrorCode &status) const;
  virtual TestData* createTestData(const char* name, UErrorCode &status) const;

private:
  UResourceBundle *getTestBundle(const char* bundleName, UErrorCode &status);

private:
  UResourceBundle *fModuleBundle;
  UResourceBundle *fTestData;
  UResourceBundle *fInfoRB;
  UBool fDataTestValid;
  char *tdpath;

 
  int32_t fNumberOfTests;

};


#endif

