



































#include <stdio.h>
#include <malloc.h>

void s1(int, int)
{
    malloc(100);
}

void s2()
{
    s1(1, 2);
    malloc(100);
}

void s3()
{
    s2();
    malloc(100);
    malloc(200);
}

void s4()
{
    s3();
    char* cp = new char[300];
    cp = cp;
}


void s6(int recurse);

void s5(int recurse)
{
    malloc(100);
    if (recurse > 0) {
      s6(recurse - 1);
    }
}

void s6(int recurse)
{
    malloc(100);
    if (recurse > 0) {
      s5(recurse - 1);
    }
}





void C()
{
  malloc(10);
}

void D()
{
  malloc(10);
}

void B(int way)
{
  malloc(10);
  if (way) {
    C();
    C();
    C();
  } else {
    D();
  }
}

void A()
{
  malloc(10);
  B(1);
}

void X()
{
  malloc(10);
  B(0);
}

int main()
{
  s1(1, 2);
  s2();
  s3();
  s4();
  s5(10);
  A();
  X();
}
