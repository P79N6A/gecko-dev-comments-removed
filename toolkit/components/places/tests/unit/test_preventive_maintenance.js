





 






Components.utils.import("resource://gre/modules/PlacesDBUtils.jsm");

const FINISHED_MAINTENANCE_NOTIFICATION_TOPIC = "places-maintenance-finished";


let hs = PlacesUtils.history;
let bs = PlacesUtils.bookmarks;
let ts = PlacesUtils.tagging;
let as = PlacesUtils.annotations;
let fs = PlacesUtils.favicons;

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

function addBookmark(aPlaceId, aType, aParent, aKeywordId, aFolderType, aTitle) {
  let stmt = mDBConn.createStatement(
    `INSERT INTO moz_bookmarks (fk, type, parent, keyword_id, folder_type,
                                title, guid)
     VALUES (:place_id, :type, :parent, :keyword_id, :folder_type, :title,
             GENERATE_GUID())`);
  stmt.params["place_id"] = aPlaceId || null;
  stmt.params["type"] = aType || bs.TYPE_BOOKMARK;
  stmt.params["parent"] = aParent || bs.unfiledBookmarksFolder;
  stmt.params["keyword_id"] = aKeywordId || null;
  stmt.params["folder_type"] = aFolderType || null;
  stmt.params["title"] = typeof(aTitle) == "string" ? aTitle : null;
  stmt.execute();
  stmt.finalize();
  return mDBConn.lastInsertRowID;
}




let tests = [];



tests.push({
  name: "A.1",
  desc: "Remove obsolete annotations from moz_annos",

  _obsoleteWeaveAttribute: "weave/test",
  _placeId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    let stmt = mDBConn.createStatement(
      "INSERT INTO moz_anno_attributes (name) VALUES (:anno)"
    );
    stmt.params['anno'] = this._obsoleteWeaveAttribute;
    stmt.execute();
    stmt.finalize();
    stmt = mDBConn.createStatement(
      `INSERT INTO moz_annos (place_id, anno_attribute_id)
       VALUES (:place_id,
         (SELECT id FROM moz_anno_attributes WHERE name = :anno)
       )`
    );
    stmt.params['place_id'] = this._placeId;
    stmt.params['anno'] = this._obsoleteWeaveAttribute;
    stmt.execute();
    stmt.finalize();
  },

  check: function() {
    
    let stmt = mDBConn.createStatement(
      "SELECT id FROM moz_anno_attributes WHERE name = :anno"
    );
    stmt.params['anno'] = this._obsoleteWeaveAttribute;
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});

tests.push({
  name: "A.2",
  desc: "Remove obsolete annotations from moz_items_annos",

  _obsoleteSyncAttribute: "sync/children",
  _obsoleteGuidAttribute: "placesInternal/GUID",
  _obsoleteWeaveAttribute: "weave/test",
  _placeId: null,
  _bookmarkId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    this._bookmarkId = addBookmark(this._placeId);
    
    let stmt = mDBConn.createStatement(
      `INSERT INTO moz_anno_attributes (name)
       VALUES (:anno1), (:anno2), (:anno3)`
    );
    stmt.params['anno1'] = this._obsoleteSyncAttribute;
    stmt.params['anno2'] = this._obsoleteGuidAttribute;
    stmt.params['anno3'] = this._obsoleteWeaveAttribute;
    stmt.execute();
    stmt.finalize();
    stmt = mDBConn.createStatement(
      `INSERT INTO moz_items_annos (item_id, anno_attribute_id)
       SELECT :item_id, id
       FROM moz_anno_attributes
       WHERE name IN (:anno1, :anno2, :anno3)`
    );
    stmt.params['item_id'] = this._bookmarkId;
    stmt.params['anno1'] = this._obsoleteSyncAttribute;
    stmt.params['anno2'] = this._obsoleteGuidAttribute;
    stmt.params['anno3'] = this._obsoleteWeaveAttribute;
    stmt.execute();
    stmt.finalize();
  },

  check: function() {
    
    let stmt = mDBConn.createStatement(
      `SELECT id FROM moz_anno_attributes
       WHERE name IN (:anno1, :anno2, :anno3)`
    );
    stmt.params['anno1'] = this._obsoleteSyncAttribute;
    stmt.params['anno2'] = this._obsoleteGuidAttribute;
    stmt.params['anno3'] = this._obsoleteWeaveAttribute;
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});

tests.push({
  name: "A.3",
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
    let stmt = mDBConn.createStatement("SELECT id FROM moz_bookmarks WHERE parent = 0");
    do_check_false(stmt.executeStep());
    stmt.finalize();
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
                PlacesUtils.getString("BookmarksMenuFolderTitle"));
    do_check_eq(bs.getItemTitle(bs.tagsFolder),
                PlacesUtils.getString("TagsFolderTitle"));
    do_check_eq(bs.getItemTitle(bs.unfiledBookmarksFolder),
                PlacesUtils.getString("UnsortedBookmarksFolderTitle"));
    do_check_eq(bs.getItemTitle(bs.toolbarFolder),
                PlacesUtils.getString("BookmarksToolbarFolderTitle"));
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
  _placeId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    this._tagId = addBookmark(null, bs.TYPE_FOLDER, bs.tagsFolder);
    
    this._bookmarkId = addBookmark(this._placeId, bs.TYPE_BOOKMARK, this._tagId);
    
    this._separatorId = addBookmark(null, bs.TYPE_SEPARATOR, this._tagId);
    
    this._folderId = addBookmark(null, bs.TYPE_FOLDER, this._tagId);
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
  name: "D.6",
  desc: "Fix wrong item types | bookmarks",

  _separatorId: null,
  _folderId: null,
  _placeId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    this._separatorId = addBookmark(this._placeId, bs.TYPE_SEPARATOR);
    
    this._folderId = addBookmark(this._placeId, bs.TYPE_FOLDER);
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
  name: "D.9",
  desc: "Fix wrong parents",

  _bookmarkId: null,
  _separatorId: null,
  _bookmarkId1: null,
  _bookmarkId2: null,
  _placeId: null,

  setup: function() {
    
    this._placeId = addPlace();
    
    this._bookmarkId = addBookmark(this._placeId, bs.TYPE_BOOKMARK);
    
    this._separatorId = addBookmark(null, bs.TYPE_SEPARATOR);
    
    this._bookmarkId1 = addBookmark(this._placeId, bs.TYPE_BOOKMARK, this._bookmarkId);
    this._bookmarkId2 = addBookmark(this._placeId, bs.TYPE_BOOKMARK, this._separatorId);
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
    stmt.finalize();
  }
});



tests.push({
  name: "D.10",
  desc: "Recalculate positions",

  _unfiledBookmarks: [],
  _toolbarBookmarks: [],

  setup: function() {
    const NUM_BOOKMARKS = 20;
    bs.runInBatchMode({
      runBatched: function (aUserData) {
        
        for (let i = 0; i < NUM_BOOKMARKS; i++) {
          bs.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                            NetUtil.newURI("http://example.com/"),
                            bs.DEFAULT_INDEX, "testbookmark");
        }
        for (let i = 0; i < NUM_BOOKMARKS; i++) {
          bs.insertBookmark(PlacesUtils.toolbarFolderId,
                            NetUtil.newURI("http://example.com/"),
                            bs.DEFAULT_INDEX, "testbookmark");
        }
      }
    }, null);

    function randomize_positions(aParent, aResultArray) {
      let stmt = mDBConn.createStatement(
        `UPDATE moz_bookmarks SET position = :rand
         WHERE id IN (
           SELECT id FROM moz_bookmarks WHERE parent = :parent
           ORDER BY RANDOM() LIMIT 1
         )`
      );
      for (let i = 0; i < (NUM_BOOKMARKS / 2); i++) {
        stmt.params["parent"] = aParent;
        stmt.params["rand"] = Math.round(Math.random() * (NUM_BOOKMARKS - 1));
        stmt.execute();
        stmt.reset();
      }
      stmt.finalize();

      
      stmt = mDBConn.createStatement(
        `SELECT id, position
         FROM moz_bookmarks WHERE parent = :parent
         ORDER BY position ASC, ROWID ASC`
      );
      stmt.params["parent"] = aParent;
      while (stmt.executeStep()) {
        aResultArray.push(stmt.row.id);
        print(stmt.row.id + "\t" + stmt.row.position + "\t" +
              (aResultArray.length - 1));
      }
      stmt.finalize();
    }

    
    randomize_positions(PlacesUtils.unfiledBookmarksFolderId,
                        this._unfiledBookmarks);
    randomize_positions(PlacesUtils.toolbarFolderId, this._toolbarBookmarks);
  },

  check: function() {
    function check_order(aParent, aResultArray) {
      
      let stmt = mDBConn.createStatement(
        `SELECT id, position FROM moz_bookmarks WHERE parent = :parent
         ORDER BY position ASC`
      );
      stmt.params["parent"] = aParent;
      let pass = true;
      while (stmt.executeStep()) {
        print(stmt.row.id + "\t" + stmt.row.position);
        if (aResultArray.indexOf(stmt.row.id) != stmt.row.position) {
          pass = false;
        }
      }
      stmt.finalize();
      if (!pass) {
        dump_table("moz_bookmarks");
        do_throw("Unexpected unfiled bookmarks order.");
      }
    }

    check_order(PlacesUtils.unfiledBookmarksFolderId, this._unfiledBookmarks);
    check_order(PlacesUtils.toolbarFolderId, this._toolbarBookmarks);
  }
});



tests.push({
  name: "D.12",
  desc: "Fix empty-named tags",

  setup: function() {
    
    let placeId = addPlace();
    
    this._untitledTagId = addBookmark(null, bs.TYPE_FOLDER, bs.tagsFolder, null, null, "");
    
    addBookmark(placeId, bs.TYPE_BOOKMARK, this._untitledTagId);
    
    this._untitledFolderId = addBookmark(null, bs.TYPE_FOLDER, bs.toolbarFolder, null, null, "");
    
    this._titledTagId = addBookmark(null, bs.TYPE_FOLDER, bs.tagsFolder, null, null, "titledTag");
    
    addBookmark(placeId, bs.TYPE_BOOKMARK, this._titledTagId);
    
    this._titledFolderId = addBookmark(null, bs.TYPE_FOLDER, bs.toolbarFolder, null, null, "titledFolder");
  },

  check: function() {
    
    let stmt = mDBConn.createStatement(
      "SELECT title FROM moz_bookmarks WHERE id = :id"
    );
    stmt.params["id"] = this._untitledTagId;
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.row.title, "(notitle)");
    stmt.reset();
    stmt.params["id"] = this._untitledFolderId;
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.row.title, "");
    stmt.reset();
    stmt.params["id"] = this._titledTagId;
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.row.title, "titledTag");
    stmt.reset();
    stmt.params["id"] = this._titledFolderId;
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.row.title, "titledFolder");
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
    
    let stmt = mDBConn.createStatement("INSERT INTO moz_keywords (id, keyword, place_id) VALUES(:id, :keyword, :place_id)");
    stmt.params["id"] = 1;
    stmt.params["keyword"] = "unused";
    stmt.params["place_id"] = 100;
    stmt.execute();
    stmt.finalize();
  },

  check: function() {
    
    let stmt = mDBConn.createStatement("SELECT id FROM moz_keywords WHERE keyword = :keyword");
    
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
  desc: "Recalculate visit_count and last_visit_date",

  setup: function() {
    function setVisitCount(aURL, aValue) {
      let stmt = mDBConn.createStatement(
        "UPDATE moz_places SET visit_count = :count WHERE url = :url"
      );
      stmt.params.count = aValue;
      stmt.params.url = aURL;
      stmt.execute();
      stmt.finalize();
    }
    function setLastVisitDate(aURL, aValue) {
      let stmt = mDBConn.createStatement(
        "UPDATE moz_places SET last_visit_date = :date WHERE url = :url"
      );
      stmt.params.date = aValue;
      stmt.params.url = aURL;
      stmt.execute();
      stmt.finalize();
    }

    let now = Date.now() * 1000;
    
    let url = "http://1.moz.org/";
    yield PlacesTestUtils.addVisits({ uri: uri(url), visitDate: now++ });
    
    url = "http://2.moz.org/";
    yield PlacesTestUtils.addVisits({ uri: uri(url), visitDate: now++ });
    setVisitCount(url, 10);
    
    url = "http://3.moz.org/";
    yield PlacesTestUtils.addVisits({ uri: uri(url), visitDate: now++ });
    setLastVisitDate(url, now++);
    
    url = "http://4.moz.org/";
    yield PlacesTestUtils.addVisits({ uri: uri(url), visitDate: now++ });
    setVisitCount(url, 10);
    setLastVisitDate(url, now++);

    
    url = "http://5.moz.org/";
    addPlace(url);
    
    url = "http://6.moz.org/";
    addPlace(url);
    setVisitCount(url, 10);
    
    url = "http://7.moz.org/";
    addPlace(url);
    setLastVisitDate(url, now++);
    
    url = "http://8.moz.org/";
    addPlace(url);
    setVisitCount(url, 10);
    setLastVisitDate(url, now++);
  },

  check: function() {
    let stmt = mDBConn.createStatement(
      `SELECT h.id FROM moz_places h
       JOIN moz_historyvisits v ON v.place_id = h.id AND visit_type NOT IN (0,4,7,8)
       GROUP BY h.id HAVING h.visit_count <> count(*)
       UNION ALL
       SELECT h.id FROM moz_places h
       JOIN moz_historyvisits v ON v.place_id = h.id
       GROUP BY h.id HAVING h.last_visit_date <> MAX(v.visit_date)`
    );
    do_check_false(stmt.executeStep());
    stmt.finalize();
  }
});



tests.push({
  name: "L.3",
  desc: "recalculate hidden for redirects.",

  setup: function() {
    PlacesTestUtils.addVisits([
      { uri: NetUtil.newURI("http://l3.moz.org/"),
        transition: TRANSITION_TYPED },
      { uri: NetUtil.newURI("http://l3.moz.org/redirecting/"),
        transition: TRANSITION_TYPED },
      { uri: NetUtil.newURI("http://l3.moz.org/redirecting2/"),
        transition: TRANSITION_REDIRECT_TEMPORARY,
        referrer: NetUtil.newURI("http://l3.moz.org/redirecting/") },
      { uri: NetUtil.newURI("http://l3.moz.org/target/"),
        transition: TRANSITION_REDIRECT_PERMANENT,
        referrer: NetUtil.newURI("http://l3.moz.org/redirecting2/") },
    ]);
  },

  check: function () {
    return new Promise(resolve => {
      let stmt = mDBConn.createAsyncStatement(
        "SELECT h.url FROM moz_places h WHERE h.hidden = 1"
      );
      stmt.executeAsync({
        _count: 0,
        handleResult: function(aResultSet) {
          for (let row; (row = aResultSet.getNextRow());) {
            let url = row.getResultByIndex(0);
            do_check_true(/redirecting/.test(url));
            this._count++;
          }
        },
        handleError: function(aError) {
        },
        handleCompletion: function(aReason) {
          dump_table("moz_places");
          dump_table("moz_historyvisits");
          do_check_eq(aReason, Ci.mozIStorageStatementCallback.REASON_FINISHED);
          do_check_eq(this._count, 2);
          resolve();
        }
      });
      stmt.finalize();
    });
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

  setup: function* () {
    
    yield PlacesTestUtils.addVisits([
      { uri: this._uri1 },
      { uri: this._uri2 },
    ]);

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
    fs.setAndFetchFaviconForPage(this._uri2, SMALLPNG_DATA_URI, false,
                                 PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);
    yield PlacesUtils.keywords.insert({ url: this._uri1.spec, keyword: "testkeyword" });
    as.setPageAnnotation(this._uri2, "anno", "anno", 0, as.EXPIRE_NEVER);
    as.setItemAnnotation(this._bookmarkId, "anno", "anno", 0, as.EXPIRE_NEVER);
  },

  check: Task.async(function* () {
    
    let isVisited = yield promiseIsURIVisited(this._uri1);
    do_check_true(isVisited);
    isVisited = yield promiseIsURIVisited(this._uri2);
    do_check_true(isVisited);

    do_check_eq(bs.getBookmarkURI(this._bookmarkId).spec, this._uri1.spec);
    do_check_eq(bs.getItemIndex(this._folderId), 0);
    do_check_eq(bs.getItemType(this._folderId), bs.TYPE_FOLDER);
    do_check_eq(bs.getItemType(this._separatorId), bs.TYPE_SEPARATOR);

    do_check_eq(ts.getTagsForURI(this._uri1).length, 1);
    do_check_eq((yield PlacesUtils.keywords.fetch({ url: this._uri1.spec })).keyword, "testkeyword");
    do_check_eq(as.getPageAnnotation(this._uri2, "anno"), "anno");
    do_check_eq(as.getItemAnnotation(this._bookmarkId, "anno"), "anno");

    yield new Promise(resolve => {
      fs.getFaviconURLForPage(this._uri2, aFaviconURI => {
        do_check_true(aFaviconURI.equals(SMALLPNG_DATA_URI));
        resolve();
      });
    });
  })
});




function run_test()
{
  run_next_test();
}

add_task(function test_preventive_maintenance()
{
  
  
  yield PlacesTestUtils.addVisits(uri("http://force.bookmarks.hash"));
  do_check_false(bs.isBookmarked(uri("http://force.bookmarks.hash")));

  
  let stmt = mDBConn.createStatement("SELECT MAX(id) FROM moz_bookmarks");
  stmt.executeStep();
  defaultBookmarksMaxId = stmt.getInt32(0);
  stmt.finalize();
  do_check_true(defaultBookmarksMaxId > 0);

  for ([, test] in Iterator(tests)) {
    dump("\nExecuting test: " + test.name + "\n" + "*** " + test.desc + "\n");
    yield test.setup();

    let promiseMaintenanceFinished =
        promiseTopicObserved(FINISHED_MAINTENANCE_NOTIFICATION_TOPIC);
    PlacesDBUtils.maintenanceOnIdle();
    yield promiseMaintenanceFinished;

    
    do_check_neq(Services.prefs.getIntPref("places.database.lastMaintenance"), null);

    yield test.check();

    cleanDatabase();
  }

  
  do_check_eq(bs.getFolderIdForItem(bs.placesRoot), 0);
  do_check_eq(bs.getFolderIdForItem(bs.bookmarksMenuFolder), bs.placesRoot);
  do_check_eq(bs.getFolderIdForItem(bs.tagsFolder), bs.placesRoot);
  do_check_eq(bs.getFolderIdForItem(bs.unfiledBookmarksFolder), bs.placesRoot);
  do_check_eq(bs.getFolderIdForItem(bs.toolbarFolder), bs.placesRoot);
});
