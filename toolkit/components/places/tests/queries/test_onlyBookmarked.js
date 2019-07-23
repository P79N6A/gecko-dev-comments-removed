

















































var testData = [
  
  { isBookmark: true,
    uri: "http://bookmarked.com/",
    parentFolder: PlacesUtils.toolbarFolderId,
    index: PlacesUtils.bookmarks.DEFAULT_INDEX,
    title: "",
    isInQuery: true },

  
  { isBookmark: true,
    uri: "http://bookmarked-elsewhere.com/",
    parentFolder: PlacesUtils.bookmarksMenuFolderId,
    index: PlacesUtils.bookmarks.DEFAULT_INDEX,
    title: "",
    isInQuery: false },

  
  { isVisit: true,
    uri: "http://notbookmarked.com/",
    isInQuery: false }
];







function run_test() {

  
  populateDB(testData);

  
  var query = PlacesUtils.history.getNewQuery();
  query.setFolders([PlacesUtils.toolbarFolderId], 1);
  query.onlyBookmarked = true;
  
  
  var options = PlacesUtils.history.getNewQueryOptions();
  options.queryType = options.QUERY_TYPE_HISTORY;

  
  var result = PlacesUtils.history.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  
  
  
  
  LOG("begin first test");
  compareArrayToResult(testData, root);
  LOG("end first test");

  


 
  var liveUpdateTestData = [
    
    { isBookmark: true,
      uri: "http://bookmarked2.com/",
      parentFolder: PlacesUtils.toolbarFolderId,
      index: PlacesUtils.bookmarks.DEFAULT_INDEX,
      title: "",
      isInQuery: true },

    
    { isBookmark: true,
      uri: "http://bookmarked-elsewhere2.com/",
      parentFolder: PlacesUtils.bookmarksMenuFolderId,
      index: PlacesUtils.bookmarks.DEFAULT_INDEX,
      title: "",
      isInQuery: false }
  ];
  
  populateDB(liveUpdateTestData); 

  
  testData.push(liveUpdateTestData[0]);
  testData.push(liveUpdateTestData[1]);

  
  LOG("begin live-update test");
  compareArrayToResult(testData, root);
  LOG("end live-update test");






















  
  root.containerOpen = false;
}
