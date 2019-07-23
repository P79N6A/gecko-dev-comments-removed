#include "mozilla/test/TestProtocolParent.h"

namespace mozilla {
namespace test {


class TestParent :
    public TestProtocolParent
{
protected:
#if 1


    virtual nsresult RecvWorld();


#elif 0


    virtual nsresult RecvPing();
    virtual nsresult RecvPong(const int& status);
    virtual nsresult RecvGetValue(const String& key);
    virtual nsresult RecvGetValues(const StringArray& keys);
    virtual nsresult RecvSetValue(
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
