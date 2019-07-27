











#ifndef _INTLTESTDATADRIVENFORMAT
#define _INTLTESTDATADRIVENFORMAT

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "tsdate.h"
#include "uvector.h"
#include "unicode/format.h"


class TestDataModule;
class TestData;
class DataMap;


class DataDrivenFormatTest : public IntlTest {
    void runIndexedTest(int32_t index, UBool exec, const char* &name,
            char* par = NULL);
public:
    DataDrivenFormatTest();
    virtual ~DataDrivenFormatTest();
protected:

    void DataDrivenTest(char *par);
    void processTest(TestData *testData);
private:
    void testConvertDate(TestData *testData, const DataMap *settings, UBool fmt);




private:
    TestDataModule *driver;
};

#endif 

#endif
