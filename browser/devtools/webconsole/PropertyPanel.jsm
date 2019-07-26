





"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "WebConsoleUtils",
                                  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

this.EXPORTED_SYMBOLS = ["PropertyPanel", "PropertyTreeView"];









this.PropertyTreeView = function() {
  this._rows = [];
  this._objectActors = [];
};

PropertyTreeView.prototype = {
  



  _rows: null,

  



  _treeBox: null,

  






  _objectActors: null,

  





  _localObjectActors: null,

  _releaseObject: null,
  _objectPropertiesProvider: null,

  



























  set data(aData) {
    let oldLen = this._rows.length;

    this.cleanup();

    if (!aData) {
      return;
    }

    if (aData.objectPropertiesProvider) {
      this._objectPropertiesProvider = aData.objectPropertiesProvider;
      this._releaseObject = aData.releaseObject;
      this._propertiesToRows(aData.objectProperties, 0);
      this._rows = aData.objectProperties;
    }
    else if (aData.object) {
      this._localObjectActors = Object.create(null);
      this._rows = this._inspectObject(aData.object);
    }
    else {
      throw new Error("First argument must have an objectActor or an " +
                      "object property!");
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

  









  _propertiesToRows: function PTV__propertiesToRows(aObject, aLevel)
  {
    aObject.forEach(function(aItem) {
      aItem._level = aLevel;
      aItem._open = false;
      aItem._children = null;

      if (this._releaseObject) {
        ["value", "get", "set"].forEach(function(aProp) {
          let val = aItem[aProp];
          if (val && val.actor) {
            this._objectActors.push(val.actor);
            if (typeof val.displayString == "object" &&
                val.displayString.type == "longString") {
              this._objectActors.push(val.displayString.actor);
            }
          }
        }, this);
      }
    }, this);
  },

  









  _inspectObject: function PTV__inspectObject(aObject)
  {
    this._objectPropertiesProvider = this._localPropertiesProvider.bind(this);
    let children =
      WebConsoleUtils.inspectObject(aObject, this._localObjectGrip.bind(this));
    this._propertiesToRows(children, 0);
    return children;
  },

  








  _localObjectGrip: function PTV__localObjectGrip(aObject)
  {
    let grip = WebConsoleUtils.getObjectGrip(aObject);
    grip.actor = "obj" + gSequenceId();
    this._localObjectActors[grip.actor] = aObject;
    return grip;
  },

  









  _localPropertiesProvider:
  function PTV__localPropertiesProvider(aActor, aCallback)
  {
    let object = this._localObjectActors[aActor];
    let properties =
      WebConsoleUtils.inspectObject(object, this._localObjectGrip.bind(this));
    aCallback(properties);
  },

  

  selection: null,

  get rowCount()                     { return this._rows.length; },
  setTree: function(treeBox)         { this._treeBox = treeBox;  },
  getCellText: function PTV_getCellText(idx, column)
  {
    let row = this._rows[idx];
    return row.name + ": " + WebConsoleUtils.getPropertyPanelValue(row);
  },
  getLevel: function(idx) {
    return this._rows[idx]._level;
  },
  isContainer: function(idx) {
    return typeof this._rows[idx].value == "object" && this._rows[idx].value &&
           this._rows[idx].value.inspectable;
  },
  isContainerOpen: function(idx) {
    return this._rows[idx]._open;
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
    let thisLevel = this.getLevel(idx);
    return this._rows.slice(after + 1).some(function (r) r._level == thisLevel);
  },

  toggleOpenState: function(idx)
  {
    let item = this._rows[idx];
    if (!this.isContainer(idx)) {
      return;
    }

    if (item._open) {
      this._treeBox.beginUpdateBatch();
      item._open = false;

      var thisLevel = item._level;
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
      let callback = function _onRemoteResponse(aProperties) {
        this._treeBox.beginUpdateBatch();
        if (levelUpdate) {
          this._propertiesToRows(aProperties, item._level + 1);
          item._children = aProperties;
        }

        this._rows.splice.apply(this._rows, [idx + 1, 0].concat(item._children));

        this._treeBox.rowCountChanged(idx + 1, item._children.length);
        this._treeBox.invalidateRow(idx);
        this._treeBox.endUpdateBatch();
        item._open = true;
      }.bind(this);

      if (!item._children) {
        this._objectPropertiesProvider(item.value.actor, callback);
      }
      else {
        levelUpdate = false;
        callback(item._children);
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
  getRowProperties: function(idx) { return ""; },
  getCellProperties: function(idx, column) { return ""; },
  getColumnProperties: function(column, element) { return ""; },

  setCellValue: function(row, col, value)               { },
  setCellText: function(row, col, value)                { },
  drop: function(index, orientation, dataTransfer)      { },
  canDrop: function(index, orientation, dataTransfer)   { return false; },

  


  cleanup: function PTV_cleanup()
  {
    if (this._releaseObject) {
      this._objectActors.forEach(this._releaseObject);
      delete this._objectPropertiesProvider;
      delete this._releaseObject;
    }
    if (this._localObjectActors) {
      delete this._localObjectActors;
      delete this._objectPropertiesProvider;
    }

    this._rows = [];
    this._objectActors = [];
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


















this.PropertyPanel = function PropertyPanel(aParent, aTitle, aObject, aButtons)
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


function gSequenceId()
{
  return gSequenceId.n++;
}
gSequenceId.n = 0;
