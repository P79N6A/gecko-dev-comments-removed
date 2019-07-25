





#ifndef PREPROCESSOR_TESTS_MOCK_DIAGNOSTICS_H_
#define PREPROCESSOR_TESTS_MOCK_DIAGNOSTICS_H_

#include "gmock/gmock.h"
#include "Diagnostics.h"

class MockDiagnostics : public pp::Diagnostics
{
  public:
    MOCK_METHOD3(print,
        void(ID id, const pp::SourceLocation& loc, const std::string& text));
};

#endif  
