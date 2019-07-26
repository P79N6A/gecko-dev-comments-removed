









#include <stdlib.h>
#include <string.h>

#include "libyuv/basic_types.h"
#include "libyuv/version.h"
#include "../unit_test/unit_test.h"

namespace libyuv {




TEST_F(libyuvTest, TestVersion) {
  EXPECT_GE(LIBYUV_VERSION, 169);  
  printf("LIBYUV_VERSION %d\n", LIBYUV_VERSION);
#ifdef LIBYUV_SVNREVISION
  const char *ver = strchr(LIBYUV_SVNREVISION, ':');
  if (ver) {
    ++ver;
  } else {
    ver = LIBYUV_SVNREVISION;
  }
  int svn_revision = atoi(ver);  
  printf("LIBYUV_SVNREVISION %d\n", svn_revision);
  EXPECT_NEAR(LIBYUV_VERSION, svn_revision, 3);  
  if (LIBYUV_VERSION != svn_revision) {
    printf("WARNING - Versions do not match.\n");
  }
#endif
}

}  
