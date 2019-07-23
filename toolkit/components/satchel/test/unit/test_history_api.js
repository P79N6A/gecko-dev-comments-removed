




































var testnum = 0;
var fh;

function run_test()
{
  try {

  
  var testfile = do_get_file("formhistory_apitest.sqlite");
  var profileDir = dirSvc.get("ProfD", Ci.nsIFile);

  
  var destFile = profileDir.clone();
  destFile.append("formhistory.sqlite");
  if (destFile.exists())
    destFile.remove(false);

  testfile.copyTo(profileDir, "formhistory.sqlite");

  fh = Cc["@mozilla.org/satchel/form-history;1"].
       getService(Ci.nsIFormHistory2);


  
  
  testnum++;
  do_check_true(fh.hasEntries);
  do_check_true(fh.nameExists("name-A"));
  do_check_true(fh.nameExists("name-B"));
  do_check_true(fh.nameExists("name-C"));
  do_check_true(fh.nameExists("name-D"));
  do_check_true(fh.entryExists("name-A", "value-A"));
  do_check_true(fh.entryExists("name-B", "value-B1"));
  do_check_true(fh.entryExists("name-B", "value-B2"));
  do_check_true(fh.entryExists("name-C", "value-C"));
  do_check_true(fh.entryExists("name-D", "value-D"));
  

  
  
  testnum++;
  do_check_false(fh.nameExists("blah"));
  do_check_false(fh.nameExists(""));
  do_check_false(fh.nameExists(null));
  do_check_false(fh.entryExists("name-A", "blah"));
  do_check_false(fh.entryExists("name-A", ""));
  do_check_false(fh.entryExists("name-A", null));
  do_check_false(fh.entryExists("blah", "value-A"));
  do_check_false(fh.entryExists("", "value-A"));
  do_check_false(fh.entryExists(null, "value-A"));

  
  
  testnum++;
  fh.removeEntriesForName("name-A");
  do_check_false(fh.entryExists("name-A", "value-A"));
  do_check_true(fh.entryExists("name-B", "value-B1"));
  do_check_true(fh.entryExists("name-B", "value-B2"));
  do_check_true(fh.entryExists("name-C", "value-C"));
  do_check_true(fh.entryExists("name-D", "value-D"));

  
  
  testnum++;
  fh.removeEntriesForName("name-B");
  do_check_false(fh.entryExists("name-A", "value-A"));
  do_check_false(fh.entryExists("name-B", "value-B1"));
  do_check_false(fh.entryExists("name-B", "value-B2"));
  do_check_true(fh.entryExists("name-C", "value-C"));
  do_check_true(fh.entryExists("name-D", "value-D"));

  
  
  testnum++;
  do_check_true(fh.nameExists("time-A")); 
  do_check_true(fh.nameExists("time-B")); 
  do_check_true(fh.nameExists("time-C")); 
  do_check_true(fh.nameExists("time-D")); 
  fh.removeEntriesByTimeframe(1050, 2000);
  do_check_true(fh.nameExists("time-A"));
  do_check_true(fh.nameExists("time-B"));
  do_check_false(fh.nameExists("time-C"));
  do_check_true(fh.nameExists("time-D"));

  
  
  testnum++;
  fh.removeEntriesByTimeframe(1000, 2000);
  do_check_false(fh.nameExists("time-A"));
  do_check_false(fh.nameExists("time-B"));
  do_check_false(fh.nameExists("time-C"));
  do_check_true(fh.nameExists("time-D"));

  
  
  testnum++;
  fh.removeAllEntries();
  do_check_false(fh.hasEntries);
  do_check_false(fh.nameExists("name-C"));
  do_check_false(fh.nameExists("name-D"));
  do_check_false(fh.entryExists("name-C", "value-C"));
  do_check_false(fh.entryExists("name-D", "value-D"));

  
  
  testnum++;
  fh.addEntry("newname-A", "newvalue-A");
  do_check_true(fh.hasEntries);
  do_check_true(fh.entryExists("newname-A", "newvalue-A"));

  
  
  testnum++;
  fh.removeEntry("newname-A", "newvalue-A");
  do_check_false(fh.hasEntries);
  do_check_false(fh.entryExists("newname-A", "newvalue-A"));

  } catch (e) {
    throw "FAILED in test #" + testnum + " -- " + e;
  }
}
