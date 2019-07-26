





const bmsvc = PlacesUtils.bookmarks;
const histsvc = PlacesUtils.history;

function run_test() {
  run_next_test();
}

add_task(function test_addBookmarksAndCheckGuids() {
  let folder = bmsvc.createFolder(bmsvc.placesRoot, "test folder", bmsvc.DEFAULT_INDEX);
  let b1 = bmsvc.insertBookmark(folder, uri("http://test1.com/"),
                                bmsvc.DEFAULT_INDEX, "1 title");
  let b2 = bmsvc.insertBookmark(folder, uri("http://test2.com/"),
                                bmsvc.DEFAULT_INDEX, "2 title");
  let b3 = bmsvc.insertBookmark(folder, uri("http://test3.com/"),
                                bmsvc.DEFAULT_INDEX, "3 title");
  let s1 = bmsvc.insertSeparator(folder, bmsvc.DEFAULT_INDEX);
  let f1 = bmsvc.createFolder(folder, "test folder 2", bmsvc.DEFAULT_INDEX);

  let options = histsvc.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  let query = histsvc.getNewQuery();
  query.setFolders([folder], 1);
  let result = histsvc.executeQuery(query, options);
  let root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 5);

  
  let bookmarkGuidZero = root.getChild(0).bookmarkGuid;
  do_check_eq(bookmarkGuidZero.length, 12);
  
  do_check_eq(root.getChild(1).bookmarkGuid.length, 12);
  do_check_eq(root.getChild(2).bookmarkGuid.length, 12);
  
  do_check_eq(root.getChild(3).bookmarkGuid.length, 12);
  
  do_check_eq(root.getChild(4).bookmarkGuid.length, 12);
  
  do_check_neq(bookmarkGuidZero, root.getChild(1).bookmarkGuid);
  do_check_neq(root.getChild(1).bookmarkGuid, root.getChild(2).bookmarkGuid);
  do_check_neq(root.getChild(2).bookmarkGuid, root.getChild(3).bookmarkGuid);
  do_check_neq(root.getChild(3).bookmarkGuid, root.getChild(4).bookmarkGuid);

  
  let pageGuidZero = root.getChild(0).pageGuid;
  do_check_eq(pageGuidZero.length, 12);
  
  do_check_eq(root.getChild(1).pageGuid.length, 12);
  do_check_eq(root.getChild(2).pageGuid.length, 12);
  
  do_check_eq(root.getChild(3).pageGuid, "");
  do_check_eq(root.getChild(4).pageGuid, "");

  do_check_neq(pageGuidZero, root.getChild(1).pageGuid);
  do_check_neq(root.getChild(1).pageGuid, root.getChild(2).pageGuid);

  root.containerOpen = false;

  remove_all_bookmarks();
});

add_task(function test_updateBookmarksAndCheckGuids() {
  let folder = bmsvc.createFolder(bmsvc.placesRoot, "test folder", bmsvc.DEFAULT_INDEX);
  let b1 = bmsvc.insertBookmark(folder, uri("http://test1.com/"),
                                bmsvc.DEFAULT_INDEX, "1 title");
  let f1 = bmsvc.createFolder(folder, "test folder 2", bmsvc.DEFAULT_INDEX);

  let options = histsvc.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  let query = histsvc.getNewQuery();
  query.setFolders([folder], 1);
  let result = histsvc.executeQuery(query, options);
  let root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 2);

  
  let bookmarkGuidZero = root.getChild(0).bookmarkGuid;
  let pageGuidZero = root.getChild(0).pageGuid;
  bmsvc.setItemTitle(b1, "1 title mod");
  do_check_eq(root.getChild(0).title, "1 title mod");
  do_check_eq(root.getChild(0).bookmarkGuid, bookmarkGuidZero);
  do_check_eq(root.getChild(0).pageGuid, pageGuidZero);

  let bookmarkGuidOne = root.getChild(1).bookmarkGuid;
  let pageGuidOne = root.getChild(1).pageGuid;
  bmsvc.setItemTitle(f1, "test foolder 234");
  do_check_eq(root.getChild(1).title, "test foolder 234");
  do_check_eq(root.getChild(1).bookmarkGuid, bookmarkGuidOne);
  do_check_eq(root.getChild(1).pageGuid, pageGuidOne);

  root.containerOpen = false;

  remove_all_bookmarks();
});

add_task(function test_addVisitAndCheckGuid() {
  
  let now = Date.now() * 1000;
  let sourceURI = uri("http://test4.com/");
  yield promiseAddVisits({ uri: sourceURI });
  do_check_eq(bmsvc.getBookmarkedURIFor(sourceURI), null);

  let options = histsvc.getNewQueryOptions();
  let query = histsvc.getNewQuery();
  query.uri = sourceURI;
  result = histsvc.executeQuery(query, options);
  let root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 1);

  pageGuidZero = root.getChild(0).pageGuid;
  do_check_eq(pageGuidZero.length, 12);

  do_check_eq(root.getChild(0).bookmarkGuid, "");

  root.containerOpen = false;

  yield promiseClearHistory();
});

