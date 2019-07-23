









































const BACKUP_FILE_NAME = "test_storage.sqlite.backup";

function test_backup_bad_connection()
{
  var msc = getDatabase(do_get_file("storage/test/unit/corruptDB.sqlite"));
  do_check_false(msc.connectionReady);

  var backup = msc.backupDB(BACKUP_FILE_NAME);
  do_check_eq(BACKUP_FILE_NAME, backup.leafName);
  do_check_true(backup.exists());
  
  backup.remove(false);
}

var tests = [test_backup_bad_connection];

function run_test()
{
  cleanup();

  for (var i = 0; i < tests.length; i++)
    tests[i]();
    
  cleanup();
}

