





#include "gtest/gtest.h"

#include "MockDiagnostics.h"
#include "MockDirectiveHandler.h"
#include "Preprocessor.h"

#ifndef PREPROCESSOR_TESTS_PREPROCESSOR_TEST_H_
#define PREPROCESSOR_TESTS_PREPROCESSOR_TEST_H_

class PreprocessorTest : public testing::Test
{
  protected:
    PreprocessorTest() : mPreprocessor(&mDiagnostics, &mDirectiveHandler) { }

    
    
    void preprocess(const char* input, const char* expected);

    MockDiagnostics mDiagnostics;
    MockDirectiveHandler mDirectiveHandler;
    pp::Preprocessor mPreprocessor;
};

#endif  
