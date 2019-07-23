





































Components.utils.import("resource://gre/modules/utils.js");
var tests = [];


try {
  var mDBConn = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                                   .DBConnection;
}
catch(ex) {
  do_throw("Could not get database connection\n");
}







var invalidTagChildTest = {
  _itemTitle: "invalid uri",
  _itemUrl: "http://test.mozilla.org/",
  _itemId: -1,
  _tag: "testTag",
  _tagItemId: -1,

  populate: function () {
    
    this._itemId = PlacesUtils.bookmarks
                              .insertBookmark(PlacesUtils.toolbarFolderId,
                                              PlacesUtils._uri(this._itemUrl),
                                              PlacesUtils.bookmarks.DEFAULT_INDEX,
                                              this._itemTitle);

    
    PlacesUtils.tagging.tagURI(PlacesUtils._uri(this._itemUrl), [this._tag]);
    
    var options = PlacesUtils.history.getNewQueryOptions();
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([PlacesUtils.bookmarks.tagsFolder], 1);
    var result = PlacesUtils.history.executeQuery(query, options);
    var tagRoot = result.root;
    tagRoot.containerOpen = true;
    do_check_eq(tagRoot.childCount, 1);
    var tagNode = tagRoot.getChild(0)
                          .QueryInterface(Ci.nsINavHistoryContainerResultNode);
    this._tagItemId = tagNode.itemId;
    tagRoot.containerOpen = false;

    
    PlacesUtils.bookmarks.insertSeparator(this._tagItemId,
                                         PlacesUtils.bookmarks.DEFAULT_INDEX);
    PlacesUtils.bookmarks.createFolder(this._tagItemId,
                                       "test folder",
                                       PlacesUtils.bookmarks.DEFAULT_INDEX);

    
    PlacesUtils.bookmarks.insertSeparator(PlacesUtils.bookmarks.tagsFolder,
                                          PlacesUtils.bookmarks.DEFAULT_INDEX);
    PlacesUtils.bookmarks.createFolder(PlacesUtils.bookmarks.tagsFolder,
                                       "test tags root folder",
                                       PlacesUtils.bookmarks.DEFAULT_INDEX);
  },

  clean: function () {
    PlacesUtils.tagging.untagURI(PlacesUtils._uri(this._itemUrl), [this._tag]);
    PlacesUtils.bookmarks.removeItem(this._itemId);
  },

  validate: function () {
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([PlacesUtils.bookmarks.toolbarFolder], 1);
    var options = PlacesUtils.history.getNewQueryOptions();
    var result = PlacesUtils.history.executeQuery(query, options);

    var toolbar = result.root;
    toolbar.containerOpen = true;

    
    do_check_eq(toolbar.childCount, 1);
    for (var i = 0; i < toolbar.childCount; i++) {
      var folderNode = toolbar.getChild(0);
      do_check_eq(folderNode.type, folderNode.RESULT_TYPE_URI);
      do_check_eq(folderNode.title, this._itemTitle);
    }
    toolbar.containerOpen = false;

    
    var tags = PlacesUtils.tagging.getTagsForURI(PlacesUtils._uri(this._itemUrl), {});
    do_check_eq(tags.length, 1);
    do_check_eq(tags[0], this._tag);
  }
}
tests.push(invalidTagChildTest);

function run_test() {
  do_check_eq(typeof PlacesUtils, "object");

  
  var jsonFile = dirSvc.get("ProfD", Ci.nsILocalFile);
  jsonFile.append("bookmarks.json");
  if (jsonFile.exists())
    jsonFile.remove(false);
  jsonFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, 0600);
  if (!jsonFile.exists())
    do_throw("couldn't create file: bookmarks.exported.json");

  
  tests.forEach(function(aTest) {
    aTest.populate();
    
    aTest.validate();
  });

  
  try {
    PlacesUtils.backups.saveBookmarksToJSONFile(jsonFile);
  } catch(ex) { do_throw("couldn't export to file: " + ex); }

  
  tests.forEach(function(aTest) {
    aTest.clean();
  });

  
  try {
    PlacesUtils.restoreBookmarksFromJSONFile(jsonFile);
  } catch(ex) { do_throw("couldn't import the exported file: " + ex); }

  
  tests.forEach(function(aTest) {
    aTest.validate();
  });

  
  jsonFile.remove(false);
}
