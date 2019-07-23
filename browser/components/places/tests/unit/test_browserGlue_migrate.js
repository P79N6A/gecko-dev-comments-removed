











































const PREF_SMART_BOOKMARKS_VERSION = "browser.places.smartBookmarksVersion";

const TOPIC_PLACES_INIT_COMPLETE = "places-init-complete";

function run_test() {
  
  
  create_bookmarks_html("bookmarks.glue.html");

  
  let db = gProfD.clone();
  db.append("places.sqlite");
  if (db.exists()) {
    db.remove(false);
    do_check_false(db.exists());
  }

  
  let ps = Cc["@mozilla.org/preferences-service;1"].
           getService(Ci.nsIPrefBranch);
  ps.setIntPref(PREF_SMART_BOOKMARKS_VERSION, -1);

  
  let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  
  
  do_check_eq(hs.databaseStatus, hs.DATABASE_STATUS_CREATE);

  
  
  let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);
  bs.insertBookmark(bs.bookmarksMenuFolder, uri("http://mozilla.org/"),
                    bs.DEFAULT_INDEX, "migrated");

  
  Cc["@mozilla.org/browser/browserglue;1"].getService(Ci.nsIBrowserGlue);

  
  
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.notifyObservers(null, TOPIC_PLACES_INIT_COMPLETE, null);

  
  
  do_test_pending();
  do_timeout(3000, continue_test);
}

function continue_test() {
  let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);

  
  let itemId = bs.getIdForItemAt(bs.bookmarksMenuFolder, 0);
  do_check_eq(bs.getItemTitle(itemId), "migrated");

  
  do_check_eq(bs.getIdForItemAt(bs.bookmarksMenuFolder, 1), -1);
  do_check_eq(bs.getIdForItemAt(bs.toolbarFolder, 0), -1);

  do_test_finished();
}
