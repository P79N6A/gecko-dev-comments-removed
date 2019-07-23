











































var bg = Cc["@mozilla.org/browser/browserglue;1"].
         getService(Ci.nsIBrowserGlue);


var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);


var ps = Cc["@mozilla.org/preferences-service;1"].
         getService(Ci.nsIPrefBranch);
var os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
var as = Cc["@mozilla.org/browser/annotation-service;1"].
         getService(Ci.nsIAnnotationService);

const PREF_SMART_BOOKMARKS_VERSION = "browser.places.smartBookmarksVersion";
const SMART_BOOKMARKS_ANNO = "Places/SmartBookmark";

const TOPIC_PLACES_INIT_COMPLETE = "places-init-complete";

var tests = [];



tests.push({
  description: "All smart bookmarks are created if smart bookmarks version is 0.",
  exec: function() {
    
    do_check_eq(bs.getIdForItemAt(bs.toolbarFolder, 0), -1);
    
    do_check_eq(bs.getIdForItemAt(bs.bookmarksMenuFolder, 0), -1);

    
    ps.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 0);
    
    os.notifyObservers(null, TOPIC_PLACES_INIT_COMPLETE, null);

    
    do_check_eq(countFolderChildren(bs.toolbarFolder), SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
    
    do_check_eq(countFolderChildren(bs.bookmarksMenuFolder), SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

    
    do_check_eq(ps.getIntPref(PREF_SMART_BOOKMARKS_VERSION), SMART_BOOKMARKS_VERSION);

    next_test();
  }
});



tests.push({
  description: "An existing smart bookmark is replaced when version changes.",
  exec: function() {
    
    var itemId = bs.getIdForItemAt(bs.toolbarFolder, 0);
    do_check_neq(itemId, -1);
    do_check_true(as.itemHasAnnotation(itemId, SMART_BOOKMARKS_ANNO));

    
    bs.setItemTitle(itemId, "new title");
    do_check_eq(bs.getItemTitle(itemId), "new title");

    
    ps.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 1);
    
    os.notifyObservers(null, TOPIC_PLACES_INIT_COMPLETE, null);

    
    do_check_eq(countFolderChildren(bs.toolbarFolder), SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
    
    do_check_eq(countFolderChildren(bs.bookmarksMenuFolder), SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

    
    itemId = bs.getIdForItemAt(bs.toolbarFolder, 0);
    do_check_neq(itemId, -1);
    do_check_neq(bs.getItemTitle(itemId), "new title");
    do_check_true(as.itemHasAnnotation(itemId, SMART_BOOKMARKS_ANNO));

    
    do_check_eq(ps.getIntPref(PREF_SMART_BOOKMARKS_VERSION), SMART_BOOKMARKS_VERSION);

    next_test();
  }
});



tests.push({
  description: "An explicitly removed smart bookmark should not be recreated.",
  exec: function() {   
    
    ps.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 1);
    
    bs.removeItem(bs.getIdForItemAt(bs.toolbarFolder, 0));

    
    os.notifyObservers(null, TOPIC_PLACES_INIT_COMPLETE, null);

    
    do_check_eq(countFolderChildren(bs.toolbarFolder),  DEFAULT_BOOKMARKS_ON_TOOLBAR);
    
    do_check_eq(countFolderChildren(bs.bookmarksMenuFolder), SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

    
    do_check_eq(ps.getIntPref(PREF_SMART_BOOKMARKS_VERSION), SMART_BOOKMARKS_VERSION);

    next_test();
  }
});



tests.push({
  description: "Even if a smart bookmark has been removed recreate it if version is 0.",
  exec: function() {   
    
    ps.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 0);

    
    os.notifyObservers(null, TOPIC_PLACES_INIT_COMPLETE, null);

    
    do_check_eq(countFolderChildren(bs.toolbarFolder), SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
    
    do_check_eq(countFolderChildren(bs.bookmarksMenuFolder), SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

    
    do_check_eq(ps.getIntPref(PREF_SMART_BOOKMARKS_VERSION), SMART_BOOKMARKS_VERSION);

    finish_test();
  }
});


function countFolderChildren(aFolderItemId) {
  var query = hs.getNewQuery();
  query.setFolders([aFolderItemId], 1);
  var options = hs.getNewQueryOptions();
  var rootNode = hs.executeQuery(query, options).root;
  rootNode.containerOpen = true;
  var cc = rootNode.childCount;
  rootNode.containerOpen = false;
  return cc;
}

function finish_test() {
  
  remove_all_bookmarks();

  do_test_finished();
}

var testIndex = 0;
function next_test() {
  
  
  os.addObserver(bg, TOPIC_PLACES_INIT_COMPLETE, false);

  
  let test = tests.shift();
  print("\nTEST " + (++testIndex) + ": " + test.description);
  test.exec();
}

function run_test() {
  
  remove_all_bookmarks();

  
  do_test_pending();
  next_test();
}
