






































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
}
catch(ex) {
  do_throw("Could not get the history service\n");
}


try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
}
catch(ex) {
  do_throw("Could not get the nav-bookmarks-service\n");
}


try {
  var annosvc= Cc["@mozilla.org/browser/annotation-service;1"].getService(Ci.nsIAnnotationService);
} catch(ex) {
  do_throw("Could not get annotation service\n");
}


function add_visit(aURI, aTime) {
  histsvc.addVisit(aURI,
                    aTime,
                    0, 
                    histsvc.TRANSITION_TYPED, 
                    false, 
                    0);
}


function run_test() {
  var testRoot = bmsvc.createFolder(root,
                                    "Result-sort functionality tests root",
                                    bmsvc.DEFAULT_INDEX);
  var uri1 = uri("http://foo.tld/a");
  var uri2 = uri("http://foo.tld/b");
  var id1 = bmsvc.insertItem(testRoot, uri1, bmsvc.DEFAULT_INDEX);
  bmsvc.setItemTitle(id1, "b");
  var id2 = bmsvc.insertItem(testRoot, uri2, bmsvc.DEFAULT_INDEX);
  bmsvc.setItemTitle(id2, "a");

  
  var id3 = bmsvc.insertItem(testRoot, uri1, bmsvc.DEFAULT_INDEX);
  bmsvc.setItemTitle(id3, "a");

  
  var options = histsvc.getNewQueryOptions();
  var query = histsvc.getNewQuery();
  query.setFolders([testRoot], 1);
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;

  do_check_eq(root.childCount, 3);

  const NHQO = Ci.nsINavHistoryQueryOptions;

  function checkOrder(a, b, c) {
    do_check_eq(root.getChild(0).itemId, a);
    do_check_eq(root.getChild(1).itemId, b);
    do_check_eq(root.getChild(2).itemId, c);
  }

  
  checkOrder(id1, id2, id3);

  
  result.sortingMode = NHQO.SORT_BY_TITLE_ASCENDING;
  checkOrder(id3, id2, id1);

  
  result.sortingMode = NHQO.SORT_BY_TITLE_DESCENDING;
  checkOrder(id1, id2, id3);

  
  result.sortingMode = NHQO.SORT_BY_URI_ASCENDING;
  checkOrder(id1, id3, id2);

  
  bmsvc.changeBookmarkURI(id1, uri2);
  checkOrder(id3, id1, id2);
  bmsvc.changeBookmarkURI(id1, uri1);
  checkOrder(id1, id3, id2);

  
  
  

  annosvc.setItemAnnotationString(id1, "testAnno", "a", 0, 0);
  annosvc.setItemAnnotationString(id3, "testAnno", "b", 0, 0);
  result.sortingAnnotation = "testAnno";
  result.sortingMode = NHQO.SORT_BY_ANNOTATION_DESCENDING;

  
  checkOrder(id3, id1, id2);

  
  annosvc.setItemAnnotationString(id1, "testAnno", "c", 0, 0);
  checkOrder(id1, id3, id2);
}
