










const PREF_SMART_BOOKMARKS_VERSION = "browser.places.smartBookmarksVersion";
const PREF_AUTO_EXPORT_HTML = "browser.bookmarks.autoExportHTML";
const PREF_IMPORT_BOOKMARKS_HTML = "browser.places.importBookmarksHTML";
const PREF_RESTORE_DEFAULT_BOOKMARKS = "browser.bookmarks.restore_default_bookmarks";

function run_test() {
  remove_bookmarks_html();
  remove_all_JSON_backups();
  run_next_test();
}

do_register_cleanup(() => PlacesUtils.bookmarks.eraseEverything());

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

add_task(function* setup() {
  
  let bg = Cc["@mozilla.org/browser/browserglue;1"].getService(Ci.nsIObserver);

  
  PlacesUtils.history;

  
  yield promiseTopicObserved("places-browser-init-complete");

  
  Assert.ok(!Services.prefs.getBoolPref(PREF_AUTO_EXPORT_HTML));
  Assert.ok(!Services.prefs.getBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS));
  Assert.throws(() => Services.prefs.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));
});

add_task(function* test_version_0() {
  do_print("All smart bookmarks are created if smart bookmarks version is 0.");

  
  Assert.ok(yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: 0
  }));

  Assert.ok(yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    index: 0
  }));

  
  Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 0);

  yield rebuildSmartBookmarks();

  
  Assert.equal(countFolderChildren(PlacesUtils.toolbarFolderId),
               SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
  Assert.equal(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
               SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

  
  Assert.equal(Services.prefs.getIntPref(PREF_SMART_BOOKMARKS_VERSION),
               SMART_BOOKMARKS_VERSION);
});

add_task(function* test_version_change() {
  do_print("An existing smart bookmark is replaced when version changes.");

  
  let bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: 0
  });
  yield checkItemHasAnnotation(bm.guid, SMART_BOOKMARKS_ANNO);

  
  yield PlacesUtils.bookmarks.update({guid: bm.guid, title: "new title"});
  bm = yield PlacesUtils.bookmarks.fetch({guid: bm.guid});
  Assert.equal(bm.title, "new title");

  
  Assert.equal(countFolderChildren(PlacesUtils.toolbarFolderId),
               SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
  Assert.equal(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
               SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

  
  Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 1);

  yield rebuildSmartBookmarks();

  
  Assert.equal(countFolderChildren(PlacesUtils.toolbarFolderId),
               SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
  Assert.equal(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
               SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

  
  bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: 0
  });
  yield checkItemHasAnnotation(bm.guid, SMART_BOOKMARKS_ANNO);
  Assert.notEqual(bm.title, "new title");

  
  Assert.equal(Services.prefs.getIntPref(PREF_SMART_BOOKMARKS_VERSION),
               SMART_BOOKMARKS_VERSION);
});

add_task(function* test_version_change_pos() {
  do_print("bookmarks position is retained when version changes.");

  
  Assert.equal(countFolderChildren(PlacesUtils.toolbarFolderId),
               SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
  Assert.equal(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
               SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

  let bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    index: 0
  });
  yield checkItemHasAnnotation(bm.guid, SMART_BOOKMARKS_ANNO);
  let firstItemTitle = bm.title;

  bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    index: 1
  });
  yield checkItemHasAnnotation(bm.guid, SMART_BOOKMARKS_ANNO);
  let secondItemTitle = bm.title;

  
  Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 1);

  yield rebuildSmartBookmarks();

  
  Assert.equal(countFolderChildren(PlacesUtils.toolbarFolderId),
               SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
  Assert.equal(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
               SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

  
  bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    index: 0
  });
  yield checkItemHasAnnotation(bm.guid, SMART_BOOKMARKS_ANNO);
  Assert.equal(bm.title, firstItemTitle);

  bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    index: 1
  });
  yield checkItemHasAnnotation(bm.guid, SMART_BOOKMARKS_ANNO);
  Assert.equal(bm.title, secondItemTitle);

  
  Assert.equal(Services.prefs.getIntPref(PREF_SMART_BOOKMARKS_VERSION),
               SMART_BOOKMARKS_VERSION);
});

add_task(function* test_version_change_pos_moved() {
  do_print("moved bookmarks position is retained when version changes.");

  
  Assert.equal(countFolderChildren(PlacesUtils.toolbarFolderId),
               SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
  Assert.equal(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
               SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

  let bm1 = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    index: 0
  });
  yield checkItemHasAnnotation(bm1.guid, SMART_BOOKMARKS_ANNO);
  let firstItemTitle = bm1.title;

  let bm2 = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    index: 1
  });
  yield checkItemHasAnnotation(bm2.guid, SMART_BOOKMARKS_ANNO);
  let secondItemTitle = bm2.title;

  
  yield PlacesUtils.bookmarks.update({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    guid: bm1.guid,
    index: PlacesUtils.bookmarks.DEFAULT_INDEX
  });

  let bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    index: PlacesUtils.bookmarks.DEFAULT_INDEX
  });
  Assert.equal(bm.guid, bm1.guid);

  
  Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 1);

  yield rebuildSmartBookmarks();

  
  Assert.equal(countFolderChildren(PlacesUtils.toolbarFolderId),
               SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
  Assert.equal(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
               SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

  
  bm2 = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    index: 0
  });
  yield checkItemHasAnnotation(bm2.guid, SMART_BOOKMARKS_ANNO);
  Assert.equal(bm2.title, secondItemTitle);

  bm1 = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    index: PlacesUtils.bookmarks.DEFAULT_INDEX
  });
  yield checkItemHasAnnotation(bm1.guid, SMART_BOOKMARKS_ANNO);
  Assert.equal(bm1.title, firstItemTitle);

  
  yield PlacesUtils.bookmarks.update({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    guid: bm1.guid,
    index: 1
  });

  
  Assert.equal(Services.prefs.getIntPref(PREF_SMART_BOOKMARKS_VERSION),
              SMART_BOOKMARKS_VERSION);
});

add_task(function* test_recreation() {
  do_print("An explicitly removed smart bookmark should not be recreated.");

  
  let bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: 0
  });
  yield PlacesUtils.bookmarks.remove(bm.guid);

  
  Assert.equal(countFolderChildren(PlacesUtils.toolbarFolderId),
               DEFAULT_BOOKMARKS_ON_TOOLBAR);
  Assert.equal(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
               SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

  
  Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 1);

  yield rebuildSmartBookmarks();

  
  
  Assert.equal(countFolderChildren(PlacesUtils.toolbarFolderId),
               DEFAULT_BOOKMARKS_ON_TOOLBAR);
  Assert.equal(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
               SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

  
  Assert.equal(Services.prefs.getIntPref(PREF_SMART_BOOKMARKS_VERSION),
               SMART_BOOKMARKS_VERSION);
});

add_task(function* test_recreation_version_0() {
  do_print("Even if a smart bookmark has been removed recreate it if version is 0.");

  
  Assert.equal(countFolderChildren(PlacesUtils.toolbarFolderId),
               DEFAULT_BOOKMARKS_ON_TOOLBAR);
  Assert.equal(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
               SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

  
  Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 0);

  yield rebuildSmartBookmarks();

  
  
  Assert.equal(countFolderChildren(PlacesUtils.toolbarFolderId),
               SMART_BOOKMARKS_ON_TOOLBAR + DEFAULT_BOOKMARKS_ON_TOOLBAR);
  Assert.equal(countFolderChildren(PlacesUtils.bookmarksMenuFolderId),
               SMART_BOOKMARKS_ON_MENU + DEFAULT_BOOKMARKS_ON_MENU);

  
  Assert.equal(Services.prefs.getIntPref(PREF_SMART_BOOKMARKS_VERSION),
               SMART_BOOKMARKS_VERSION);
});
