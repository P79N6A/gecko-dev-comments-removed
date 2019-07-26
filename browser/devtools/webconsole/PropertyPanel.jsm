





"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "WebConsoleUtils",
                                  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

var EXPORTED_SYMBOLS = ["PropertyPanel", "PropertyTreeView"];









var PropertyTreeView = function() {
  this._rows = [];
  this._objectCache = {};
};

PropertyTreeView.prototype = {
  



  _rows: null,

  



  _treeBox: null,

  



  _objectCache: null,

  


































  set data(aData) {
    let oldLen = this._rows.length;

    this._cleanup();

    if (!aData) {
      return;
    }

    if (aData.remoteObject) {
      this._rootCacheId = aData.rootCacheId;
      this._panelCacheId = aData.panelCacheId;
      this._remoteObjectProvider = aData.remoteObjectProvider;
      this._rows = [].concat(aData.remoteObject);
      this._updateRemoteObject(this._rows, 0);
    }
    else if (aData.object) {
      this._rows = this._inspectObject(aData.object);
    }
    else {
      throw new Error("First argument must have a .remoteObject or " +
                      "an .object property!");
    }

    if (this._treeBox) {
      this._treeBox.beginUpdateBatch();
      if (oldLen) {
        this._treeBox.rowCountChanged(0, -oldLen);
      }
      this._treeBox.rowCountChanged(0, this._rows.length);
      this._treeBox.endUpdateBatch();
    }
  },

  









  _updateRemoteObject: function PTV__updateRemoteObject(aObject, aLevel)
  {
    aObject.forEach(function(aElement) {
      aElement.level = aLevel;
      aElement.isOpened = false;
      aElement.children = null;
    });
  },

  






  _inspectObject: function PTV__inspectObject(aObject)
  {
    this._objectCache = {};
    this._remoteObjectProvider = this._localObjectProvider.bind(this);
    let children = WebConsoleUtils.namesAndValuesOf(aObject, this._objectCache);
    this._updateRemoteObject(children, 0);
    return children;
  },

  














  _localObjectProvider:
  function PTV__localObjectProvider(aFromCacheId, aObjectId, aDestCacheId,
                                    aCallback)
  {
    let object = WebConsoleUtils.namesAndValuesOf(this._objectCache[aObjectId],
                                                  this._objectCache);
    aCallback({cacheId: aFromCacheId,
               objectId: aObjectId,
               object: object,
               childrenCacheId: aDestCacheId || aFromCacheId,
    });
  },

  

  selection: null,

  get rowCount()                     { return this._rows.length; },
  setTree: function(treeBox)         { this._treeBox = treeBox;  },
  getCellText: function(idx, column) {
    let row = this._rows[idx];
    return row.name + ": " + row.value;
  },
  getLevel: function(idx) {
    return this._rows[idx].level;
  },
  isContainer: function(idx) {
    return !!this._rows[idx].inspectable;
  },
  isContainerOpen: function(idx) {
    return this._rows[idx].isOpened;
  },
  isContainerEmpty: function(idx)    { return false; },
  isSeparator: function(idx)         { return false; },
  isSorted: function()               { return false; },
  isEditable: function(idx, column)  { return false; },
  isSelectable: function(row, col)   { return true; },

  getParentIndex: function(idx)
  {
    if (this.getLevel(idx) == 0) {
      return -1;
    }
    for (var t = idx - 1; t >= 0; t--) {
      if (this.isContainer(t)) {
        return t;
      }
    }
    return -1;
  },

  hasNextSibling: function(idx, after)
  {
    var thisLevel = this.getLevel(idx);
    return this._rows.slice(after + 1).some(function (r) r.level == thisLevel);
  },

  toggleOpenState: function(idx)
  {
    let item = this._rows[idx];
    if (!item.inspectable) {
      return;
    }

    if (item.isOpened) {
      this._treeBox.beginUpdateBatch();
      item.isOpened = false;

      var thisLevel = item.level;
      var t = idx + 1, deleteCount = 0;
      while (t < this._rows.length && this.getLevel(t++) > thisLevel) {
        deleteCount++;
      }

      if (deleteCount) {
        this._rows.splice(idx + 1, deleteCount);
        this._treeBox.rowCountChanged(idx + 1, -deleteCount);
      }
      this._treeBox.invalidateRow(idx);
      this._treeBox.endUpdateBatch();
    }
    else {
      let levelUpdate = true;
      let callback = function _onRemoteResponse(aResponse) {
        this._treeBox.beginUpdateBatch();
        item.isOpened = true;

        if (levelUpdate) {
          this._updateRemoteObject(aResponse.object, item.level + 1);
          item.children = aResponse.object;
        }

        this._rows.splice.apply(this._rows, [idx + 1, 0].concat(item.children));

        this._treeBox.rowCountChanged(idx + 1, item.children.length);
        this._treeBox.invalidateRow(idx);
        this._treeBox.endUpdateBatch();
      }.bind(this);

      if (!item.children) {
        let fromCacheId = item.level > 0 ? this._panelCacheId :
                                           this._rootCacheId;
        this._remoteObjectProvider(fromCacheId, item.objectId,
                                   this._panelCacheId, callback);
      }
      else {
        levelUpdate = false;
        callback({object: item.children});
      }
    }
  },

  getImageSrc: function(idx, column) { },
  getProgressMode : function(idx,column) { },
  getCellValue: function(idx, column) { },
  cycleHeader: function(col, elem) { },
  selectionChanged: function() { },
  cycleCell: function(idx, column) { },
  performAction: function(action) { },
  performActionOnCell: function(action, index, column) { },
  performActionOnRow: function(action, row) { },
  getRowProperties: function(idx, column, prop) { },
  getCellProperties: function(idx, column, prop) { },
  getColumnProperties: function(column, element, prop) { },

  setCellValue: function(row, col, value)               { },
  setCellText: function(row, col, value)                { },
  drop: function(index, orientation, dataTransfer)      { },
  canDrop: function(index, orientation, dataTransfer)   { return false; },

  _cleanup: function PTV__cleanup()
  {
    if (this._rows.length) {
      
      this._updateRemoteObject(this._rows, 0);
      this._rows = [];
    }

    delete this._objectCache;
    delete this._rootCacheId;
    delete this._panelCacheId;
    delete this._remoteObjectProvider;
  },
};
















function createElement(aDocument, aTag, aAttributes)
{
  let node = aDocument.createElement(aTag);
  for (var attr in aAttributes) {
    node.setAttribute(attr, aAttributes[attr]);
  }
  return node;
}














function appendChild(aDocument, aParent, aTag, aAttributes)
{
  let node = createElement(aDocument, aTag, aAttributes);
  aParent.appendChild(node);
  return node;
}


















function PropertyPanel(aParent, aTitle, aObject, aButtons)
{
  let document = aParent.ownerDocument;

  
  this.panel = createElement(document, "panel", {
    label: aTitle,
    titlebar: "normal",
    noautofocus: "true",
    noautohide: "true",
    close: "true",
  });

  
  let tree = this.tree = createElement(document, "tree", {
    flex: 1,
    hidecolumnpicker: "true"
  });

  let treecols = document.createElement("treecols");
  appendChild(document, treecols, "treecol", {
    primary: "true",
    flex: 1,
    hideheader: "true",
    ignoreincolumnpicker: "true"
  });
  tree.appendChild(treecols);

  tree.appendChild(document.createElement("treechildren"));
  this.panel.appendChild(tree);

  
  let footer = createElement(document, "hbox", { align: "end" });
  appendChild(document, footer, "spacer", { flex: 1 });

  
  let self = this;
  if (aButtons) {
    aButtons.forEach(function(button) {
      let buttonNode = appendChild(document, footer, "button", {
        label: button.label,
        accesskey: button.accesskey || "",
        class: button.class || "",
      });
      buttonNode.addEventListener("command", button.oncommand, false);
    });
  }

  appendChild(document, footer, "resizer", { dir: "bottomend" });
  this.panel.appendChild(footer);

  aParent.appendChild(this.panel);

  
  this.treeView = new PropertyTreeView();
  this.treeView.data = aObject;

  
  
  this.panel.addEventListener("popupshown", function onPopupShow()
  {
    self.panel.removeEventListener("popupshown", onPopupShow, false);
    self.tree.view = self.treeView;
  }, false);

  this.panel.addEventListener("popuphidden", function onPopupHide()
  {
    self.panel.removeEventListener("popuphidden", onPopupHide, false);
    self.destroy();
  }, false);
}





PropertyPanel.prototype.destroy = function PP_destroy()
{
  this.treeView.data = null;
  this.panel.parentNode.removeChild(this.panel);
  this.treeView = null;
  this.panel = null;
  this.tree = null;
}

