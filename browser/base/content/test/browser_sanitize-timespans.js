
var now_uSec = Date.now() * 1000;

const dm = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
const bhist = Cc["@mozilla.org/browser/global-history;2"].getService(Ci.nsIBrowserHistory);
const iosvc = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

Cc["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader)
                                           .loadSubScript("chrome://browser/content/sanitize.js");

function test() {
  
  var hoursSinceMidnight = new Date().getHours();

  setupHistory();
  setupDownloads();
  
  
  
  
  let s = new Sanitizer();
  s.ignoreTimespan = false;
  s.prefDomain = "privacy.cpd.";
  var itemPrefs = Cc["@mozilla.org/preferences-service;1"]
                  .getService(Components.interfaces.nsIPrefService)
                  .getBranch(s.prefDomain);
  itemPrefs.setBoolPref("history", true);
  itemPrefs.setBoolPref("downloads", true);
  itemPrefs.setBoolPref("cache", false);
  itemPrefs.setBoolPref("cookies", false);
  itemPrefs.setBoolPref("formdata", false);
  itemPrefs.setBoolPref("offlineApps", false);
  itemPrefs.setBoolPref("passwords", false);
  itemPrefs.setBoolPref("sessions", false);
  itemPrefs.setBoolPref("siteprefs", false);
  
  
  Sanitizer.prefs.setIntPref("timeSpan", 1);
  s.sanitize();
  
  ok(!bhist.isVisited(uri("http://1hour.com")), "1hour.com should now be deleted");
  ok(bhist.isVisited(uri("http://2hour.com")), "Pretend visit to 2hour.com should still exist");
  ok(bhist.isVisited(uri("http://4hour.com")), "Pretend visit to 4hour.com should still exist");
  
  if(hoursSinceMidnight > 1)
    ok(bhist.isVisited(uri("http://today.com")), "Pretend visit to today.com should still exist");
  ok(bhist.isVisited(uri("http://before-today.com")), "Pretend visit to before-today.com should still exist");
  
  ok(!downloadExists(5555551), "<1 hour download should now be deleted");
  ok(downloadExists(5555550), "Year old download should still be present");
  ok(downloadExists(5555552), "<2 hour old download should still be present");
  ok(downloadExists(5555553), "<4 hour old download should still be present");

  if(hoursSinceMidnight > 1)
    ok(downloadExists(5555554), "'Today' download should still be present");
  
  
  Sanitizer.prefs.setIntPref("timeSpan", 2);
  s.sanitize();
  
  ok(!bhist.isVisited(uri("http://2hour.com")), "Pretend visit to 2hour.com should now be deleted");
  ok(bhist.isVisited(uri("http://4hour.com")), "Pretend visit to 4hour.com should still exist");
  if(hoursSinceMidnight > 2)
    ok(bhist.isVisited(uri("http://today.com")), "Pretend visit to today.com should still exist");
  ok(bhist.isVisited(uri("http://before-today.com")), "Pretend visit to before-today.com should still exist");
  
  ok(!downloadExists(5555552), "<2 hour old download should now be deleted");
  ok(downloadExists(5555550), "Year old download should still be present");
  ok(downloadExists(5555553), "<4 hour old download should still be present");
  if(hoursSinceMidnight > 2)
    ok(downloadExists(5555554), "'Today' download should still be present");
  
  
  Sanitizer.prefs.setIntPref("timeSpan", 3);
  s.sanitize();
  
  ok(!bhist.isVisited(uri("http://4hour.com")), "Pretend visit to 4hour.com should now be deleted");
  if(hoursSinceMidnight > 4)
    ok(bhist.isVisited(uri("http://today.com")), "Pretend visit to today.com should still exist");
  ok(bhist.isVisited(uri("http://before-today.com")), "Pretend visit to before-today.com should still exist");
  
  ok(!downloadExists(5555553), "<4 hour old download should now be deleted");
  ok(downloadExists(5555550), "Year old download should still be present");
  if(hoursSinceMidnight > 4)
    ok(downloadExists(5555554), "'Today' download should still be present");

  
  Sanitizer.prefs.setIntPref("timeSpan", 4);
  s.sanitize();
  
  ok(!bhist.isVisited(uri("http://today.com")), "Pretend visit to today.com should now be deleted");
  ok(bhist.isVisited(uri("http://before-today.com")), "Pretend visit to before-today.com should still exist");

  ok(!downloadExists(5555554), "'Today' download should now be deleted");
  ok(downloadExists(5555550), "Year old download should still be present");

  
  Sanitizer.prefs.setIntPref("timeSpan", 0);
  s.sanitize();
  
  ok(!bhist.isVisited(uri("http://before-today.com")), "Pretend visit to before-today.com should now be deleted");
  
  ok(!downloadExists(5555550), "Year old download should now be deleted");

}

function setupHistory() {
  bhist.addPageWithDetails(uri("http://1hour.com/"), "Less than 1 hour ago", now_uSec - 45*60*1000000);
  bhist.addPageWithDetails(uri("http://2hour.com/"), "Less than 2 hours ago", now_uSec - 90*60*1000000);
  bhist.addPageWithDetails(uri("http://4hour.com/"), "Less than 4 hours ago", now_uSec - 180*60*1000000);
  
  let today = new Date();
  today.setHours(0);
  today.setMinutes(0);
  today.setSeconds(30);
  bhist.addPageWithDetails(uri("http://today.com/"), "Today", today.valueOf() * 1000);
  
  let lastYear = new Date();
  lastYear.setFullYear(lastYear.year - 1);
  bhist.addPageWithDetails(uri("http://before-today.com/"), "Before Today", lastYear.valueOf() * 1000);
  
  
  ok(bhist.isVisited(uri("http://1hour.com")), "Pretend visit to 1hour.com should exist");
  ok(bhist.isVisited(uri("http://2hour.com")), "Pretend visit to 2hour.com should exist");
  ok(bhist.isVisited(uri("http://4hour.com")), "Pretend visit to 4hour.com should exist");
  ok(bhist.isVisited(uri("http://today.com")), "Pretend visit to today.com should exist");
  ok(bhist.isVisited(uri("http://before-today.com")), "Pretend visit to before-today.com should exist");
}

function setupDownloads() {

  
  let data = {
    id:   "5555551",
    name: "fakefile-1-hour",
    source: "https://bugzilla.mozilla.org/show_bug.cgi?id=453440",
    target: "fakefile-1-hour",
    startTime: now_uSec - 45*60*1000000,  
    endTime: now_uSec - 44*60*1000000, 
    state: Ci.nsIDownloadManager.DOWNLOAD_FINISHED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0
  };

  let db = dm.DBConnection;
  let stmt = db.createStatement(
    "INSERT INTO moz_downloads (id, name, source, target, startTime, endTime, " +
      "state, currBytes, maxBytes, preferredAction, autoResume) " +
    "VALUES (:id, :name, :source, :target, :startTime, :endTime, :state, " +
      ":currBytes, :maxBytes, :preferredAction, :autoResume)");
  try {
    for (let prop in data)
      stmt.params[prop] = data[prop];
    stmt.execute();
  }
  finally {
    stmt.reset();
  }
  
  
  data = {
    id:   "5555552",
    name: "fakefile-2-hour",
    source: "https://bugzilla.mozilla.org/show_bug.cgi?id=453440",
    target: "fakefile-2-hour",
    startTime: now_uSec - 90*60*1000000,  
    endTime: now_uSec - 89*60*1000000, 
    state: Ci.nsIDownloadManager.DOWNLOAD_FINISHED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0
  };

  try {
    for (let prop in data)
      stmt.params[prop] = data[prop];
    stmt.execute();
  }
  finally {
    stmt.reset();
  }
  
  
  data = {
    id:   "5555553",
    name: "fakefile-4-hour",
    source: "https://bugzilla.mozilla.org/show_bug.cgi?id=453440",
    target: "fakefile-4-hour",
    startTime: now_uSec - 180*60*1000000,  
    endTime: now_uSec - 179*60*1000000, 
    state: Ci.nsIDownloadManager.DOWNLOAD_FINISHED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0
  };
  
  try {
    for (let prop in data)
      stmt.params[prop] = data[prop];
    stmt.execute();
  }
  finally {
    stmt.reset();
  }
  
  
  let today = new Date();
  today.setHours(0);
  today.setMinutes(0);
  today.setSeconds(30);
  
  data = {
    id:   "5555554",
    name: "fakefile-today",
    source: "https://bugzilla.mozilla.org/show_bug.cgi?id=453440",
    target: "fakefile-today",
    startTime: today.valueOf() * 1000,  
    endTime: (today.valueOf() + 1000) * 1000, 
    state: Ci.nsIDownloadManager.DOWNLOAD_FINISHED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0
  };
  
  try {
    for (let prop in data)
      stmt.params[prop] = data[prop];
    stmt.execute();
  }
  finally {
    stmt.reset();
  }
  
  
  let lastYear = new Date();
  lastYear.setFullYear(lastYear.year - 1);
  data = {
    id:   "5555550",
    name: "fakefile-old",
    source: "https://bugzilla.mozilla.org/show_bug.cgi?id=453440",
    target: "fakefile-old",
    startTime: lastYear.valueOf() * 1000,  
    endTime: (lastYear.valueOf() + 1000) * 1000, 
    state: Ci.nsIDownloadManager.DOWNLOAD_FINISHED,
    currBytes: 0, maxBytes: -1, preferredAction: 0, autoResume: 0
  };
  
  try {
    for (let prop in data)
      stmt.params[prop] = data[prop];
    stmt.execute();
  }
  finally {
    stmt.finalize();
  }
  
  
  ok(downloadExists(5555550), "Pretend download for everything case should exist");
  ok(downloadExists(5555551), "Pretend download for 1-hour case should exist");
  ok(downloadExists(5555552), "Pretend download for 2-hour case should exist");
  ok(downloadExists(5555553), "Pretend download for 4-hour case should exist");
  ok(downloadExists(5555554), "Pretend download for Today case should exist");
}







function downloadExists(aID)
{
  let db = dm.DBConnection;
  let stmt = db.createStatement(
    "SELECT * " +
    "FROM moz_downloads " +
    "WHERE id = :id"
  );
  stmt.params.id = aID;
  var rows = stmt.step();
  stmt.finalize();
  return rows;
}

function uri(spec) {
  return iosvc.newURI(spec, null, null);
}
