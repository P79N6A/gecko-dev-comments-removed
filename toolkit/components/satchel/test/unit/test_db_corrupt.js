




































function run_test()
{
  try {
  var testnum = 0;

  
  var testfile = do_get_file("formhistory_CORRUPT.sqlite");
  var profileDir = dirSvc.get("ProfD", Ci.nsIFile);

  
  var destFile = profileDir.clone();
  destFile.append("formhistory.sqlite");
  if (destFile.exists())
    destFile.remove(false);

  var bakFile = profileDir.clone();
  bakFile.append("formhistory.sqlite.corrupt");
  if (bakFile.exists())
    bakFile.remove(false);

  testfile.copyTo(profileDir, "formhistory.sqlite");

  
  testnum++;
  
  do_check_false(bakFile.exists());
  var fh = Cc["@mozilla.org/satchel/form-history;1"].
           getService(Ci.nsIFormHistory2);
  
  do_check_false(bakFile.exists());
  
  fh.DBConnection;
  do_check_true(bakFile.exists());
  bakFile.remove(false);

  
  testnum++;
  
  do_check_false(fh.hasEntries);
  do_check_false(fh.entryExists("name-A", "value-A"));


  
  testnum++;
  
  fh.addEntry("name-A", "value-A");
  do_check_true(fh.hasEntries);
  do_check_true(fh.entryExists("name-A", "value-A"));


  
  testnum++;
  
  fh.removeEntry("name-A", "value-A");
  do_check_false(fh.hasEntries);
  do_check_false(fh.entryExists("name-A", "value-A"));


  } catch (e) {
    throw "FAILED in test #" + testnum + " -- " + e;
  }
}
