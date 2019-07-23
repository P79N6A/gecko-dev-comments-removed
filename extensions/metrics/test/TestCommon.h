







































#define ASSERT_TRUE_RET(cond, ret) \
  if (!cond) { \
    fprintf(stderr, "FAILED: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
    return ret; \
  }

#define ASSERT_TRUE(cond) \
  if (!cond) { \
    fprintf(stderr, "FAILED: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
    return ; \
  }

#define ASSERT_SUCCESS(res) ASSERT_TRUE(NS_SUCCEEDED(res))
#define ASSERT_FALSE(cond) ASSERT_TRUE(! cond)
