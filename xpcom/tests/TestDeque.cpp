




































#include "nsDeque.h"
#include "nsCRT.h"
#include <stdio.h>




class _TestDeque {
public:
  _TestDeque() {
    SelfTest();
  }
  int SelfTest();
  nsresult OriginalTest();
  nsresult OriginalFlaw();
  nsresult AssignFlaw();
};
static _TestDeque sTestDeque;

class _Dealloc: public nsDequeFunctor {
  virtual void* operator()(void* aObject) {
    return 0;
  }
};







int _TestDeque::SelfTest() {
  
  int results=0;
  results+=OriginalTest();
  results+=OriginalFlaw();
  results+=AssignFlaw();
  return results;
}

nsresult _TestDeque::OriginalTest() {
  int ints[200];
  int count=sizeof(ints)/sizeof(int);
  int i=0;
  int* temp;
  nsDeque theDeque(new _Dealloc); 
 
  for (i=0;i<count;i++) { 
    ints[i]=10*(1+i);
  }
  for (i=0;i<70;i++) {
    theDeque.Push(&ints[i]);
  }
  for (i=0;i<56;i++) {
    temp=(int*)theDeque.Pop();
  }
  for (i=0;i<55;i++) {
    theDeque.Push(&ints[i]);
  }
  for (i=0;i<35;i++) {
    temp=(int*)theDeque.Pop();
  }
  for (i=0;i<35;i++) {
    theDeque.Push(&ints[i]);
  }
  for (i=0;i<38;i++) {
    temp=(int*)theDeque.Pop();
  }
  return NS_OK;
}

nsresult _TestDeque::OriginalFlaw() {
  int ints[200];
  int i=0;
  int* temp;
  nsDeque secondDeque(new _Dealloc);
  



  printf("fill array\n");
  for (i=32; i; --i)
    ints[i]=i*3+10;
  printf("push 6 times\n");
  for (i=0; i<6; i++)
    secondDeque.Push(&ints[i]);
  printf("popfront 4 times:\n");
  for (i=4; i; --i) {
    temp=(int*)secondDeque.PopFront();
    printf("%d\t",*temp);
  }
  printf("push 4 times\n");
  for (int j=4; j; --j)
    secondDeque.Push(&ints[++i]);
  printf("origin should now be about 4\n");
  printf("and size should be 6\n");
  printf("origin+size>capacity\n");

   
  printf("but the old code wasn't behaving accordingly.\n");

   
  printf("we shouldn't crash or anything interesting, ");

  temp=(int*)secondDeque.Peek();
  printf("peek: %d\n",*temp);
  return NS_OK;
}

nsresult _TestDeque::AssignFlaw() {
  nsDeque src(new _Dealloc),dest(new _Dealloc);
  return NS_OK;
}

int main (void) {
  _TestDeque test;
  return 0;
}
