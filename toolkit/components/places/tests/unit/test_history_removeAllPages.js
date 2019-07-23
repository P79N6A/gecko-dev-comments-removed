







































let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
let bh = hs.QueryInterface(Ci.nsIBrowserHistory);
let mDBConn = hs.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;
let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
let as = Cc["@mozilla.org/browser/annotation-service;1"].
         getService(Ci.nsIAnnotationService);
let os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
let lms = Cc["@mozilla.org/browser/livemark-service;2"].
          getService(Ci.nsILivemarkService);

const kSyncFinished = "places-sync-finished";

const EXPECTED_SYNCS = 4;

function add_fake_livemark() {
  let lmId = lms.createLivemarkFolderOnly(bs.toolbarFolder,
                                          "Livemark",
                                          uri("http://www.mozilla.org/"),
                                          uri("http://www.mozilla.org/test.xml"),
                                          bs.DEFAULT_INDEX);
  
  bs.insertBookmark(lmId, uri("http://visited.livemark.com/"),
                    bs.DEFAULT_INDEX, "visited");
  hs.addVisit(uri("http://visited.livemark.com/"), Date.now(), null,
              hs.TRANSITION_BOOKMARK, false, 0);
  
  bs.insertBookmark(lmId, uri("http://unvisited.livemark.com/"),
                    bs.DEFAULT_INDEX, "unvisited");
}

let observer = {
  onBeginUpdateBatch: function() {
  },
  onEndUpdateBatch: function() {
  },
  onVisit: function(aURI, aVisitID, aTime, aSessionID, aReferringID, aTransitionType) {
  },
  onTitleChanged: function(aURI, aPageTitle) {
  },
  onBeforeDeleteURI: function(aURI) {
  },
  onDeleteURI: function(aURI) {
  },

  onClearHistory: function() {
    
    do_check_eq(0, bh.count);

    
    
    
    
    stmt = mDBConn.createStatement(
      "SELECT id FROM moz_places_temp WHERE frecency > 0 LIMIT 1");
    do_check_false(stmt.executeStep());
    stmt.finalize();

    stmt = mDBConn.createStatement(
      "SELECT h.id FROM moz_places_temp h WHERE h.frecency = -2 " +
        "AND EXISTS (SELECT id FROM moz_bookmarks WHERE fk = h.id) LIMIT 1");
    do_check_true(stmt.executeStep());
    stmt.finalize();

    
    stmt = mDBConn.createStatement(
      "SELECT id FROM moz_places_temp WHERE visit_count <> 0 LIMIT 1");
    do_check_false(stmt.executeStep());
    stmt.finalize();

    
    stmt = mDBConn.createStatement(
      "SELECT * FROM (SELECT id FROM moz_historyvisits_temp LIMIT 1) " +
      "UNION ALL " +
      "SELECT * FROM (SELECT id FROM moz_historyvisits LIMIT 1)");
    do_check_false(stmt.executeStep());
    stmt.finalize();

    
    bs.insertBookmark(bs.unfiledBookmarksFolder, uri("place:folder=4"),
                      bs.DEFAULT_INDEX, "shortcut");
  },

  onPageChanged: function(aURI, aWhat, aValue) {
  },
  onPageExpired: function(aURI, aVisitTime, aWholeEntry) {
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsINavHistoryObserver) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
}
hs.addObserver(observer, false);

let syncObserver = {
  _runCount: 0,
  observe: function (aSubject, aTopic, aData) {
    if (++this._runCount < EXPECTED_SYNCS)
      return;
    if (this._runCount == EXPECTED_SYNCS) {
      bh.removeAllPages();
      return;
    }

    
    stmt = mDBConn.createStatement(
      "SELECT id FROM moz_places_temp LIMIT 1");
    do_check_false(stmt.executeStep());
    stmt.finalize();

    
    
    
    stmt = mDBConn.createStatement(
      "SELECT id FROM moz_places WHERE frecency > 0 LIMIT 1");
    do_check_false(stmt.executeStep());
    stmt.finalize();

    stmt = mDBConn.createStatement(
      "SELECT h.id FROM moz_places h WHERE h.frecency = -2 " +
        "AND EXISTS (SELECT id FROM moz_bookmarks WHERE fk = h.id) LIMIT 1");
    do_check_true(stmt.executeStep());
    stmt.finalize();

    
    stmt = mDBConn.createStatement(
      "SELECT id FROM moz_places WHERE visit_count <> 0 LIMIT 1");
    do_check_false(stmt.executeStep());
    stmt.finalize();


    
    stmt = mDBConn.createStatement(
      "SELECT h.id FROM moz_places h WHERE SUBSTR(h.url, 1, 6) <> 'place:' "+
        "AND NOT EXISTS (SELECT id FROM moz_bookmarks WHERE fk = h.id) LIMIT 1");
    do_check_false(stmt.executeStep());
    stmt.finalize();

    
    stmt = mDBConn.createStatement(
      "SELECT f.id FROM moz_favicons f WHERE NOT EXISTS " +
        "(SELECT id FROM moz_places WHERE favicon_id = f.id) LIMIT 1");
    do_check_false(stmt.executeStep());
    stmt.finalize();

    
    stmt = mDBConn.createStatement(
      "SELECT a.id FROM moz_annos a WHERE NOT EXISTS " +
        "(SELECT id FROM moz_places WHERE id = a.place_id) LIMIT 1");
    do_check_false(stmt.executeStep());
    stmt.finalize();

    
    stmt = mDBConn.createStatement(
      "SELECT i.place_id FROM moz_inputhistory i WHERE NOT EXISTS " +
        "(SELECT id FROM moz_places WHERE id = i.place_id) LIMIT 1");
    do_check_false(stmt.executeStep());
    stmt.finalize();

    
    stmt = mDBConn.createStatement(
      "SELECT h.id FROM moz_places h " +
      "WHERE SUBSTR(h.url, 1, 6) = 'place:' AND h.frecency <> 0 LIMIT 1");
    do_check_false(stmt.executeStep());
    stmt.finalize();

    
    stmt = mDBConn.createStatement(
      "SELECT h.id FROM moz_places h " +
      "JOIN moz_bookmarks b ON h.id = b.fk " +
      "JOIN moz_bookmarks bp ON bp.id = b.parent " +
      "JOIN moz_items_annos t ON t.item_id = bp.id " +
      "JOIN moz_anno_attributes n ON t.anno_attribute_id = n.id " +
      "WHERE n.name = 'livemark/feedURI' AND h.frecency <> 0 LIMIT 1");
    do_check_false(stmt.executeStep());
    stmt.finalize();

    do_test_finished();
  }
}
os.addObserver(syncObserver, kSyncFinished, false);



function run_test() {
  
  add_fake_livemark();

  
  hs.addVisit(uri("http://typed.mozilla.org"), Date.now(), null,
              hs.TRANSITION_TYPED, false, 0);

  hs.addVisit(uri("http://link.mozilla.org"), Date.now(), null,
              hs.TRANSITION_LINK, false, 0);
  hs.addVisit(uri("http://download.mozilla.org"), Date.now(), null,
              hs.TRANSITION_DOWNLOAD, false, 0);
  hs.addVisit(uri("http://invalid.mozilla.org"), Date.now(), null,
              0, false, 0); 
  hs.addVisit(uri("http://redir_temp.mozilla.org"), Date.now(),
              uri("http://link.mozilla.org"), hs.TRANSITION_REDIRECT_TEMPORARY,
              true, 0);
  hs.addVisit(uri("http://redir_perm.mozilla.org"), Date.now(),
              uri("http://link.mozilla.org"), hs.TRANSITION_REDIRECT_PERMANENT,
              true, 0);

  
  bs.insertBookmark(bs.unfiledBookmarksFolder, uri("place:folder=4"),
                    bs.DEFAULT_INDEX, "shortcut");
  
  
  
  
  as.setPageAnnotation(uri("http://download.mozilla.org"), "never", "never", 0,
                       as.EXPIRE_NEVER);

  
  
  
  bs.insertBookmark(bs.unfiledBookmarksFolder, uri("http://typed.mozilla.org"),
                    bs.DEFAULT_INDEX, "bookmark");

  
  hs.addVisit(uri("http://typed.mozilla.org"), Date.now(), null,
              hs.TRANSITION_BOOKMARK, false, 0);

  do_test_pending();
}
