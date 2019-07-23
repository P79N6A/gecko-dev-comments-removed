





































Components.utils.import("resource://gre/modules/utils.js");

const bmsvc = PlacesUtils.bookmarks;
const testFolderId = PlacesUtils.bookmarksMenuFolderId;


function run_test() {
  var testURI = uri("http://foo.com");

  




  var bm1 = bmsvc.insertBookmark(testFolderId, testURI, -1, "blah");
  bmsvc.setKeywordForBookmark(bm1, "foo");
  PlacesUtils.setPostDataForBookmark(bm1, "pdata1");
  var bm2 = bmsvc.insertBookmark(testFolderId, testURI, -1, "blah");
  bmsvc.setKeywordForBookmark(bm2, "bar");
  PlacesUtils.setPostDataForBookmark(bm2, "pdata2");

  
  var url, postdata;
  [url, postdata] = PlacesUtils.getURLAndPostDataForKeyword("foo");
  do_check_eq(testURI.spec, url);
  do_check_eq(postdata, "pdata1");

  
  [url, postdata] = PlacesUtils.getURLAndPostDataForKeyword("bar");
  do_check_eq(testURI.spec, url);
  do_check_eq(postdata, "pdata2");

  
  bmsvc.removeItem(bm1);
  bmsvc.removeItem(bm2);

  



  var bm1 = bmsvc.insertBookmark(testFolderId, testURI, -1, "blah");
  bmsvc.setKeywordForBookmark(bm1, "foo");
  PlacesUtils.setPostDataForBookmark(bm1, "pdata1");
  var bm2 = bmsvc.insertBookmark(testFolderId, testURI, -1, "blah");
  bmsvc.setKeywordForBookmark(bm2, "foo");
  PlacesUtils.setPostDataForBookmark(bm2, "pdata2");

  var bm1da = bmsvc.getItemDateAdded(bm1);
  var bm1lm = bmsvc.getItemLastModified(bm1);
  LOG("bm1 dateAdded: " + bm1da + ", lastModified: " + bm1lm);
  var bm2da = bmsvc.getItemDateAdded(bm2);
  var bm2lm = bmsvc.getItemLastModified(bm2);
  LOG("bm2 dateAdded: " + bm2da + ", lastModified: " + bm2lm);
  do_check_true(bm1da <= bm2da);
  do_check_true(bm1lm <= bm2lm);

  [url, postdata] = PlacesUtils.getURLAndPostDataForKeyword("foo");
  do_check_eq(testURI.spec, url);
  do_check_eq(postdata, "pdata2");

  
  bmsvc.removeItem(bm1);
  bmsvc.removeItem(bm2);

  




  var bm1 = bmsvc.insertBookmark(testFolderId, testURI, -1, "blah");
  bmsvc.setKeywordForBookmark(bm1, "foo");
  PlacesUtils.setPostDataForBookmark(bm1, "pdata1");
  var bm2 = bmsvc.insertBookmark(testFolderId, testURI, -1, "blah");
  bmsvc.setKeywordForBookmark(bm2, "foo");
  PlacesUtils.setPostDataForBookmark(bm2, "pdata2");

  
  bmsvc.setItemTitle(bm1, "change");

  var bm1da = bmsvc.getItemDateAdded(bm1);
  var bm1lm = bmsvc.getItemLastModified(bm1);
  LOG("bm1 dateAdded: " + bm1da + ", lastModified: " + bm1lm);
  var bm2da = bmsvc.getItemDateAdded(bm2);
  var bm2lm = bmsvc.getItemLastModified(bm2);
  LOG("bm2 dateAdded: " + bm2da + ", lastModified: " + bm2lm);
  do_check_true(bm1da <= bm2da);
  
  
  
  do_check_true(bm1lm >= bm2lm);

  
  
  
  if (bm1lm == bm2lm) 
    bmsvc.setItemLastModified(bm1, bm2lm + 1);

  [url, postdata] = PlacesUtils.getURLAndPostDataForKeyword("foo");
  do_check_eq(testURI.spec, url);
  do_check_eq(postdata, "pdata1");

  
  bmsvc.removeItem(bm1);
  bmsvc.removeItem(bm2);

  




  var testDate = Date.now() * 1000;
  var bm1 = bmsvc.insertBookmark(testFolderId, testURI, -1, "blah");
  bmsvc.setKeywordForBookmark(bm1, "foo");
  PlacesUtils.setPostDataForBookmark(bm1, "pdata1");
  bmsvc.setItemDateAdded(bm1, testDate);
  bmsvc.setItemLastModified(bm1, testDate);

  var bm2 = bmsvc.insertBookmark(testFolderId, testURI, -1, "blah");
  bmsvc.setKeywordForBookmark(bm2, "foo");
  PlacesUtils.setPostDataForBookmark(bm2, "pdata2");
  bmsvc.setItemDateAdded(bm2, testDate);
  bmsvc.setItemLastModified(bm2, testDate);

  var bm1da = bmsvc.getItemDateAdded(bm1, testDate);
  var bm1lm = bmsvc.getItemLastModified(bm1);
  LOG("bm1 dateAdded: " + bm1da + ", lastModified: " + bm1lm);
  var bm2da = bmsvc.getItemDateAdded(bm2);
  var bm2lm = bmsvc.getItemLastModified(bm2);
  LOG("bm2 dateAdded: " + bm2da + ", lastModified: " + bm2lm);

  do_check_eq(bm1da, bm2da);
  do_check_eq(bm1lm, bm2lm);


  var ids = bmsvc.getBookmarkIdsForURI(testURI);
  do_check_eq(ids[0], bm2);
  do_check_eq(ids[1], bm1);

  [url, postdata] = PlacesUtils.getURLAndPostDataForKeyword("foo");
  do_check_eq(testURI.spec, url);
  do_check_eq(postdata, "pdata2");

  
  bmsvc.removeItem(bm1);
  bmsvc.removeItem(bm2);
}
