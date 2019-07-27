






#include "DevTools.h"
#include "mozilla/devtools/HeapSnapshot.h"

using mozilla::devtools::UniqueString;
using mozilla::devtools::UniqueStringHashPolicy;

DEF_TEST(UniqueStringHashPolicy_match, {
    
    
    UniqueString str1(NS_strdup(MOZ_UTF16("some long string and a tail")));
    ASSERT_TRUE(!!str1);

    UniqueStringHashPolicy::Lookup lookup(MOZ_UTF16("some long string with same prefix"), 16);

    
    
    ASSERT_FALSE(UniqueStringHashPolicy::match(str1, lookup));
  });
