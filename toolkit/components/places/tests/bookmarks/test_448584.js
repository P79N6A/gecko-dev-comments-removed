





































Components.utils.import("resource://gre/modules/utils.js");
var tests = [];


try {
  var mDBConn = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                                   .DBConnection;
}
catch(ex) {
  do_throw("Could not get database connection\n");
}






var invalidURITest = {
  _itemTitle: "invalid uri",
  _itemUrl: "http://test.mozilla.org/",
  _itemId: null,

  populate: function () {
    
    PlacesUtils.bookmarks.insertBookmark(PlacesUtils.toolbarFolderId,
                                         PlacesUtils._uri(this._itemUrl),
                                         PlacesUtils.bookmarks.DEFAULT_INDEX,
                                         this._itemTitle);
    
    this._itemId = 
      PlacesUtils.bookmarks.insertBookmark(PlacesUtils.toolbarFolderId,
                                           PlacesUtils._uri(this._itemUrl),
                                           PlacesUtils.bookmarks.DEFAULT_INDEX,
                                           this._itemTitle);
  },

  clean: function () {
    PlacesUtils.bookmarks.removeItem(this._itemId);
  },

  validate: function (aExpectValidItemsCount) {
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([PlacesUtils.bookmarks.toolbarFolder], 1);
    var options = PlacesUtils.history.getNewQueryOptions();
    var result = PlacesUtils.history.executeQuery(query, options);

    var toolbar = result.root;
    toolbar.containerOpen = true;

    
    do_check_eq(toolbar.childCount, aExpectValidItemsCount);
    for (var i = 0; i < toolbar.childCount; i++) {
      var folderNode = toolbar.getChild(0);
      do_check_eq(folderNode.type, folderNode.RESULT_TYPE_URI);
      do_check_eq(folderNode.title, this._itemTitle);
    }

    
    toolbar.containerOpen = false;
  }
}
tests.push(invalidURITest);

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
    
    aTest.validate(2);
    
    
    var sql = "UPDATE moz_bookmarks SET fk = 1337 WHERE id = ?1";
    var stmt = mDBConn.createStatement(sql);
    stmt.bindUTF8StringParameter(0, aTest._itemId);
    try {
      stmt.execute();
    } finally {
      stmt.finalize();
    }
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
    aTest.validate(1);
  });

  
  jsonFile.remove(false);
}
