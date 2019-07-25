










































const PREF_SMART_BOOKMARKS_VERSION = "browser.places.smartBookmarksVersion";
const PREF_AUTO_EXPORT_HTML = "browser.bookmarks.autoExportHTML";
const PREF_IMPORT_BOOKMARKS_HTML = "browser.places.importBookmarksHTML";
const PREF_RESTORE_DEFAULT_BOOKMARKS = "browser.bookmarks.restore_default_bookmarks";

const SMART_BOOKMARKS_ANNO = "Places/SmartBookmark";





function rebuildSmartBookmarks() {
  let consoleListener = {
    observe: function(aMsg) {
      print("Got console message: " + aMsg.message);
    },

    QueryInterface: XPCOMUtils.generateQI([
      Ci.nsIConsoleListener
    ]),
  };
  Services.console.reset();
  Services.console.registerListener(consoleListener);
  Cc["@mozilla.org/browser/browserglue;1"].getService(Ci.nsIBrowserGlue)
                                          .ensurePlacesDefaultQueriesInitialized();
  Services.console.unregisterListener(consoleListener);
}


let tests = [];


tests.push({
  description: "All smart bookmarks are created if smart bookmarks version is 0.",
  exec: function() {
    
    do_check_neq(PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId, 0), -1);
    do_check_neq(PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.bookmarksMenuFolderId, 0), -1);

    
    Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 0);

    rebuildSmartBookmarks();

    
    do_check_eq(countFolderChildren(PlacesUtils.toolbarFolderId),
                SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
    do_check_eq(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
                SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

    
    do_check_eq(Services.prefs.getIntPref(PREF_SMART_BOOKMARKS_VERSION),
                SMART_BOOKMARKS_VERSION);

    next_test();
  }
});



tests.push({
  description: "An existing smart bookmark is replaced when version changes.",
  exec: function() {
    
    let itemId = PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId, 0);
    do_check_neq(itemId, -1);
    do_check_true(PlacesUtils.annotations.itemHasAnnotation(itemId, SMART_BOOKMARKS_ANNO));
    
    PlacesUtils.bookmarks.setItemTitle(itemId, "new title");
    do_check_eq(PlacesUtils.bookmarks.getItemTitle(itemId), "new title");

    
    do_check_eq(countFolderChildren(PlacesUtils.toolbarFolderId),
                SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
    do_check_eq(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
                SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

    
    Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 1);

    rebuildSmartBookmarks();

    
    do_check_eq(countFolderChildren(PlacesUtils.toolbarFolderId),
                SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
    do_check_eq(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
                SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

    
    itemId = PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId, 0);
    do_check_neq(itemId, -1);
    do_check_neq(PlacesUtils.bookmarks.getItemTitle(itemId), "new title");
    do_check_true(PlacesUtils.annotations.itemHasAnnotation(itemId, SMART_BOOKMARKS_ANNO));

    
    do_check_eq(Services.prefs.getIntPref(PREF_SMART_BOOKMARKS_VERSION),
                SMART_BOOKMARKS_VERSION);

    next_test();
  }
});



tests.push({
  description: "bookmarks position is retained when version changes.",
  exec: function() {
    
    do_check_eq(countFolderChildren(PlacesUtils.toolbarFolderId),
                SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
    do_check_eq(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
                SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

    let itemId = PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.bookmarksMenuFolderId, 0);
    do_check_true(PlacesUtils.annotations.itemHasAnnotation(itemId, SMART_BOOKMARKS_ANNO));
    let firstItemTitle = PlacesUtils.bookmarks.getItemTitle(itemId);

    itemId = PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.bookmarksMenuFolderId, 1);
    do_check_true(PlacesUtils.annotations.itemHasAnnotation(itemId, SMART_BOOKMARKS_ANNO));
    let secondItemTitle = PlacesUtils.bookmarks.getItemTitle(itemId);

    
    Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 1);

    rebuildSmartBookmarks();

    
    do_check_eq(countFolderChildren(PlacesUtils.toolbarFolderId),
                SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
    do_check_eq(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
                SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

    
    itemId = PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.bookmarksMenuFolderId, 0);
    do_check_true(PlacesUtils.annotations.itemHasAnnotation(itemId, SMART_BOOKMARKS_ANNO));
    do_check_eq(PlacesUtils.bookmarks.getItemTitle(itemId), firstItemTitle);

    itemId = PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.bookmarksMenuFolderId, 1);
    do_check_true(PlacesUtils.annotations.itemHasAnnotation(itemId, SMART_BOOKMARKS_ANNO));
    do_check_eq(PlacesUtils.bookmarks.getItemTitle(itemId), secondItemTitle);

    
    do_check_eq(Services.prefs.getIntPref(PREF_SMART_BOOKMARKS_VERSION),
                SMART_BOOKMARKS_VERSION);

    next_test();
  }
});



tests.push({
  description: "moved bookmarks position is retained when version changes.",
  exec: function() {
    
    do_check_eq(countFolderChildren(PlacesUtils.toolbarFolderId),
                SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
    do_check_eq(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
                SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

    let itemId1 = PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.bookmarksMenuFolderId, 0);
    do_check_true(PlacesUtils.annotations.itemHasAnnotation(itemId1, SMART_BOOKMARKS_ANNO));
    let firstItemTitle = PlacesUtils.bookmarks.getItemTitle(itemId1);

    let itemId2 = PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.bookmarksMenuFolderId, 1);
    do_check_true(PlacesUtils.annotations.itemHasAnnotation(itemId2, SMART_BOOKMARKS_ANNO));
    let secondItemTitle = PlacesUtils.bookmarks.getItemTitle(itemId2);

    
    PlacesUtils.bookmarks.moveItem(itemId1, PlacesUtils.bookmarksMenuFolderId,
                                   PlacesUtils.bookmarks.DEFAULT_INDEX);

    do_check_eq(itemId1, PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.bookmarksMenuFolderId,
                                                   PlacesUtils.bookmarks.DEFAULT_INDEX));

    
    Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 1);

    rebuildSmartBookmarks();

    
    do_check_eq(countFolderChildren(PlacesUtils.toolbarFolderId),
                SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
    do_check_eq(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
                SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

    
    itemId2 = PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.bookmarksMenuFolderId, 0);
    do_check_true(PlacesUtils.annotations.itemHasAnnotation(itemId2, SMART_BOOKMARKS_ANNO));
    do_check_eq(PlacesUtils.bookmarks.getItemTitle(itemId2), secondItemTitle);

    itemId1 = PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.bookmarksMenuFolderId,
                                                   PlacesUtils.bookmarks.DEFAULT_INDEX);
    do_check_true(PlacesUtils.annotations.itemHasAnnotation(itemId1, SMART_BOOKMARKS_ANNO));
    do_check_eq(PlacesUtils.bookmarks.getItemTitle(itemId1), firstItemTitle);

    
    PlacesUtils.bookmarks.moveItem(itemId1, PlacesUtils.bookmarksMenuFolderId, 1);

    
    do_check_eq(Services.prefs.getIntPref(PREF_SMART_BOOKMARKS_VERSION),
                SMART_BOOKMARKS_VERSION);

    next_test();
  }
});



tests.push({
  description: "An explicitly removed smart bookmark should not be recreated.",
  exec: function() {   
    
    PlacesUtils.bookmarks.removeItem(PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId, 0));

    
    do_check_eq(countFolderChildren(PlacesUtils.toolbarFolderId),
                DEFAULT_BOOKMARKS_ON_TOOLBAR);
    do_check_eq(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
                SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

    
    Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 1);

    rebuildSmartBookmarks();

    
    
    do_check_eq(countFolderChildren(PlacesUtils.toolbarFolderId),
                DEFAULT_BOOKMARKS_ON_TOOLBAR);
    do_check_eq(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
                SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

    
    do_check_eq(Services.prefs.getIntPref(PREF_SMART_BOOKMARKS_VERSION),
                SMART_BOOKMARKS_VERSION);

    next_test();
  }
});



tests.push({
  description: "Even if a smart bookmark has been removed recreate it if version is 0.",
  exec: function() {
    
    do_check_eq(countFolderChildren(PlacesUtils.toolbarFolderId),
                DEFAULT_BOOKMARKS_ON_TOOLBAR);
    do_check_eq(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
                SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

    
    Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 0);

    rebuildSmartBookmarks();

    
    
    do_check_eq(countFolderChildren(PlacesUtils.toolbarFolderId),
                SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
    do_check_eq(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
                SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

    
    do_check_eq(Services.prefs.getIntPref(PREF_SMART_BOOKMARKS_VERSION),
                SMART_BOOKMARKS_VERSION);

    next_test();
  }
});


function countFolderChildren(aFolderItemId) {
  let rootNode = PlacesUtils.getFolderContents(aFolderItemId).root;
  let cc = rootNode.childCount;
  
  for (let i = 0; i < cc ; i++) {
    let node = rootNode.getChild(i);
    let title = PlacesUtils.nodeIsSeparator(node) ? "---" : node.title;
    print("Found child(" + i + "): " + title);
  }
  rootNode.containerOpen = false;
  return cc;
}

function next_test() {
  if (tests.length) {
    
    let test = tests.shift();
    print("\nTEST: " + test.description);
    test.exec();
  }
  else {
    
    remove_all_bookmarks();
    do_test_finished();
  }
}

function run_test() {
  do_test_pending();

  remove_bookmarks_html();
  remove_all_JSON_backups();

  
  let bg = Cc["@mozilla.org/browser/browserglue;1"].getService(Ci.nsIObserver);
  
  PlacesUtils.history;
  
  
  bg.observe(null, "places-init-complete", null);

  
  do_check_false(Services.prefs.getBoolPref(PREF_AUTO_EXPORT_HTML));
  do_check_false(Services.prefs.getBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS));
  try {
    do_check_false(Services.prefs.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));
    do_throw("importBookmarksHTML pref should not exist");
  }
  catch(ex) {}

  
  next_test();
}
