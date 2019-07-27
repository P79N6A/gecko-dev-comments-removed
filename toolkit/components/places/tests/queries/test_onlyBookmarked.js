

















var testData = [
  
  { isBookmark: true,
    uri: "http://bookmarked.com/",
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: PlacesUtils.bookmarks.DEFAULT_INDEX,
    isInQuery: true },

  
  { isBookmark: true,
    uri: "http://bookmarked-elsewhere.com/",
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    index: PlacesUtils.bookmarks.DEFAULT_INDEX,
    isInQuery: false },

  
  { isVisit: true,
    uri: "http://notbookmarked.com/",
    isInQuery: false }
];







function run_test()
{
  run_next_test();
}

add_task(function test_onlyBookmarked()
{
  
  yield task_populateDB(testData);

  
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
      parentGuid: PlacesUtils.bookmarks.toolbarGuid,
      index: PlacesUtils.bookmarks.DEFAULT_INDEX,
      isInQuery: true },

    
    { isBookmark: true,
      uri: "http://bookmarked-elsewhere2.com/",
      parentGuid: PlacesUtils.bookmarks.menuGuid,
      index: PlacesUtils.bookmarks.DEFAULT_INDEX,
      isInQuery: false }
  ];
  
  yield task_populateDB(liveUpdateTestData); 

  
  testData.push(liveUpdateTestData[0]);
  testData.push(liveUpdateTestData[1]);

  
  LOG("begin live-update test");
  compareArrayToResult(testData, root);
  LOG("end live-update test");






















  
  root.containerOpen = false;
});
