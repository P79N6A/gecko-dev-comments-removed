function run_test() {
  var testURI = uri("http://foo.com");

  




  var bm1 = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.bookmarksMenuFolderId, testURI, -1, "blah");
  PlacesUtils.bookmarks.setKeywordForBookmark(bm1, "foo");
  PlacesUtils.setPostDataForBookmark(bm1, "pdata1");
  var bm2 = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.bookmarksMenuFolderId, testURI, -1, "blah");
  PlacesUtils.bookmarks.setKeywordForBookmark(bm2, "bar");
  PlacesUtils.setPostDataForBookmark(bm2, "pdata2");

  
  var url, postdata;
  [url, postdata] = PlacesUtils.getURLAndPostDataForKeyword("foo");
  do_check_eq(testURI.spec, url);
  do_check_eq(postdata, "pdata1");

  
  [url, postdata] = PlacesUtils.getURLAndPostDataForKeyword("bar");
  do_check_eq(testURI.spec, url);
  do_check_eq(postdata, "pdata2");

  
  PlacesUtils.bookmarks.removeItem(bm1);
  PlacesUtils.bookmarks.removeItem(bm2);
}
