





#include "storage_test_harness.h"
#include "nsIFile.h"






void
test_file_perms()
{
  nsCOMPtr<mozIStorageConnection> db(getDatabase());
  nsCOMPtr<nsIFile> dbFile;
  do_check_success(db->GetDatabaseFile(getter_AddRefs(dbFile)));

  uint32_t perms = 0;
  do_check_success(dbFile->GetPermissions(&perms));

  
  
#ifdef ANDROID
  do_check_true(perms == (PR_IRUSR | PR_IWUSR));
#elif defined(XP_WIN)
  do_check_true(perms == (PR_IRUSR | PR_IWUSR | PR_IRGRP | PR_IWGRP | PR_IROTH | PR_IWOTH));
#else
  do_check_true(perms == (PR_IRUSR | PR_IWUSR | PR_IRGRP | PR_IROTH));
#endif
}

void (*gTests[])(void) = {
  test_file_perms,
};

const char *file = __FILE__;
#define TEST_NAME "file perms"
#define TEST_FILE file
#include "storage_test_harness_tail.h"
