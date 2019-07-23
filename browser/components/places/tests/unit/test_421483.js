






































try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
} catch(ex) {
  do_throw("Could not get Bookmarks service\n");
}


try {
  var annosvc = Cc["@mozilla.org/browser/annotation-service;1"].
                getService(Ci.nsIAnnotationService);
} catch(ex) {
  do_throw("Could not get Annotation service\n");
}


try {
  var gluesvc = Cc["@mozilla.org/browser/browserglue;1"].
                getService(Ci.nsIBrowserGlue);
} catch(ex) {
  do_throw("Could not get BrowserGlue service\n");
}


try {
  var pref =  Cc["@mozilla.org/preferences-service;1"].
              getService(Ci.nsIPrefBranch);
} catch(ex) {
  do_throw("Could not get Preferences service\n");
}

const SMART_BOOKMARKS_ANNO = "Places/SmartBookmark";
const SMART_BOOKMARKS_PREF = "browser.places.smartBookmarksVersion";


function run_test() {
  
  pref.setIntPref("browser.places.smartBookmarksVersion", -1);
  gluesvc.ensurePlacesDefaultQueriesInitialized();
  var smartBookmarkItemIds = annosvc.getItemsWithAnnotation(SMART_BOOKMARKS_ANNO);
  do_check_eq(smartBookmarkItemIds.length, 0);
  
  do_check_eq(pref.getIntPref("browser.places.smartBookmarksVersion"), -1);

  
  pref.setIntPref("browser.places.smartBookmarksVersion", 0);
  gluesvc.ensurePlacesDefaultQueriesInitialized();
  smartBookmarkItemIds = annosvc.getItemsWithAnnotation(SMART_BOOKMARKS_ANNO);
  do_check_neq(smartBookmarkItemIds.length, 0);
  
  do_check_true(pref.getIntPref("browser.places.smartBookmarksVersion") > 0);

  var smartBookmarksCount = smartBookmarkItemIds.length;

  
  
  bmsvc.removeItem(smartBookmarkItemIds[0]);
  pref.setIntPref("browser.places.smartBookmarksVersion", 0);
  gluesvc.ensurePlacesDefaultQueriesInitialized();
  smartBookmarkItemIds = annosvc.getItemsWithAnnotation(SMART_BOOKMARKS_ANNO);
  do_check_eq(smartBookmarkItemIds.length, smartBookmarksCount);
  
  do_check_true(pref.getIntPref("browser.places.smartBookmarksVersion") > 0);

  
  
  var parent = bmsvc.getFolderIdForItem(smartBookmarkItemIds[0]);
  var oldTitle = bmsvc.getItemTitle(smartBookmarkItemIds[0]);
  
  var newParent = bmsvc.createFolder(parent, "test", bmsvc.DEFAULT_INDEX);
  bmsvc.moveItem(smartBookmarkItemIds[0], newParent, bmsvc.DEFAULT_INDEX);
  
  bmsvc.setItemTitle(smartBookmarkItemIds[0], "new title");
  
  pref.setIntPref("browser.places.smartBookmarksVersion", 0);
  gluesvc.ensurePlacesDefaultQueriesInitialized();
  smartBookmarkItemIds = annosvc.getItemsWithAnnotation(SMART_BOOKMARKS_ANNO);
  do_check_eq(smartBookmarkItemIds.length, smartBookmarksCount);
  do_check_eq(bmsvc.getFolderIdForItem(smartBookmarkItemIds[0]), newParent);
  do_check_eq(bmsvc.getItemTitle(smartBookmarkItemIds[0]), oldTitle);
  
  do_check_true(pref.getIntPref("browser.places.smartBookmarksVersion") > 0);
}
