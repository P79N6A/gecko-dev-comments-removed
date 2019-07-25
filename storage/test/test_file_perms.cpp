






































#include "storage_test_harness.h"
#include "nsILocalFile.h"






void
test_file_perms()
{
  nsCOMPtr<nsIFile> profDir;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                       getter_AddRefs(profDir));
  nsCOMPtr<nsILocalFile> sqlite_file = do_QueryInterface(profDir);
  sqlite_file->Append(NS_LITERAL_STRING("places.sqlite"));
  PRUint32 perms = 0;
  sqlite_file->GetPermissions(&perms);

  
  
#ifdef ANDROID
  do_check_true(perms == PR_IRUSR | PR_IWUSR);
#else
  do_check_true(perms == PR_IRUSR | PR_IWUSR | PR_IRGRP | PR_IWGRP | PR_IROTH | PR_IWOTH);
#endif
}

void (*gTests[])(void) = {
  test_file_perms,
};

const char *file = __FILE__;
#define TEST_NAME "file perms"
#define TEST_FILE file
#include "storage_test_harness_tail.h"
