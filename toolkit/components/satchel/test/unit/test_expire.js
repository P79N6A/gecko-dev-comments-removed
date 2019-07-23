




































var testnum = 0;
var fh, prefs;

function countAllEntries() {
    let stmt = fh.DBConnection.createStatement("SELECT COUNT(*) as numEntries FROM moz_formhistory");
    do_check_true(stmt.executeStep());
    let numEntries = stmt.row.numEntries;
    stmt.finalize();
    return numEntries;
}

function triggerExpiration() {
    
    
    
    var os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
    os.notifyObservers(null, "formhistory-expire-now", null);
}

function run_test()
{
  try {

  
  var testfile = do_get_file("formhistory_expire.sqlite");
  var profileDir = dirSvc.get("ProfD", Ci.nsIFile);

  
  var dbFile = profileDir.clone();
  dbFile.append("formhistory.sqlite");
  if (dbFile.exists())
    dbFile.remove(false);

  testfile.copyTo(profileDir, "formhistory.sqlite");
  do_check_true(dbFile.exists());

  fh = Cc["@mozilla.org/satchel/form-history;1"].
       getService(Ci.nsIFormHistory2);

  prefs = Cc["@mozilla.org/preferences-service;1"].
          getService(Ci.nsIPrefBranch);

  
  do_check_false(prefs.prefHasUserValue("browser.history_expire_days"));
  do_check_false(prefs.prefHasUserValue("browser.formfill.expire_days"));


  
  testnum++;

  
  do_check_eq(CURRENT_SCHEMA, fh.DBConnection.schemaVersion);
  do_check_eq(508, countAllEntries());
  do_check_true(fh.entryExists("name-A", "value-A")); 
  do_check_true(fh.entryExists("name-B", "value-B")); 

  
  do_check_false(fh.entryExists("name-C", "value-C"));
  fh.addEntry("name-C", "value-C");
  do_check_true(fh.entryExists("name-C", "value-C"));

  
  var now = 1000 * Date.now();
  var age181 = now - 181 * 24 * PR_HOURS;
  var age179 = now - 179 * 24 * PR_HOURS;
  var age31  = now -  31 * 24 * PR_HOURS;
  var age29  = now -  29 * 24 * PR_HOURS;
  var age11  = now -  11 * 24 * PR_HOURS;
  var age9   = now -   9 * 24 * PR_HOURS;

  fh.DBConnection.executeSimpleSQL("UPDATE moz_formhistory SET lastUsed=" + age181 + " WHERE lastUsed=181");
  fh.DBConnection.executeSimpleSQL("UPDATE moz_formhistory SET lastUsed=" + age179 + " WHERE lastUsed=179");
  fh.DBConnection.executeSimpleSQL("UPDATE moz_formhistory SET lastUsed=" + age31  + " WHERE lastUsed=31");
  fh.DBConnection.executeSimpleSQL("UPDATE moz_formhistory SET lastUsed=" + age29  + " WHERE lastUsed=29");
  fh.DBConnection.executeSimpleSQL("UPDATE moz_formhistory SET lastUsed=" + age11  + " WHERE lastUsed=9999");
  fh.DBConnection.executeSimpleSQL("UPDATE moz_formhistory SET lastUsed=" + age9   + " WHERE lastUsed=9");


  
  testnum++;

  
  do_check_true(fh.entryExists("name-A", "value-A"));
  do_check_true(fh.entryExists("181DaysOld", "foo"));
  do_check_true(fh.entryExists("179DaysOld", "foo"));
  do_check_eq(509, countAllEntries());

  
  triggerExpiration();

  do_check_false(fh.entryExists("name-A", "value-A"));
  do_check_false(fh.entryExists("181DaysOld", "foo"));
  do_check_true(fh.entryExists("179DaysOld", "foo"));
  do_check_eq(507, countAllEntries());


  
  testnum++;

  
  triggerExpiration();
  do_check_eq(507, countAllEntries());


  
  testnum++;

  
  
  
  if ("nsIMsgFolder" in Ci)
    prefs.setIntPref("browser.formfill.expire_days", 30);
  
  prefs.setIntPref("browser.history_expire_days", 30);
  do_check_true(fh.entryExists("179DaysOld", "foo"));
  do_check_true(fh.entryExists("bar", "31days"));
  do_check_true(fh.entryExists("bar", "29days"));
  do_check_eq(507, countAllEntries());

  triggerExpiration();

  do_check_false(fh.entryExists("179DaysOld", "foo"));
  do_check_false(fh.entryExists("bar", "31days"));
  do_check_true(fh.entryExists("bar", "29days"));
  do_check_eq(505, countAllEntries());


  
  testnum++;

  
  
  prefs.setIntPref("browser.formfill.expire_days", 10);
  prefs.setIntPref("browser.history_expire_days", 1);

  do_check_true(fh.entryExists("bar", "29days"));
  do_check_true(fh.entryExists("9DaysOld", "foo"));
  do_check_eq(505, countAllEntries());

  do_check_true(dbFile.fileSize > 70000);

  triggerExpiration();

  do_check_false(fh.entryExists("bar", "29days"));
  do_check_true(fh.entryExists("9DaysOld", "foo"));
  do_check_true(fh.entryExists("name-B", "value-B"));
  do_check_true(fh.entryExists("name-C", "value-C"));
  do_check_eq(3, countAllEntries());

  
  
  dbFile = dbFile.clone();
  do_check_true(dbFile.fileSize < 6000);


  } catch (e) {
      throw "FAILED in test #" + testnum + " -- " + e;
  } finally {
      
      if (prefs.prefHasUserValue("browser.history_expire_days"))
        prefs.clearUserPref("browser.history_expire_days");
      if (prefs.prefHasUserValue("browser.formfill.expire_days"))
        prefs.clearUserPref("browser.formfill.expire_days");
  }
}
