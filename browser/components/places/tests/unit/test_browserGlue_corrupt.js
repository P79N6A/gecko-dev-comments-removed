










































function run_test() {
  do_test_pending();

  
  
  create_bookmarks_html("bookmarks.glue.html");

  
  create_JSON_backup("bookmarks.glue.json");

  
  let db = gProfD.clone();
  db.append("places.sqlite");
  if (db.exists()) {
    db.remove(false);
    do_check_false(db.exists());
  }
  
  let corruptDB = gTestDir.clone();
  corruptDB.append("corruptDB.sqlite");
  corruptDB.copyTo(gProfD, "places.sqlite");
  do_check_true(db.exists());

  
  Cc["@mozilla.org/browser/browserglue;1"].getService(Ci.nsIBrowserGlue);

  
  let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  
  
  do_check_eq(hs.databaseStatus, hs.DATABASE_STATUS_CORRUPT);

  
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  let observer = {
    observe: function(aSubject, aTopic, aData) {
      os.removeObserver(observer, "bookmarks-restore-success");
      os.removeObserver(observer, "bookmarks-restore-failed");
      do_check_eq(aTopic, "bookmarks-restore-success");
      do_check_eq(aData, "json");
      continue_test();
    }
  }
  os.addObserver(observer, "bookmarks-restore-success", false);
  os.addObserver(observer, "bookmarks-restore-failed", false);
}

function continue_test() {
  let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);

  
  
  let itemId = bs.getIdForItemAt(bs.toolbarFolder, 0);
  do_check_neq(itemId, -1);
  do_check_eq(bs.getItemTitle(itemId), "examplejson");

  remove_bookmarks_html();
  remove_all_JSON_backups();

  do_test_finished();
}
