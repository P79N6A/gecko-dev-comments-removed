




































function run_test()
{
  try {
  var testnum = 0;

  
  var testfile = do_get_file("toolkit/components/satchel/test/unit/formhistory_CORRUPT.sqlite");
  var profileDir = dirSvc.get("ProfD", Ci.nsIFile);

  
  var destFile = profileDir.clone();
  destFile.append("formhistory.sqlite");
  if (destFile.exists())
    destFile.remove(false);

  testfile.copyTo(profileDir, "formhistory.sqlite");

  var fh = Cc["@mozilla.org/satchel/form-history;1"].
           getService(Ci.nsIFormHistory2);


  
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
