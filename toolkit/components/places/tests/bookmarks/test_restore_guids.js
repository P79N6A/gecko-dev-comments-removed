





































Components.utils.import("resource://gre/modules/utils.js");
var tests = [];














const DEFAULT_INDEX = PlacesUtils.bookmarks.DEFAULT_INDEX;

var test = {
  _testBookmarkId: null,
  _testBookmarkGUID: null, 

  populate: function populate() {
    var bookmarkURI = uri("http://bookmark");
    this._testBookmarkId =
      PlacesUtils.bookmarks.insertBookmark(PlacesUtils.toolbarFolderId, bookmarkURI,
                                           DEFAULT_INDEX, "bookmark");
    this._testBookmarkGUID = PlacesUtils.bookmarks.getItemGUID(this._testBookmarkId);
  },

  clean: function () {},

  validate: function validate() {
    var guid = PlacesUtils.bookmarks.getItemGUID(this._testBookmarkId);
    do_check_eq(this._testBookmarkGUID, guid);
  }
}
tests.push(test);

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
