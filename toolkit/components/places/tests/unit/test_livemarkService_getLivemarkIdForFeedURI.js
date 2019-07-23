






































const LMANNO_FEEDURI = "livemark/feedURI";

Components.utils.import("resource://gre/modules/utils.js");
var PU = PlacesUtils;
var bs = PU.bookmarks;
var as = PU.annotations;

















function createFakeLivemark(aParentId, aTitle, aSiteURI, aFeedURI, aIndex) {
  var folderId = bs.createFolder(aParentId, aTitle, aIndex);
  bs.setFolderReadonly(folderId, true);

  
  as.setItemAnnotation(folderId, LMANNO_FEEDURI, aFeedURI.spec,
                       0, as.EXPIRE_NEVER);
  return folderId;
}


function run_test() {
  var feedURI = uri("http://example.com/rss.xml");

  
  
  var fakeLivemarkId = createFakeLivemark(bs.bookmarksMenuFolder, "foo",
                                          uri("http://example.com/"),
                                          feedURI, bs.DEFAULT_INDEX);
  do_check_eq(PU.getMostRecentFolderForFeedURI(feedURI), fakeLivemarkId);

  bs.removeItem(fakeLivemarkId);
  do_check_eq(PU.getMostRecentFolderForFeedURI(feedURI), -1);

  
  var ls = PU.livemarks;
  var livemarkId = ls.createLivemarkFolderOnly(bs.bookmarksMenuFolder, "foo",
                                               uri("http://example.com/"),
                                               feedURI, bs.DEFAULT_INDEX);

  do_check_eq(ls.getLivemarkIdForFeedURI(feedURI), livemarkId);
  do_check_eq(PU.getMostRecentFolderForFeedURI(feedURI), livemarkId);

  bs.removeItem(livemarkId);
  do_check_eq(ls.getLivemarkIdForFeedURI(feedURI), -1);
  do_check_eq(PU.getMostRecentFolderForFeedURI(feedURI), -1);
}
