












































Components.utils.import("resource://gre/modules/PlacesBackground.jsm");

const TEST_URI = "http://test.com/";

let db = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsPIPlacesDatabase).
         DBConnection;

function run_test()
{
  
  let bh = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);
  let id1 = bh.insertBookmark(bh.unfiledBookmarksFolder, uri(TEST_URI),
                              bh.DEFAULT_INDEX, "test");

  
  PlacesBackground.dispatch(new_test_bookmark_uri_event(id1, TEST_URI, true),
                            Ci.nsIEventTarget.DISPATCH_SYNC);

  
  let stmt = db.createStatement(
    "SELECT fk " +
    "FROM moz_bookmarks " +
    "WHERE id = ?"
  );
  stmt.bindInt64Parameter(0, id1);
  do_check_true(stmt.executeStep());
  let place_id = stmt.getInt64(0);
  stmt.finalize();
  stmt = null;

  
  let id2 = bh.insertBookmark(bh.toolbarFolder, uri(TEST_URI),
                              bh.DEFAULT_INDEX, "test");
  do_check_neq(id1, id2);

  
  PlacesBackground.dispatch(new_test_bookmark_uri_event(id2, TEST_URI, true),
                            Ci.nsIEventTarget.DISPATCH_SYNC);

  
  stmt = db.createStatement(
    "SELECT * " +
    "FROM moz_bookmarks " +
    "WHERE id = ?1 " +
    "AND fk = ?2"
  );
  stmt.bindInt64Parameter(0, id2);
  stmt.bindInt64Parameter(1, place_id);
  do_check_true(stmt.executeStep());
  stmt.finalize();
  stmt = null;

  
  do_test_pending();
  finish_test();
}
