







































try {
  var lmsvc = Cc["@mozilla.org/browser/livemark-service;2"].
              getService(Ci.nsILivemarkService);
} catch(ex) {
  do_throw("Could not get livemark-service\n");
} 


try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
} catch(ex) {
  do_throw("Could not get nav-bookmarks-service\n");
}


var root = bmsvc.bookmarksRoot;


function run_test() {
  var livemarkId = 
    lmsvc.createLivemarkFolderOnly(bmsvc, root, "foo",
                                   uri("http://example.com/"),
                                   uri("http://example.com/rss.xml"), -1);

  do_check_true(lmsvc.isLivemark(livemarkId));
  do_check_true(lmsvc.getSiteURI(livemarkId).spec == "http://example.com/");
  do_check_true(lmsvc.getFeedURI(livemarkId).spec == "http://example.com/rss.xml");
  do_check_true(bmsvc.getFolderReadonly(livemarkId));

  lmsvc.setSiteURI(livemarkId, uri("http://foo.example.com/"));
  do_check_true(lmsvc.getSiteURI(livemarkId).spec == "http://foo.example.com/");
  
  lmsvc.setFeedURI(livemarkId, uri("http://foo.example.com/rss.xml"));
  do_check_true(lmsvc.getFeedURI(livemarkId).spec == "http://foo.example.com/rss.xml");

  
  var livemarkId2 = null;
  try {
    var livemarkId2 = lmsvc.createLivemark(livemarkId, "foo", uri("http://example.com/"), 
                                           uri("http://example.com/rss.xml"), -1);
  } catch (ex) {
    livemarkId2 = null;
  }
  do_check_true(livemarkId2 == null);
  
  
  do_check_true(lmsvc.isLivemark(livemarkId));
  
  
  
  var randomFolder = bmsvc.createFolder(root, "Random", -1);
  do_check_true(!lmsvc.isLivemark(randomFolder));
}
