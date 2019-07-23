






































try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
} catch(ex) {
  do_throw("Could not get nav-bookmarks-service\n");
}

var gTestRoot;
var gURI;
var gItemId1;
var gItemId2;


function run_test() {
  gURI = uri("http://foo.tld.com/");
  gTestRoot = bmsvc.createFolder(bmsvc.placesRoot, "test folder",
                                 bmsvc.DEFAULT_INDEX);

  
  
  
  
  
  do_test_pending();

  gItemId1 = bmsvc.insertBookmark(gTestRoot, gURI, bmsvc.DEFAULT_INDEX, "");
  do_timeout(100, phase2);
}

function phase2() {
  gItemId2 = bmsvc.insertBookmark(gTestRoot, gURI, bmsvc.DEFAULT_INDEX, "");  
  var b = bmsvc.getBookmarkIdsForURI(gURI);
  do_check_eq(b[0], gItemId2);
  do_check_eq(b[1], gItemId1);
  do_timeout(100, phase3);
}

function phase3() {
  
  bmsvc.setItemTitle(gItemId1, "");
  var b = bmsvc.getBookmarkIdsForURI(gURI);
  do_check_eq(b[0], gItemId1);
  do_check_eq(b[1], gItemId2);
  do_test_finished();
}
