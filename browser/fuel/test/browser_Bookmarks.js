var gLastFolderAction = "";
var gLastBookmarkAction = "";
var gLastRootAction = "";

function url(spec) {
  return Services.io.newURI(spec, null, null);
}

function test() {
  
  var tags = Application.bookmarks.tags;
  ok(tags, "Check access to bookmark tags root");
  ok(!tags.parent, "Check tags parent (should be null)");

  

  
  var unfiled = Application.bookmarks.unfiled;
  ok(unfiled, "Check access to bookmark unfiled root");
  ok(!unfiled.parent, "Check unfiled parent (should be null)");

  

  
  var toolbar = Application.bookmarks.toolbar;
  ok(toolbar, "Check access to bookmark toolbar root");
  ok(!toolbar.parent, "Check toolbar parent (should be null)");

  var toolbarKidCount = toolbar.children.length;

  
  var testFolderToolbar = toolbar.addFolder("FUEL in Toolbar");
  ok(testFolderToolbar, "Check folder creation");
  is(testFolderToolbar.type, "folder", "Check 'folder.type' after creation");
  ok(testFolderToolbar.parent, "Check parent after folder creation");

  toolbarKidCount++;
  is(toolbar.children.length, toolbarKidCount, "Check toolbar folder child count after adding a child folder");

  

  
  var root = Application.bookmarks.menu;
  ok(root, "Check access to bookmark root");
  ok(!root.parent, "Check root parent (should be null)");

  var rootKidCount = root.children.length;

  
  var testFolder = root.addFolder("FUEL");
  ok(testFolder, "Check folder creation");
  is(testFolder.type, "folder", "Check 'folder.type' after creation");
  ok(testFolder.parent, "Check parent after folder creation");

  rootKidCount++;
  is(root.children.length, rootKidCount, "Check root folder child count after adding a child folder");

  
  testFolder.events.addListener("change", onFolderChange);
  testFolder.description = "FUEL folder";
  is(testFolder.description, "FUEL folder", "Check setting 'folder.description'");
  is(gLastFolderAction, "bookmarkProperties/description", "Check event handler for setting 'folder.description'");

  testFolder.title = "fuel-is-cool";
  is(testFolder.title, "fuel-is-cool", "Check setting 'folder.title'");
  is(gLastFolderAction, "title", "Check event handler for setting 'folder.title'");

  testFolder.annotations.set("testing/folder", "annotate-this", 0);
  ok(testFolder.annotations.has("testing/folder"), "Checking existence of added annotation");
  is(gLastFolderAction, "testing/folder", "Check event handler for setting annotation");
  gLastFolderAction = "";
  is(testFolder.annotations.get("testing/folder"), "annotate-this", "Checking existence of added annotation");
  testFolder.annotations.remove("testing/folder");
  ok(!testFolder.annotations.has("testing/folder"), "Checking existence of removed annotation");
  is(gLastFolderAction, "testing/folder", "Check event handler for removing annotation");

  testFolder.events.addListener("addchild", onFolderAddChild);
  testFolder.events.addListener("removechild", onFolderRemoveChild);

  
  var testBookmark = testFolder.addBookmark("Mozilla", url("https://www.mozilla.org/"));
  ok(testBookmark, "Check bookmark creation");
  ok(testBookmark.parent, "Check parent after bookmark creation");
  is(gLastFolderAction, "addchild", "Check event handler for adding a child to a folder");
  is(testBookmark.type, "bookmark", "Check 'bookmark.type' after creation");
  is(testBookmark.title, "Mozilla", "Check 'bookmark.title' after creation");
  is(testBookmark.uri.spec, "https://www.mozilla.org/", "Check 'bookmark.uri' after creation");

  is(testFolder.children.length, 1, "Check test folder child count after adding a child bookmark");

  
  testBookmark.events.addListener("change", onBookmarkChange);
  testBookmark.description = "mozcorp";
  is(testBookmark.description, "mozcorp", "Check setting 'bookmark.description'");
  is(gLastBookmarkAction, "bookmarkProperties/description", "Check event handler for setting 'bookmark.description'");

  testBookmark.keyword = "moz"
  is(testBookmark.keyword, "moz", "Check setting 'bookmark.keyword'");
  is(gLastBookmarkAction, "keyword", "Check event handler for setting 'bookmark.keyword'");

  testBookmark.title = "MozCorp"
  is(testBookmark.title, "MozCorp", "Check setting 'bookmark.title'");
  is(gLastBookmarkAction, "title", "Check event handler for setting 'bookmark.title'");

  testBookmark.uri = url("http://www.mozilla.org/");
  is(testBookmark.uri.spec, "http://www.mozilla.org/", "Check setting 'bookmark.uri'");
  is(gLastBookmarkAction, "uri", "Check event handler for setting 'bookmark.uri'");

  
  testBookmark.annotations.set("testing/bookmark", "annotate-this", 0);
  ok(testBookmark.annotations.has("testing/bookmark"), "Checking existence of added annotation");
  is(gLastBookmarkAction, "testing/bookmark", "Check event handler for setting annotation");
  gLastBookmarkAction = "";
  is(testBookmark.annotations.get("testing/bookmark"), "annotate-this", "Checking existence of added annotation");
  testBookmark.annotations.remove("testing/bookmark");
  ok(!testBookmark.annotations.has("testing/bookmark"), "Checking existence of removed annotation");
  is(gLastBookmarkAction, "testing/bookmark", "Check event handler for removing annotation");
  is(testBookmark.annotations.get("testing/bookmark"), null, "Check existence of a missing annotation");

  
  testBookmark.annotations.set("testing/bookmark/string", "annotate-this", 0);
  ok(testBookmark.annotations.has("testing/bookmark/string"), "Checking existence of added string annotation");
  is(testBookmark.annotations.get("testing/bookmark/string"), "annotate-this", "Checking value of added string annotation");
  is(gLastBookmarkAction, "testing/bookmark/string", "Check event handler for setting annotation");
  gLastBookmarkAction = "";
  testBookmark.annotations.set("testing/bookmark/int", 100, 0);
  ok(testBookmark.annotations.has("testing/bookmark/int"), "Checking existence of added integer annotation");
  is(testBookmark.annotations.get("testing/bookmark/int"), 100, "Checking value of added integer annotation");
  is(gLastBookmarkAction, "testing/bookmark/int", "Check event handler for setting annotation");
  gLastBookmarkAction = "";
  testBookmark.annotations.set("testing/bookmark/double", 3.333, 0);
  ok(testBookmark.annotations.has("testing/bookmark/double"), "Checking existence of added double annotation");
  is(testBookmark.annotations.get("testing/bookmark/double"), 3.333, "Checking value of added double annotation");
  is(gLastBookmarkAction, "testing/bookmark/double", "Check event handler for setting annotation");
  gLastBookmarkAction = "";

  
  var names = testBookmark.annotations.names;
  ok(names.some(function (f) f == "bookmarkProperties/description"), "Checking for description annotation");
  ok(names.some(function (f) f == "testing/bookmark/string"), "Checking for string test annotation");
  ok(names.some(function (f) f == "testing/bookmark/int"), "Checking for int test annotation");
  ok(names.some(function (f) f == "testing/bookmark/double"), "Checking for double test annotation");

  
  var testSeparator = testFolder.addSeparator();
  ok(testSeparator, "Check bookmark creation");
  ok(testSeparator.parent, "Check parent after separator creation");
  is(gLastFolderAction, "addchild", "Check event handler for adding a child separator to a folder");
  is(testSeparator.type, "separator", "Check 'bookmark.type' after separator creation");

  is(testFolder.children.length, 2, "Check test folder child count after adding a child separator");

  
  testSeparator.events.addListener("remove", onBookmarkRemove);
  testSeparator.remove();
  is(gLastBookmarkAction, "remove", "Check event handler for removing separator");
  is(gLastFolderAction, "removechild", "Check event handler for removing a child separator from a folder");
  is(testFolder.children.length, 1, "Check test folder child count after removing a child separator");

  
  testBookmark.events.addListener("remove", onBookmarkRemove);
  testBookmark.remove();
  is(gLastBookmarkAction, "remove", "Check event handler for removing bookmark");
  is(gLastFolderAction, "removechild", "Check event handler for removing a child from a folder");
  is(testFolder.children.length, 0, "Check test folder child count after removing a child bookmark");

  
  testFolder.events.addListener("remove", onFolderRemove);
  testFolder.remove();
  is(gLastFolderAction, "remove", "Check event handler for removing child folder");
  rootKidCount--;
  is(root.children.length, rootKidCount, "Check root folder child count after removing a child folder");

  
  var testFolderA = root.addFolder("folder-a");
  var testFolderB = root.addFolder("folder-b");

  var testMove = testFolderA.addBookmark("Mozilla", url("https://www.mozilla.org/"));
  testMove.events.addListener("move", onBookmarkMove);
  is(testMove.parent.title, "folder-a", "Checking for new parent before moving bookmark");

  testMove.parent = testFolderB;
  is(testMove.parent.title, "folder-b", "Checking for new parent after moving bookmark");
  is(gLastBookmarkAction, "move", "Checking for event handler after moving bookmark");

  
  testFolderA.events.addListener("move", onFolderMove);
  testFolderA.parent = testFolderB;
  is(testFolderA.parent.title, "folder-b", "Checking for new parent after moving folder");
  is(gLastFolderAction, "move", "Checking for event handler after moving folder");

  
  root.events.addListener("add", onRootAdd);
  root.events.addListener("remove", onRootRemove);
  root.events.addListener("change", onRootChange);
  var testFolderC = root.addFolder("folder-c");
  is(gLastRootAction, "add");

  root.events.removeListener("add", onRootAdd);
  gLastRootAction = "";
  var testFolderD = root.addFolder("folder-d");
  is(gLastRootAction, "");

  testFolderC.remove();
  is(gLastRootAction, "remove");

  testFolderD.description = "Foo";
  is(gLastRootAction, "bookmarkProperties/description");
}

function onFolderChange(evt) {
  gLastFolderAction = evt.data;
}

function onFolderRemove(evt) {
  gLastFolderAction = evt.type;
}

function onFolderAddChild(evt) {
  gLastFolderAction = evt.type;
}

function onFolderRemoveChild(evt) {
  gLastFolderAction = evt.type;
}

function onFolderMove(evt) {
  gLastFolderAction = evt.type;
}

function onBookmarkChange(evt) {
  gLastBookmarkAction = evt.data;
}

function onBookmarkRemove(evt) {
  gLastBookmarkAction = evt.type;
}

function onBookmarkMove(evt) {
  gLastBookmarkAction = evt.type;
}

function onRootAdd(evt) {
  gLastRootAction = evt.type;
}

function onRootRemove(evt) {
  gLastRootAction = evt.type;
}

function onRootChange(evt) {
  gLastRootAction = evt.data;
}
