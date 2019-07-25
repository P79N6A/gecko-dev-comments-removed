








































function run_test()
{
  let livemarkId = PlacesUtils.livemarks.createLivemarkFolderOnly(
    PlacesUtils.bookmarksMenuFolderId, "foo", uri("http://example.com/"),
    uri("http://example.com/rss.xml"), PlacesUtils.bookmarks.DEFAULT_INDEX
  );

  do_check_true(PlacesUtils.livemarks.isLivemark(livemarkId));
  do_check_eq(PlacesUtils.livemarks.getSiteURI(livemarkId).spec, "http://example.com/");
  do_check_eq(PlacesUtils.livemarks.getFeedURI(livemarkId).spec, "http://example.com/rss.xml");
  do_check_true(PlacesUtils.bookmarks.getFolderReadonly(livemarkId));

  
  let livemarkId2 = null;
  try {
    let livemarkId2 = PlacesUtils.livemarks.createLivemark(
      livemarkId, "foo", uri("http://example.com/"),
      uri("http://example.com/rss.xml"), PlacesUtils.bookmarks.DEFAULT_INDEX
    );
  } catch (ex) {
    livemarkId2 = null;
  }
  do_check_true(livemarkId2 == null);
  
  
  do_check_true(PlacesUtils.livemarks.isLivemark(livemarkId));

  do_check_eq(
    PlacesUtils.livemarks.getLivemarkIdForFeedURI(uri("http://example.com/rss.xml")),
    livemarkId
  );

  
  
  let randomFolder = PlacesUtils.bookmarks.createFolder(
    PlacesUtils.bookmarksMenuFolderId, "Random",
    PlacesUtils.bookmarks.DEFAULT_INDEX
  );
  do_check_true(!PlacesUtils.livemarks.isLivemark(randomFolder));
}
