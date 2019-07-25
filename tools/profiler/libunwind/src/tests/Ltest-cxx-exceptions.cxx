






















#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <libunwind.h>

#define panic(args...)				\
	{ fprintf (stderr, args); exit (-1); }

struct Test
{
  public: 
    Test() { ++counter_; }
    ~Test() { -- counter_; }
    Test(const Test&) { ++counter_; }

  public: 
    static int counter_;
};

int Test::counter_ = 0;


extern "C" void bar()
{
  Test t;
  try {
    Test t;
    throw 5;
  } catch (...) {
    Test t;
    printf("Throwing an int\n");
    throw 6;
  }
}

int main()
{
  try {
    Test t;
    bar();
  } catch (int) {
    
    if (Test::counter_ != 0)
      panic("Counter non-zero\n");
    return Test::counter_;
  } catch (...) {
    
    panic("Int was thrown why are we here?\n");
  }
  exit(0);
}
