




































function test() {
  
  ok(PlacesUtils, "checking PlacesUtils, running in chrome context?");
  ok(PlacesUIUtils, "checking PlacesUIUtils, running in chrome context?");
  ok(PlacesControllerDragHelper, "checking PlacesControllerDragHelper, running in chrome context?");

  const IDX = PlacesUtils.bookmarks.DEFAULT_INDEX;

  
  var rootId = PlacesUtils.bookmarks.createFolder(PlacesUtils.toolbarFolderId, "", IDX);
  var rootNode = PlacesUtils.getFolderContents(rootId, false, true).root;
  is(rootNode.childCount, 0, "confirm test root is empty");

  var tests = [];

  
  tests.push({
    populate: function() {
      this.id =
        PlacesUtils.bookmarks.createFolder(rootId, "", IDX);
    },
    validate: function() {
      is(rootNode.childCount, 1,
        "populate added data to the test root");
      is(PlacesControllerDragHelper.canMoveContainer(this.id),
         true, "can move regular folder id");
      is(PlacesControllerDragHelper.canMoveNode(rootNode.getChild(0)),
         true, "can move regular folder node");
    }
  });

  
  tests.push({
    populate: function() {
      this.folderId =
        PlacesUtils.bookmarks.createFolder(rootId, "foo", IDX);
      this.shortcutId =
        PlacesUtils.bookmarks.insertBookmark(rootId, makeURI("place:folder="+this.folderId), IDX, "bar");
    },
    validate: function() {
      is(rootNode.childCount, 2,
        "populated data to the test root");

      var folderNode = rootNode.getChild(0);
      is(folderNode.type, 6, "node is folder");
      is(this.folderId, folderNode.itemId, "folder id and folder node item id match");

      var shortcutNode = rootNode.getChild(1);
      is(shortcutNode.type, 9, "node is folder shortcut");
      is(this.shortcutId, shortcutNode.itemId, "shortcut id and shortcut node item id match");

      var concreteId = PlacesUtils.getConcreteItemId(shortcutNode);
      is(concreteId, folderNode.itemId, "shortcut node id and concrete id match");

      is(PlacesControllerDragHelper.canMoveContainer(this.shortcutId),
         true, "can move folder shortcut id");

      is(PlacesControllerDragHelper.canMoveNode(shortcutNode),
         true, "can move folder shortcut node");
    }
  });

  
  tests.push({
    populate: function() {
      this.bookmarkId =
        PlacesUtils.bookmarks.insertBookmark(rootId, makeURI("http://foo.com"), IDX, "foo");
      this.queryId =
        PlacesUtils.bookmarks.insertBookmark(rootId, makeURI("place:terms=foo"), IDX, "bar");
    },
    validate: function() {
      is(rootNode.childCount, 2,
        "populated data to the test root");

      var bmNode = rootNode.getChild(0);
      is(bmNode.itemId, this.bookmarkId, "bookmark id and bookmark node item id match");

      var queryNode = rootNode.getChild(1);
      is(queryNode.itemId, this.queryId, "query id and query node item id match");

      is(PlacesControllerDragHelper.canMoveContainer(this.queryId),
         true, "can move query id");

      is(PlacesControllerDragHelper.canMoveNode(queryNode),
         true, "can move query node");
    }
  });

  
  
  tests.push({
    folders: [PlacesUtils.bookmarksMenuFolderId,
              PlacesUtils.tagsFolderId, PlacesUtils.unfiledBookmarksFolderId,
              PlacesUtils.toolbarFolderId],
    shortcuts: {},
    populate: function() {
      for (var i = 0; i < this.folders.length; i++) {
        var id = this.folders[i];
        this.shortcuts[id] =
          PlacesUtils.bookmarks.insertBookmark(rootId, makeURI("place:folder=" + id), IDX, "");
      }
    },
    validate: function() {
      
      is(rootNode.childCount, this.folders.length,
        "populated data to the test root");

      function getRootChildNode(aId) {
        var node = PlacesUtils.getFolderContents(PlacesUtils.placesRootId, false, true).root;
        for (var i = 0; i < node.childCount; i++) {
          var child = node.getChild(i);
          if (child.itemId == aId) {
            node.containerOpen = false;
            return child;
          }
        }
        node.containerOpen = false;
        ok(false, "Unable to find child node");
        return null;
      }

      for (var i = 0; i < this.folders.length; i++) {
        var id = this.folders[i];

        is(PlacesControllerDragHelper.canMoveContainer(id),
           false, "shouldn't be able to move special folder id");

        var node = getRootChildNode(id);
        isnot(node, null, "Node found");
        is(PlacesControllerDragHelper.canMoveNode(node),
           false, "shouldn't be able to move special folder node");

        var shortcutId = this.shortcuts[id];
        var shortcutNode = rootNode.getChild(i);

        is(shortcutNode.itemId, shortcutId, "shortcut id and shortcut node item id match");

        LOG("can move shortcut id?");
        is(PlacesControllerDragHelper.canMoveContainer(shortcutId),
           true, "should be able to move special folder shortcut id");

        LOG("can move shortcut node?");
        is(PlacesControllerDragHelper.canMoveNode(shortcutNode),
           true, "should be able to move special folder shortcut node");
      }
    }
  });

  
  tests.push({
    populate: function() {
      
      this.uri = makeURI("http://foo.com");
      PlacesUtils.tagging.tagURI(this.uri, ["bar"]);
    },
    validate: function() {
      
      var query = PlacesUtils.history.getNewQuery();
      var options = PlacesUtils.history.getNewQueryOptions();
      options.resultType = Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_QUERY;
      var tagsNode = PlacesUtils.history.executeQuery(query, options).root;

      tagsNode.containerOpen = true;
      is(tagsNode.childCount, 1, "has new tag");

      var tagNode = tagsNode.getChild(0);
      
      is(PlacesControllerDragHelper.canMoveNode(tagNode),
         false, "should not be able to move tag container node");

      tagsNode.containerOpen = false;
    }
  });

  
  tests.push({
    populate: function() {
      this.id =
        PlacesUtils.bookmarks.createFolder(rootId, "foo", IDX);
      PlacesUtils.bookmarks.createFolder(this.id, "bar", IDX);
      PlacesUtils.bookmarks.setFolderReadonly(this.id, true);
    },
    validate: function() {
      is(rootNode.childCount, 1,
        "populate added data to the test root");
      var readOnlyFolder = rootNode.getChild(0);

      
      is(PlacesControllerDragHelper.canMoveContainer(this.id),
         true, "can move read-only folder id");
      is(PlacesControllerDragHelper.canMoveNode(readOnlyFolder),
         true, "can move read-only folder node");

      
      readOnlyFolder.QueryInterface(Ci.nsINavHistoryContainerResultNode);
      readOnlyFolder.containerOpen = true;
      var childFolder = readOnlyFolder.getChild(0);

      is(PlacesControllerDragHelper.canMoveContainer(childFolder.itemId),
         false, "cannot move a child of a read-only folder");
      is(PlacesControllerDragHelper.canMoveNode(childFolder),
         false, "cannot move a child node of a read-only folder node");
    }
  });

  tests.forEach(function(aTest) {
    PlacesUtils.bookmarks.removeFolderChildren(rootId);
    aTest.populate();
    aTest.validate();
  });

  PlacesUtils.bookmarks.removeItem(rootId);
}
