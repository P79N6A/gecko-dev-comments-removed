





































var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var pip = hs.QueryInterface(Ci.nsPIPlacesDatabase);
var mDBConn = pip.DBConnection;
var gh = hs.QueryInterface(Ci.nsIGlobalHistory2);
var iconsvc = Cc["@mozilla.org/browser/favicon-service;1"].
              getService(Ci.nsIFaviconService);

const TEST_URI = "http://www.mozilla.org/";
const TEST_TITLE = "testTitle";


function run_test() {
  var testURI = uri(TEST_URI);
  var faviconURI = uri(TEST_URI + "favicon.ico");

  
  gh.addURI(testURI, false, true, null);
  
  iconsvc.setFaviconUrlForPage(testURI, faviconURI);
  
  hs.setPageTitle(testURI, TEST_TITLE);

  
  pip.commitPendingChanges();

  
  
  let stmt = mDBConn.createStatement(
    "SELECT id FROM moz_places_temp WHERE url = :url AND title = :title " +
      "AND favicon_id = (SELECT id FROM moz_favicons WHERE url = :favicon_url)");
  stmt.params["url"] = testURI.spec;
  stmt.params["title"] = TEST_TITLE;
  stmt.params["favicon_url"] = faviconURI.spec;
  do_check_true(stmt.executeStep());
  stmt.finalize();
}
