














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

DMDFuncs::Singleton DMDFuncs::sSingleton;

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
TestEmpty(const char* aTestName, const char* aMode)
{
  char filename[128];
  sprintf(filename, "full-%s-%s.json", aTestName, aMode);
  auto f = MakeUnique<FpWriteFunc>(filename);

  char options[128];
  sprintf(options, "--mode=%s --sample-below=1", aMode);
  ResetEverything(options);

  
  Analyze(Move(f));
}

void
TestUnsampled(const char* aTestName, int aNum, const char* aMode, int aSeven)
{
  char filename[128];
  sprintf(filename, "full-%s%d-%s.json", aTestName, aNum, aMode);
  auto f = MakeUnique<FpWriteFunc>(filename);

  
  
  char options[128];
  sprintf(options, "--mode=%s --sample-below=1 --show-dump-stats=yes", aMode);
  ResetEverything(options);

  
  
  int i;
  char* a = nullptr;
  for (i = 0; i < aSeven + 3; i++) {
      a = (char*) malloc(100);
      UseItOrLoseIt(a, aSeven);
  }
  free(a);

  
  free(nullptr);

  
  
  
  
  char* a2 = (char*) malloc(8);
  Report(a2);

  
  
  char* b = (char*) malloc(10);
  ReportOnAlloc(b);

  
  
  
  char* b2 = (char*) malloc(8);
  ReportOnAlloc(b2);
  free(b2);

  
  
  char* c = (char*) calloc(10, 3);
  Report(c);
  for (int i = 0; i < aSeven - 4; i++) {
    Report(c);
  }

  
  
  Report((void*)(intptr_t)i);

  
  
  
  char* e = (char*) malloc(4096);
  e = (char*) realloc(e, 7169);
  Report(e);

  
  
  
  char* e2 = (char*) realloc(nullptr, 1024);
  e2 = (char*) realloc(e2, 512);
  Report(e2);

  
  
  
  
  char* e3 = (char*) realloc(nullptr, 1023);

  MOZ_ASSERT(e3);
  Report(e3);

  
  
  char* f1 = (char*) malloc(64);
  free(f1);

  
  
  Report((void*)(intptr_t)0x0);

  
  
  Foo(aSeven);

  
  
  char* g1 = (char*) malloc(77);
  ReportOnAlloc(g1);
  ReportOnAlloc(g1);

  
  
  
  
  Foo(aSeven);

  
  
  char* g2 = (char*) malloc(78);
  Report(g2);
  ReportOnAlloc(g2);

  
  
  char* g3 = (char*) malloc(79);
  ReportOnAlloc(g3);
  Report(g3);

  
  
  
  



  




  



  



  if (aNum == 1) {
    
    Analyze(Move(f));
  }

  ClearReports();

  

  Report(a2);
  Report(a2);
  free(c);
  free(e);
  Report(e2);
  free(e3);





  
  for (int i = 0; i < 100; i++) {
    free(malloc(128));
  }

  if (aNum == 2) {
    
    Analyze(Move(f));
  }
}

void
TestSampled(const char* aTestName, const char* aMode, int aSeven)
{
  char filename[128];
  sprintf(filename, "full-%s-%s.json", aTestName, aMode);
  auto f = MakeUnique<FpWriteFunc>(filename);

  char options[128];
  sprintf(options, "--mode=%s --sample-below=128", aMode);
  ResetEverything(options);

  char* s;

  
  
  s = (char*) malloc(128);
  UseItOrLoseIt(s, aSeven);

  
  s = (char*) malloc(160);
  UseItOrLoseIt(s, aSeven);

  
  for (int i = 0; i < aSeven + 9; i++) {
    s = (char*) malloc(8);
    UseItOrLoseIt(s, aSeven);
  }

  
  for (int i = 0; i < aSeven + 8; i++) {
    s = (char*) malloc(8);
    UseItOrLoseIt(s, aSeven);
  }

  
  s = (char*) malloc(256);
  UseItOrLoseIt(s, aSeven);

  
  s = (char*) malloc(96);
  UseItOrLoseIt(s, aSeven);

  
  for (int i = 0; i < aSeven - 2; i++) {
    s = (char*) malloc(8);
    UseItOrLoseIt(s, aSeven);
  }

  
  
  
  for (int i = 1; i <= aSeven + 1; i++) {
    s = (char*) malloc(i * 16);
    UseItOrLoseIt(s, aSeven);
  }

  
  

  Analyze(Move(f));
}

void
TestScan(int aSeven)
{
  auto f = MakeUnique<FpWriteFunc>("basic-scan.json");

  ResetEverything("--mode=scan");

  uintptr_t* p = (uintptr_t*) malloc(6 * sizeof(uintptr_t*));
  UseItOrLoseIt(p, aSeven);

  
  p[0] = 0x123; 
  p[1] = 0x0; 
  p[2] = (uintptr_t)((uint8_t*)p - 1); 
  p[3] = (uintptr_t)p; 
  p[4] = (uintptr_t)((uint8_t*)p + 1); 
  p[5] = 0x0; 

  Analyze(Move(f));
}

void
RunTests()
{
  
  
  
  
  
  
  int seven = 7;

  
  
  int *x = (int*)malloc(100);
  UseItOrLoseIt(x, seven);
  MOZ_RELEASE_ASSERT(IsRunning());

  

  TestEmpty("empty", "live");
  TestEmpty("empty", "dark-matter");
  TestEmpty("empty", "cumulative");

  TestUnsampled("unsampled", 1, "live",        seven);
  TestUnsampled("unsampled", 1, "dark-matter", seven);

  TestUnsampled("unsampled", 2, "dark-matter", seven);
  TestUnsampled("unsampled", 2, "cumulative",  seven);

  TestSampled("sampled", "live", seven);

  TestScan(seven);
}

int main()
{
  RunTests();

  return 0;
}
