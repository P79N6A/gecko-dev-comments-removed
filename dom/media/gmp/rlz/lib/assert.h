




#ifndef FAKE_ASSERT_H_
#define FAKE_ASSERT_H_

#include <assert.h>

#define ASSERT_STRING(x) { assert(false); }
#define VERIFY(x) { assert(x); };

#endif
