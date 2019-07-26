



var dbFile, oldSize;
var currentTestIndex = 0;

function triggerExpiration() {
  
  
  
  Services.obs.notifyObservers(null, "formhistory-expire-now", null);
}

let checkExists = function(num) { do_check_true(num > 0); next_test(); }
let checkNotExists = function(num) { do_check_true(!num); next_test(); }

var TestObserver = {
  QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupportsWeakReference]),

  observe : function (subject, topic, data) {
    do_check_eq(topic, "satchel-storage-changed");

    if (data == "formhistory-expireoldentries") {
      next_test();
    }
  }
};

function test_finished() {
  
  if (Services.prefs.prefHasUserValue("browser.formfill.expire_days"))
    Services.prefs.clearUserPref("browser.formfill.expire_days");

  do_test_finished();
}

let iter = tests();

function run_test()
{
  do_test_pending();
  iter.next();
}

function next_test()
{
  iter.next();
}

function tests()
{
  Services.obs.addObserver(TestObserver, "satchel-storage-changed", true);

  
  var testfile = do_get_file("asyncformhistory_expire.sqlite");
  var profileDir = do_get_profile();

  
  dbFile = profileDir.clone();
  dbFile.append("formhistory.sqlite");
  if (dbFile.exists())
    dbFile.remove(false);

  testfile.copyTo(profileDir, "formhistory.sqlite");
  do_check_true(dbFile.exists());

  
  do_check_false(Services.prefs.prefHasUserValue("browser.formfill.expire_days"));

  
  yield countEntries(null, null, function(num) { do_check_eq(508, num); next_test(); });
  yield countEntries("name-A", "value-A", checkExists); 
  yield countEntries("name-B", "value-B", checkExists); 

  do_check_eq(CURRENT_SCHEMA, FormHistory.schemaVersion);

  
  yield countEntries("name-C", "value-C", checkNotExists);
  yield addEntry("name-C", "value-C", next_test);
  yield countEntries("name-C", "value-C", checkExists);

  
  var now = 1000 * Date.now();
  let updateLastUsed = function updateLastUsedFn(results, age)
  {
    let lastUsed = now - age * 24 * PR_HOURS;

    let changes = [ ];
    for (let r = 0; r < results.length; r++) {
      changes.push({ op: "update", lastUsed: lastUsed, guid: results[r].guid });
    }

    return changes;
  }

  let results = yield searchEntries(["guid"], { lastUsed: 181 }, iter);
  yield updateFormHistory(updateLastUsed(results, 181), next_test);

  results = yield searchEntries(["guid"], { lastUsed: 179 }, iter);
  yield updateFormHistory(updateLastUsed(results, 179), next_test);

  results = yield searchEntries(["guid"], { lastUsed: 31 }, iter);
  yield updateFormHistory(updateLastUsed(results, 31), next_test);

  results = yield searchEntries(["guid"], { lastUsed: 29 }, iter);
  yield updateFormHistory(updateLastUsed(results, 29), next_test);

  results = yield searchEntries(["guid"], { lastUsed: 9999 }, iter);
  yield updateFormHistory(updateLastUsed(results, 11), next_test);

  results = yield searchEntries(["guid"], { lastUsed: 9 }, iter);
  yield updateFormHistory(updateLastUsed(results, 9), next_test);

  yield countEntries("name-A", "value-A", checkExists);
  yield countEntries("181DaysOld", "foo", checkExists);
  yield countEntries("179DaysOld", "foo", checkExists);
  yield countEntries(null, null, function(num) { do_check_eq(509, num); next_test(); });

  
  triggerExpiration();
  yield;

  yield countEntries("name-A", "value-A", checkNotExists);
  yield countEntries("181DaysOld", "foo", checkNotExists);
  yield countEntries("179DaysOld", "foo", checkExists);
  yield countEntries(null, null, function(num) { do_check_eq(507, num); next_test(); });

  
  triggerExpiration();
  yield;

  yield countEntries(null, null, function(num) { do_check_eq(507, num); next_test(); });

  
  Services.prefs.setIntPref("browser.formfill.expire_days", 30);
  yield countEntries("179DaysOld", "foo", checkExists);
  yield countEntries("bar", "31days", checkExists);
  yield countEntries("bar", "29days", checkExists);
  yield countEntries(null, null, function(num) { do_check_eq(507, num); next_test(); });

  triggerExpiration();
  yield;

  yield countEntries("179DaysOld", "foo", checkNotExists);
  yield countEntries("bar", "31days", checkNotExists);
  yield countEntries("bar", "29days", checkExists);
  yield countEntries(null, null, function(num) { do_check_eq(505, num); next_test(); });

  
  
  Services.prefs.setIntPref("browser.formfill.expire_days", 10);

  yield countEntries("bar", "29days", checkExists);
  yield countEntries("9DaysOld", "foo", checkExists);
  yield countEntries(null, null, function(num) { do_check_eq(505, num); next_test(); });

  triggerExpiration();
  yield;

  yield countEntries("bar", "29days", checkNotExists);
  yield countEntries("9DaysOld", "foo", checkExists);
  yield countEntries("name-B", "value-B", checkExists);
  yield countEntries("name-C", "value-C", checkExists);
  yield countEntries(null, null, function(num) { do_check_eq(3, num); next_test(); });

  test_finished();
};
