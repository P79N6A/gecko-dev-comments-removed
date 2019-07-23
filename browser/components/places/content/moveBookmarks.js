





































var gMoveBookmarksDialog = {
  _nodes: null,
  _tm: null,

  _foldersTree: null,
  get foldersTree() {
    if (!this._foldersTree)
      this._foldersTree = document.getElementById("foldersTree");

    return this._foldersTree;
  },

  init: function() {
    this._nodes = window.arguments[0];
    this._tm = window.arguments[1];

    this.foldersTree.place =
      "place:excludeItems=1&excludeQueries=1&excludeReadOnlyFolders=1&folder=" +
      PlacesUtils.allBookmarksFolderId;
  },

  onOK: function MBD_onOK(aEvent) {
    var selectedNode = this.foldersTree.selectedNode;
    NS_ASSERT(selectedNode,
              "selectedNode must be set in a single-selection tree with initial selection set");
    var selectedFolderID = selectedNode.itemId;

    var transactions = [];
    for (var i=0; i < this._nodes.length; i++) {
      
      if (this._nodes[i].parent.itemId == selectedFolderID)
        continue;

      transactions.push(new
        PlacesUtils.ptm.moveItem(this._nodes[i].itemId, selectedFolderID, -1));
    }

    if (transactions.length != 0) {
      var txn = PlacesUtils.ptm.aggregateTransactions("Move Items", transactions);
      this._tm.doTransaction(txn);
    }
  },

  newFolder: function MBD_newFolder() {
    
    this.foldersTree.focus();
    goDoCommand("placesCmd_new:folder");
  }
};
