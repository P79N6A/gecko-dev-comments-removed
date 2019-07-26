




#ifndef GTEST_MOZGTESTFRIEND_H
#define GTEST_MOZGTESTFRIEND_H

#ifdef ENABLE_TESTS
#include "gtest_prod.h"
#else
#define FRIEND_TEST(a, b)
#endif

#endif 
