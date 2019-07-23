




































#ifndef ipc_test_harness_TestChild_h
#define ipc_test_harness_TestChild_h 1

#include "mozilla/test/PTestChild.h"

namespace mozilla {
namespace test {


class TestChild : public PTestChild
{
protected:

#if 1


    virtual bool RecvHello();


#elif 0


    virtual bool RecvPing();
    virtual bool RecvPong(const int& status);
    virtual bool RecvTellValue(
                const String& key,
                const String& val);
    virtual bool RecvTellValues(
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
