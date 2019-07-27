





const EXCLUDE_FROM_BACKUP_ANNO = "places/excludeFromBackup";

const PLACES_ROOTS_COUNT  = 4;
var tests = [];














var test = {
  populate: function populate() {
    
    var rootNode = PlacesUtils.getFolderContents(PlacesUtils.placesRootId,
                                                 false, false).root;
    do_check_eq(rootNode.childCount, PLACES_ROOTS_COUNT );
    rootNode.containerOpen = false;

    var idx = PlacesUtils.bookmarks.DEFAULT_INDEX;

    
    this._restoreRootTitle = "restore root";
    var restoreRootId = PlacesUtils.bookmarks
                                   .createFolder(PlacesUtils.placesRootId,
                                                 this._restoreRootTitle, idx);
    
    this._restoreRootURI = uri("http://restore.uri");
    PlacesUtils.bookmarks.insertBookmark(restoreRootId, this._restoreRootURI,
                                         idx, "restore uri");
    
    this._restoreRootExcludeURI = uri("http://exclude.uri");
    var exItemId = PlacesUtils.bookmarks
                              .insertBookmark(restoreRootId,
                                              this._restoreRootExcludeURI,
                                              idx, "exclude uri");
    
    PlacesUtils.annotations.setItemAnnotation(exItemId,
                                              EXCLUDE_FROM_BACKUP_ANNO, 1, 0,
                                              PlacesUtils.annotations.EXPIRE_NEVER);

    
    this._excludeRootTitle = "exclude root";
    this._excludeRootId = PlacesUtils.bookmarks
                                     .createFolder(PlacesUtils.placesRootId,
                                                   this._excludeRootTitle, idx);
    
    PlacesUtils.annotations.setItemAnnotation(this._excludeRootId,
                                              EXCLUDE_FROM_BACKUP_ANNO, 1, 0,
                                              PlacesUtils.annotations.EXPIRE_NEVER);
    
    PlacesUtils.bookmarks.insertBookmark(this._excludeRootId,
                                         this._restoreRootExcludeURI,
                                         idx, "exclude uri");
  },

  validate: function validate(aEmptyBookmarks) {
    var rootNode = PlacesUtils.getFolderContents(PlacesUtils.placesRootId,
                                                 false, false).root;

    if (!aEmptyBookmarks) {
      
      
      do_check_eq(rootNode.childCount, PLACES_ROOTS_COUNT + 2);
      
      var restoreRootIndex = PLACES_ROOTS_COUNT;
      var excludeRootIndex = PLACES_ROOTS_COUNT+1;
      var excludeRootNode = rootNode.getChild(excludeRootIndex);
      do_check_eq(this._excludeRootTitle, excludeRootNode.title);
      excludeRootNode.QueryInterface(Ci.nsINavHistoryQueryResultNode);
      excludeRootNode.containerOpen = true;
      do_check_eq(excludeRootNode.childCount, 1);
      var excludeRootChildNode = excludeRootNode.getChild(0);
      do_check_eq(excludeRootChildNode.uri, this._restoreRootExcludeURI.spec);
      excludeRootNode.containerOpen = false;
    }
    else {
      
      do_check_eq(rootNode.childCount, PLACES_ROOTS_COUNT + 1);
      var restoreRootIndex = PLACES_ROOTS_COUNT;
    }

    var restoreRootNode = rootNode.getChild(restoreRootIndex);
    do_check_eq(this._restoreRootTitle, restoreRootNode.title);
    restoreRootNode.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    restoreRootNode.containerOpen = true;
    do_check_eq(restoreRootNode.childCount, 1);
    var restoreRootChildNode = restoreRootNode.getChild(0);
    do_check_eq(restoreRootChildNode.uri, this._restoreRootURI.spec);
    restoreRootNode.containerOpen = false;

    rootNode.containerOpen = false;
  }
}

function run_test() {
  run_next_test();
}

add_task(function() {
  
  let jsonFile = OS.Path.join(OS.Constants.Path.profileDir, "bookmarks.json");

  
  test.populate();

  yield BookmarkJSONUtils.exportToFile(jsonFile);

  
  yield BookmarkJSONUtils.importFromFile(jsonFile, true);

  
  
  test.validate(false);

  
  yield PlacesUtils.bookmarks.eraseEverything();
  
  PlacesUtils.bookmarks.removeItem(test._excludeRootId);
  
  yield BookmarkJSONUtils.importFromFile(jsonFile, true);

  
  test.validate(true);

  
  yield OS.File.remove(jsonFile);
});
