




































#ifndef ipc_test_harness_TestParent_h
#define ipc_test_harness_TestParent_h 1

#include "mozilla/test/PTestParent.h"

namespace mozilla {
namespace test {


class TestParent : public PTestParent
{
protected:
#if 1


    virtual bool RecvWorld();


#elif 0


    virtual bool RecvPing();
    virtual bool RecvPong(const int& status);
    virtual bool RecvGetValue(const String& key);
    virtual bool RecvGetValues(const StringArray& keys);
    virtual bool RecvSetValue(
                const String& key,
                const String& val,
                bool* ok);
#endif

public:
    TestParent();
    virtual ~TestParent();

    void DoStuff();
};

} 
} 

#endif 
