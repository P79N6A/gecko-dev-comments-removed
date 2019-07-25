




































#include "TestHarness.h"
#include "nsDeque.h"
#include "nsCRT.h"
#include <stdio.h>




class _TestDeque {
public:
  int Test();
private:
  int OriginalTest();
  int OriginalFlaw();
  int AssignFlaw();
  int TestRemove();
};
static _TestDeque sTestDeque;

class _Dealloc: public nsDequeFunctor {
  virtual void* operator()(void* aObject) {
    return 0;
  }
};

#define TEST(aCondition, aMsg) \
  if (!(aCondition)) { fail("TestDeque: "#aMsg); return 1; }








int _TestDeque::Test() {
  
  int results=0;
  results+=OriginalTest();
  results+=OriginalFlaw();
  results+=AssignFlaw();
  results+=TestRemove();
  return results;
}

int _TestDeque::OriginalTest() {
  const int size = 200;
  int ints[size];
  int i=0;
  int temp;
  nsDeque theDeque(new _Dealloc); 
 
  
  for (i=0;i<size;i++) { 
    ints[i]=i;
  }
  
  for (i=0;i<70;i++) {
    theDeque.Push(&ints[i]);
    temp=*(int*)theDeque.Peek();
    TEST(temp == i, "Verify end after push #1");
    TEST(theDeque.GetSize() == i + 1, "Verify size after push #1");
  }
  TEST(theDeque.GetSize() == 70, "Verify overall size after pushes #1");
  
  for (i=1;i<=55;i++) {
    temp=*(int*)theDeque.Pop();
    TEST(temp == 70-i, "Verify end after pop # 1");
    TEST(theDeque.GetSize() == 70 - i, "Verify size after pop # 1");
  }
  TEST(theDeque.GetSize() == 15, "Verify overall size after pops");

  
  for (i=0;i<55;i++) {
    theDeque.Push(&ints[i]);
    temp=*(int*)theDeque.Peek();
    TEST(temp == i, "Verify end after push #2");
    TEST(theDeque.GetSize() == i + 15 + 1, "Verify size after push # 2");
  }
  TEST(theDeque.GetSize() == 70, "Verify size after end of all pushes #2");

  
  for (i=1;i<=35;i++) {
    temp=*(int*)theDeque.Pop();
    TEST(temp == 55-i, "Verify end after pop # 2");
    TEST(theDeque.GetSize() == 70 - i, "Verify size after pop #2");
  }
  TEST(theDeque.GetSize() == 35, "Verify overall size after end of all pops #2");

  
  for (i=0;i<35;i++) {
    theDeque.Push(&ints[i]);
    temp = *(int*)theDeque.Peek();
    TEST(temp == i, "Verify end after push # 3");
    TEST(theDeque.GetSize() == 35 + 1 + i, "Verify size after push #3");
  }

  
  for (i=0;i<35;i++) {
    temp=*(int*)theDeque.Pop();
    TEST(temp == 34 - i, "Verify end after pop # 3");
  }

  
  for (i=0;i<20;i++) {
    temp=*(int*)theDeque.Pop();
    TEST(temp == 19 - i, "Verify end after pop # 4");
  }

  
  for (i=0;i<15;i++) {
    temp=*(int*)theDeque.Pop();
    TEST(temp == 14 - i, "Verify end after pop # 5");
  }

  TEST(theDeque.GetSize() == 0, "Deque should finish empty.");

  return 0;
}

int _TestDeque::OriginalFlaw() {
  int ints[200];
  int i=0;
  int temp;
  nsDeque d(new _Dealloc);
  



  printf("fill array\n");
  for (i=0; i<30; i++)
    ints[i]=i;

  for (i=0; i<6; i++) {
    d.Push(&ints[i]);
    temp = *(int*)d.Peek();
    TEST(temp == i, "OriginalFlaw push #1");
  }
  TEST(d.GetSize() == 6, "OriginalFlaw size check #1");

  for (i=0; i<4; i++) {
    temp=*(int*)d.PopFront();
    TEST(temp == i, "PopFront test");
  }
  
  TEST(d.GetSize() == 2, "OriginalFlaw size check #2");

  for (i=0; i<4; i++) {
    d.Push(&ints[6 + i]);
  }
  

  for (i=4; i<=9; i++) {
    temp=*(int*)d.PopFront();
    TEST(temp == i, "OriginalFlaw empty check");
  }

  return 0;
}

int _TestDeque::AssignFlaw() {
  nsDeque src(new _Dealloc),dest(new _Dealloc);
  return 0;
}

static bool VerifyContents(const nsDeque& aDeque, const int* aContents, int aLength) {
  for (int i=0; i<aLength; ++i) {
    if (*(int*)aDeque.ObjectAt(i) != aContents[i]) {
      return false;
    }
  }
  return true;
}

int _TestDeque::TestRemove() {
  nsDeque d;
  const int count = 10;
  int ints[count];
  for (int i=0; i<count; i++) {
    ints[i] = i;
  }

  for (int i=0; i<6; i++) {
    d.Push(&ints[i]);
  }
  
  d.PopFront();
  d.PopFront();

  
  for (int i=2; i<=5; i++) {
    int t = *(int*)d.ObjectAt(i-2);
    TEST(t == i, "Verify ObjectAt()");
  }

  d.RemoveObjectAt(1);
  
  static const int t1[] = {2,4,5};
  TEST(VerifyContents(d, t1, 3), "verify contents t1");

  d.PushFront(&ints[1]);
  d.PushFront(&ints[0]);
  d.PushFront(&ints[7]);
  d.PushFront(&ints[6]);
  
  static const int t2[] = {6,7,0,1,2,4,5};
  TEST(VerifyContents(d, t2, 7), "verify contents t2");

  d.RemoveObjectAt(1);
  
  static const int t3[] = {6,0,1,2,4,5};
  TEST(VerifyContents(d, t3, 6), "verify contents t3");

  d.RemoveObjectAt(5);
  
  static const int t4[] = {6,0,1,2,4};
  TEST(VerifyContents(d, t4, 5), "verify contents t4");

  d.RemoveObjectAt(0);
  
  static const int t5[] = {0,1,2,4};
  TEST(VerifyContents(d, t5, 4), "verify contents t5");


  return 0;
}

int main (void) {
  ScopedXPCOM xpcom("TestTimers");
  NS_ENSURE_FALSE(xpcom.failed(), 1);

  _TestDeque test;
  int result = test.Test();
  TEST(result == 0, "All tests pass");
  return 0;
}
