









































 
function run_test()
{
  var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);

  var itemId = bs.insertBookmark(bs.unfiledBookmarksFolder,
                                 uri("http://www.mozilla.org/"),
                                 bs.DEFAULT_INDEX,
                                 "we love mozilla");
  
  var options = hs.getNewQueryOptions();
  var query = hs.getNewQuery();
  query.setFolders([bs.unfiledBookmarksFolder], 1);
  var result = hs.executeQuery(query, options);
  var rootNode = result.root;

  rootNode.containerOpen = true;
  do_check_eq(rootNode.childCount, 1);
  rootNode.propertyBag.setProperty("testProperty", "testValue");
  do_check_eq(rootNode.propertyBag.getProperty("testProperty"), "testValue");
  rootNode.containerOpen = false;
  do_check_eq(rootNode.propertyBag.getProperty("testProperty"), "testValue");

  rootNode.containerOpen = true;
  do_check_eq(rootNode.propertyBag.getProperty("testProperty"), "testValue");
  rootNode.propertyBag.deleteProperty("testProperty");
  try {
    rootNode.propertyBag.getProperty("testProperty");
    do_throw("Found property in the propertyBag after removing it");
  }
  catch(ex) {}
  rootNode.containerOpen = false;
  try {
    rootNode.propertyBag.getProperty("testProperty");
    do_throw("Found property in the propertyBag after removing it");
  }
  catch(ex) {}

  bs.removeItem(itemId);
}
