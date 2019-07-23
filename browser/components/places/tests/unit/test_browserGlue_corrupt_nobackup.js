










































const NS_PLACES_INIT_COMPLETE_TOPIC = "places-init-complete";


var os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
var observer = {
  observe: function thn_observe(aSubject, aTopic, aData) {
    if (aTopic == NS_PLACES_INIT_COMPLETE_TOPIC) {
        os.removeObserver(this, NS_PLACES_INIT_COMPLETE_TOPIC);
        var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
                 getService(Ci.nsINavHistoryService);
      
      
      do_check_eq(hs.databaseStatus, hs.DATABASE_STATUS_CORRUPT);

      
      var tm = Cc["@mozilla.org/thread-manager;1"].
               getService(Ci.nsIThreadManager);
      tm.mainThread.dispatch({
        run: function() {
          continue_test();
        }
      }, Ci.nsIThread.DISPATCH_NORMAL);
    }
  }
};
os.addObserver(observer, NS_PLACES_INIT_COMPLETE_TOPIC, false);

function run_test() {
  
  create_bookmarks_html("bookmarks.glue.html");
  
  remove_all_JSON_backups();

  
  var db = gProfD.clone();
  db.append("places.sqlite");
  if (db.exists()) {
    db.remove(false);
    do_check_false(db.exists());
  }
  
  var corruptDB = gTestDir.clone();
  corruptDB.append("corruptDB.sqlite");
  corruptDB.copyTo(gProfD, "places.sqlite");
  do_check_true(db.exists());

  
  Cc["@mozilla.org/browser/browserglue;1"].getService(Ci.nsIBrowserGlue);

  
  var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);

  
  do_test_pending();
}

function continue_test() {
  var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);

  var itemId = bs.getIdForItemAt(bs.toolbarFolder, SMART_BOOKMARKS_ON_TOOLBAR);
  do_check_neq(itemId, -1);
  do_check_eq(bs.getItemTitle(itemId), "example");

  do_test_finished();
}
