
































#include "gtest/gtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdexcept>





void Fail(const char* msg) {
  printf("FAILURE: %s\n", msg);
  fflush(stdout);
  exit(1);
}



void TestFailureThrowsRuntimeError() {
  testing::GTEST_FLAG(throw_on_failure) = true;

  
  try {
    EXPECT_EQ(3, 3);
  } catch(...) {
    Fail("A successful assertion wrongfully threw.");
  }

  
  try {
    EXPECT_EQ(2, 3) << "Expected failure";
  } catch(const std::runtime_error& e) {
    if (strstr(e.what(), "Expected failure") != NULL)
      return;

    printf("%s",
           "A failed assertion did throw an exception of the right type, "
           "but the message is incorrect.  Instead of containing \"Expected "
           "failure\", it is:\n");
    Fail(e.what());
  } catch(...) {
    Fail("A failed assertion threw the wrong type of exception.");
  }
  Fail("A failed assertion should've thrown but didn't.");
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  
  
  
  
  

  TestFailureThrowsRuntimeError();
  return 0;
}
