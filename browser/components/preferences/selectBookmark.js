














































 
var SelectBookmarkDialog = {
  init: function SBD_init() {
    
    this.selectionChanged();
  },

  



  selectionChanged: function SBD_selectionChanged() {
    var accept = document.documentElement.getButton("accept");
    var bookmarks = document.getElementById("bookmarks");
    var disableAcceptButton = true;
    if (bookmarks.hasSelection) {
      if (!PlacesUtils.nodeIsSeparator(bookmarks.selectedNode))
        disableAcceptButton = false;
    }
    accept.disabled = disableAcceptButton;
  },

  onItemDblClick: function SBD_onItemDblClick() {
    var bookmarks = document.getElementById("bookmarks");
    if (bookmarks.hasSingleSelection && 
        PlacesUtils.nodeIsURI(bookmarks.selectedNode)) {
      



      document.documentElement.getButton("accept").click();
    }
  },

  



  accept: function SBD_accept() {
    var bookmarks = document.getElementById("bookmarks");
    NS_ASSERT(bookmarks.hasSelection,
              "Should not be able to accept dialog if there is no selected URL!");
    var urls = [];
    var names = [];
    var selectedNode = bookmarks.selectedNode;
    if (PlacesUtils.nodeIsFolder(selectedNode)) {
      var contents = PlacesUtils.getFolderContents(selectedNode.itemId);
      var cc = contents.childCount;
      for (var i = 0; i < cc; ++i) {
        var node = contents.getChild(i);
        if (PlacesUtils.nodeIsURI(node)) {
          urls.push(node.uri);
          names.push(node.title);
        }
      }
    }
    else {
      urls.push(selectedNode.uri);
      names.push(selectedNode.title);
    }
    window.arguments[0].urls = urls;
    window.arguments[0].names = names;
  }
};
