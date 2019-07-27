





const NHQO = Ci.nsINavHistoryQueryOptions;




function promiseOnItemVisited() {
  let defer = Promise.defer();
  let bookmarksObserver = {
    __proto__: NavBookmarkObserver.prototype,
    onItemVisited: function BO_onItemVisited() {
      PlacesUtils.bookmarks.removeObserver(this);
      
      do_execute_soon(defer.resolve);
    }
  };
  PlacesUtils.bookmarks.addObserver(bookmarksObserver, false);
  return defer.promise;
}

function run_test() {
  run_next_test();
}

add_task(function test() {
  let testFolder = PlacesUtils.bookmarks.createFolder(
    PlacesUtils.bookmarks.placesRoot,
    "Result-sort functionality tests root",
    PlacesUtils.bookmarks.DEFAULT_INDEX);

  let uri1 = NetUtil.newURI("http://foo.tld/a");
  let uri2 = NetUtil.newURI("http://foo.tld/b");

  let id1 = PlacesUtils.bookmarks.insertBookmark(
    testFolder, uri1, PlacesUtils.bookmarks.DEFAULT_INDEX, "b");
  let id2 = PlacesUtils.bookmarks.insertBookmark(
    testFolder, uri2, PlacesUtils.bookmarks.DEFAULT_INDEX, "a");
  
  let id3 = PlacesUtils.bookmarks.insertBookmark(
    testFolder, uri1, PlacesUtils.bookmarks.DEFAULT_INDEX, "a");

  
  let result = PlacesUtils.getFolderContents(testFolder);
  let root = result.root;

  do_check_eq(root.childCount, 3);

  function checkOrder(a, b, c) {
    do_check_eq(root.getChild(0).itemId, a);
    do_check_eq(root.getChild(1).itemId, b);
    do_check_eq(root.getChild(2).itemId, c);
  }

  
  do_print("Natural order");
  checkOrder(id1, id2, id3);

  
  do_print("Sort by title asc");
  result.sortingMode = NHQO.SORT_BY_TITLE_ASCENDING;
  checkOrder(id3, id2, id1);

  
  do_print("Sort by title desc");
  result.sortingMode = NHQO.SORT_BY_TITLE_DESCENDING;
  checkOrder(id1, id2, id3);

  
  do_print("Sort by uri asc");
  result.sortingMode = NHQO.SORT_BY_URI_ASCENDING;
  checkOrder(id1, id3, id2);

  
  do_print("Change bookmark uri liveupdate");
  PlacesUtils.bookmarks.changeBookmarkURI(id1, uri2);
  checkOrder(id3, id1, id2);
  PlacesUtils.bookmarks.changeBookmarkURI(id1, uri1);
  checkOrder(id1, id3, id2);

  
  do_print("Sort by keyword asc");
  result.sortingMode = NHQO.SORT_BY_KEYWORD_ASCENDING;
  checkOrder(id3, id2, id1);  
  yield PlacesUtils.keywords.insert({ url: uri1.spec, keyword: "a" });
  yield PlacesUtils.keywords.insert({ url: uri2.spec, keyword: "z" });
  checkOrder(id3, id1, id2);

  
  
  

  do_print("Sort by annotation desc");
  PlacesUtils.annotations.setItemAnnotation(id1, "testAnno", "a", 0, 0);
  PlacesUtils.annotations.setItemAnnotation(id3, "testAnno", "b", 0, 0);
  result.sortingAnnotation = "testAnno";
  result.sortingMode = NHQO.SORT_BY_ANNOTATION_DESCENDING;

  
  checkOrder(id3, id1, id2);

  
  

  
  do_print("Annotation liveupdate");
  PlacesUtils.annotations.setItemAnnotation(id1, "testAnno", "c", 0, 0);
  checkOrder(id1, id3, id2);

  

  
  
  
  let waitForVisited = promiseOnItemVisited();
  yield PlacesTestUtils.addVisits({ uri: uri2, transition: TRANSITION_TYPED });
  yield waitForVisited;

  do_print("Sort by frecency desc");
  result.sortingMode = NHQO.SORT_BY_FRECENCY_DESCENDING;
  for (let i = 0; i < root.childCount; ++i) {
    print(root.getChild(i).uri + " " + root.getChild(i).title);
  }
  
  
  checkOrder(id2, id3, id1);
  do_print("Sort by frecency asc");
  result.sortingMode = NHQO.SORT_BY_FRECENCY_ASCENDING;
  for (let i = 0; i < root.childCount; ++i) {
    print(root.getChild(i).uri + " " + root.getChild(i).title);
  }
  checkOrder(id1, id3, id2);

  root.containerOpen = false;
});
