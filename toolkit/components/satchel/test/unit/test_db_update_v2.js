




































var testnum = 0;
var fh;

function run_test()
{
  try {

  
  var testfile = do_get_file("formhistory_v1.sqlite");
  var profileDir = dirSvc.get("ProfD", Ci.nsIFile);

  
  var destFile = profileDir.clone();
  destFile.append("formhistory.sqlite");
  if (destFile.exists())
    destFile.remove(false);

  testfile.copyTo(profileDir, "formhistory.sqlite");
  do_check_eq(1, getDBVersion(testfile));

  fh = Cc["@mozilla.org/satchel/form-history;1"].
       getService(Ci.nsIFormHistory2);


  
  testnum++;

  
  do_check_true(fh.DBConnection.indexExists("moz_formhistory_lastused_index"));
  
  do_check_eq(CURRENT_SCHEMA, fh.DBConnection.schemaVersion);
  
  do_check_false(fh.DBConnection.tableExists("moz_dummy_table"));


  
  testnum++;

  
  do_check_true(fh.entryExists("name-A", "value-A"));
  do_check_false(fh.entryExists("name-B", "value-B"));
  fh.addEntry("name-B", "value-B");
  do_check_true(fh.entryExists("name-B", "value-B"));
  fh.removeEntry("name-B", "value-B");
  do_check_false(fh.entryExists("name-B", "value-B"));


  } catch (e) {
    throw "FAILED in test #" + testnum + " -- " + e;
  }
}
