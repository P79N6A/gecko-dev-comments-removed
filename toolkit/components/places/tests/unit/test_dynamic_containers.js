





































var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);
var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
            getService(Ci.nsINavBookmarksService);
var annosvc = Cc["@mozilla.org/browser/annotation-service;1"].
              getService(Ci.nsIAnnotationService);


function run_test() {
  
  do_load_module("/toolkit/components/places/tests/unit/nsDynamicContainerServiceSample.js");
  var testRoot = bmsvc.createFolder(bmsvc.placesRoot, "test root", bmsvc.DEFAULT_INDEX);
  var exposedFolder = bmsvc.createFolder(testRoot, "exposed folder", bmsvc.DEFAULT_INDEX);
  var efId1 = bmsvc.insertBookmark(exposedFolder, uri("http://uri1.tld"), bmsvc.DEFAULT_INDEX, "");

  
  var remoteContainer =
    bmsvc.createDynamicContainer(testRoot, "remote container sample",
                                "@mozilla.org/browser/remote-container-sample;1",
                                bmsvc.DEFAULT_INDEX);

  
  
  annosvc.setItemAnnotation(remoteContainer, "exposedFolder",
                            exposedFolder, 0, 0);

  
  var options = histsvc.getNewQueryOptions();
  var query = histsvc.getNewQuery();
  query.setFolders([remoteContainer], 1);
  var result = histsvc.executeQuery(query, options);
  var rootNode = result.root;

  
  
  
  rootNode.containerOpen = true;
  do_check_eq(rootNode.childCount, 2);

  do_check_eq(rootNode.getChild(0).uri, "http://foo.tld/");
  var folder = rootNode.getChild(1).QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(folder.itemId, exposedFolder);
  folder.containerOpen = true;
  do_check_eq(folder.childCount, 1);

  
  bmsvc.insertBookmark(exposedFolder, uri("http://uri2.tld"), bmsvc.DEFAULT_INDEX, "");
  do_check_eq(folder.childCount, 2);
}
