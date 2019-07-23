






































let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
let bh = hs.QueryInterface(Ci.nsIBrowserHistory);
let dbConn = hs.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;
let os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
let prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService).
            getBranch("places.");

const LAST_VACUUM_PREF = "last_vacuum";
const VACUUM_THRESHOLD = 0.1;
const PLACES_VACUUM_STARTING_TOPIC = "places-vacuum-starting";

function getDBVacuumRatio() {
  let freelistStmt = dbConn.createStatement("PRAGMA freelist_count");
  freelistStmt.step();
  let freelistCount = freelistStmt.row.freelist_count;
  freelistStmt.finalize();
  let pageCountStmt = dbConn.createStatement("PRAGMA page_count");
  pageCountStmt.step();
  let pageCount = pageCountStmt.row.page_count;
  pageCountStmt.finalize();

  let ratio = (freelistCount / pageCount);
  return ratio;
}

function generateSparseDB(aType) {
  let limit = 0;
  if (aType == "low")
    limit = 10;
  else if (aType == "high")
    limit = 200;

  let batch = {
    runBatched: function batch_runBatched() {
      for (let i = 0; i < limit; i++) {
        hs.addVisit(uri("http://" + i + ".mozilla.com/"),
                    Date.now() * 1000 + i, null, hs.TRANSITION_TYPED, false, 0);
      }
      for (let i = 0; i < limit; i++) {
        bs.insertBookmark(bs.unfiledBookmarksFolder,
                          uri("http://" + i + "." + i + ".mozilla.com/"),
                          bs.DEFAULT_INDEX, "bookmark " + i);
      }
    }
  }
  hs.runInBatchMode(batch, null);
  bh.removeAllPages();
  bs.removeFolderChildren(bs.unfiledBookmarksFolder);
}

var gTests = [
  {
    desc: "Low ratio, last vacuum today",
    ratio: "low",
    elapsedDays: 0,
    vacuum: false
  },

  {
    desc: "Low ratio, last vacuum one month ago",
    ratio: "low",
    elapsedDays: 31,
    vacuum: false
  },

  {
    desc: "Low ratio, last vacuum two months ago",
    ratio: "low",
    elapsedDays: 61,
    vacuum: true
  },

  {
    desc: "High ratio, last vacuum today",
    ratio: "high",
    elapsedDays: 0,
    vacuum: false
  },
];

var observer = {
  vacuum: false,
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == PLACES_VACUUM_STARTING_TOPIC) {
      this.vacuum = true;
    }
  }
}
os.addObserver(observer, PLACES_VACUUM_STARTING_TOPIC, false);

function run_test() {
  
  return;

  while (gTests.length) {
    observer.vacuum = false;
    let test = gTests.shift();
    print("PLACES TEST: " + test.desc);
    let ratio = getDBVacuumRatio();
    if (test.ratio == "high") {
      if (ratio < VACUUM_THRESHOLD)
        generateSparseDB("high");
      do_check_true(getDBVacuumRatio() > VACUUM_THRESHOLD);
    }
    else if (test.ratio == "low") {
      if (ratio == 0)
        generateSparseDB("low");
      do_check_true(getDBVacuumRatio() < VACUUM_THRESHOLD);
    }
    print("current ratio is " + getDBVacuumRatio());
    prefs.setIntPref(LAST_VACUUM_PREF, ((Date.now() / 1000) - (test.elapsedDays * 24 * 60 * 60)));
    os.notifyObservers(null, "idle-daily", null);
    let testFunc = test.vacuum ? do_check_true : do_check_false;
    testFunc(observer.vacuum);
  }

  os.removeObserver(observer, PLACES_VACUUM_STARTING_TOPIC);
}
