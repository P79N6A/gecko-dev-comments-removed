




































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
  },

  onOK: function MBD_onOK(aEvent) {
    var selectedNode = this.foldersTree.selectedNode;
    if (!selectedNode) {
      
      
      
      
      return;
    }
    var selectedFolderID = selectedNode.itemId;

    var transactions = [];
    for (var i=0; i < this._nodes.length; i++) {
      
      if (this._nodes[i].parent.itemId == selectedFolderID)
        continue;

      transactions.push(new
        PlacesMoveItemTransaction(this._nodes[i].itemId, selectedFolderID, -1));
    }

    if (transactions.length != 0) {
      var txn = new PlacesAggregateTransaction("Move Items", transactions);
      this._tm.doTransaction(txn);
    }
  },

  newFolder: function MBD_newFolder() {
    
    this.foldersTree.focus();
    goDoCommand("placesCmd_new:folder");
  }
};
