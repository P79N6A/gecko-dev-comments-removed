











































const PREF_SMART_BOOKMARKS_VERSION = "browser.places.smartBookmarksVersion";
const PREF_BMPROCESSED = "distribution.516444.bookmarksProcessed";
const PREF_DISTRIBUTION_ID = "distribution.id";

const TOPIC_FINAL_UI_STARTUP = "final-ui-startup";
const TOPIC_CUSTOMIZATION_COMPLETE = "distribution-customization-complete";

function run_test() {
  
  
  let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
  
  return;

  do_test_pending();

  
  let distroDir = Services.dirsvc.get("XCurProcD", Ci.nsIFile);
  distroDir.append("distribution");
  let iniFile = distroDir.clone();
  iniFile.append("distribution.ini");
  if (iniFile.exists()) {
    iniFile.remove(false);
    print("distribution.ini already exists, did some test forget to cleanup?");
  }

  let testDistributionFile = gTestDir.clone();
  testDistributionFile.append("distribution.ini");
  testDistributionFile.copyTo(distroDir, "distribution.ini");
  do_check_true(testDistributionFile.exists());

  
  let ps = Cc["@mozilla.org/preferences-service;1"].
           getService(Ci.nsIPrefBranch);
  ps.setIntPref(PREF_SMART_BOOKMARKS_VERSION, -1);
  
  ps.setIntPref("browser.migration.version", 4);

  
  let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  
  
  do_check_eq(hs.databaseStatus, hs.DATABASE_STATUS_CREATE);

  
  let bg = Cc["@mozilla.org/browser/browserglue;1"].
           getService(Ci.nsIBrowserGlue);

  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  let observer = {
    observe: function(aSubject, aTopic, aData) {
      os.removeObserver(this, PlacesUtils.TOPIC_INIT_COMPLETE);

      
      bg.QueryInterface(Ci.nsIObserver).observe(null,
                                                TOPIC_FINAL_UI_STARTUP,
                                                null);
      
      let cObserver = {
        observe: function(aSubject, aTopic, aData) {
          os.removeObserver(this, TOPIC_CUSTOMIZATION_COMPLETE);
          do_execute_soon(continue_test);
        }
      }
      os.addObserver(cObserver, TOPIC_CUSTOMIZATION_COMPLETE, false);
    }
  }
  os.addObserver(observer, PlacesUtils.TOPIC_INIT_COMPLETE, false);
}

function continue_test() {
  let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);

  dump_table("moz_bookmarks");

  
  let menuItemId = bs.getIdForItemAt(bs.bookmarksMenuFolder, 0);
  do_check_neq(menuItemId, -1);
  do_check_eq(bs.getItemTitle(menuItemId), "Menu Link Before");
  menuItemId = bs.getIdForItemAt(bs.bookmarksMenuFolder, 1 + DEFAULT_BOOKMARKS_ON_MENU);
  do_check_neq(menuItemId, -1);
  do_check_eq(bs.getItemTitle(menuItemId), "Menu Link After");

  
  let toolbarItemId = bs.getIdForItemAt(bs.toolbarFolder, 0);
  do_check_neq(toolbarItemId, -1);
  do_check_eq(bs.getItemTitle(toolbarItemId), "Toolbar Link Before");
  toolbarItemId = bs.getIdForItemAt(bs.toolbarFolder, 1 + DEFAULT_BOOKMARKS_ON_TOOLBAR);
  do_check_neq(toolbarItemId, -1);
  do_check_eq(bs.getItemTitle(toolbarItemId), "Toolbar Link After");

  
  let ps = Cc["@mozilla.org/preferences-service;1"].
           getService(Ci.nsIPrefBranch);
  do_check_true(ps.getBoolPref(PREF_BMPROCESSED));

  
  do_check_eq(ps.getCharPref(PREF_DISTRIBUTION_ID), "516444");

  do_test_finished();
}

do_register_cleanup(function() {
  
  
  let iniFile = Services.dirsvc.get("XCurProcD", Ci.nsIFile);
  iniFile.append("distribution");
  iniFile.append("distribution.ini");
  if (iniFile.exists())
    iniFile.remove(false);
  do_check_false(iniFile.exists());
});
