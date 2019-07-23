











































 
 bmsvc.createFolder(bmsvc.placesRoot, "Folder 1", bmsvc.DEFAULT_INDEX);
 var folder1Id = bmsvc.getChildFolder(bmsvc.placesRoot, "Folder 1");

 
 bmsvc.createFolder(folder1Id, "Folder 1a", bmsvc.DEFAULT_INDEX);
 var folder1aId = bmsvc.getChildFolder(folder1Id, "Folder 1a");
 












var testData = [

  
  
  
  
  {isInQuery: true, isVisit: true, uri: "http://foo.com/"},

  
  
  
  
  {isInQuery: true, isVisit: true, isDetails: true, title: "taggariffic",
   uri: "http://foo.com/tagging/test.html", lastVisit: beginTime, isTag: true,
   tagArray: ["moz"] }];

  





function run_test() {

  
  populateDB(testData);

  
  var query = histsvc.getNewQuery();
  
  
  
  var options = histsvc.getNewQueryOptions();
  

  
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  
  
  
  
  compareArrayToResult(testData, root);

  
  
  
  var addItem = [{isInQuery: true, isVisit: true, isDetails: true, title: "moz",
                 uri: "http://foo.com/i-am-added.html", lastVisit: jan11_800}];
  populateDB(addItem);
  
  
  var change1 = [{isDetails: true, uri: "http://foo.com/",
                  lastVisit: jan12_1730, title: "moz moz mozzie"}];
  populateDB(change1);

  
  var updateBatch = {
    runBatched: function (aUserData) {
      var batchChange = [{isDetails: true, uri: "http://foo.com/changeme2",
                          title: "moz", lastVisit: jan7_800},
                         {isPageAnnotation: true, uri: "http://foo.com/begin.html",
                          annoName: badAnnoName, annoVal: val}];
      populateDB(batchChange);
    }
  };

  histsvc.runInBatchMode(updateBatch, null);

  
  root.containerOpen = false;
}
