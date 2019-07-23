




































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
    var selectedFolderID = asFolder(selectedNode).folderId;

    var transactions = [];
    for (var i=0; i < this._nodes.length; i++) {
      var parentId = asFolder(this._nodes[i].parent).folderId;

      
      if (parentId == selectedFolderID)
        continue;

      var nodeIndex = PlacesUtils.getIndexOfNode(this._nodes[i]);
      if (PlacesUtils.nodeIsFolder(this._nodes[i])) {
        
        if (asFolder(this._nodes[i]).folderId != selectedFolderID) {
          transactions.push(new
            PlacesMoveFolderTransaction(asFolder(this._nodes[i]).folderId,
                                        parentId, nodeIndex,
                                        selectedFolderID, -1));
        }
      }
      else if (PlacesUtils.nodeIsBookmark(this._nodes[i])) {
        transactions.push(new
          PlacesMoveItemTransaction(this._nodes[i].bookmarkId,
                                    PlacesUtils._uri(this._nodes[i].uri),
                                    parentId, nodeIndex, selectedFolderID, -1));
      }
      else if (PlacesUtils.nodeIsSeparator(this._nodes[i])) { 
        
        var removeTxn =
          new PlacesRemoveSeparatorTransaction(parentId, nodeIndex);
        var createTxn =
          new PlacesCreateSeparatorTransaction(selectedFolderID, -1);
        transactions.push(new
          PlacesAggregateTransaction("SeparatorMove", [removeTxn, createTxn]));
      }
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
