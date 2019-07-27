




































#include "gtest/gtest.h"

#include <stdio.h>                      
#include <stdlib.h>                     
#include <exception>                    




void TerminateHandler() {
  fprintf(stderr, "%s\n", "Unhandled C++ exception terminating the program.");
  fflush(NULL);
  exit(1);
}

int main(int argc, char** argv) {
#if GTEST_HAS_EXCEPTIONS
  std::set_terminate(&TerminateHandler);
#endif
  testing::InitGoogleTest(&argc, argv);

  
  
  
  
  

  
  
  EXPECT_EQ(2, 3);

  
  
  return 0;
}
