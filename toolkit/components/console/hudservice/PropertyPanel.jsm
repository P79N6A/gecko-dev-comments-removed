






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

var EXPORTED_SYMBOLS = ["PropertyPanel", "PropertyTreeView", "namesAndValuesOf"];




const TYPE_OBJECT = 0, TYPE_FUNCTION = 1, TYPE_ARRAY = 2, TYPE_OTHER = 3;













function presentableValueFor(aObject)
{
  if (aObject === null || aObject === undefined) {
    return {
      type: TYPE_OTHER,
      display: aObject === undefined ? "undefined" : "null"
    };
  }

  let presentable;
  switch (aObject.constructor && aObject.constructor.name) {
    case "Array":
      return {
        type: TYPE_ARRAY,
        display: "Array"
      };

    case "String":
      return {
        type: TYPE_OTHER,
        display: "\"" + aObject + "\""
      };

    case "Date":
    case "RegExp":
    case "Number":
    case "Boolean":
      return {
        type: TYPE_OTHER,
        display: aObject
      };

    case "Iterator":
      return {
        type: TYPE_OTHER,
        display: "Iterator"
      };

    case "Function":
      presentable = aObject.toString();
      return {
        type: TYPE_FUNCTION,
        display: presentable.substring(0, presentable.indexOf(')') + 1)
      };

    default:
      presentable = aObject.toString();
      let m = /^\[object (\S+)\]/.exec(presentable);

      if (typeof aObject == "object" && typeof aObject.next == "function" &&
          m && m[1] == "Generator") {
        return {
          type: TYPE_OTHER,
          display: m[1]
        };
      }

      if (typeof aObject == "object" && typeof aObject.__iterator__ == "function") {
        return {
          type: TYPE_OTHER,
          display: "Iterator"
        };
      }

      return {
        type: TYPE_OBJECT,
        display: m ? m[1] : "Object"
      };
  }
}










function isNativeFunction(aFunction)
{
  return typeof aFunction == "function" && !("prototype" in aFunction);
}









function namesAndValuesOf(aObject)
{
  let pairs = [];
  let value, presentable, getter;

  let isDOMDocument = aObject instanceof Ci.nsIDOMDocument;

  for (var propName in aObject) {
    
    if (isDOMDocument && (propName == "width" || propName == "height")) {
      continue;
    }

    
    
    getter = aObject.__lookupGetter__(propName);
    if (getter && !isNativeFunction(getter)) {
      value = ""; 
      presentable = {type: TYPE_OTHER, display: "Getter"};
    }
    else {
      try {
        value = aObject[propName];
        presentable = presentableValueFor(value);
      }
      catch (ex) {
        continue;
      }
    }

    let pair = {};
    pair.name = propName;
    pair.display = propName + ": " + presentable.display;
    pair.type = presentable.type;
    pair.value = value;

    
    pair.nameNumber = parseFloat(pair.name)
    if (isNaN(pair.nameNumber)) {
      pair.nameNumber = false;
    }

    pairs.push(pair);
  }

  pairs.sort(function(a, b)
  {
    
    if (a.nameNumber !== false && b.nameNumber === false) {
      return -1;
    }
    else if (a.nameNumber === false && b.nameNumber !== false) {
      return 1;
    }
    else if (a.nameNumber !== false && b.nameNumber !== false) {
      return a.nameNumber - b.nameNumber;
    }
    
    else if (a.name < b.name) {
      return -1;
    }
    else if (a.name > b.name) {
      return 1;
    }
    else {
      return 0;
    }
  });

  return pairs;
}










var PropertyTreeView = function() {
  this._rows = [];
};

PropertyTreeView.prototype = {

  


  _rows: null,

  


  _treeBox: null,

  






  set data(aObject) {
    let oldLen = this._rows.length;
    this._rows = this.getChildItems(aObject, true);
    if (this._treeBox) {
      this._treeBox.beginUpdateBatch();
      if (oldLen) {
        this._treeBox.rowCountChanged(0, -oldLen);
      }
      this._treeBox.rowCountChanged(0, this._rows.length);
      this._treeBox.endUpdateBatch();
    }
  },

  











  getChildItems: function(aItem, aRootElement)
  {
    
    
    
    
    
    
    
    
    
    if (!aRootElement && aItem && aItem.children instanceof Array) {
      return aItem.children;
    }

    let pairs;
    let newPairLevel;

    if (!aRootElement) {
      newPairLevel = aItem.level + 1;
      aItem = aItem.value;
    }
    else {
      newPairLevel = 0;
    }

    pairs = namesAndValuesOf(aItem);

    for each (var pair in pairs) {
      pair.level = newPairLevel;
      pair.isOpened = false;
      pair.children = pair.type == TYPE_OBJECT || pair.type == TYPE_FUNCTION ||
                      pair.type == TYPE_ARRAY;
    }

    return pairs;
  },

  

  selection: null,

  get rowCount()                     { return this._rows.length; },
  setTree: function(treeBox)         { this._treeBox = treeBox;  },
  getCellText: function(idx, column) { return this._rows[idx].display; },
  getLevel: function(idx)            { return this._rows[idx].level; },
  isContainer: function(idx)         { return !!this._rows[idx].children; },
  isContainerOpen: function(idx)     { return this._rows[idx].isOpened; },
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
    for (var t = idx - 1; t >= 0 ; t--) {
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
    var item = this._rows[idx];
    if (!item.children) {
      return;
    }

    this._treeBox.beginUpdateBatch();
    if (item.isOpened) {
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
    }
    else {
      item.isOpened = true;

      var toInsert = this.getChildItems(item);
      item.children = toInsert;
      this._rows.splice.apply(this._rows, [idx + 1, 0].concat(toInsert));

      this._treeBox.rowCountChanged(idx + 1, toInsert.length);
    }
    this._treeBox.invalidateRow(idx);
    this._treeBox.endUpdateBatch();
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
  canDrop: function(index, orientation, dataTransfer)   { return false; }
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


















function PropertyPanel(aParent, aDocument, aTitle, aObject, aButtons)
{
  
  this.panel = createElement(aDocument, "panel", {
    label: aTitle,
    titlebar: "normal",
    noautofocus: "true",
    noautohide: "true",
    close: "true",
  });

  
  let tree = this.tree = createElement(aDocument, "tree", {
    flex: 1,
    hidecolumnpicker: "true"
  });

  let treecols = aDocument.createElement("treecols");
  appendChild(aDocument, treecols, "treecol", {
    primary: "true",
    flex: 1,
    hideheader: "true",
    ignoreincolumnpicker: "true"
  });
  tree.appendChild(treecols);

  tree.appendChild(aDocument.createElement("treechildren"));
  this.panel.appendChild(tree);

  
  let footer = createElement(aDocument, "hbox", { align: "end" });
  appendChild(aDocument, footer, "spacer", { flex: 1 });

  
  let self = this;
  if (aButtons) {
    aButtons.forEach(function(button) {
      let buttonNode = appendChild(aDocument, footer, "button", {
        label: button.label,
        accesskey: button.accesskey || "",
        class: button.class || "",
      });
      buttonNode.addEventListener("command", button.oncommand, false);
    });
  }

  appendChild(aDocument, footer, "resizer", { dir: "bottomend" });
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
  this.panel.parentNode.removeChild(this.panel);
  this.treeView = null;
  this.panel = null;
  this.tree = null;

  if (this.linkNode) {
    this.linkNode._panelOpen = false;
    this.linkNode = null;
  }
}
