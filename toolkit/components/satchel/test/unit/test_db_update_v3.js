




































var testnum = 0;
var fh;

function run_test()
{
  try {

  
  var testfile = do_get_file("formhistory_v2.sqlite");
  var profileDir = dirSvc.get("ProfD", Ci.nsIFile);

  
  var destFile = profileDir.clone();
  destFile.append("formhistory.sqlite");
  if (destFile.exists())
    destFile.remove(false);

  testfile.copyTo(profileDir, "formhistory.sqlite");
  do_check_eq(2, getDBVersion(testfile));

  fh = Cc["@mozilla.org/satchel/form-history;1"].
       getService(Ci.nsIFormHistory2);


  
  testnum++;

  
  do_check_true(fh.DBConnection.indexExists("moz_formhistory_guid_index"));
  
  do_check_eq(CURRENT_SCHEMA, fh.DBConnection.schemaVersion);

  do_check_true(fh.entryExists("name-A", "value-A"));
  var guid = getGUIDforID(fh.DBConnection, 1);
  do_check_true(isGUID.test(guid));

  
  do_check_false(fh.entryExists("name-B", "value-B"));
  fh.addEntry("name-B", "value-B");
  do_check_true(fh.entryExists("name-B", "value-B"));

  guid = getGUIDforID(fh.DBConnection, 2);
  do_check_true(isGUID.test(guid));

  } catch (e) {
    throw "FAILED in test #" + testnum + " -- " + e;
  }
}
