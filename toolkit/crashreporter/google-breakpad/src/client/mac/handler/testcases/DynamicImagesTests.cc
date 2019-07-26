



































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
  std::vector<uint8_t> buf(getpagesize());

  fprintf(stderr, "reading 0x%p\n", addr);
  kr = google_breakpad::ReadTaskMemory(mach_task_self(),
                                       (uint64_t)addr,
                                       getpagesize(),
                                       buf);

  CPTAssert(kr == KERN_SUCCESS);

  CPTAssert(0 == memcmp(&buf[0], (const void*)addr, getpagesize()));
}

void DynamicImagesTests::ReadLibrariesFromLocalTaskTest() {

  mach_port_t me = mach_task_self();
  google_breakpad::DynamicImages *d = new google_breakpad::DynamicImages(me);

  fprintf(stderr,"Local task image count: %d\n", d->GetImageCount());

  CPTAssert(d->GetImageCount() > 0);
}
