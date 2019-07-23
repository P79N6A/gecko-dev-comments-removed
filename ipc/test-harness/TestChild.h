




































#ifndef ipc_test_harness_TestChild_h
#define ipc_test_harness_TestChild_h 1

#include "mozilla/test/PTestChild.h"

namespace mozilla {
namespace test {


class TestChild : public PTestChild
{
protected:

#if 1


    virtual nsresult RecvHello();


#elif 0


    virtual nsresult RecvPing();
    virtual nsresult RecvPong(const int& status);
    virtual nsresult RecvTellValue(
                const String& key,
                const String& val);
    virtual nsresult RecvTellValues(
                const StringArray& keys,
                const StringArray& vals);
#endif

public:
    TestChild();
    virtual ~TestChild();
};

} 
} 

#endif 
