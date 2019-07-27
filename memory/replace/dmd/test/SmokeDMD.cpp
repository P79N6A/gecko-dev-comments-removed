














#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "mozilla/Assertions.h"
#include "mozilla/JSONWriter.h"
#include "mozilla/UniquePtr.h"
#include "DMD.h"

using mozilla::JSONWriter;
using mozilla::MakeUnique;
using namespace mozilla::dmd;

class FpWriteFunc : public mozilla::JSONWriteFunc
{
public:
  explicit FpWriteFunc(const char* aFilename)
  {
    mFp = fopen(aFilename, "w");
    if (!mFp) {
      fprintf(stderr, "SmokeDMD: can't create %s file: %s\n",
              aFilename, strerror(errno));
      exit(1);
    }
  }

  ~FpWriteFunc() { fclose(mFp); }

  void Write(const char* aStr) { fputs(aStr, mFp); }

private:
  FILE* mFp;
};


static void
UseItOrLoseIt(void* aPtr, int aSeven)
{
  char buf[64];
  int n = sprintf(buf, "%p\n", aPtr);
  if (n == 20 + aSeven) {
    fprintf(stderr, "well, that is surprising");
  }
}



void Foo(int aSeven)
{
  char* a[6];
  for (int i = 0; i < aSeven - 1; i++) {
    a[i] = (char*) malloc(128 - 16*i);
  }

  
  
  

  Report(a[2]);                     

  UseItOrLoseIt(a[2], aSeven);

  for (int i = 0; i < aSeven - 5; i++) {
    Report(a[i]);                   
  }

  UseItOrLoseIt(a[2], aSeven);

  Report(a[3]);                     

  
}

void
RunTests()
{
  
  auto f1 = MakeUnique<FpWriteFunc>("full-empty.json");
  auto f2 = MakeUnique<FpWriteFunc>("full-unsampled1.json");
  auto f3 = MakeUnique<FpWriteFunc>("full-unsampled2.json");
  auto f4 = MakeUnique<FpWriteFunc>("full-sampled.json");

  
  
  
  
  
  
  int seven = 7;

  
  
  int *x = (int*)malloc(100);
  UseItOrLoseIt(x, seven);
  MOZ_RELEASE_ASSERT(IsRunning());

  
  SetSampleBelowSize(1);

  
  
  ClearBlocks();

  

  
  JSONWriter writer1(Move(f1));
  AnalyzeReports(writer1);

  

  
  
  int i;
  char* a = nullptr;
  for (i = 0; i < seven + 3; i++) {
      a = (char*) malloc(100);
      UseItOrLoseIt(a, seven);
  }
  free(a);

  
  
  
  
  char* a2 = (char*) malloc(8);
  Report(a2);

  
  
  char* b = (char*) malloc(10);
  ReportOnAlloc(b);

  
  
  
  char* b2 = (char*) malloc(1);
  ReportOnAlloc(b2);
  free(b2);

  
  
  char* c = (char*) calloc(10, 3);
  Report(c);
  for (int i = 0; i < seven - 4; i++) {
    Report(c);
  }

  
  
  Report((void*)(intptr_t)i);

  
  
  
  char* e = (char*) malloc(4096);
  e = (char*) realloc(e, 4097);
  Report(e);

  
  
  
  char* e2 = (char*) realloc(nullptr, 1024);
  e2 = (char*) realloc(e2, 512);
  Report(e2);

  
  
  
  
  char* e3 = (char*) realloc(nullptr, 1023);

  MOZ_ASSERT(e3);
  Report(e3);

  
  
  char* f = (char*) malloc(64);
  free(f);

  
  
  Report((void*)(intptr_t)0x0);

  
  
  Foo(seven);

  
  
  char* g1 = (char*) malloc(77);
  ReportOnAlloc(g1);
  ReportOnAlloc(g1);

  
  
  
  
  Foo(seven);

  
  
  char* g2 = (char*) malloc(78);
  Report(g2);
  ReportOnAlloc(g2);

  
  
  char* g3 = (char*) malloc(79);
  ReportOnAlloc(g3);
  Report(g3);

  
  
  
  



  




  



  



  
  JSONWriter writer2(Move(f2));
  AnalyzeReports(writer2);

  

  Report(a2);
  Report(a2);
  free(c);
  free(e);
  Report(e2);
  free(e3);





  
  JSONWriter writer3(Move(f3));
  AnalyzeReports(writer3);

  

  
  SetSampleBelowSize(128);

  
  ClearBlocks();

  char* s;

  
  
  s = (char*) malloc(128);
  UseItOrLoseIt(s, seven);

  
  s = (char*) malloc(144);
  UseItOrLoseIt(s, seven);

  
  for (int i = 0; i < seven + 9; i++) {
    s = (char*) malloc(8);
    UseItOrLoseIt(s, seven);
  }

  
  for (int i = 0; i < seven + 8; i++) {
    s = (char*) malloc(8);
    UseItOrLoseIt(s, seven);
  }

  
  s = (char*) malloc(256);
  UseItOrLoseIt(s, seven);

  
  s = (char*) malloc(96);
  UseItOrLoseIt(s, seven);

  
  for (int i = 0; i < seven - 2; i++) {
    s = (char*) malloc(8);
    UseItOrLoseIt(s, seven);
  }

  
  
  
  for (int i = 1; i <= seven + 1; i++) {
    s = (char*) malloc(i * 16);
    UseItOrLoseIt(s, seven);
  }

  
  

  
  JSONWriter writer4(Move(f4));
  AnalyzeReports(writer4);
}

int main()
{
  RunTests();

  return 0;
}
