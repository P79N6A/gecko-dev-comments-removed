










































function run_test() {
  
  remove_bookmarks_html();
  
  remove_all_JSON_backups();

  
  let db = gProfD.clone();
  db.append("places.sqlite");
  if (db.exists()) {
    db.remove(false);
    do_check_false(db.exists());
  }
  
  corruptDB = gTestDir.clone();
  corruptDB.append("corruptDB.sqlite");
  corruptDB.copyTo(gProfD, "places.sqlite");
  do_check_true(db.exists());

  
  Cc["@mozilla.org/browser/browserglue;1"].getService(Ci.nsIBrowserGlue);

  
  let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  
  
  do_check_eq(hs.databaseStatus, hs.DATABASE_STATUS_CORRUPT);

  
  
  do_test_pending();
  do_timeout(1000, "continue_test();");
}

function continue_test() {
  let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);

  if (bs.getIdForItemAt(bs.toolbarFolder, 1) == -1) {
    
    do_timeout(1000, "continue_test();");
    return;
  }

  
  let itemId = bs.getIdForItemAt(bs.toolbarFolder, SMART_BOOKMARKS_ON_TOOLBAR + 1);
  do_check_true(itemId > 0);

  do_test_finished();
}
