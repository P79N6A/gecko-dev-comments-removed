



var testnum = 0;

let iter;

function run_test()
{
  do_test_pending();
  iter = next_test();
  iter.next();
}

function next_test()
{
  try {

  
  var testfile = do_get_file("formhistory_v3v4.sqlite");
  var profileDir = dirSvc.get("ProfD", Ci.nsIFile);

  
  var destFile = profileDir.clone();
  destFile.append("formhistory.sqlite");
  if (destFile.exists())
    destFile.remove(false);

  testfile.copyTo(profileDir, "formhistory.sqlite");
  do_check_eq(3, getDBVersion(testfile));

  
  testnum++;

  destFile = profileDir.clone();
  destFile.append("formhistory.sqlite");
  dbConnection = Services.storage.openUnsharedDatabase(destFile);

  
  do_check_eq(CURRENT_SCHEMA, FormHistory.schemaVersion);

  
  do_check_true(dbConnection.tableExists("moz_deleted_formhistory"));
  dbConnection.close();

  
  yield countEntries("name-A", "value-A",
    function (num) {
      do_check_true(num > 0);
      do_test_finished();
    }
  );

  } catch (e) {
    throw "FAILED in test #" + testnum + " -- " + e;
  }
}
