



#include <shlobj.h>

#include "base/scoped_comptr_win.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(ScopedComPtrTest, ScopedComPtr) {
  EXPECT_TRUE(memcmp(&ScopedComPtr<IUnknown>::iid(), &IID_IUnknown,
                     sizeof(IID)) == 0);

  EXPECT_TRUE(SUCCEEDED(::CoInitialize(NULL)));

  {
    ScopedComPtr<IUnknown> unk;
    EXPECT_TRUE(SUCCEEDED(unk.CreateInstance(CLSID_ShellLink)));
    ScopedComPtr<IUnknown> unk2;
    unk2.Attach(unk.Detach());
    EXPECT_TRUE(unk == NULL);
    EXPECT_TRUE(unk2 != NULL);

    ScopedComPtr<IMalloc> mem_alloc;
    EXPECT_TRUE(SUCCEEDED(CoGetMalloc(1, mem_alloc.Receive())));

    
    ScopedComPtr<IMalloc> copy1(mem_alloc);
    EXPECT_TRUE(copy1.IsSameObject(mem_alloc));
    EXPECT_FALSE(copy1.IsSameObject(unk2));  
    EXPECT_FALSE(copy1.IsSameObject(unk));  

    IMalloc* naked_copy = copy1.Detach();
    copy1 = naked_copy;  
    naked_copy->Release();

    copy1.Release();
    EXPECT_FALSE(copy1.IsSameObject(unk2));  

    
    ScopedComPtr<IMalloc> copy2(static_cast<IMalloc*>(mem_alloc));
    EXPECT_TRUE(copy2.IsSameObject(mem_alloc));

    EXPECT_TRUE(SUCCEEDED(unk.QueryFrom(mem_alloc)));
    EXPECT_TRUE(unk != NULL);
    unk.Release();
    EXPECT_TRUE(unk == NULL);
    EXPECT_TRUE(unk.IsSameObject(copy1));  
  }

  ::CoUninitialize();
}
