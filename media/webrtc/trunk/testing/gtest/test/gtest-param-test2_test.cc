

































#include "gtest/gtest.h"

#include "test/gtest-param-test_test.h"

#if GTEST_HAS_PARAM_TEST

using ::testing::Values;
using ::testing::internal::ParamGenerator;




ParamGenerator<int> extern_gen = Values(33);





INSTANTIATE_TEST_CASE_P(MultiplesOf33,
                        ExternalInstantiationTest,
                        Values(33, 66));





INSTANTIATE_TEST_CASE_P(Sequence2,
                        InstantiationInMultipleTranslaionUnitsTest,
                        Values(42*3, 42*4, 42*5));

#endif  
