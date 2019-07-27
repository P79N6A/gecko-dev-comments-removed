




#ifndef LayoutLogging_h
#define LayoutLogging_h

#include "mozilla/Logging.h"




PRLogModuleInfo* GetLayoutLog();







#ifdef DEBUG
#define LAYOUT_WARN_IF_FALSE(_cond, _msg)                                  \
  PR_BEGIN_MACRO                                                           \
    if (MOZ_LOG_TEST(GetLayoutLog(), mozilla::LogLevel::Warning) &&        \
        !(_cond)) {                                                        \
      mozilla::detail::LayoutLogWarning(_msg, #_cond, __FILE__, __LINE__); \
    }                                                                      \
  PR_END_MACRO
#else
#define LAYOUT_WARN_IF_FALSE(_cond, _msg) \
  PR_BEGIN_MACRO                          \
  PR_END_MACRO
#endif

namespace mozilla {
namespace detail {

void LayoutLogWarning(const char* aStr, const char* aExpr,
                      const char* aFile, int32_t aLine);

} 
} 

#endif 
