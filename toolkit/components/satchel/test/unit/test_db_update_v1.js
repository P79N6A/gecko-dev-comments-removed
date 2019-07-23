





































function is_about_now(timestamp) {
  var delta = Math.abs(timestamp - 1000 * Date.now());
  var seconds = 30 * 1000000;
  return delta < seconds;
}

var testnum = 0;
var fh;
var timesUsed, firstUsed, lastUsed;

function run_test()
{
  try {

  
  var testfile = do_get_file("formhistory_v0.sqlite");
  var profileDir = dirSvc.get("ProfD", Ci.nsIFile);

  
  var destFile = profileDir.clone();
  destFile.append("formhistory.sqlite");
  if (destFile.exists())
    destFile.remove(false);

  testfile.copyTo(profileDir, "formhistory.sqlite");
  do_check_eq(0, getDBVersion(testfile));

  fh = Cc["@mozilla.org/satchel/form-history;1"].
       getService(Ci.nsIFormHistory2);


  
  testnum++;
  
  do_check_true(fh.entryExists("name-A", "value-A"));
  do_check_true(fh.entryExists("name-B", "value-B"));
  do_check_true(fh.entryExists("name-C", "value-C1"));
  do_check_true(fh.entryExists("name-C", "value-C2"));
  
  do_check_eq(CURRENT_SCHEMA, fh.DBConnection.schemaVersion);


  
  testnum++;
  

  var query = "SELECT timesUsed, firstUsed, lastUsed " +
              "FROM moz_formhistory WHERE fieldname = 'name-A'";
  var stmt = fh.DBConnection.createStatement(query);
  stmt.executeStep();

  timesUsed = stmt.getInt32(0);
  firstUsed = stmt.getInt64(1);
  lastUsed  = stmt.getInt64(2);

  do_check_eq(1, timesUsed);
  do_check_true(firstUsed == lastUsed);
  
  do_check_true(is_about_now(firstUsed + 24 * PR_HOURS));


  
  testnum++;
  
  do_check_false(fh.entryExists("name-D", "value-D"));
  fh.addEntry("name-D", "value-D");
  do_check_true(fh.entryExists("name-D", "value-D"));
  fh.removeEntry("name-D", "value-D");
  do_check_false(fh.entryExists("name-D", "value-D"));


  
  testnum++;
  
  do_check_false(fh.entryExists("name-E", "value-E"));
  fh.addEntry("name-E", "value-E");
  do_check_true(fh.entryExists("name-E", "value-E"));

  query = "SELECT timesUsed, firstUsed, lastUsed " +
          "FROM moz_formhistory WHERE fieldname = 'name-E'";
  stmt = fh.DBConnection.createStatement(query);
  stmt.executeStep();

  timesUsed = stmt.getInt32(0);
  firstUsed = stmt.getInt64(1);
  lastUsed  = stmt.getInt64(2);

  do_check_eq(1, timesUsed);
  do_check_true(firstUsed == lastUsed);
  do_check_true(is_about_now(firstUsed));

  
  
  
  
  
  do_test_pending();
  do_timeout(50, delayed_test);

  } catch (e) {
    throw "FAILED in test #" + testnum + " -- " + e;
  }
}

function delayed_test() {
  try {

  
  testnum++;
  
  do_check_true(fh.entryExists("name-E", "value-E"));
  fh.addEntry("name-E", "value-E");
  do_check_true(fh.entryExists("name-E", "value-E"));

  var query = "SELECT timesUsed, firstUsed, lastUsed " +
              "FROM moz_formhistory WHERE fieldname = 'name-E'";
  var stmt = fh.DBConnection.createStatement(query);
  stmt.executeStep();

  timesUsed = stmt.getInt32(0);
  var firstUsed2 = stmt.getInt64(1);
  var lastUsed2  = stmt.getInt64(2);

  do_check_eq(2, timesUsed);
  do_check_true(is_about_now(lastUsed2));

  do_check_true(firstUsed  == firstUsed2); 
  do_check_true(lastUsed   != lastUsed2);  
  do_check_true(firstUsed2 != lastUsed2);

  do_test_finished();
  } catch (e) {
    throw "FAILED in test #" + testnum + " -- " + e;
  }
}
