


































#include <stdio.h>

#include "gtest/gtest.h"






#define GTEST_IMPLEMENTATION_ 1
#include "src/gtest-internal-inl.h"
#undef GTEST_IMPLEMENTATION_

using testing::internal::ShouldUseColor;





TEST(GTestColorTest, Dummy) {
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  if (ShouldUseColor(true)) {
    
    
    printf("YES\n");
    return 1;
  } else {
    
    printf("NO\n");
    return 0;
  }
}
