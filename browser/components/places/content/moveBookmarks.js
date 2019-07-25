





































var gMoveBookmarksDialog = {
  _nodes: null,

  _foldersTree: null,
  get foldersTree() {
    if (!this._foldersTree)
      this._foldersTree = document.getElementById("foldersTree");

    return this._foldersTree;
  },

  init: function() {
    this._nodes = window.arguments[0];

    this.foldersTree.place =
      "place:excludeItems=1&excludeQueries=1&excludeReadOnlyFolders=1&folder=" +
      PlacesUIUtils.allBookmarksFolderId;
  },

  onOK: function MBD_onOK(aEvent) {
    var selectedNode = this.foldersTree.selectedNode;
    NS_ASSERT(selectedNode,
              "selectedNode must be set in a single-selection tree with initial selection set");
    var selectedFolderID = PlacesUtils.getConcreteItemId(selectedNode);

    var transactions = [];
    for (var i=0; i < this._nodes.length; i++) {
      
      if (this._nodes[i].parent.itemId == selectedFolderID)
        continue;

      let txn = new PlacesMoveItemTransaction(this._nodes[i].itemId,
                                              selectedFolderID,
                                              PlacesUtils.bookmarks.DEFAULT_INDEX);
      transactions.push(txn);
    }

    if (transactions.length != 0) {
      let txn = new PlacesAggregatedTransaction("Move Items", transactions);
      PlacesUtils.transactionManager.doTransaction(txn);
    }
  },

  newFolder: function MBD_newFolder() {
    
    this.foldersTree.focus();
    goDoCommand("placesCmd_new:folder");
  }
};
