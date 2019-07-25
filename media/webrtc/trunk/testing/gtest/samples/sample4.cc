
































#include <stdio.h>

#include "sample4.h"


int Counter::Increment() {
  return counter_++;
}


void Counter::Print() const {
  printf("%d", counter_);
}
