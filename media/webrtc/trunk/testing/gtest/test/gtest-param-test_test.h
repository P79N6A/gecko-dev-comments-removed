



































#ifndef GTEST_TEST_GTEST_PARAM_TEST_TEST_H_
#define GTEST_TEST_GTEST_PARAM_TEST_TEST_H_

#include "gtest/gtest.h"

#if GTEST_HAS_PARAM_TEST



class ExternalInstantiationTest : public ::testing::TestWithParam<int> {
};



class InstantiationInMultipleTranslaionUnitsTest
    : public ::testing::TestWithParam<int> {
};

#endif  

#endif  
