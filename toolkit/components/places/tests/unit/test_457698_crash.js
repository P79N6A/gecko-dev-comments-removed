





































 try {
  var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  var bms = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
            getService(Ci.nsINavBookmarksService);
  var ans = Cc["@mozilla.org/browser/annotation-service;1"].
            getService(Ci.nsIAnnotationService);
} catch (ex) {
  do_throw("Could not get services\n");
}


var observer = {
  onBeginUpdateBatch: function() {},
  onEndUpdateBatch: function() {},
  onItemAdded: function(id, folder, index) {
    do_check_true(id > 0);
  },
  onBeforeItemRemoved: function() {},
  onItemRemoved: function() {},
  onItemChanged: function() {},
  onItemVisited: function() {},
  onItemMoved: function() {},
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsINavBookmarkObserver) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};
bms.addObserver(observer, false);


function run_test() {
  
  do_load_module("nsDynamicContainerServiceSample.js");
  var testRoot = bms.createFolder(bms.placesRoot, "test root", bms.DEFAULT_INDEX);

  var options = hs.getNewQueryOptions();
  var query = hs.getNewQuery();
  query.setFolders([testRoot], 1);
  var result = hs.executeQuery(query, options);
  var rootNode = result.root;
  rootNode.containerOpen = true;

  
  var remoteContainer =
    bms.createDynamicContainer(testRoot, "remote container sample",
                                "@mozilla.org/browser/remote-container-sample;1",
                                bms.DEFAULT_INDEX);

  rootNode.containerOpen = false;
}
