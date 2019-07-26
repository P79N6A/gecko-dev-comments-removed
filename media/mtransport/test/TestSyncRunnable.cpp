




#include "nsThreadUtils.h"
#include "mozilla/SyncRunnable.h"

#include "gtest/gtest.h"

using namespace mozilla;

nsIThread *gThread = nullptr;

class TestRunnable : public nsRunnable {
public:
  TestRunnable() : ran_(false) {}

  NS_IMETHOD Run()
  {
    ran_ = true;

    return NS_OK;
  }

  bool ran() const { return ran_; }

private:
  bool ran_;
};

class TestSyncRunnable : public ::testing::Test {
public:
  static void SetUpTestCase()
  {
    nsresult rv = NS_NewNamedThread("thread", &gThread);
    ASSERT_TRUE(NS_SUCCEEDED(rv));
  }

  static void TearDownTestCase()
  {
    if (gThread)
      gThread->Shutdown();
  }
};

TEST_F(TestSyncRunnable, TestDispatch)
{
  nsRefPtr<TestRunnable> r(new TestRunnable());
  nsRefPtr<SyncRunnable> s(new SyncRunnable(r));
  s->DispatchToThread(gThread);

  ASSERT_TRUE(r->ran());
}

TEST_F(TestSyncRunnable, TestDispatchStatic)
{
  nsRefPtr<TestRunnable> r(new TestRunnable());
  SyncRunnable::DispatchToThread(gThread, r);
  ASSERT_TRUE(r->ran());
}


#include "mtransport_test_utils.h"
MtransportTestUtils *test_utils;

int main(int argc, char **argv)
{
  test_utils = new MtransportTestUtils();
  
  ::testing::InitGoogleTest(&argc, argv);

  RUN_ALL_TESTS();

  delete test_utils;
  return 0;
}
