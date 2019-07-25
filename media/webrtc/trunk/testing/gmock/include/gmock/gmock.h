


































#ifndef GMOCK_INCLUDE_GMOCK_GMOCK_H_
#define GMOCK_INCLUDE_GMOCK_GMOCK_H_




















#include "gmock/gmock-actions.h"
#include "gmock/gmock-cardinalities.h"
#include "gmock/gmock-generated-actions.h"
#include "gmock/gmock-generated-function-mockers.h"
#include "gmock/gmock-generated-matchers.h"
#include "gmock/gmock-more-actions.h"
#include "gmock/gmock-generated-nice-strict.h"
#include "gmock/gmock-matchers.h"
#include "gmock/internal/gmock-internal-utils.h"

namespace testing {


GMOCK_DECLARE_bool_(catch_leaked_mocks);
GMOCK_DECLARE_string_(verbose);












void InitGoogleMock(int* argc, char** argv);



void InitGoogleMock(int* argc, wchar_t** argv);

}  

#endif  
