





































Components.utils.import("resource://gre/modules/utils.js");
var tests = [];














var quotesTest = {
  _folderTitle: '"quoted folder"',
  _folderId: null,

  populate: function () {
    this._folderId = 
      PlacesUtils.bookmarks.createFolder(PlacesUtils.toolbarFolderId,
                                         this._folderTitle,
                                         PlacesUtils.bookmarks.DEFAULT_INDEX);
  },

  clean: function () {
    PlacesUtils.bookmarks.removeItem(this._folderId);
  },

  validate: function () {
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([PlacesUtils.bookmarks.toolbarFolder], 1);
    var result = PlacesUtils.history.executeQuery(query, PlacesUtils.history.getNewQueryOptions());

    var toolbar = result.root;
    toolbar.containerOpen = true;

    
    do_check_true(toolbar.childCount, 1);
    var folderNode = toolbar.getChild(0);
    do_check_eq(folderNode.type, folderNode.RESULT_TYPE_FOLDER);
    do_check_eq(folderNode.title, this._folderTitle);

    
    toolbar.containerOpen = false;
  }
}
tests.push(quotesTest);

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
