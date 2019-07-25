# -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

var SidebarUtils = {
  handleTreeClick: function SU_handleTreeClick(aTree, aEvent, aGutterSelect) {
    
    if (aEvent.button == 2)
      return;

    var tbo = aTree.treeBoxObject;
    var row = { }, col = { }, obj = { };
    tbo.getCellAt(aEvent.clientX, aEvent.clientY, row, col, obj);

    if (row.value == -1 || obj.value == "twisty")
      return;

    var mouseInGutter = false;
    if (aGutterSelect) {
      var x = { }, y = { }, w = { }, h = { };
      tbo.getCoordsForCellItem(row.value, col.value, "image",
                               x, y, w, h);
      
      
      
      
      
      var isRTL = window.getComputedStyle(aTree, null).direction == "rtl";
      if (isRTL)
        mouseInGutter = aEvent.clientX > x.value;
      else
        mouseInGutter = aEvent.clientX < x.value;
    }

#ifdef XP_MACOSX
    var modifKey = aEvent.metaKey || aEvent.shiftKey;
#else
    var modifKey = aEvent.ctrlKey || aEvent.shiftKey;
#endif

    var isContainer = tbo.view.isContainer(row.value);
    var openInTabs = isContainer &&
                     (aEvent.button == 1 ||
                      (aEvent.button == 0 && modifKey)) &&
                     PlacesUtils.hasChildURIs(tbo.view.nodeForTreeIndex(row.value));

    if (aEvent.button == 0 && isContainer && !openInTabs) {
      tbo.view.toggleOpenState(row.value);
      return;
    }
    else if (!mouseInGutter && openInTabs &&
            aEvent.originalTarget.localName == "treechildren") {
      tbo.view.selection.select(row.value);
      PlacesUIUtils.openContainerNodeInTabs(aTree.selectedNode, aEvent, aTree);
    }
    else if (!mouseInGutter && !isContainer &&
             aEvent.originalTarget.localName == "treechildren") {
      
      
      
      tbo.view.selection.select(row.value);
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
    var row = { }, col = { }, obj = { };
    tbo.getCellAt(aEvent.clientX, aEvent.clientY, row, col, obj);

    
    
    
    if (row.value != -1) {
      var node = tree.view.nodeForTreeIndex(row.value);
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
