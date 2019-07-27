# -*- indent-tabs-mode: nil; js-indent-level: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

var SidebarUtils = {
  handleTreeClick: function SU_handleTreeClick(aTree, aEvent, aGutterSelect) {
    
    if (aEvent.button == 2)
      return;

    var tbo = aTree.treeBoxObject;
    var cell = tbo.getCellAt(aEvent.clientX, aEvent.clientY);

    if (cell.row == -1 || cell.childElt == "twisty")
      return;

    var mouseInGutter = false;
    if (aGutterSelect) {
      var rect = tbo.getCoordsForCellItem(cell.row, cell.col, "image");
      
      
      
      
      
      var isRTL = window.getComputedStyle(aTree, null).direction == "rtl";
      if (isRTL)
        mouseInGutter = aEvent.clientX > rect.x;
      else
        mouseInGutter = aEvent.clientX < rect.x;
    }

#ifdef XP_MACOSX
    var modifKey = aEvent.metaKey || aEvent.shiftKey;
#else
    var modifKey = aEvent.ctrlKey || aEvent.shiftKey;
#endif

    var isContainer = tbo.view.isContainer(cell.row);
    var openInTabs = isContainer &&
                     (aEvent.button == 1 ||
                      (aEvent.button == 0 && modifKey)) &&
                     PlacesUtils.hasChildURIs(tbo.view.nodeForTreeIndex(cell.row));

    if (aEvent.button == 0 && isContainer && !openInTabs) {
      tbo.view.toggleOpenState(cell.row);
      return;
    }
    else if (!mouseInGutter && openInTabs &&
            aEvent.originalTarget.localName == "treechildren") {
      tbo.view.selection.select(cell.row);
      PlacesUIUtils.openContainerNodeInTabs(aTree.selectedNode, aEvent, aTree);
    }
    else if (!mouseInGutter && !isContainer &&
             aEvent.originalTarget.localName == "treechildren") {
      
      
      
      tbo.view.selection.select(cell.row);
      PlacesUIUtils.openNodeWithEvent(aTree.selectedNode, aEvent, aTree);
    }
  },

  handleTreeKeyPress: function SU_handleTreeKeyPress(aEvent) {
    
    let tree = aEvent.target;
    let node = tree.selectedNode;
    if (node) {
      if (aEvent.keyCode == KeyEvent.DOM_VK_RETURN)
        PlacesUIUtils.openNodeWithEvent(node, aEvent, tree);
    }
  },

  



  handleTreeMouseMove: function SU_handleTreeMouseMove(aEvent) {
    if (aEvent.target.localName != "treechildren")
      return;

    var tree = aEvent.target.parentNode;
    var tbo = tree.treeBoxObject;
    var cell = tbo.getCellAt(aEvent.clientX, aEvent.clientY);

    
    
    
    if (cell.row != -1) {
      var node = tree.view.nodeForTreeIndex(cell.row);
      if (PlacesUtils.nodeIsURI(node))
        this.setMouseoverURL(node.uri);
      else
        this.setMouseoverURL("");
    }
    else
      this.setMouseoverURL("");
  },

  setMouseoverURL: function SU_setMouseoverURL(aURL) {
    
    
    
    if (top.XULBrowserWindow) {
      top.XULBrowserWindow.setOverLink(aURL, null);
    }
  }
};
