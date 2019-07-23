





































 






Components.utils.import("resource://gre/modules/PlacesDBUtils.jsm");

const FINISHED_MAINTANANCE_NOTIFICATION_TOPIC = "places-maintenance-finished";

const PLACES_STRING_BUNDLE_URI = "chrome://places/locale/places.properties";


let os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
let bh = hs.QueryInterface(Ci.nsIBrowserHistory);
let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
let ts = Cc["@mozilla.org/browser/tagging-service;1"].
         getService(Ci.nsITaggingService);
let as = Cc["@mozilla.org/browser/annotation-service;1"].
         getService(Ci.nsIAnnotationService);
let fs = Cc["@mozilla.org/browser/favicon-service;1"].
         getService(Ci.nsIFaviconService);
let bundle = Cc["@mozilla.org/intl/stringbundle;1"].
             getService(Ci.nsIStringBundleService).
             createBundle(PLACES_STRING_BUNDLE_URI);

let mDBConn = hs.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;




let defaultBookmarksMaxId = 0;
function cleanDatabase() {
  mDBConn.executeSimpleSQL("DELETE FROM moz_places");
  mDBConn.executeSimpleSQL("DELETE FROM moz_historyvisits");
  mDBConn.executeSimpleSQL("DELETE FROM moz_anno_attributes");
  mDBConn.executeSimpleSQL("DELETE FROM moz_annos");
  mDBConn.executeSimpleSQL("DELETE FROM moz_items_annos");
  mDBConn.executeSimpleSQL("DELETE FROM moz_inputhistory");
  mDBConn.executeSimpleSQL("DELETE FROM moz_keywords");
  mDBConn.executeSimpleSQL("DELETE FROM moz_favicons");
  mDBConn.executeSimpleSQL("DELETE FROM moz_bookmarks WHERE id > " + defaultBookmarksMaxId);
}

function addPlace(aUrl, aFavicon) {
  let stmt = mDBConn.createStatement(
    "INSERT INTO moz_places (url, favicon_id) VALUES (:url, :favicon)");
  stmt.params["url"] = aUrl || "http://www.mozilla.org";
  stmt.params["favicon"] = aFavicon || null;
  stmt.execute();
  stmt.finalize();
  return mDBConn.lastInsertRowID;
}

function addBookmark(aPlaceId, aType, aParent, aKeywordId, aFolderType) {
  let stmt = mDBConn.createStatement(
    "INSERT INTO moz_bookmarks (fk, type, parent, keyword_id, folder_type) " +
    "VALUES (:place_id, :type, :parent, :keyword_id, :folder_type)");
  stmt.params["place_id"] = aPlaceId || null;
  stmt.params["type"] = aType || bs.TYPE_BOOKMARK;
  stmt.params["parent"] = aParent || bs.unfiledBookmarksFolder;
  stmt.params["keyword_id"] = aKeywordId || null;
  stmt.params["folder_type"] = aFolderType || null;
  stmt.execute();
  stmt.finalize();
  return mDBConn.lastInsertRowID;
}




let tests = [];
let current_test = null;



tests.push({
  name: "A.1",
  desc: "Remove unused attributes",

  _usedPageAttribute: "usedPage",
  _usedItemAttribute: "usedItem",
  _unusedAttribute: "unused",
  _placeId: null,
  _bookmarkId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    this._bookmarkId = addBookmark(this._placeId);
    
    let stmt = mDBConn.createStatement("INSERT INTO moz_anno_attributes (name) VALUES (:anno)");
    stmt.params['anno'] = this._usedPageAttribute;
    stmt.execute();
    stmt.reset();
    stmt.params['anno'] = this._usedItemAttribute;
    stmt.execute();
    stmt.reset();
    stmt.params['anno'] = this._unusedAttribute;
    stmt.execute();
    stmt.finalize();

    stmt = mDBConn.createStatement("INSERT INTO moz_annos (place_id, anno_attribute_id) VALUES(:place_id, (SELECT id FROM moz_anno_attributes WHERE name = :anno))");
    stmt.params['place_id'] = this._placeId;
    stmt.params['anno'] = this._usedPageAttribute;
    stmt.execute();
    stmt.finalize();
    stmt = mDBConn.createStatement("INSERT INTO moz_items_annos (item_id, anno_attribute_id) VALUES(:item_id, (SELECT id FROM moz_anno_attributes WHERE name = :anno))");
    stmt.params['item_id'] = this._bookmarkId;
    stmt.params['anno'] = this._usedItemAttribute;
    stmt.execute();
    stmt.finalize();    
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_anno_attributes WHERE name = :anno");
    stmt.params['anno'] = this._usedPageAttribute;
    do_check_true(stmt.executeStep());
    stmt.reset();
    stmt.params['anno'] = this._usedItemAttribute;
    do_check_true(stmt.executeStep());
    stmt.reset();
    
    stmt.params['anno'] = this._unusedAttribute;
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "B.1",
  desc: "Remove annotations with an invalid attribute",

  _usedPageAttribute: "usedPage",
  _placeId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    let stmt = mDBConn.createStatement("INSERT INTO moz_anno_attributes (name) VALUES (:anno)");
    stmt.params['anno'] = this._usedPageAttribute;
    stmt.execute();
    stmt.finalize();
    stmt = mDBConn.createStatement("INSERT INTO moz_annos (place_id, anno_attribute_id) VALUES(:place_id, (SELECT id FROM moz_anno_attributes WHERE name = :anno))");
    stmt.params['place_id'] = this._placeId;
    stmt.params['anno'] = this._usedPageAttribute;
    stmt.execute();
    stmt.finalize();
    
    stmt = mDBConn.createStatement("INSERT INTO moz_annos (place_id, anno_attribute_id) VALUES(:place_id, 1337)");
    stmt.params['place_id'] = this._placeId;
    stmt.execute();
    stmt.finalize();
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_anno_attributes WHERE name = :anno");
    stmt.params['anno'] = this._usedPageAttribute;
    do_check_true(stmt.executeStep());
    stmt.finalize();
    
    stmt = mDBConn.createStatement("SELECT id FROM moz_annos WHERE anno_attribute_id = (SELECT id FROM moz_anno_attributes WHERE name = :anno)");
    stmt.params['anno'] = this._usedPageAttribute;
    do_check_true(stmt.executeStep());
    stmt.finalize();
    
    stmt = mDBConn.createStatement("SELECT id FROM moz_annos WHERE anno_attribute_id = 1337");
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "B.2",
  desc: "Remove orphan page annotations",

  _usedPageAttribute: "usedPage",
  _placeId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    let stmt = mDBConn.createStatement("INSERT INTO moz_anno_attributes (name) VALUES (:anno)");
    stmt.params['anno'] = this._usedPageAttribute;
    stmt.execute();
    stmt.finalize();
    stmt = mDBConn.createStatement("INSERT INTO moz_annos (place_id, anno_attribute_id) VALUES(:place_id, (SELECT id FROM moz_anno_attributes WHERE name = :anno))");
    stmt.params['place_id'] = this._placeId;
    stmt.params['anno'] = this._usedPageAttribute;
    stmt.execute();
    stmt.reset();
    
    stmt.params['place_id'] = 1337;
    stmt.params['anno'] = this._usedPageAttribute;
    stmt.execute();
    stmt.finalize();
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_anno_attributes WHERE name = :anno");
    stmt.params['anno'] = this._usedPageAttribute;
    do_check_true(stmt.executeStep());
    stmt.finalize();
    
    stmt = mDBConn.createStatement("SELECT id FROM moz_annos WHERE anno_attribute_id = (SELECT id FROM moz_anno_attributes WHERE name = :anno)");
    stmt.params['anno'] = this._usedPageAttribute;
    do_check_true(stmt.executeStep());
    stmt.finalize();
    
    stmt = mDBConn.createStatement("SELECT id FROM moz_annos WHERE place_id = 1337");
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});


tests.push({
  name: "C.1",
  desc: "fix missing Places root",

  setup: function() {
    
    do_check_eq(bs.getFolderIdForItem(bs.placesRoot), 0);
    do_check_eq(bs.getFolderIdForItem(bs.bookmarksMenuFolder), bs.placesRoot);
    do_check_eq(bs.getFolderIdForItem(bs.tagsFolder), bs.placesRoot);
    do_check_eq(bs.getFolderIdForItem(bs.unfiledBookmarksFolder), bs.placesRoot);
    do_check_eq(bs.getFolderIdForItem(bs.toolbarFolder), bs.placesRoot);

    
    mDBConn.executeSimpleSQL("DELETE FROM moz_bookmarks WHERE parent = 0");
    try {
      bs.getFolderIdForItem(bs.placesRoot);
      do_throw("Places root should not exist now!");
    } catch(e) {
      
    }
  },

  check: function() {
    
    do_check_eq(bs.getFolderIdForItem(bs.placesRoot), 0);
    do_check_eq(bs.getFolderIdForItem(bs.bookmarksMenuFolder), bs.placesRoot);
    do_check_eq(bs.getFolderIdForItem(bs.tagsFolder), bs.placesRoot);
    do_check_eq(bs.getFolderIdForItem(bs.unfiledBookmarksFolder), bs.placesRoot);
    do_check_eq(bs.getFolderIdForItem(bs.toolbarFolder), bs.placesRoot);
  }
});


tests.push({
  name: "C.2",
  desc: "Fix roots titles",

  setup: function() {
    
    this.check();
    
    bs.setItemTitle(bs.placesRoot, "bad title");
    do_check_eq(bs.getItemTitle(bs.placesRoot), "bad title");
    bs.setItemTitle(bs.unfiledBookmarksFolder, "bad title");
    do_check_eq(bs.getItemTitle(bs.unfiledBookmarksFolder), "bad title");
  },

  check: function() {
    
    do_check_eq(bs.getItemTitle(bs.placesRoot), "");
    do_check_eq(bs.getItemTitle(bs.bookmarksMenuFolder),
                bundle.GetStringFromName("BookmarksMenuFolderTitle"));
    do_check_eq(bs.getItemTitle(bs.tagsFolder),
                bundle.GetStringFromName("TagsFolderTitle"));
    do_check_eq(bs.getItemTitle(bs.unfiledBookmarksFolder),
                bundle.GetStringFromName("UnsortedBookmarksFolderTitle"));
    do_check_eq(bs.getItemTitle(bs.toolbarFolder),
                bundle.GetStringFromName("BookmarksToolbarFolderTitle"));
  }
});



tests.push({
  name: "D.1",
  desc: "Remove items without a valid place",

  _validItemId: null,
  _invalidItemId: null,
  _placeId: null,

  setup: function() {
    
    this.placeId = addPlace();
    
    this._validItemId = addBookmark(this.placeId);
    
    this._invalidItemId = addBookmark(1337);
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_bookmarks WHERE id = :item_id");
    stmt.params["item_id"] = this._validItemId;
    do_check_true(stmt.executeStep());
    stmt.reset();
    
    stmt.params["item_id"] = this._invalidItemId;
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "D.2",
  desc: "Remove items that are not uri bookmarks from tag containers",

  _tagId: null,
  _bookmarkId: null,
  _separatorId: null,
  _folderId: null,
  _dynamicContainerId: null,
  _placeId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    this._tagId = addBookmark(null, bs.TYPE_FOLDER, bs.tagsFolder);
    
    this._bookmarkId = addBookmark(this._placeId, bs.TYPE_BOOKMARK, this._tagId);
    
    this._separatorId = addBookmark(null, bs.TYPE_SEPARATOR, this._tagId);
    
    this._folderId = addBookmark(null, bs.TYPE_FOLDER, this._tagId);
    
    this._dynamicContainerId = addBookmark(null, bs.TYPE_DYNAMIC_CONTAINER, this._tagId);
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_bookmarks WHERE type = :type AND parent = :parent");
    stmt.params["type"] = bs.TYPE_BOOKMARK;
    stmt.params["parent"] = this._tagId;
    do_check_true(stmt.executeStep());
    stmt.reset();
    
    stmt.params["type"] = bs.TYPE_SEPARATOR;
    stmt.params["parent"] = this._tagId;
    do_check_false(stmt.executeStep());
    stmt.reset();
    
    stmt.params["type"] = bs.TYPE_FOLDER;
    stmt.params["parent"] = this._tagId;
    do_check_false(stmt.executeStep());
    stmt.reset();
    
    stmt.params["type"] = bs.TYPE_DYNAMIC_CONTAINER;
    stmt.params["parent"] = this._tagId;
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "D.3",
  desc: "Remove empty tags",

  _tagId: null,
  _bookmarkId: null,
  _emptyTagId: null,
  _placeId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    this._tagId = addBookmark(null, bs.TYPE_FOLDER, bs.tagsFolder);
    
    this._bookmarkId = addBookmark(this._placeId, bs.TYPE_BOOKMARK, this._tagId);
    
    this._emptyTagId = addBookmark(null, bs.TYPE_FOLDER, bs.tagsFolder);
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_bookmarks WHERE id = :id AND type = :type AND parent = :parent");
    stmt.params["id"] = this._bookmarkId;
    stmt.params["type"] = bs.TYPE_BOOKMARK;
    stmt.params["parent"] = this._tagId;
    do_check_true(stmt.executeStep());
    stmt.reset();
    stmt.params["id"] = this._tagId;
    stmt.params["type"] = bs.TYPE_FOLDER;
    stmt.params["parent"] = bs.tagsFolder;
    do_check_true(stmt.executeStep());
    stmt.reset();
    stmt.params["id"] = this._emptyTagId;
    stmt.params["type"] = bs.TYPE_FOLDER;
    stmt.params["parent"] = bs.tagsFolder;
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "D.4",
  desc: "Move orphan items to unsorted folder",

  _orphanBookmarkId: null,
  _orphanSeparatorId: null,
  _orphanFolderId: null,
  _bookmarkId: null,
  _placeId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    this._orphanBookmarkId = addBookmark(this._placeId, bs.TYPE_BOOKMARK, 8888);
    
    this._orphanSeparatorId = addBookmark(null, bs.TYPE_SEPARATOR, 8888);
    
    this._orphanFolderId = addBookmark(null, bs.TYPE_FOLDER, 8888);
    
    this._bookmarkId = addBookmark(this._placeId, bs.TYPE_BOOKMARK, this._orphanFolderId);
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_bookmarks WHERE id = :item_id AND parent = :parent");
    stmt.params["item_id"] = this._orphanBookmarkId;
    stmt.params["parent"] = bs.unfiledBookmarksFolder;
    do_check_true(stmt.executeStep());
    stmt.reset();
    stmt.params["item_id"] = this._orphanSeparatorId;
    stmt.params["parent"] = bs.unfiledBookmarksFolder;
    do_check_true(stmt.executeStep());
    stmt.reset();
    stmt.params["item_id"] = this._orphanFolderId;
    stmt.params["parent"] = bs.unfiledBookmarksFolder;
    do_check_true(stmt.executeStep());
    stmt.reset();
    stmt.params["item_id"] = this._bookmarkId;
    stmt.params["parent"] = this._orphanFolderId;
    do_check_true(stmt.executeStep());
    stmt.finalize();    
  }
});



tests.push({
  name: "D.5",
  desc: "Fix wrong keywords",

  _validKeywordItemId: null,
  _invalidKeywordItemId: null,
  _validKeywordId: 1,
  _invalidKeywordId: 8888,
  _placeId: null,

  setup: function() {
    
    let stmt = mDBConn.createStatement("INSERT INTO moz_keywords (id, keyword) VALUES(:id, :keyword)");
    stmt.params["id"] = this._validKeywordId;
    stmt.params["keyword"] = "used";
    stmt.execute();
    stmt.finalize();
    
    this._placeId = addPlace();
    
    this._validKeywordItemId = addBookmark(this._placeId, bs.TYPE_BOOKMARK, bs.unfiledBookmarksFolder, this._validKeywordId);
    
    this._invalidKeywordItemId = addBookmark(this._placeId, bs.TYPE_BOOKMARK, bs.unfiledBookmarksFolder, this._invalidKeywordId);
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_bookmarks WHERE id = :item_id AND keyword_id = :keyword");
    stmt.params["item_id"] = this._validKeywordItemId;
    stmt.params["keyword"] = this._validKeywordId;
    do_check_true(stmt.executeStep());
    stmt.reset();
    
    stmt.params["item_id"] = this._invalidKeywordItemId;
    stmt.params["keyword"] = this._invalidKeywordId;
    do_check_false(stmt.executeStep());
    stmt.finalize();
    
    stmt = mDBConn.createStatement("SELECT id FROM moz_bookmarks WHERE id = :item_id");
    stmt.params["item_id"] = this._invalidKeywordItemId;
    do_check_true(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "D.6",
  desc: "Fix wrong item types | bookmarks",

  _separatorId: null,
  _folderId: null,
  _dynamicContainerId: null,
  _placeId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    this._separatorId = addBookmark(this._placeId, bs.TYPE_SEPARATOR);
    
    this._folderId = addBookmark(this._placeId, bs.TYPE_FOLDER);
    
    this._dynamicContainerId = addBookmark(this._placeId, bs.TYPE_DYNAMIC_CONTAINER, null, null, "test");
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_bookmarks WHERE id = :item_id AND type = :type");
    stmt.params["item_id"] = this._separatorId;
    stmt.params["type"] = bs.TYPE_BOOKMARK;
    do_check_true(stmt.executeStep());
    stmt.reset();
    stmt.params["item_id"] = this._folderId;
    stmt.params["type"] = bs.TYPE_BOOKMARK;
    do_check_true(stmt.executeStep());
    stmt.reset();
    stmt.params["item_id"] = this._dynamicContainerId;
    stmt.params["type"] = bs.TYPE_BOOKMARK;
    do_check_true(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "D.7",
  desc: "Fix wrong item types | bookmarks",

  _validBookmarkId: null,
  _invalidBookmarkId: null,
  _placeId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    this._validBookmarkId = addBookmark(this._placeId, bs.TYPE_BOOKMARK);
    
    this._invalidBookmarkId = addBookmark(null, bs.TYPE_BOOKMARK);
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_bookmarks WHERE id = :item_id AND type = :type");
    stmt.params["item_id"] = this._validBookmarkId;
    stmt.params["type"] = bs.TYPE_BOOKMARK;
    do_check_true(stmt.executeStep());
    stmt.reset();
    
    stmt.params["item_id"] = this._invalidBookmarkId;
    stmt.params["type"] = bs.TYPE_FOLDER;
    do_check_true(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "D.8",
  desc: "Fix wrong item types | dynamic containers",

  _validDynamicContainerId: null,
  _invalidDynamicContainerId: null,

  setup: function() {
    
    this._validDynamicContainerId = addBookmark(null, bs.TYPE_DYNAMIC_CONTAINER, null, null, "test");
    
    this._invalidDynamicContainerId = addBookmark(null, bs.TYPE_DYNAMIC_CONTAINER, null, null, null);    
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_bookmarks WHERE id = :item_id AND type = :type");
    stmt.params["item_id"] = this._validDynamicContainerId;
    stmt.params["type"] = bs.TYPE_DYNAMIC_CONTAINER;
    do_check_true(stmt.executeStep());
    stmt.reset();
    
    stmt.params["item_id"] = this._invalidDynamicContainerId;
    stmt.params["type"] = bs.TYPE_FOLDER;
    do_check_true(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "D.9",
  desc: "Fix wrong parents",

  _bookmarkId: null,
  _separatorId: null,
  _dynamicContainerId: null,
  _bookmarkId1: null,
  _bookmarkId2: null,
  _bookmarkId3: null,
  _placeId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    this._bookmarkId = addBookmark(this._placeId, bs.TYPE_BOOKMARK);
    
    this._separatorId = addBookmark(null, bs.TYPE_SEPARATOR);
    
    this.dynamicContainerId = addBookmark(null, bs.TYPE_DYNAMIC_CONTAINER, null, null, "test");
    
    this._bookmarkId1 = addBookmark(this._placeId, bs.TYPE_BOOKMARK, this._bookmarkId);
    this._bookmarkId2 = addBookmark(this._placeId, bs.TYPE_BOOKMARK, this._separatorId);
    this._bookmarkId3 = addBookmark(this._placeId, bs.TYPE_BOOKMARK, this._dynamicContainerId);
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_bookmarks WHERE id = :item_id AND parent = :parent");
    stmt.params["item_id"] = this._bookmarkId1;
    stmt.params["parent"] = bs.unfiledBookmarksFolder;
    do_check_true(stmt.executeStep());
    stmt.reset();
    stmt.params["item_id"] = this._bookmarkId2;
    stmt.params["parent"] = bs.unfiledBookmarksFolder;
    do_check_true(stmt.executeStep());
    stmt.reset();
    stmt.params["item_id"] = this._bookmarkId3;
    stmt.params["parent"] = bs.unfiledBookmarksFolder;
    do_check_true(stmt.executeStep());
    stmt.finalize();    
  }
});



tests.push({
  name: "D.10",
  desc: "Recalculate positions",

  setup: function() {

  },

  check: function() {

  }
});



tests.push({
  name: "D.11",
  desc: "Remove old livemarks status items",

  _bookmarkId: null,
  _livemarkLoadingStatusId: null,
  _livemarkFailedStatusId: null,
  _placeId: null,
  _lmLoadingPlaceId: null,
  _lmFailedPlaceId: null,

  setup: function() {
    
    this._placeId = addPlace();

    
    this._bookmarkId = addBookmark(this._placeId);
    
    this._lmLoadingPlaceId = addPlace("about:livemark-loading");
    this._lmFailedPlaceId = addPlace("about:livemark-failed");
    
    this._livemarkLoadingStatusId = addBookmark(this._lmLoadingPlaceId);
    this._livemarkFailedStatusId = addBookmark(this._lmFailedPlaceId);
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_bookmarks WHERE id = :item_id");
    stmt.params["item_id"] = this._bookmarkId;
    do_check_true(stmt.executeStep());
    stmt.reset();
    
    stmt.params["item_id"] = this._livemarkLoadingStatusId;
    do_check_false(stmt.executeStep());
    stmt.reset();
    stmt.params["item_id"] = this._livemarkFailedStatusId;
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "E.1",
  desc: "Remove orphan icons",

  _placeId: null,

  setup: function() {
    
    let stmt = mDBConn.createStatement("INSERT INTO moz_favicons (id, url) VALUES(:favicon_id, :url)");
    stmt.params["favicon_id"] = 1;
    stmt.params["url"] = "http://www1.mozilla.org/favicon.ico";
    stmt.execute();
    stmt.reset();
    stmt.params["favicon_id"] = 2;
    stmt.params["url"] = "http://www2.mozilla.org/favicon.ico";
    stmt.execute();
    stmt.finalize();
    
    this._placeId = addPlace("http://www.mozilla.org", 1);
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_favicons WHERE id = :favicon_id");
    stmt.params["favicon_id"] = 1;
    do_check_true(stmt.executeStep());
    stmt.reset();
    
    stmt.params["favicon_id"] = 2;
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "F.1",
  desc: "Remove orphan visits",

  _placeId: null,
  _invalidPlaceId: 1337,

  setup: function() {
    
    this._placeId = addPlace();
    
    stmt = mDBConn.createStatement("INSERT INTO moz_historyvisits(place_id) VALUES (:place_id)");
    stmt.params["place_id"] = this._placeId;
    stmt.execute();
    stmt.reset();
    stmt.params["place_id"] = this._invalidPlaceId;
    stmt.execute();
    stmt.finalize();
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_historyvisits WHERE place_id = :place_id");
    stmt.params["place_id"] = this._placeId;
    do_check_true(stmt.executeStep());
    stmt.reset();
    
    stmt.params["place_id"] = this._invalidPlaceId;
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "G.1",
  desc: "Remove orphan input history",

  _placeId: null,
  _invalidPlaceId: 1337,

  setup: function() {
    
    this._placeId = addPlace();
    
    let stmt = mDBConn.createStatement("INSERT INTO moz_inputhistory (place_id, input) VALUES (:place_id, :input)");
    stmt.params["place_id"] = this._placeId;
    stmt.params["input"] = "moz";
    stmt.execute();
    stmt.reset();
    stmt.params["place_id"] = this._invalidPlaceId;
    stmt.params["input"] = "moz";
    stmt.execute();    
    stmt.finalize();
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT place_id FROM moz_inputhistory WHERE place_id = :place_id");
    stmt.params["place_id"] = this._placeId;
    do_check_true(stmt.executeStep());
    stmt.reset();
    
    stmt.params["place_id"] = this._invalidPlaceId;
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "H.1",
  desc: "Remove item annos with an invalid attribute",

  _usedItemAttribute: "usedItem",
  _bookmarkId: null,
  _placeId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    this._bookmarkId = addBookmark(this._placeId);
    
    let stmt = mDBConn.createStatement("INSERT INTO moz_anno_attributes (name) VALUES (:anno)");
    stmt.params['anno'] = this._usedItemAttribute;
    stmt.execute();
    stmt.finalize();
    stmt = mDBConn.createStatement("INSERT INTO moz_items_annos (item_id, anno_attribute_id) VALUES(:item_id, (SELECT id FROM moz_anno_attributes WHERE name = :anno))");
    stmt.params['item_id'] = this._bookmarkId;
    stmt.params['anno'] = this._usedItemAttribute;
    stmt.execute();
    stmt.finalize();
    
    stmt = mDBConn.createStatement("INSERT INTO moz_items_annos (item_id, anno_attribute_id) VALUES(:item_id, 1337)");
    stmt.params['item_id'] = this._bookmarkId;
    stmt.execute();
    stmt.finalize();
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_anno_attributes WHERE name = :anno");
    stmt.params['anno'] = this._usedItemAttribute;
    do_check_true(stmt.executeStep());
    stmt.finalize();
    
    stmt = mDBConn.createStatement("SELECT id FROM moz_items_annos WHERE anno_attribute_id = (SELECT id FROM moz_anno_attributes WHERE name = :anno)");
    stmt.params['anno'] = this._usedItemAttribute;
    do_check_true(stmt.executeStep());
    stmt.finalize();
    
    stmt = mDBConn.createStatement("SELECT id FROM moz_items_annos WHERE anno_attribute_id = 1337");
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "H.2",
  desc: "Remove orphan item annotations",

  _usedItemAttribute: "usedItem",
  _bookmarkId: null,
  _invalidBookmarkId: 8888,
  _placeId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    this._bookmarkId = addBookmark(this._placeId);
    
    stmt = mDBConn.createStatement("INSERT INTO moz_anno_attributes (name) VALUES (:anno)");
    stmt.params['anno'] = this._usedItemAttribute;
    stmt.execute();
    stmt.finalize();
    stmt = mDBConn.createStatement("INSERT INTO moz_items_annos (item_id, anno_attribute_id) VALUES (:item_id, (SELECT id FROM moz_anno_attributes WHERE name = :anno))");
    stmt.params["item_id"] = this._bookmarkId;
    stmt.params["anno"] = this._usedItemAttribute;
    stmt.execute();
    stmt.reset();
    
    stmt.params["item_id"] = this._invalidBookmarkId;
    stmt.params["anno"] = this._usedItemAttribute;
    stmt.execute();
    stmt.finalize();
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_anno_attributes WHERE name = :anno");
    stmt.params['anno'] = this._usedItemAttribute;
    do_check_true(stmt.executeStep());
    stmt.finalize();
    
    stmt = mDBConn.createStatement("SELECT id FROM moz_items_annos WHERE anno_attribute_id = (SELECT id FROM moz_anno_attributes WHERE name = :anno)");
    stmt.params['anno'] = this._usedItemAttribute;
    do_check_true(stmt.executeStep());
    stmt.finalize();
    
    stmt = mDBConn.createStatement("SELECT id FROM moz_items_annos WHERE item_id = 8888");
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});




tests.push({
  name: "I.1",
  desc: "Remove unused keywords",

  _bookmarkId: null,
  _placeId: null,

  setup: function() {
    
    let stmt = mDBConn.createStatement("INSERT INTO moz_keywords (id, keyword) VALUES(:id, :keyword)");
    stmt.params["id"] = 1;
    stmt.params["keyword"] = "used";
    stmt.execute();
    stmt.reset();
    stmt.params["id"] = 2;
    stmt.params["keyword"] = "unused";
    stmt.execute();
    stmt.finalize();
    
    this._placeId = addPlace();
    
    this._bookmarkId = addBookmark(this._placeId, bs.TYPE_BOOKMARK, bs.unfiledBookmarksFolder, 1);
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_keywords WHERE keyword = :keyword");
    stmt.params["keyword"] = "used";
    do_check_true(stmt.executeStep());
    stmt.reset();
    
    stmt.params["keyword"] = "unused";
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});




tests.push({
  name: "L.1",
  desc: "Fix wrong favicon ids",

  _validIconPlaceId: null,
  _invalidIconPlaceId: null,

  setup: function() {
    
    let stmt = mDBConn.createStatement("INSERT INTO moz_favicons (id, url) VALUES(1, :url)");
    stmt.params["url"] = "http://www.mozilla.org/favicon.ico";
    stmt.execute();
    stmt.finalize();
    
    this._validIconPlaceId = addPlace("http://www1.mozilla.org", 1);

    
    this._invalidIconPlaceId = addPlace("http://www2.mozilla.org", 1337);
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_places WHERE favicon_id = :favicon_id");
    stmt.params["favicon_id"] = 1337;
    do_check_false(stmt.executeStep());
    stmt.reset();
    
    stmt.params["favicon_id"] = 1;
    do_check_true(stmt.executeStep());
    stmt.finalize();
    
    stmt = mDBConn.createStatement("SELECT id FROM moz_places WHERE id = :place_id");
    stmt.params["place_id"] = this._validIconPlaceId;
    do_check_true(stmt.executeStep());
    stmt.reset();
    stmt.params["place_id"] = this._invalidIconPlaceId;
    do_check_true(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "L.2",
  desc: "Recalculate visit_count",

  setup: function() {

  },

  check: function() {

  }
});



tests.push({
  name: "Z",
  desc: "Sanity: Preventive maintenance does not touch valid items",

  _uri1: uri("http://www1.mozilla.org"),
  _uri2: uri("http://www2.mozilla.org"),
  _folderId: null,
  _bookmarkId: null,
  _separatorId: null,

  setup: function() {
    
    hs.addVisit(this._uri1, Date.now() * 1000, null,
                hs.TRANSITION_TYPED, false, 0);
    hs.addVisit(this._uri2, Date.now() * 1000, null,
                hs.TRANSITION_TYPED, false, 0);

    this._folderId = bs.createFolder(bs.toolbarFolder, "testfolder",
                                     bs.DEFAULT_INDEX);
    do_check_true(this._folderId > 0);
    this._bookmarkId = bs.insertBookmark(this._folderId, this._uri1,
                                         bs.DEFAULT_INDEX, "testbookmark");
    do_check_true(this._bookmarkId > 0);
    this._separatorId = bs.insertSeparator(bs.unfiledBookmarksFolder,
                                           bs.DEFAULT_INDEX);
    do_check_true(this._separatorId > 0);
    ts.tagURI(this._uri1, ["testtag"]);
    fs.setFaviconUrlForPage(this._uri2,
                            uri("http://www2.mozilla.org/favicon.ico"));
    bs.setKeywordForBookmark(this._bookmarkId, "testkeyword");
    as.setPageAnnotation(this._uri2, "anno", "anno", 0, as.EXPIRE_NEVER);
    as.setItemAnnotation(this._bookmarkId, "anno", "anno", 0, as.EXPIRE_NEVER);
  },

  check: function() {
    
    do_check_true(bh.isVisited(this._uri1));
    do_check_true(bh.isVisited(this._uri2));
    
    do_check_eq(bs.getBookmarkURI(this._bookmarkId).spec, this._uri1.spec);
    do_check_eq(bs.getItemIndex(this._folderId), 0);

    do_check_eq(bs.getItemType(this._folderId), bs.TYPE_FOLDER);
    do_check_eq(bs.getItemType(this._separatorId), bs.TYPE_SEPARATOR);

    do_check_eq(ts.getTagsForURI(this._uri1).length, 1);
    do_check_eq(bs.getKeywordForBookmark(this._bookmarkId), "testkeyword");
    do_check_eq(fs.getFaviconForPage(this._uri2).spec,
                "http://www2.mozilla.org/favicon.ico");
    do_check_eq(as.getPageAnnotation(this._uri2, "anno"), "anno");
    do_check_eq(as.getItemAnnotation(this._bookmarkId, "anno"), "anno");
  }
});



let observer = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == FINISHED_MAINTANANCE_NOTIFICATION_TOPIC) {
      try {current_test.check();}
      catch (ex){ do_throw(ex);}
      cleanDatabase();
      if (tests.length) {
        current_test = tests.shift();
        dump("\nExecuting test: " + current_test.name + "\n" + "*** " + current_test.desc + "\n");
        current_test.setup();
        PlacesDBUtils.maintenanceOnIdle();
      }
      else {
        os.removeObserver(this, FINISHED_MAINTANANCE_NOTIFICATION_TOPIC);
        
        do_check_eq(bs.getFolderIdForItem(bs.placesRoot), 0);
        do_check_eq(bs.getFolderIdForItem(bs.bookmarksMenuFolder), bs.placesRoot);
        do_check_eq(bs.getFolderIdForItem(bs.tagsFolder), bs.placesRoot);
        do_check_eq(bs.getFolderIdForItem(bs.unfiledBookmarksFolder), bs.placesRoot);
        do_check_eq(bs.getFolderIdForItem(bs.toolbarFolder), bs.placesRoot);
        do_test_finished();
      }
    }
  }
}
os.addObserver(observer, FINISHED_MAINTANANCE_NOTIFICATION_TOPIC, false);



function run_test() {
  
  
  hs.addVisit(uri("http://force.bookmarks.hash"), Date.now() * 1000, null,
              hs.TRANSITION_TYPED, false, 0);
  do_check_false(bs.isBookmarked(uri("http://force.bookmarks.hash")));

  
  let stmt = mDBConn.createStatement("SELECT MAX(id) FROM moz_bookmarks");
  stmt.executeStep();
  defaultBookmarksMaxId = stmt.getInt32(0);
  stmt.finalize();
  do_check_true(defaultBookmarksMaxId > 0);

  
  
  do_test_pending();

  current_test = tests.shift();
  dump("\nExecuting test: " + current_test.name + "\n" + "*** " + current_test.desc + "\n");
  current_test.setup();
  PlacesDBUtils.maintenanceOnIdle();
}
