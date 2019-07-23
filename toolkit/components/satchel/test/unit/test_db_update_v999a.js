













































function run_test()
{
  try {
  var testnum = 0;

  
  var testfile = do_get_file("formhistory_v999a.sqlite");
  var profileDir = dirSvc.get("ProfD", Ci.nsIFile);

  
  var destFile = profileDir.clone();
  destFile.append("formhistory.sqlite");
  if (destFile.exists())
    destFile.remove(false);

  testfile.copyTo(profileDir, "formhistory.sqlite");
  do_check_eq(999, getDBVersion(testfile));

  var fh = Cc["@mozilla.org/satchel/form-history;1"].
           getService(Ci.nsIFormHistory2);


  
  testnum++;
  
  do_check_true(fh.hasEntries);
  do_check_true(fh.entryExists("name-A", "value-A"));
  do_check_true(fh.entryExists("name-B", "value-B"));
  do_check_true(fh.entryExists("name-C", "value-C1"));
  do_check_true(fh.entryExists("name-C", "value-C2"));
  do_check_true(fh.entryExists("name-E", "value-E"));
  
  do_check_eq(CURRENT_SCHEMA, fh.DBConnection.schemaVersion);

  
  testnum++;
  
  do_check_false(fh.entryExists("name-D", "value-D"));
  fh.addEntry("name-D", "value-D");
  do_check_true(fh.entryExists("name-D", "value-D"));
  fh.removeEntry("name-D", "value-D");
  do_check_false(fh.entryExists("name-D", "value-D"));

  } catch (e) {
    throw "FAILED in test #" + testnum + " -- " + e;
  }
}
