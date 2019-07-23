





































Components.utils.import("resource://gre/modules/utils.js");
var tests = [];






























const DEFAULT_INDEX = PlacesUtils.bookmarks.DEFAULT_INDEX;

var test = {
  _testRootId: null,
  _testRootTitle: "test root",
  _folderIds: [],
  _bookmarkURIs: [],
  _count: 3,

  populate: function populate() {
    
    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.toolbarFolderId);
    this._testRootId =
      PlacesUtils.bookmarks.createFolder(PlacesUtils.toolbarFolderId,
                                         this._testRootTitle, DEFAULT_INDEX);

    
    for (var i = 0; i < this._count; i++) {
      var folderId = 
        PlacesUtils.bookmarks.createFolder(this._testRootId, "folder" + i, DEFAULT_INDEX);
      this._folderIds.push(folderId)
      
      var bookmarkURI = uri("http://" + i);
      PlacesUtils.bookmarks.insertBookmark(folderId, bookmarkURI,
                                           DEFAULT_INDEX, "bookmark" + i);
      this._bookmarkURIs.push(bookmarkURI);
    }

    
    this._queryURI1 = uri("place:folder=" + this._folderIds[0] + "&queryType=1");
    this._queryTitle1 = "query1";
    PlacesUtils.bookmarks.insertBookmark(this._testRootId, this._queryURI1,
                                         DEFAULT_INDEX, this._queryTitle1);

    
    this._queryURI2 = uri("place:folder=" + this._folderIds.join("&folder=") + "&queryType=1");
    this._queryTitle2 = "query2";
    PlacesUtils.bookmarks.insertBookmark(this._testRootId, this._queryURI2,
                                         DEFAULT_INDEX, this._queryTitle2);

    
    
    var queries = this._folderIds.map(function(aFolderId) {
      var query = PlacesUtils.history.getNewQuery();
      query.setFolders([aFolderId], 1);
      return query;
    });
    var options = PlacesUtils.history.getNewQueryOptions();
    options.queryType = options.QUERY_TYPE_BOOKMARKS;
    this._queryURI3 =
      uri(PlacesUtils.history.queriesToQueryString(queries, queries.length, options));
    this._queryTitle3 = "query3";
    PlacesUtils.bookmarks.insertBookmark(this._testRootId, this._queryURI3,
                                         DEFAULT_INDEX, this._queryTitle3);
  },

  clean: function () {},

  validate: function validate() {
    
    
    for (var i = 0; i < 10; i++) {
      PlacesUtils.bookmarks.
                  insertBookmark(PlacesUtils.bookmarksMenuFolderId, uri("http://aaaa"+i), DEFAULT_INDEX, "");
    }

    var toolbar =
      PlacesUtils.getFolderContents(PlacesUtils.toolbarFolderId,
                                    false, true).root;
    do_check_true(toolbar.childCount, 1);

    var folderNode = toolbar.getChild(0);
    do_check_eq(folderNode.type, folderNode.RESULT_TYPE_FOLDER);
    do_check_eq(folderNode.title, this._testRootTitle);
    folderNode.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    folderNode.containerOpen = true;

    
    do_check_eq(folderNode.childCount, this._count+3);

    for (var i = 0; i < this._count; i++) {
      var subFolder = folderNode.getChild(i);
      do_check_eq(subFolder.title, "folder"+i);
      subFolder.QueryInterface(Ci.nsINavHistoryContainerResultNode);
      subFolder.containerOpen = true;
      do_check_eq(subFolder.childCount, 1);
      var child = subFolder.getChild(0);
      do_check_eq(child.title, "bookmark"+i);
      do_check_true(uri(child.uri).equals(uri("http://" + i)))
    }

    
    this.validateQueryNode1(folderNode.getChild(this._count));

    
    this.validateQueryNode2(folderNode.getChild(this._count + 1));

    
    this.validateQueryNode3(folderNode.getChild(this._count + 2));

    
    folderNode.containerOpen = false;
    toolbar.containerOpen = false;
  },

  validateQueryNode1: function validateQueryNode1(aNode) {
    do_check_eq(aNode.title, this._queryTitle1);
    do_check_true(PlacesUtils.nodeIsFolder(aNode));

    aNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
    aNode.containerOpen = true;
    do_check_eq(aNode.childCount, 1);
    var child = aNode.getChild(0);
    do_check_true(uri(child.uri).equals(uri("http://0")))
    do_check_eq(child.title, "bookmark0")
    aNode.containerOpen = false;
  },

  validateQueryNode2: function validateQueryNode2(aNode) {
    do_check_eq(aNode.title, this._queryTitle2);
    do_check_true(PlacesUtils.nodeIsQuery(aNode));

    aNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
    aNode.containerOpen = true;
    do_check_eq(aNode.childCount, this._count);
    for (var i = 0; i < aNode.childCount; i++) {
      var child = aNode.getChild(i);
      do_check_true(uri(child.uri).equals(uri("http://" + i)))
      do_check_eq(child.title, "bookmark" + i)
    }
    aNode.containerOpen = false;
  },

  validateQueryNode3: function validateQueryNode3(aNode) {
    do_check_eq(aNode.title, this._queryTitle3);
    do_check_true(PlacesUtils.nodeIsQuery(aNode));

    aNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
    aNode.containerOpen = true;
    do_check_eq(aNode.childCount, this._count);
    for (var i = 0; i < aNode.childCount; i++) {
      var child = aNode.getChild(i);
      do_check_true(uri(child.uri).equals(uri("http://" + i)))
      do_check_eq(child.title, "bookmark" + i)
    }
    aNode.containerOpen = false;
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
