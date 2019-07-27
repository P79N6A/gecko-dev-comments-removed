




"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
Cu.import("resource://gre/modules/devtools/event-emitter.js");

this.EXPORTED_SYMBOLS = ["SideMenuWidget"];















this.SideMenuWidget = function SideMenuWidget(aNode, aOptions={}) {
  this.document = aNode.ownerDocument;
  this.window = this.document.defaultView;
  this._parent = aNode;

  let { contextMenu, showArrows, showItemCheckboxes, showGroupCheckboxes } = aOptions;
  this._contextMenu = contextMenu || null;
  this._showArrows = showArrows || false;
  this._showItemCheckboxes = showItemCheckboxes || false;
  this._showGroupCheckboxes = showGroupCheckboxes || false;

  
  this._list = this.document.createElement("scrollbox");
  this._list.className = "side-menu-widget-container theme-sidebar";
  this._list.setAttribute("flex", "1");
  this._list.setAttribute("orient", "vertical");
  this._list.setAttribute("with-arrows", this._showArrows);
  this._list.setAttribute("with-item-checkboxes", this._showItemCheckboxes);
  this._list.setAttribute("with-group-checkboxes", this._showGroupCheckboxes);
  this._list.setAttribute("tabindex", "0");
  this._list.addEventListener("contextmenu", e => this._showContextMenu(e), false);
  this._list.addEventListener("keypress", e => this.emit("keyPress", e), false);
  this._list.addEventListener("mousedown", e => this.emit("mousePress", e), false);
  this._parent.appendChild(this._list);

  
  this._groupsByName = new Map(); 
  this._orderedGroupElementsArray = [];
  this._orderedMenuElementsArray = [];
  this._itemsByElement = new Map();

  
  EventEmitter.decorate(this);

  
  
  ViewHelpers.delegateWidgetAttributeMethods(this, aNode);
  ViewHelpers.delegateWidgetEventMethods(this, aNode);
};

SideMenuWidget.prototype = {
  


  sortedGroups: true,

  


  groupSortPredicate: function(a, b) a.localeCompare(b),

  





  autoscrollWithAppendedItems: false,

  















  insertItemAt: function(aIndex, aContents, aAttachment={}) {
    
    
    
    let maintainScrollAtBottom =
      
      this.autoscrollWithAppendedItems &&
      
      !this._selectedItem &&
      
      (aIndex < 0 || aIndex >= this._orderedMenuElementsArray.length) &&
      
      (this._list.scrollTop + this._list.clientHeight >= this._list.scrollHeight);

    let group = this._getMenuGroupForName(aAttachment.group);
    let item = this._getMenuItemForGroup(group, aContents, aAttachment);
    let element = item.insertSelfAt(aIndex);

    if (maintainScrollAtBottom) {
      this._list.scrollTop = this._list.scrollHeight;
    }

    return element;
  },

  







  getItemAtIndex: function(aIndex) {
    return this._orderedMenuElementsArray[aIndex];
  },

  





  removeChild: function(aChild) {
    this._getNodeForContents(aChild).remove();

    this._orderedMenuElementsArray.splice(
      this._orderedMenuElementsArray.indexOf(aChild), 1);

    this._itemsByElement.delete(aChild);

    if (this._selectedItem == aChild) {
      this._selectedItem = null;
    }
  },

  


  removeAllItems: function() {
    let parent = this._parent;
    let list = this._list;

    while (list.hasChildNodes()) {
      list.firstChild.remove();
    }

    this._selectedItem = null;

    this._groupsByName.clear();
    this._orderedGroupElementsArray.length = 0;
    this._orderedMenuElementsArray.length = 0;
    this._itemsByElement.clear();
  },

  



  get selectedItem() {
    return this._selectedItem;
  },

  



  set selectedItem(aChild) {
    let menuArray = this._orderedMenuElementsArray;

    if (!aChild) {
      this._selectedItem = null;
    }
    for (let node of menuArray) {
      if (node == aChild) {
        this._getNodeForContents(node).classList.add("selected");
        this._selectedItem = node;
      } else {
        this._getNodeForContents(node).classList.remove("selected");
      }
    }
  },

  





  ensureElementIsVisible: function(aElement) {
    if (!aElement) {
      return;
    }

    
    let boxObject = this._list.boxObject;
    boxObject.ensureElementIsVisible(aElement);
    boxObject.scrollBy(-this._list.clientWidth, 0);
  },

  


  showEmptyGroups: function() {
    for (let group of this._orderedGroupElementsArray) {
      group.hidden = false;
    }
  },

  


  hideEmptyGroups: function() {
    let visibleChildNodes = ".side-menu-widget-item-contents:not([hidden=true])";

    for (let group of this._orderedGroupElementsArray) {
      group.hidden = group.querySelectorAll(visibleChildNodes).length == 0;
    }
    for (let menuItem of this._orderedMenuElementsArray) {
      menuItem.parentNode.hidden = menuItem.hidden;
    }
  },

  







  setAttribute: function(aName, aValue) {
    this._parent.setAttribute(aName, aValue);

    if (aName == "emptyText") {
      this._textWhenEmpty = aValue;
    }
  },

  





  removeAttribute: function(aName) {
    this._parent.removeAttribute(aName);

    if (aName == "emptyText") {
      this._removeEmptyText();
    }
  },

  







  checkItem: function(aNode, aCheckState) {
    const widgetItem = this._itemsByElement.get(aNode);
    if (!widgetItem) {
      throw new Error("No item for " + aNode);
    }
    widgetItem.check(aCheckState);
  },

  



  set _textWhenEmpty(aValue) {
    if (this._emptyTextNode) {
      this._emptyTextNode.setAttribute("value", aValue);
    }
    this._emptyTextValue = aValue;
    this._showEmptyText();
  },

  


  _showEmptyText: function() {
    if (this._emptyTextNode || !this._emptyTextValue) {
      return;
    }
    let label = this.document.createElement("label");
    label.className = "plain side-menu-widget-empty-text";
    label.setAttribute("value", this._emptyTextValue);

    this._parent.insertBefore(label, this._list);
    this._emptyTextNode = label;
  },

  


  _removeEmptyText: function() {
    if (!this._emptyTextNode) {
      return;
    }

    this._parent.removeChild(this._emptyTextNode);
    this._emptyTextNode = null;
  },

  








  _getMenuGroupForName: function(aName) {
    let cachedGroup = this._groupsByName.get(aName);
    if (cachedGroup) {
      return cachedGroup;
    }

    let group = new SideMenuGroup(this, aName, {
      showCheckbox: this._showGroupCheckboxes
    });

    this._groupsByName.set(aName, group);
    group.insertSelfAt(this.sortedGroups ? group.findExpectedIndexForSelf(this.groupSortPredicate) : -1);

    return group;
  },

  










  _getMenuItemForGroup: function(aGroup, aContents, aAttachment) {
    return new SideMenuItem(aGroup, aContents, aAttachment, {
      showArrow: this._showArrows,
      showCheckbox: this._showItemCheckboxes
    });
  },

  










  _getNodeForContents: function(aChild) {
    if (aChild.hasAttribute("merged-item-contents")) {
      return aChild;
    } else {
      return aChild.parentNode;
    }
  },

  


  _showContextMenu: function(e) {
    if (!this._contextMenu) {
      return;
    }

    
    let node = e.originalTarget;
    while (node && node !== this._list) {
      if (node.hasAttribute("contextmenu")) {
        return;
      }
      node = node.parentNode;
    }

    this._contextMenu.openPopupAtScreen(e.screenX, e.screenY, true);
  },

  window: null,
  document: null,
  _showArrows: false,
  _showItemCheckboxes: false,
  _showGroupCheckboxes: false,
  _parent: null,
  _list: null,
  _selectedItem: null,
  _groupsByName: null,
  _orderedGroupElementsArray: null,
  _orderedMenuElementsArray: null,
  _itemsByElement: null,
  _emptyTextNode: null,
  _emptyTextValue: ""
};













function SideMenuGroup(aWidget, aName, aOptions={}) {
  this.document = aWidget.document;
  this.window = aWidget.window;
  this.ownerView = aWidget;
  this.identifier = aName;

  
  if (aName) {
    let target = this._target = this.document.createElement("vbox");
    target.className = "side-menu-widget-group";
    target.setAttribute("name", aName);

    let list = this._list = this.document.createElement("vbox");
    list.className = "side-menu-widget-group-list";

    let title = this._title = this.document.createElement("hbox");
    title.className = "side-menu-widget-group-title";

    let name = this._name = this.document.createElement("label");
    name.className = "plain name";
    name.setAttribute("value", aName);
    name.setAttribute("crop", "end");
    name.setAttribute("flex", "1");

    
    if (aOptions.showCheckbox) {
      let checkbox = this._checkbox = makeCheckbox(title, { description: aName });
      checkbox.className = "side-menu-widget-group-checkbox";
    }

    title.appendChild(name);
    target.appendChild(title);
    target.appendChild(list);
  }
  
  else {
    let target = this._target = this._list = this.document.createElement("vbox");
    target.className = "side-menu-widget-group side-menu-widget-group-list";
    target.setAttribute("merged-group-contents", "");
  }
}

SideMenuGroup.prototype = {
  get _orderedGroupElementsArray() this.ownerView._orderedGroupElementsArray,
  get _orderedMenuElementsArray() this.ownerView._orderedMenuElementsArray,
  get _itemsByElement() { return this.ownerView._itemsByElement; },

  





  insertSelfAt: function(aIndex) {
    let ownerList = this.ownerView._list;
    let groupsArray = this._orderedGroupElementsArray;

    if (aIndex >= 0) {
      ownerList.insertBefore(this._target, groupsArray[aIndex]);
      groupsArray.splice(aIndex, 0, this._target);
    } else {
      ownerList.appendChild(this._target);
      groupsArray.push(this._target);
    }
  },

  





  findExpectedIndexForSelf: function(sortPredicate) {
    let identifier = this.identifier;
    let groupsArray = this._orderedGroupElementsArray;

    for (let group of groupsArray) {
      let name = group.getAttribute("name");
      if (sortPredicate(name, identifier) > 0 && 
          !name.contains(identifier)) { 
        return groupsArray.indexOf(group);
      }
    }
    return -1;
  },

  window: null,
  document: null,
  ownerView: null,
  identifier: "",
  _target: null,
  _checkbox: null,
  _title: null,
  _name: null,
  _list: null
};















function SideMenuItem(aGroup, aContents, aAttachment={}, aOptions={}) {
  this.document = aGroup.document;
  this.window = aGroup.window;
  this.ownerView = aGroup;

  if (aOptions.showArrow || aOptions.showCheckbox) {
    let container = this._container = this.document.createElement("hbox");
    container.className = "side-menu-widget-item";

    let target = this._target = this.document.createElement("vbox");
    target.className = "side-menu-widget-item-contents";

    
    if (aOptions.showCheckbox) {
      let checkbox = this._checkbox = makeCheckbox(container, aAttachment);
      checkbox.className = "side-menu-widget-item-checkbox";
    }

    container.appendChild(target);

    
    if (aOptions.showArrow) {
      let arrow = this._arrow = this.document.createElement("hbox");
      arrow.className = "side-menu-widget-item-arrow";
      container.appendChild(arrow);
    }
  }
  
  else {
    let target = this._target = this._container = this.document.createElement("hbox");
    target.className = "side-menu-widget-item side-menu-widget-item-contents";
    target.setAttribute("merged-item-contents", "");
  }

  this._target.setAttribute("flex", "1");
  this.contents = aContents;
}

SideMenuItem.prototype = {
  get _orderedGroupElementsArray() this.ownerView._orderedGroupElementsArray,
  get _orderedMenuElementsArray() this.ownerView._orderedMenuElementsArray,
  get _itemsByElement() { return this.ownerView._itemsByElement; },

  







  insertSelfAt: function(aIndex) {
    let ownerList = this.ownerView._list;
    let menuArray = this._orderedMenuElementsArray;

    if (aIndex >= 0) {
      ownerList.insertBefore(this._container, ownerList.childNodes[aIndex]);
      menuArray.splice(aIndex, 0, this._target);
    } else {
      ownerList.appendChild(this._container);
      menuArray.push(this._target);
    }
    this._itemsByElement.set(this._target, this);

    return this._target;
  },

  





  check: function(aCheckState) {
    if (!this._checkbox) {
      throw new Error("Cannot check items that do not have checkboxes.");
    }
    
    
    this._checkbox.checked = !!aCheckState;
  },

  





  set contents(aContents) {
    
    if (this._target.hasChildNodes()) {
      this._target.replaceChild(aContents, this._target.firstChild);
      return;
    }
    
    this._target.appendChild(aContents);
  },

  window: null,
  document: null,
  ownerView: null,
  _target: null,
  _container: null,
  _checkbox: null,
  _arrow: null
};













function makeCheckbox(aParentNode, aOptions) {
  let checkbox = aParentNode.ownerDocument.createElement("checkbox");
  checkbox.setAttribute("tooltiptext", aOptions.checkboxTooltip);

  if (aOptions.checkboxState) {
    checkbox.setAttribute("checked", true);
  } else {
    checkbox.removeAttribute("checked");
  }

  
  checkbox.addEventListener("mousedown", e => {
    e.stopPropagation();
  }, false);

  
  
  checkbox.addEventListener("CheckboxStateChange", e => {
    ViewHelpers.dispatchEvent(checkbox, "check", {
      description: aOptions.description || "item",
      checked: checkbox.checked
    });
  }, false);

  aParentNode.appendChild(checkbox);
  return checkbox;
}
