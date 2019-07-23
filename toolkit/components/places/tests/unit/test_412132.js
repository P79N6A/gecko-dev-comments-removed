













































const bmServ =
  Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
  getService(Ci.nsINavBookmarksService);
const histServ =
  Cc["@mozilla.org/browser/nav-history-service;1"].
  getService(Ci.nsINavHistoryService);
const lmServ =
  Cc["@mozilla.org/browser/livemark-service;2"].
  getService(Ci.nsILivemarkService);

var defaultBookmarksMaxId;
var dbConn;
var tests = [];

tests.push({
  desc: ["Frecency of unvisited, separately bookmarked livemark item's URI ",
         "should be zero after bookmark's URI changed."].join(""),
  run: function ()
  {
    var lmItemURL;
    var lmItemURI;
    var bmId;
    var frecencyBefore;
    var frecencyAfter;

    
    
    lmItemURL = "http://example.com/livemark-item";
    lmItemURI = uri(lmItemURL);
    createLivemark(lmItemURI);
    bmId = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                 lmItemURI,
                                 bmServ.DEFAULT_INDEX,
                                 "bookmark title");

    
    frecencyBefore = getFrecency(lmItemURL);
    do_check_neq(frecencyBefore, 0);

    bmServ.changeBookmarkURI(bmId, uri("http://example.com/new-uri"));

    
    frecencyAfter = getFrecency(lmItemURL);
    do_check_eq(frecencyAfter, 0);
  }
});

tests.push({
  desc: ["Frecency of visited, separately bookmarked livemark item's URI ",
         "should not be zero after bookmark's URI changed."].join(""),
  run: function ()
  {
    var lmItemURL;
    var lmItemURI;
    var bmId;
    var frecencyBefore;
    var frecencyAfter;

    
    
    lmItemURL = "http://example.com/livemark-item";
    lmItemURI = uri(lmItemURL);
    createLivemark(lmItemURI);
    bmId = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                 lmItemURI,
                                 bmServ.DEFAULT_INDEX,
                                 "bookmark title");

    
    frecencyBefore = getFrecency(lmItemURL);
    do_check_neq(frecencyBefore, 0);

    visit(lmItemURI);

    bmServ.changeBookmarkURI(bmId, uri("http://example.com/new-uri"));

    
    frecencyAfter = getFrecency(lmItemURL);
    do_check_neq(frecencyAfter, 0);
  }
});

tests.push({
  desc: ["After changing URI of bookmark, frecency of bookmark's original URI ",
         "should be zero if original URI is unvisited and no longer ",
         "bookmarked."].join(""),
  run: function ()
  {
    var url;
    var bmId;
    var frecencyBefore;
    var frecencyAfter;

    url = "http://example.com/1";
    bmId = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                 uri(url),
                                 bmServ.DEFAULT_INDEX,
                                 "bookmark title");

    
    frecencyBefore = getFrecency(url);
    do_check_neq(frecencyBefore, 0);

    bmServ.changeBookmarkURI(bmId, uri("http://example.com/2"));

    
    frecencyAfter = getFrecency(url);
    do_check_eq(frecencyAfter, 0);
  }
});

tests.push({
  desc: ["After changing URI of bookmark, frecency of bookmark's original URI ",
         "should not be zero if original URI is visited."].join(""),
  run: function ()
  {
    var bmURL;
    var bmURI;
    var bmId;
    var frecencyBefore;
    var frecencyAfter;

    bmURL = "http://example.com/1";
    bmURI = uri(bmURL);

    bmId = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                 bmURI,
                                 bmServ.DEFAULT_INDEX,
                                 "bookmark title");

    
    frecencyBefore = getFrecency(bmURL);
    do_check_neq(frecencyBefore, 0);

    visit(bmURI);
    bmServ.changeBookmarkURI(bmId, uri("http://example.com/2"));

    
    frecencyAfter = getFrecency(bmURL);
    do_check_neq(frecencyAfter, 0);
  }
});

tests.push({
  desc: ["After changing URI of bookmark, frecency of bookmark's original URI ",
         "should not be zero if original URI is still bookmarked."].join(""),
  run: function ()
  {
    var bmURL;
    var bmURI;
    var bm1Id;
    var bm2Id;
    var frecencyBefore;
    var frecencyAfter;

    bmURL = "http://example.com/1";
    bmURI = uri(bmURL);

    bm1Id = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                 bmURI,
                                 bmServ.DEFAULT_INDEX,
                                 "bookmark 1 title");

    bm2Id = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                  bmURI,
                                  bmServ.DEFAULT_INDEX,
                                  "bookmark 2 title");


    
    frecencyBefore = getFrecency(bmURL);
    do_check_neq(frecencyBefore, 0);

    bmServ.changeBookmarkURI(bm1Id, uri("http://example.com/2"));

    
    frecencyAfter = getFrecency(bmURL);
    do_check_neq(frecencyAfter, 0);
  }
});

tests.push({
  desc: "Changing the URI of a nonexistent bookmark should fail.",
  run: function ()
  {
    var stmt;
    var maxId;
    var bmId;

    function tryChange(itemId)
    {
      var failed;

      failed= false;
      try
      {
        bmServ.changeBookmarkURI(itemId + 1, uri("http://example.com/2"));
      }
      catch (exc)
      {
        failed= true;
      }
      do_check_true(failed);
    }

    
    
    stmt = dbConn.createStatement("SELECT MAX(id) FROM moz_bookmarks");
    stmt.executeStep();
    maxId = stmt.getInt32(0);
    stmt.finalize();
    tryChange(maxId + 1);

    
    bmId = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                 uri("http://example.com/"),
                                 bmServ.DEFAULT_INDEX,
                                 "bookmark title");
    bmServ.removeItem(bmId);
    tryChange(bmId);
  }
});



function createLivemark(lmItemURI)
{
  var lmId;
  var lmItemId;

  lmId = lmServ.createLivemarkFolderOnly(bmServ.unfiledBookmarksFolder,
                                         "livemark title",
                                         uri("http://www.mozilla.org/"),
                                         uri("http://www.mozilla.org/news.rdf"),
                                         -1);
  lmItemId = bmServ.insertBookmark(lmId,
                                   lmItemURI,
                                   bmServ.DEFAULT_INDEX,
                                   "livemark item title");
  return lmItemId;
}

function getFrecency(url)
{
  var sql;
  var stmt;
  var frecency;

  sql = "SELECT frecency FROM moz_places_view WHERE url = ?1";
  stmt = dbConn.createStatement(sql);
  stmt.bindUTF8StringParameter(0, url);
  do_check_true(stmt.executeStep());
  frecency = stmt.getInt32(0);
  print("frecency=" + frecency);
  stmt.finalize();

  return frecency;
}

function prepTest(testIndex, testName)
{
  print("Test " + testIndex + ": " + testName);
  histServ.QueryInterface(Ci.nsIBrowserHistory).removeAllPages();
  dbConn.executeSimpleSQL("DELETE FROM moz_places_view");
  dbConn.executeSimpleSQL("DELETE FROM moz_bookmarks WHERE id > " +
                          defaultBookmarksMaxId);
}

function visit(uri)
{
  histServ.addVisit(uri,
                    Date.now() * 1000,
                    null,
                    histServ.TRANSITION_BOOKMARK,
                    false,
                    0);
}



function run_test()
{
  var stmt;

  dbConn =
    Cc["@mozilla.org/browser/nav-history-service;1"].
    getService(Ci.nsPIPlacesDatabase).
    DBConnection;

  stmt = dbConn.createStatement("SELECT MAX(id) FROM moz_bookmarks");
  stmt.executeStep();
  defaultBookmarksMaxId = stmt.getInt32(0);
  stmt.finalize();
  do_check_true(defaultBookmarksMaxId > 0);

  for (let i= 0; i < tests.length; i++)
  {
    prepTest(i, tests[i].desc);
    tests[i].run();
  }
}
