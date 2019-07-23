





































version(170);












function run_test() {
  return;

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

  [url, postdata] = PlacesUtils.getURLAndPostDataForKeyword("foo");
  do_check_eq(testURI.spec, url);
  do_check_eq(postdata, "pdata1");

  
  bmsvc.removeItem(bm1);
  bmsvc.removeItem(bm2);
}
