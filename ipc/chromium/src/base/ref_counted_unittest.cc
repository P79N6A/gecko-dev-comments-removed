



#include "testing/gtest/include/gtest/gtest.h"
#include "base/ref_counted.h"

namespace {

class SelfAssign : public base::RefCounted<SelfAssign> {
};

class CheckDerivedMemberAccess : public scoped_refptr<SelfAssign> {
 public:
  CheckDerivedMemberAccess() {
    
    SelfAssign** pptr = &ptr_;
    EXPECT_EQ(*pptr, ptr_);
  }
};

}  

TEST(RefCountedUnitTest, TestSelfAssignment) {
  SelfAssign* p = new SelfAssign;
  scoped_refptr<SelfAssign> var = p;
  var = var;
  EXPECT_EQ(var.get(), p);
}

TEST(RefCountedUnitTest, ScopedRefPtrMemberAccess) {
  CheckDerivedMemberAccess check;
}
