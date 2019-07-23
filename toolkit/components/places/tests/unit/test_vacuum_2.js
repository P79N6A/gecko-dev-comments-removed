






































const LAST_VACUUM_PREF = "last_vacuum";
const VACUUM_THRESHOLD = 0.1;
const PLACES_VACUUM_STARTING_TOPIC = "places-vacuum-starting";

function getDBVacuumRatio() {
  let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  let dbConn = hs.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;
  let freelistStmt = dbConn.createStatement("PRAGMA freelist_count");
  freelistStmt.executeStep();
  let freelistCount = freelistStmt.row.freelist_count;
  freelistStmt.finalize();
  let pageCountStmt = dbConn.createStatement("PRAGMA page_count");
  pageCountStmt.executeStep();
  let pageCount = pageCountStmt.row.page_count;
  pageCountStmt.finalize();

  let ratio = (freelistCount / pageCount);
  return ratio;
}


var gTests = [
  {
    desc: "High ratio, last vacuum today",
    ratio: "high",
    elapsedDays: 0,
    vacuum: false
  },
  {
    desc: "High ratio, last vacuum two months ago",
    ratio: "high",
    elapsedDays: 61,
    vacuum: true
  },
];

function run_test() {
  let sparseDB = do_get_file("places.sparse.sqlite");
  sparseDB.copyTo(do_get_profile(), "places.sqlite");

  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  let observer = {
    vacuum: false,
    observe: function(aSubject, aTopic, aData) {
      if (aTopic == PLACES_VACUUM_STARTING_TOPIC) {
        this.vacuum = true;
      }
    }
  }
  os.addObserver(observer, PLACES_VACUUM_STARTING_TOPIC, false);

  let prefs = Cc["@mozilla.org/preferences-service;1"].
              getService(Ci.nsIPrefService).
              getBranch("places.");

  while (gTests.length) {
    observer.vacuum = false;
    let test = gTests.shift();
    print("PLACES TEST: " + test.desc);
    let ratio = getDBVacuumRatio();
    if (test.ratio == "high") {
      do_check_true(getDBVacuumRatio() > VACUUM_THRESHOLD);
    }
    else if (test.ratio == "low") {
      do_throw("This test only supports high ratio cases");
    }
    print("current ratio is " + getDBVacuumRatio());
    prefs.setIntPref(LAST_VACUUM_PREF, ((Date.now() / 1000) - (test.elapsedDays * 24 * 60 * 60)));
    os.notifyObservers(null, "idle-daily", null);
    let testFunc = test.vacuum ? do_check_true : do_check_false;
    testFunc(observer.vacuum);
  }

  os.removeObserver(observer, PLACES_VACUUM_STARTING_TOPIC);
}
