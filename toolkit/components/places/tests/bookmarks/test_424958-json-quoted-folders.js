





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
  run_next_test();
}

add_task(function () {
  
  let jsonFile = OS.Path.join(OS.Constants.Path.profileDir, "bookmarks.json");

  
  tests.forEach(function(aTest) {
    aTest.populate();
    
    aTest.validate();
  });

  
  yield BookmarkJSONUtils.exportToFile(jsonFile);

  
  tests.forEach(function(aTest) {
    aTest.clean();
  });

  
  yield BookmarkJSONUtils.importFromFile(jsonFile, true);

  
  tests.forEach(function(aTest) {
    aTest.validate();
  });

  
  yield OS.File.remove(jsonFile);

});
