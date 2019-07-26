
#ifndef mozilla__ipdltest_IPDLUnitTestUtils
#define mozilla__ipdltest_IPDLUnitTestUtils 1

namespace mozilla {
namespace _ipdltest {

struct Bad {};

} 
} 

namespace IPC {

template<>
struct ParamTraits<mozilla::_ipdltest::Bad>
{
  typedef mozilla::_ipdltest::Bad paramType;

  
  static void Write(Message* aMsg, const paramType& aParam);
  static bool Read(const Message* aMsg, void** aIter, paramType* aResult);
};

} 

#endif 
