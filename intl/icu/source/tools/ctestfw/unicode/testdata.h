









#ifndef U_TESTFW_TESTDATA
#define U_TESTFW_TESTDATA

#include "unicode/tstdtmod.h"
#include "unicode/datamap.h"


 



















class T_CTEST_EXPORT_API TestData {
  const char* name;

protected:
  DataMap *fInfo;
  DataMap *fCurrSettings;
  DataMap *fCurrCase;
  int32_t fSettingsSize;
  int32_t fCasesSize;
  int32_t fCurrentSettings;
  int32_t fCurrentCase;
  
  TestData(const char* name);

public:
  virtual ~TestData();

  const char* getName() const;

  




  virtual UBool getInfo(const DataMap *& info, UErrorCode &status) const = 0;

  







  virtual UBool nextSettings(const DataMap *& settings, UErrorCode &status) = 0;

  







  virtual UBool nextCase(const DataMap *& data, UErrorCode &status) = 0;
};



class T_CTEST_EXPORT_API RBTestData : public TestData {
  UResourceBundle *fData;
  UResourceBundle *fHeaders;
  UResourceBundle *fSettings;
  UResourceBundle *fCases;

public:
  RBTestData(const char* name);
  RBTestData(UResourceBundle *data, UResourceBundle *headers, UErrorCode& status);
private:


  RBTestData& operator=(const RBTestData& );

public:
  virtual ~RBTestData();

  virtual UBool getInfo(const DataMap *& info, UErrorCode &status) const;

  virtual UBool nextSettings(const DataMap *& settings, UErrorCode &status);
  virtual UBool nextCase(const DataMap *& nextCase, UErrorCode &status);
};

#endif

