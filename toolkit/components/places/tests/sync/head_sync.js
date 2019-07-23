








































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");


let (commonFile = do_get_file("../head_common.js", false)) {
  let uri = Services.io.newFileURI(commonFile);
  Services.scriptloader.loadSubScript(uri.spec, this);
}


















function new_test_bookmark_uri_event(aBookmarkId, aExpectedURI, aExpected, aFinish)
{
  let stmt = DBConn().createStatement(
    "SELECT moz_places.url " +
    "FROM moz_bookmarks INNER JOIN moz_places " +
    "ON moz_bookmarks.fk = moz_places.id " +
    "WHERE moz_bookmarks.id = ?1"
  );
  stmt.bindInt64Parameter(0, aBookmarkId);

  if (aExpected) {
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.getUTF8String(0), aExpectedURI);
  }
  else {
    do_check_false(stmt.executeStep());
  }
  stmt.reset();
  stmt.finalize();
  stmt = null;

  if (aFinish)
    do_test_finished();
}
















function new_test_visit_uri_event(aVisitId, aExpectedURI, aExpected, aFinish)
{
  let stmt = DBConn().createStatement(
    "SELECT moz_places.url " +
    "FROM moz_historyvisits INNER JOIN moz_places " +
    "ON moz_historyvisits.place_id = moz_places.id " +
    "WHERE moz_historyvisits.id = ?1"
  );
  stmt.bindInt64Parameter(0, aVisitId);

  if (aExpected) {
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.getUTF8String(0), aExpectedURI);
  }
  else {
    do_check_false(stmt.executeStep());
  }
  stmt.reset();
  stmt.finalize();
  stmt = null;

  if (aFinish)
    do_test_finished();
}

