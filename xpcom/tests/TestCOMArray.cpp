




































#include "TestHarness.h"
#include "nsCOMArray.h"


#define NS_IFOO_IID \
  {0x9e70a320, 0xbe02, 0x11d1,    \
    {0x80, 0x31, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}

class IFoo : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFOO_IID)

  NS_IMETHOD_(nsrefcnt) RefCnt() = 0;
  NS_IMETHOD_(PRInt32) ID() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(IFoo, NS_IFOO_IID)

class Foo : public IFoo {
public:

  Foo(PRInt32 aID);
  ~Foo();

  
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD_(nsrefcnt) RefCnt() { return mRefCnt; }
  NS_IMETHOD_(PRInt32) ID() { return mID; }

  static PRInt32 gCount;

  PRInt32 mID;
};

PRInt32 Foo::gCount = 0;

Foo::Foo(PRInt32 aID)
{
  mID = aID;
  ++gCount;
}

Foo::~Foo()
{
  --gCount;
}

NS_IMPL_ISUPPORTS1(Foo, IFoo)


typedef nsCOMArray<IFoo> Array;


int main(int argc, char **argv)
{
  ScopedXPCOM xpcom("nsCOMArrayTests");
  if (xpcom.failed()) {
    return 1;
  }

  int rv = 0;

  Array arr;

  for (PRInt32 i = 0; i < 20; ++i) {
    nsCOMPtr<IFoo> foo = new Foo(i);
    arr.AppendObject(foo);
  }

  if (arr.Count() != 20 || Foo::gCount != 20) {
    fail("nsCOMArray::AppendObject failed");
    rv = 1;
  }

  arr.SetCount(10);

  if (arr.Count() != 10 || Foo::gCount != 10) {
    fail("nsCOMArray::SetCount shortening of array failed");
    rv = 1;
  }

  arr.SetCount(30);

  if (arr.Count() != 30 || Foo::gCount != 10) {
    fail("nsCOMArray::SetCount lengthening of array failed");
    rv = 1;
  }

  for (PRInt32 i = 0; i < 10; ++i) {
	if (arr[i] == nsnull) {
      fail("nsCOMArray elements should be non-null");
      rv = 1;
	  break;
    }
  }

  for (PRInt32 i = 10; i < 30; ++i) {
	if (arr[i] != nsnull) {
      fail("nsCOMArray elements should be null");
      rv = 1;
	  break;
    }
  }

  return rv;
}
