



var testnum = 0;
var fh;

function run_test()
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

  fh = Cc["@mozilla.org/satchel/form-history;1"].
       getService(Ci.nsIFormHistory2);


  
  testnum++;

  
  do_check_true(fh.DBConnection.tableExists("moz_deleted_formhistory"));
  
  do_check_eq(CURRENT_SCHEMA, fh.DBConnection.schemaVersion);
  
  do_check_true(fh.entryExists("name-A", "value-A"));

  } catch (e) {
    throw "FAILED in test #" + testnum + " -- " + e;
  }
}
