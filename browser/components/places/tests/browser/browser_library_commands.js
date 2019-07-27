









const TEST_URI = NetUtil.newURI("http://www.mozilla.org/");

registerCleanupFunction(function* () {
  yield PlacesUtils.bookmarks.eraseEverything();
  yield PlacesTestUtils.clearHistory();
});

add_task(function* test_date_container() {
  let library = yield promiseLibrary();
  info("Ensure date containers under History cannot be cut but can be deleted");

  yield PlacesTestUtils.addVisits(TEST_URI);

  
  let PO = library.PlacesOrganizer;

  PO.selectLeftPaneQuery('History');
  isnot(PO._places.selectedNode, null, "We correctly selected History");

  
  
  ok(PO._places.controller.isCommandEnabled("cmd_copy"),
     "Copy command is enabled");
  ok(!PO._places.controller.isCommandEnabled("cmd_cut"),
     "Cut command is disabled");
  ok(!PO._places.controller.isCommandEnabled("cmd_delete"),
     "Delete command is disabled");
  let historyNode = PlacesUtils.asContainer(PO._places.selectedNode);
  historyNode.containerOpen = true;

  
  is(historyNode.childCount, 1, "History node has one child");
  let todayNode = historyNode.getChild(0);
  let todayNodeExpectedTitle = PlacesUtils.getString("finduri-AgeInDays-is-0");
  is(todayNode.title, todayNodeExpectedTitle,
     "History child is the expected container");

  
  PO._places.selectNode(todayNode);
  is(PO._places.selectedNode, todayNode,
     "We correctly selected Today container");
  
  
  ok(PO._places.controller.isCommandEnabled("cmd_copy"),
     "Copy command is enabled");
  ok(!PO._places.controller.isCommandEnabled("cmd_cut"),
     "Cut command is disabled");
  ok(PO._places.controller.isCommandEnabled("cmd_delete"),
     "Delete command is enabled");

  
  let promiseURIRemoved = promiseHistoryNotification("onDeleteURI",
                                                     () => TEST_URI.equals(arguments[0]));
  PO._places.controller.doCommand("cmd_delete");
  yield promiseURIRemoved;

  
  is(historyNode.childCount, 0, "History node has no more children");

  historyNode.containerOpen = false;

  ok(!(yield promiseIsURIVisited(TEST_URI)), "Visit has been removed");

  library.close();
});

add_task(function* test_query_on_toolbar() {
  let library = yield promiseLibrary();
  info("Ensure queries can be cut or deleted");

  
  let PO = library.PlacesOrganizer;

  PO.selectLeftPaneQuery('BookmarksToolbar');
  isnot(PO._places.selectedNode, null, "We have a valid selection");
  is(PlacesUtils.getConcreteItemId(PO._places.selectedNode),
     PlacesUtils.toolbarFolderId,
     "We have correctly selected bookmarks toolbar node.");

  
  
  ok(PO._places.controller.isCommandEnabled("cmd_copy"),
     "Copy command is enabled");
  ok(!PO._places.controller.isCommandEnabled("cmd_cut"),
     "Cut command is disabled");
  ok(!PO._places.controller.isCommandEnabled("cmd_delete"),
     "Delete command is disabled");

  let toolbarNode = PlacesUtils.asContainer(PO._places.selectedNode);
  toolbarNode.containerOpen = true;

  
  let query = yield PlacesUtils.bookmarks.insert({ type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                   url: "place:sort=4",
                                                   title: "special_query",
                                                   parentGuid: PlacesUtils.bookmarks.toolbarGuid,
                                                   index: 0 });

  
  ok(toolbarNode.childCount > 0, "Toolbar node has children");
  let queryNode = toolbarNode.getChild(0);
  is(queryNode.title, "special_query", "Query node is correctly selected");

  
  PO._places.selectNode(queryNode);
  is(PO._places.selectedNode, queryNode, "We correctly selected query node");

  
  ok(PO._places.controller.isCommandEnabled("cmd_copy"),
     "Copy command is enabled");
  ok(PO._places.controller.isCommandEnabled("cmd_cut"),
     "Cut command is enabled");
  ok(PO._places.controller.isCommandEnabled("cmd_delete"),
     "Delete command is enabled");

  
  let promiseItemRemoved = promiseBookmarksNotification("onItemRemoved",
                                                        () => query.guid == arguments[5]);
  PO._places.controller.doCommand("cmd_delete");
  yield promiseItemRemoved;

  is((yield PlacesUtils.bookmarks.fetch(query.guid)), null,
     "Query node bookmark has been correctly removed");

  toolbarNode.containerOpen = false;

  library.close();
});

add_task(function* test_search_contents() {
  let item = yield PlacesUtils.bookmarks.insert({ type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                  url: "http://example.com/",
                                                  title: "example page",
                                                  parentGuid: PlacesUtils.bookmarks.unfiledGuid,
                                                  index: 0 });

  let library = yield promiseLibrary();
  info("Ensure query contents can be cut or deleted");

  
  let PO = library.PlacesOrganizer;

  PO.selectLeftPaneQuery('BookmarksToolbar');
  isnot(PO._places.selectedNode, null, "We have a valid selection");
  is(PlacesUtils.getConcreteItemId(PO._places.selectedNode),
     PlacesUtils.toolbarFolderId,
     "We have correctly selected bookmarks toolbar node.");

  let searchBox = library.document.getElementById("searchFilter");
  searchBox.value = "example";
  library.PlacesSearchBox.search(searchBox.value);

  let bookmarkNode = library.ContentTree.view.selectedNode;
  is(bookmarkNode.uri, "http://example.com/", "Found the expected bookmark");

  
  ok(library.ContentTree.view.controller.isCommandEnabled("cmd_copy"),
     "Copy command is enabled");
  ok(library.ContentTree.view.controller.isCommandEnabled("cmd_cut"),
     "Cut command is enabled");
  ok(library.ContentTree.view.controller.isCommandEnabled("cmd_delete"),
     "Delete command is enabled");

  library.close();
});

add_task(function* test_tags() {
  let item = yield PlacesUtils.bookmarks.insert({ type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                  url: "http://example.com/",
                                                  title: "example page",
                                                  parentGuid: PlacesUtils.bookmarks.unfiledGuid,
                                                  index: 0 });
  PlacesUtils.tagging.tagURI(NetUtil.newURI("http://example.com/"), ["test"]);

  let library = yield promiseLibrary();
  info("Ensure query contents can be cut or deleted");

  
  let PO = library.PlacesOrganizer;

  PO.selectLeftPaneQuery('Tags');
  let tagsNode = PO._places.selectedNode;
  isnot(tagsNode, null, "We have a valid selection");
  let tagsTitle = PlacesUtils.getString("TagsFolderTitle");
  is(tagsNode.title, tagsTitle,
     "Tags has been properly selected");

  
  ok(PO._places.controller.isCommandEnabled("cmd_copy"),
     "Copy command is enabled");
  ok(!PO._places.controller.isCommandEnabled("cmd_cut"),
     "Cut command is disabled");
  ok(!PO._places.controller.isCommandEnabled("cmd_delete"),
     "Delete command is disabled");

  
  PlacesUtils.asContainer(tagsNode).containerOpen = true;
  let tag = tagsNode.getChild(0);
  PO._places.selectNode(tag);
  is(PO._places.selectedNode.title, "test",
     "The created tag has been properly selected");

  
  ok(PO._places.controller.isCommandEnabled("cmd_copy"),
     "Copy command is enabled");
  ok(!PO._places.controller.isCommandEnabled("cmd_cut"),
     "Cut command is disabled");
  ok(PO._places.controller.isCommandEnabled("cmd_delete"),
     "Delete command is enabled");

  let bookmarkNode = library.ContentTree.view.selectedNode;
  is(bookmarkNode.uri, "http://example.com/", "Found the expected bookmark");

  
  ok(library.ContentTree.view.controller.isCommandEnabled("cmd_copy"),
     "Copy command is enabled");
  ok(!library.ContentTree.view.controller.isCommandEnabled("cmd_cut"),
     "Cut command is disabled");
  ok(library.ContentTree.view.controller.isCommandEnabled("cmd_delete"),
     "Delete command is enabled");

  tagsNode.containerOpen = false;

  library.close();
});
