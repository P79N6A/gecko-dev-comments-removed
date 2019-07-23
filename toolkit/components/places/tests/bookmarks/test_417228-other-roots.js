





































Components.utils.import("resource://gre/modules/utils.js");
var tests = [];














tests.push({
  excludeItemsFromRestore: [],
  populate: function populate() {
    
    var rootNode = PlacesUtils.getFolderContents(PlacesUtils.placesRootId,
                                                 false, false).root;
    do_check_eq(rootNode.childCount, 4);

    
    this._folderTitle = "test folder";
    this._folderId = 
      PlacesUtils.bookmarks.createFolder(PlacesUtils.placesRootId,
                                         this._folderTitle,
                                         PlacesUtils.bookmarks.DEFAULT_INDEX);
    do_check_eq(rootNode.childCount, 5);

    
    this._testURI = PlacesUtils._uri("http://test");
    this._tags = ["a", "b"];
    PlacesUtils.tagging.tagURI(this._testURI, this._tags);

    
    this._roots = [PlacesUtils.bookmarksMenuFolderId, PlacesUtils.toolbarFolderId,
                   PlacesUtils.unfiledBookmarksFolderId, this._folderId];
    this._roots.forEach(function(aRootId) {
      
      PlacesUtils.bookmarks.removeFolderChildren(aRootId);
      
      PlacesUtils.bookmarks.insertBookmark(aRootId, this._testURI,
                                           PlacesUtils.bookmarks.DEFAULT_INDEX, "test");
    }, this);

    
    
    var excludedFolderId =
      PlacesUtils.bookmarks.createFolder(PlacesUtils.placesRootId,
                                         "excluded",
                                         PlacesUtils.bookmarks.DEFAULT_INDEX);
    do_check_eq(rootNode.childCount, 6);
    this.excludeItemsFromRestore.push(excludedFolderId); 

    
    PlacesUtils.bookmarks.insertBookmark(excludedFolderId, this._testURI,
                                         PlacesUtils.bookmarks.DEFAULT_INDEX, "test");
  },

  inbetween: function inbetween() {
    

    
    this._litterTitle = "otter";
    PlacesUtils.bookmarks.createFolder(PlacesUtils.placesRootId,
                                       this._litterTitle, 0);

    
    PlacesUtils.tagging.tagURI(this._testURI, ["c", "d"]);
  },

  validate: function validate() {
    
    var tags = PlacesUtils.tagging.getTagsForURI(this._testURI);
    
    do_check_eq(this._tags.toString(), tags.toString());

    var rootNode = PlacesUtils.getFolderContents(PlacesUtils.placesRootId,
                                                 false, false).root;

    
    do_check_neq(rootNode.getChild(0).title, this._litterTitle);

    
    do_check_eq(rootNode.childCount, 6);

    var foundTestFolder = 0;
    for (var i = 0; i < rootNode.childCount; i++) {
      var node = rootNode.getChild(i);

      LOG("validating " + node.title);
      if (node.itemId != PlacesUtils.tagsFolderId) {
        if (node.title == this._folderTitle) {
          
          do_check_eq(node.type, node.RESULT_TYPE_FOLDER);
          do_check_eq(node.title, this._folderTitle);
          foundTestFolder++;
        }

        
        node.QueryInterface(Ci.nsINavHistoryContainerResultNode).containerOpen = true;
        do_check_eq(node.childCount, 1);
        var child = node.getChild(0);
        do_check_true(PlacesUtils._uri(child.uri).equals(this._testURI));

        
        node.containerOpen = false;
      }
    }
    do_check_eq(foundTestFolder, 1);
    rootNode.containerOpen = false;
  }
});

function run_test() {
  do_check_eq(typeof PlacesUtils, "object");

  
  var jsonFile = dirSvc.get("ProfD", Ci.nsILocalFile);
  jsonFile.append("bookmarks.json");
  if (jsonFile.exists())
    jsonFile.remove(false);
  jsonFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, 0600);
  if (!jsonFile.exists())
    do_throw("couldn't create file: bookmarks.exported.json");

  
  var excludedItemsFromRestore = [];

  
  tests.forEach(function(aTest) {
    aTest.populate();
    
    aTest.validate();

    if (aTest.excludedItemsFromRestore)
      excludedItemsFromRestore = excludedItems.concat(aTest.excludedItemsFromRestore);
  });

  try {
    PlacesUtils.backups.saveBookmarksToJSONFile(jsonFile);
  } catch(ex) { do_throw("couldn't export to file: " + ex); }

  tests.forEach(function(aTest) {
    aTest.inbetween();
  });

  
  try {
    PlacesUtils.restoreBookmarksFromJSONFile(jsonFile, excludedItemsFromRestore);
  } catch(ex) { do_throw("couldn't import the exported file: " + ex); }

  
  tests.forEach(function(aTest) {
    aTest.validate();
  });

  
  jsonFile.remove(false);
}
