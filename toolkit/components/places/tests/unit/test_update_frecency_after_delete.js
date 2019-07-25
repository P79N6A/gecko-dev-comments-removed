














































const bmServ =
  Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
  getService(Ci.nsINavBookmarksService);
const histServ =
  Cc["@mozilla.org/browser/nav-history-service;1"].
  getService(Ci.nsINavHistoryService);
const lmServ =
  Cc["@mozilla.org/browser/livemark-service;2"].
  getService(Ci.nsILivemarkService);

const dbConn =
  Cc["@mozilla.org/browser/nav-history-service;1"].
  getService(Ci.nsPIPlacesDatabase).
  DBConnection;

var tests = [];

tests.push({
  desc: ["Frecency of unvisited, separately bookmarked livemark item's URI ",
         "should be zero after bookmark removed."].join(""),
  run: function () {
    
    
    let lmItemURL = "http://example.com/livemark-item";
    let lmItemURI = uri(lmItemURL);
    createLivemark(lmItemURI);
    let bmId = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                     lmItemURI,
                                     bmServ.DEFAULT_INDEX,
                                     "bookmark title");

    
    do_check_neq(getFrecency(lmItemURL), 0);

    bmServ.removeItem(bmId);

    
    do_check_eq(getFrecency(lmItemURL), 0);
    runNextTest();
  }
});

tests.push({
  desc: ["Frecency of visited, separately bookmarked livemark item's URI ",
         "should not be zero after bookmark removed."].join(""),
  run: function () {
    
    
    let lmItemURL = "http://example.com/livemark-item";
    let lmItemURI = uri(lmItemURL);
    createLivemark(lmItemURI);
    let bmId = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                     lmItemURI,
                                     bmServ.DEFAULT_INDEX,
                                     "bookmark title");

    
    do_check_neq(getFrecency(lmItemURL), 0);

    visit(lmItemURI);
    bmServ.removeItem(bmId);

    
    do_check_neq(getFrecency(lmItemURL), 0);
    runNextTest();
  }
});

tests.push({
  desc: ["After removing bookmark, frecency of bookmark's URI should be zero ",
         "if URI is unvisited and no longer bookmarked."].join(""),
  run: function () {
    let url = "http://example.com/1";
    let bmId = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                     uri(url),
                                     bmServ.DEFAULT_INDEX,
                                     "bookmark title");

    
    do_check_neq(getFrecency(url), 0);

    bmServ.removeItem(bmId);

    
    do_check_eq(getFrecency(url), 0);
    runNextTest();
  }
});

tests.push({
  desc: ["After removing bookmark, frecency of bookmark's URI should not be ",
         "zero if URI is visited."].join(""),
  run: function () {
    let bmURL = "http://example.com/1";
    let bmURI = uri(bmURL);

    let bmId = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                     bmURI,
                                     bmServ.DEFAULT_INDEX,
                                     "bookmark title");

    
    do_check_neq(getFrecency(bmURL), 0);

    visit(bmURI);
    bmServ.removeItem(bmId);

    
    do_check_neq(getFrecency(bmURL), 0);
    runNextTest();
  }
});

tests.push({
  desc: ["After removing bookmark, frecency of bookmark's URI should not be ",
         "zero if URI is still bookmarked."].join(""),
  run: function () {
    let bmURL = "http://example.com/1";
    let bmURI = uri(bmURL);

    let bm1Id = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                      bmURI,
                                      bmServ.DEFAULT_INDEX,
                                      "bookmark 1 title");

    bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                          bmURI,
                          bmServ.DEFAULT_INDEX,
                          "bookmark 2 title");

    
    do_check_neq(getFrecency(bmURL), 0);

    bmServ.removeItem(bm1Id);

    
    do_check_neq(getFrecency(bmURL), 0);
    runNextTest();
  }
});

tests.push({
  desc: ["Frecency of unvisited, separately bookmarked livemark item's URI ",
         "should be zero after all children removed from bookmark's ",
         "parent."].join(""),
  run: function () {
    
    
    let lmItemURL = "http://example.com/livemark-item";
    let lmItemURI = uri(lmItemURL);
    createLivemark(lmItemURI);

    bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                          lmItemURI,
                          bmServ.DEFAULT_INDEX,
                          "bookmark title");

    
    do_check_neq(getFrecency(lmItemURL), 0);

    bmServ.removeFolderChildren(bmServ.unfiledBookmarksFolder);

    
    do_check_eq(getFrecency(lmItemURL), 0);
    runNextTest();
  }
});

tests.push({
  desc: ["Frecency of visited, separately bookmarked livemark item's URI ",
         "should not be zero after all children removed from bookmark's ",
         "parent."].join(""),
  run: function () {
    
    
    let lmItemURL = "http://example.com/livemark-item";
    let lmItemURI = uri(lmItemURL);
    createLivemark(lmItemURI);
    bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                          lmItemURI,
                          bmServ.DEFAULT_INDEX,
                          "bookmark title");

    
    do_check_neq(getFrecency(lmItemURL), 0);

    visit(lmItemURI);
    bmServ.removeFolderChildren(bmServ.unfiledBookmarksFolder);

    
    do_check_neq(getFrecency(lmItemURL), 0);
    runNextTest();
  }
});

tests.push({
  desc: ["After removing all children from bookmark's parent, frecency of ",
         "bookmark's URI should be zero if URI is unvisited and no longer ",
         "bookmarked."].join(""),
  run: function () {
    let url = "http://example.com/1";
    bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                          uri(url),
                          bmServ.DEFAULT_INDEX,
                          "bookmark title");

    
    do_check_neq(getFrecency(url), 0);

    bmServ.removeFolderChildren(bmServ.unfiledBookmarksFolder);

    
    do_check_eq(getFrecency(url), 0);
    runNextTest();
  }
});

tests.push({
  desc: ["After removing all children from bookmark's parent, frecency of ",
         "bookmark's URI should not be zero if URI is visited."].join(""),
  run: function () {
    let bmURL = "http://example.com/1";
    let bmURI = uri(bmURL);

    bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                          bmURI,
                          bmServ.DEFAULT_INDEX,
                          "bookmark title");

    
    do_check_neq(getFrecency(bmURL), 0);

    visit(bmURI);
    bmServ.removeFolderChildren(bmServ.unfiledBookmarksFolder);

    
    do_check_neq(getFrecency(bmURL), 0);
    runNextTest();
  }
});

tests.push({
  desc: ["After removing all children from bookmark's parent, frecency of ",
         "bookmark's URI should not be zero if URI is still ",
         "bookmarked."].join(""),
  run: function () {
    let bmURL = "http://example.com/1";
    let bmURI = uri(bmURL);

    bmServ.insertBookmark(bmServ.toolbarFolder,
                          bmURI,
                          bmServ.DEFAULT_INDEX,
                          "bookmark 1 title");

    bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                          bmURI,
                          bmServ.DEFAULT_INDEX,
                          "bookmark 2 title");

    
    do_check_neq(getFrecency(bmURL), 0);

    bmServ.removeFolderChildren(bmServ.unfiledBookmarksFolder);

    
    do_check_neq(getFrecency(bmURL), 0);
    runNextTest();
  }
});










function createLivemark(aLmChildItemURI) {
  let lmItemId = lmServ.createLivemarkFolderOnly(bmServ.unfiledBookmarksFolder,
                                                 "livemark title",
                                                 uri("http://example.com/"),
                                                 uri("http://example.com/rdf"),
                                                 -1);
  let lmChildItemId = bmServ.insertBookmark(lmItemId,
                                            aLmChildItemURI,
                                            bmServ.DEFAULT_INDEX,
                                            "livemark item title");
  return lmChildItemId;
}








function getFrecency(aURL) {
  let sql = "SELECT frecency FROM moz_places_view WHERE url = :url";
  let stmt = dbConn.createStatement(sql);
  stmt.params.url = aURL;
  do_check_true(stmt.executeStep());
  let frecency = stmt.getInt32(0);
  print("frecency=" + frecency);
  stmt.finalize();

  return frecency;
}







function visit(aURI) {
  let visitId = histServ.addVisit(aURI,
                                  Date.now() * 1000,
                                  null,
                                  histServ.TRANSITION_BOOKMARK,
                                  false,
                                  0);
}



function run_test() {
  do_test_pending();
  runNextTest();
}

function runNextTest() {
  if (tests.length) {
    let test = tests.shift();
    print("Test " +  + ": " + test.desc);
    remove_all_bookmarks();
    waitForClearHistory(test.run);
  }
  else {
    do_test_finished();
  }
}
