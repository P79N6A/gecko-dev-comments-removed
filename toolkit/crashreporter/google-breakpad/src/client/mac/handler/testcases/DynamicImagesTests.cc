



































#include "client/mac/handler/testcases/DynamicImagesTests.h"
#include "client/mac/handler/dynamic_images.h"

DynamicImagesTests test2(TEST_INVOCATION(DynamicImagesTests,
                                         ReadTaskMemoryTest));
DynamicImagesTests test3(TEST_INVOCATION(DynamicImagesTests,
                                         ReadLibrariesFromLocalTaskTest));

DynamicImagesTests::DynamicImagesTests(TestInvocation *invocation)
    : TestCase(invocation) {
}

DynamicImagesTests::~DynamicImagesTests() {
}

void DynamicImagesTests::ReadTaskMemoryTest() {
  kern_return_t kr;

  
  
  void *addr = reinterpret_cast<void*>(&test2);
  void *buf;

  fprintf(stderr, "reading 0x%p\n", addr);
  buf = google_breakpad::ReadTaskMemory(mach_task_self(),
                                        addr,
                                        getpagesize(),
                                        &kr);

  CPTAssert(kr == KERN_SUCCESS);

  CPTAssert(buf != NULL);

  CPTAssert(0 == memcmp(buf, (const void*)addr, getpagesize()));

  free(buf);
}

void DynamicImagesTests::ReadLibrariesFromLocalTaskTest() {

  mach_port_t me = mach_task_self();
  google_breakpad::DynamicImages *d = new google_breakpad::DynamicImages(me);

  fprintf(stderr,"Local task image count: %d\n", d->GetImageCount());

  d->TestPrint();

  CPTAssert(d->GetImageCount() > 0);
}
