





































Components.utils.import("resource://gre/modules/utils.js");
var tests = [];














var test = {
  populate: function populate() {
    
    var rootNode = PlacesUtils.getFolderContents(PlacesUtils.placesRootId,
                                                 false, false).root;

    var idx = PlacesUtils.bookmarks.DEFAULT_INDEX;

    var testRoot =
      PlacesUtils.bookmarks.createFolder(PlacesUtils.placesRootId,
                                         "", idx);

    
    this._folderTitle1 = "test folder";
    this._folderId1 = 
      PlacesUtils.bookmarks.createFolder(testRoot, this._folderTitle1, idx);

    
    this._testURI = uri("http://test");
    PlacesUtils.bookmarks.insertBookmark(this._folderId1, this._testURI,
                                         idx, "test");

    
    this._folderTitle2 = "test folder";
    this._folderId2 = 
      PlacesUtils.bookmarks.createFolder(testRoot, this._folderTitle2, idx);

    
    PlacesUtils.bookmarks.insertBookmark(this._folderId2, this._testURI,
                                         idx, "test");

    rootNode.containerOpen = false;
  },

  validate: function validate() {
    var rootNode = PlacesUtils.getFolderContents(PlacesUtils.placesRootId,
                                                 false, false).root;

    var testRootNode = rootNode.getChild(rootNode.childCount-1);

    testRootNode.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    testRootNode.containerOpen = true;
    do_check_eq(testRootNode.childCount, 1);

    var childNode = testRootNode.getChild(0);
    do_check_eq(childNode.title, this._folderTitle1);

    rootNode.containerOpen = false;
  }
}

function run_test() {
  do_check_eq(typeof PlacesUtils, "object");

  
  var jsonFile = dirSvc.get("ProfD", Ci.nsILocalFile);
  jsonFile.append("bookmarks.json");
  if (jsonFile.exists())
    jsonFile.remove(false);
  jsonFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, 0600);
  if (!jsonFile.exists())
    do_throw("couldn't create file: bookmarks.exported.json");

  
  test.populate();

  try {
    PlacesUtils.backupBookmarksToFile(jsonFile, [test._folderId2]);
  } catch(ex) {
    do_throw("couldn't export to file: " + ex);
  }

  
  try {
    PlacesUtils.restoreBookmarksFromJSONFile(jsonFile);
  } catch(ex) {
    do_throw("couldn't import the exported file: " + ex);
  }

  
  test.validate();

  
  jsonFile.remove(false);
}
