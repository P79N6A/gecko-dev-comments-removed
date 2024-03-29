




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
    let selectedNode = this.foldersTree.selectedNode;
    let selectedFolderId = PlacesUtils.getConcreteItemId(selectedNode);

    if (!PlacesUIUtils.useAsyncTransactions) {
      let transactions = [];
      for (var i=0; i < this._nodes.length; i++) {
        
        if (this._nodes[i].parent.itemId == selectedFolderId)
          continue;

        let txn = new PlacesMoveItemTransaction(this._nodes[i].itemId,
                                                selectedFolderId,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX);
        transactions.push(txn);
      }
      if (transactions.length != 0) {
        let txn = new PlacesAggregatedTransaction("Move Items", transactions);
        PlacesUtils.transactionManager.doTransaction(txn);
      }
      return;
    }

    PlacesTransactions.batch(function* () {
      let newParentGuid = yield PlacesUtils.promiseItemGuid(selectedFolderId);
      for (let node of this._nodes) {
        
        if (node.parent.itemId == selectedFolderId)
          continue;
        yield PlacesTransactions.Move({ guid: node.bookmarkGuid
                                      , newParentGuid }).transact();
      }
    }.bind(this)).then(null, Components.utils.reportError);
  },

  newFolder: function MBD_newFolder() {
    
    this.foldersTree.focus();
    goDoCommand("placesCmd_new:folder");
  }
};
