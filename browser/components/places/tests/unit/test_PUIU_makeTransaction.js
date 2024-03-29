


function waitForBookmarkNotification(aNotification, aCallback, aProperty)
{
  PlacesUtils.bookmarks.addObserver({
    validate: function (aMethodName, aData)
    {
      if (aMethodName == aNotification &&
          (!aProperty || aProperty == aData.property)) {
        PlacesUtils.bookmarks.removeObserver(this);
        aCallback(aData);
      }
    },

    
    QueryInterface: XPCOMUtils.generateQI([Ci.nsINavBookmarkObserver]),
    onBeginUpdateBatch: function onBeginUpdateBatch()
      this.validate(arguments.callee.name, arguments),
    onEndUpdateBatch: function onEndUpdateBatch()
      this.validate(arguments.callee.name, arguments),
    onItemAdded: function onItemAdded(aItemId, aParentId, aIndex, aItemType,
                                      aURI, aTitle)
    {
      return this.validate(arguments.callee.name, { id: aItemId,
                                                    index: aIndex,
                                                    type: aItemType,
                                                    url: aURI ? aURI.spec : null,
                                                    title: aTitle });
    },
    onItemRemoved: function onItemRemoved()
      this.validate(arguments.callee.name, arguments),
    onItemChanged: function onItemChanged(aItemId, aProperty, aIsAnno,
                                          aNewValue, aLastModified, aItemType)
    {
      return this.validate(arguments.callee.name,
                           { id: aItemId,
                             get index() PlacesUtils.bookmarks.getItemIndex(this.id),
                             type: aItemType,
                             property: aProperty,
                             get url() aItemType == PlacesUtils.bookmarks.TYPE_BOOKMARK ?
                                                PlacesUtils.bookmarks.getBookmarkURI(this.id).spec :
                                                null,
                             get title() PlacesUtils.bookmarks.getItemTitle(this.id),
                           });
    },
    onItemVisited: function onItemVisited()
      this.validate(arguments.callee.name, arguments),
    onItemMoved: function onItemMoved(aItemId, aOldParentId, aOldIndex,
                                      aNewParentId, aNewIndex, aItemType)
    {
      this.validate(arguments.callee.name, { id: aItemId,
                                             index: aNewIndex,
                                             type: aItemType });
    }
  }, false);
}

function wrapNodeByIdAndParent(aItemId, aParentId)
{
  let wrappedNode;
  let root = PlacesUtils.getFolderContents(aParentId, false, false).root;
  for (let i = 0; i < root.childCount; ++i) {
    let node = root.getChild(i);
    if (node.itemId == aItemId) {
      let type;
      if (PlacesUtils.nodeIsContainer(node)) {
        type = PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER;
      }
      else if (PlacesUtils.nodeIsURI(node)) {
        type = PlacesUtils.TYPE_X_MOZ_PLACE;
      }
      else if (PlacesUtils.nodeIsSeparator(node)) {
        type = PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR;
      }
      else {
        do_throw("Unknown node type");
      }
      wrappedNode = PlacesUtils.wrapNode(node, type);
    }
  }
  root.containerOpen = false;
  return JSON.parse(wrappedNode);
}

add_test(function test_text_paste()
{
  const TEST_URL = "http://places.moz.org/"
  const TEST_TITLE = "Places bookmark"

  waitForBookmarkNotification("onItemAdded", function(aData)
  {
    do_check_eq(aData.title, TEST_TITLE);
    do_check_eq(aData.url, TEST_URL);
    do_check_eq(aData.type, PlacesUtils.bookmarks.TYPE_BOOKMARK);
    do_check_eq(aData.index, 0);
    run_next_test();
  });

  let txn = PlacesUIUtils.makeTransaction(
    { title: TEST_TITLE, uri: TEST_URL },
    PlacesUtils.TYPE_X_MOZ_URL,
    PlacesUtils.unfiledBookmarksFolderId,
    PlacesUtils.bookmarks.DEFAULT_INDEX,
    true 
  );
  PlacesUtils.transactionManager.doTransaction(txn);
});

add_test(function test_container()
{
  const TEST_TITLE = "Places folder"

  waitForBookmarkNotification("onItemChanged", function(aData)
  {
    do_check_eq(aData.title, TEST_TITLE);
    do_check_eq(aData.type, PlacesUtils.bookmarks.TYPE_FOLDER);
    do_check_eq(aData.index, 1);

    waitForBookmarkNotification("onItemAdded", function(aData)
    {
      do_check_eq(aData.title, TEST_TITLE);
      do_check_eq(aData.type, PlacesUtils.bookmarks.TYPE_FOLDER);
      do_check_eq(aData.index, 2);
      let id = aData.id;

      waitForBookmarkNotification("onItemMoved", function(aData)
      {
        do_check_eq(aData.id, id);
        do_check_eq(aData.type, PlacesUtils.bookmarks.TYPE_FOLDER);
        do_check_eq(aData.index, 1);

        run_next_test();
      });

      let txn = PlacesUIUtils.makeTransaction(
        wrapNodeByIdAndParent(aData.id, PlacesUtils.unfiledBookmarksFolderId),
        0, 
        PlacesUtils.unfiledBookmarksFolderId,
        1, 
        false
      );
      PlacesUtils.transactionManager.doTransaction(txn);
    });

    try {
    let txn = PlacesUIUtils.makeTransaction(
      wrapNodeByIdAndParent(aData.id, PlacesUtils.unfiledBookmarksFolderId),
      0, 
      PlacesUtils.unfiledBookmarksFolderId,
      PlacesUtils.bookmarks.DEFAULT_INDEX,
      true
    );
    PlacesUtils.transactionManager.doTransaction(txn);
    } catch(ex) {
      do_throw(ex);
    }
  }, "random-anno");

  let id = PlacesUtils.bookmarks.createFolder(PlacesUtils.unfiledBookmarksFolderId,
                                              TEST_TITLE,
                                              PlacesUtils.bookmarks.DEFAULT_INDEX);
  PlacesUtils.annotations.setItemAnnotation(id, PlacesUIUtils.DESCRIPTION_ANNO,
                                            "description", 0,
                                            PlacesUtils.annotations.EXPIRE_NEVER);
  PlacesUtils.annotations.setItemAnnotation(id, "random-anno",
                                            "random-value", 0,
                                            PlacesUtils.annotations.EXPIRE_NEVER);
});


add_test(function test_separator()
{
  waitForBookmarkNotification("onItemChanged", function(aData)
  {
    do_check_eq(aData.type, PlacesUtils.bookmarks.TYPE_SEPARATOR);
    do_check_eq(aData.index, 3);

    waitForBookmarkNotification("onItemAdded", function(aData)
    {
      do_check_eq(aData.type, PlacesUtils.bookmarks.TYPE_SEPARATOR);
      do_check_eq(aData.index, 4);
      let id = aData.id;

      waitForBookmarkNotification("onItemMoved", function(aData)
      {
        do_check_eq(aData.id, id);
        do_check_eq(aData.type, PlacesUtils.bookmarks.TYPE_SEPARATOR);
        do_check_eq(aData.index, 1);

        run_next_test();
      });

      let txn = PlacesUIUtils.makeTransaction(
        wrapNodeByIdAndParent(aData.id, PlacesUtils.unfiledBookmarksFolderId),
        0, 
        PlacesUtils.unfiledBookmarksFolderId,
        1, 
        false
      );
      PlacesUtils.transactionManager.doTransaction(txn);
    });

    try {
    let txn = PlacesUIUtils.makeTransaction(
      wrapNodeByIdAndParent(aData.id, PlacesUtils.unfiledBookmarksFolderId),
      0, 
      PlacesUtils.unfiledBookmarksFolderId,
      PlacesUtils.bookmarks.DEFAULT_INDEX,
      true
    );
    PlacesUtils.transactionManager.doTransaction(txn);
    } catch(ex) {
      do_throw(ex);
    }
  }, "random-anno");

  let id = PlacesUtils.bookmarks.insertSeparator(PlacesUtils.unfiledBookmarksFolderId,
                                                 PlacesUtils.bookmarks.DEFAULT_INDEX);
  PlacesUtils.annotations.setItemAnnotation(id, "random-anno",
                                            "random-value", 0, 
                                            PlacesUtils.annotations.EXPIRE_NEVER);
});

add_test(function test_bookmark()
{
  const TEST_URL = "http://places.moz.org/"
  const TEST_TITLE = "Places bookmark"

  waitForBookmarkNotification("onItemChanged", function(aData)
  {
    do_check_eq(aData.title, TEST_TITLE);
    do_check_eq(aData.url, TEST_URL);
    do_check_eq(aData.type, PlacesUtils.bookmarks.TYPE_BOOKMARK);
    do_check_eq(aData.index, 5);

    waitForBookmarkNotification("onItemAdded", function(aData)
    {
      do_check_eq(aData.title, TEST_TITLE);
      do_check_eq(aData.url, TEST_URL);
      do_check_eq(aData.type, PlacesUtils.bookmarks.TYPE_BOOKMARK);
      do_check_eq(aData.index, 6);
      let id = aData.id;

      waitForBookmarkNotification("onItemMoved", function(aData)
      {
        do_check_eq(aData.id, id);
        do_check_eq(aData.type, PlacesUtils.bookmarks.TYPE_BOOKMARK);
        do_check_eq(aData.index, 1);

        run_next_test();
      });

      let txn = PlacesUIUtils.makeTransaction(
        wrapNodeByIdAndParent(aData.id, PlacesUtils.unfiledBookmarksFolderId),
        0, 
        PlacesUtils.unfiledBookmarksFolderId,
        1, 
        false
      );
      PlacesUtils.transactionManager.doTransaction(txn);
    });

    try {
    let txn = PlacesUIUtils.makeTransaction(
      wrapNodeByIdAndParent(aData.id, PlacesUtils.unfiledBookmarksFolderId),
      0, 
      PlacesUtils.unfiledBookmarksFolderId,
      PlacesUtils.bookmarks.DEFAULT_INDEX,
      true
    );
    PlacesUtils.transactionManager.doTransaction(txn);
    } catch(ex) {
      do_throw(ex);
    }
  }, "random-anno");

  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                NetUtil.newURI(TEST_URL),
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                TEST_TITLE);
  PlacesUtils.annotations.setItemAnnotation(id, PlacesUIUtils.DESCRIPTION_ANNO,
                                            "description", 0,
                                            PlacesUtils.annotations.EXPIRE_NEVER);
  PlacesUtils.annotations.setItemAnnotation(id, "random-anno",
                                            "random-value", 0,
                                            PlacesUtils.annotations.EXPIRE_NEVER);
});

add_test(function test_visit()
{
  const TEST_URL = "http://places.moz.org/"
  const TEST_TITLE = "Places bookmark"

  waitForBookmarkNotification("onItemAdded", function(aData)
  {
    do_check_eq(aData.title, TEST_TITLE);
    do_check_eq(aData.url, TEST_URL);
    do_check_eq(aData.type, PlacesUtils.bookmarks.TYPE_BOOKMARK);
    do_check_eq(aData.index, 7);

    waitForBookmarkNotification("onItemAdded", function(aData)
    {
      do_check_eq(aData.title, TEST_TITLE);
      do_check_eq(aData.url, TEST_URL);
      do_check_eq(aData.type, PlacesUtils.bookmarks.TYPE_BOOKMARK);
      do_check_eq(aData.index, 8);
      run_next_test();
    });

    try {
    let node = wrapNodeByIdAndParent(aData.id, PlacesUtils.unfiledBookmarksFolderId);
    
    node.id = -1;
    let txn = PlacesUIUtils.makeTransaction(
      node,
      0, 
      PlacesUtils.unfiledBookmarksFolderId,
      PlacesUtils.bookmarks.DEFAULT_INDEX,
      true
    );
    PlacesUtils.transactionManager.doTransaction(txn);
    } catch(ex) {
      do_throw(ex);
    }
  });

  PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                       NetUtil.newURI(TEST_URL),
                                       PlacesUtils.bookmarks.DEFAULT_INDEX,
                                       TEST_TITLE);
});

add_test(function check_annotations() {
  

  
  let descriptions =
    PlacesUtils.annotations.getItemsWithAnnotation(PlacesUIUtils.DESCRIPTION_ANNO, {});
  do_check_eq(descriptions.length, 4);

  
  let others = PlacesUtils.annotations.getItemsWithAnnotation("random-anno", {});
  do_check_eq(others.length, 3);
  run_next_test();
});

function run_test()
{
  run_next_test();
}
