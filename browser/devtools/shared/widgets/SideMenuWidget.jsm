




"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
Cu.import("resource:///modules/devtools/shared/event-emitter.js");

XPCOMUtils.defineLazyModuleGetter(this, "devtools",
  "resource://gre/modules/devtools/Loader.jsm");

Object.defineProperty(this, "NetworkHelper", {
  get: function() {
    return devtools.require("devtools/toolkit/webconsole/network-helper");
  },
  configurable: true,
  enumerable: true
});

this.EXPORTED_SYMBOLS = ["SideMenuWidget"];













this.SideMenuWidget = function SideMenuWidget(aNode, aOptions={}) {
  this.document = aNode.ownerDocument;
  this.window = this.document.defaultView;
  this._parent = aNode;

  let { showArrows, showCheckboxes } = aOptions;
  this._showArrows = showArrows || false;
  this._showCheckboxes = showCheckboxes || false;

  
  this._list = this.document.createElement("scrollbox");
  this._list.className = "side-menu-widget-container";
  this._list.setAttribute("flex", "1");
  this._list.setAttribute("orient", "vertical");
  this._list.setAttribute("with-arrow", showArrows);
  this._list.setAttribute("with-checkboxes", showCheckboxes);
  this._list.setAttribute("tabindex", "0");
  this._list.addEventListener("keypress", e => this.emit("keyPress", e), false);
  this._list.addEventListener("mousedown", e => this.emit("mousePress", e), false);
  this._parent.appendChild(this._list);
  this._boxObject = this._list.boxObject.QueryInterface(Ci.nsIScrollBoxObject);

  
  this._groupsByName = new Map(); 
  this._orderedGroupElementsArray = [];
  this._orderedMenuElementsArray = [];
  this._itemsByElement = new Map();

  
  EventEmitter.decorate(this);

  
  
  ViewHelpers.delegateWidgetEventMethods(this, aNode);
};

SideMenuWidget.prototype = {
  


  sortedGroups: true,

  



  maintainSelectionVisible: true,

  





  autoscrollWithAppendedItems: false,

  
















  insertItemAt: function(aIndex, aContents, aTooltip = "", aGroup = "", aAttachment={}) {
    aTooltip = NetworkHelper.convertToUnicode(unescape(aTooltip));
    aGroup = NetworkHelper.convertToUnicode(unescape(aGroup));

    
    this.removeAttribute("notice");

    
    
    
    let maintainScrollAtBottom =
      
      this.autoscrollWithAppendedItems &&
      
      !this._selectedItem &&
      
      (aIndex < 0 || aIndex >= this._orderedMenuElementsArray.length) &&
      
      (this._list.scrollTop + this._list.clientHeight >= this._list.scrollHeight);

    let group = this._getMenuGroupForName(aGroup);
    let item = this._getMenuItemForGroup(group, aContents, aTooltip, aAttachment);
    let element = item.insertSelfAt(aIndex);

    if (this.maintainSelectionVisible) {
      this.ensureElementIsVisible(this.selectedItem);
    }
    if (maintainScrollAtBottom) {
      this._list.scrollTop = this._list.scrollHeight;
    }

    return element;
  },

  







  getItemAtIndex: function(aIndex) {
    return this._orderedMenuElementsArray[aIndex];
  },

  





  removeChild: function(aChild) {
    if (aChild.className == "side-menu-widget-item-contents") {
      
      aChild.parentNode.remove();
    } else {
      
      aChild.remove();
    }

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

  



  get selectedItem() this._selectedItem,

  



  set selectedItem(aChild) {
    let menuArray = this._orderedMenuElementsArray;

    if (!aChild) {
      this._selectedItem = null;
    }
    for (let node of menuArray) {
      if (node == aChild) {
        node.classList.add("selected");
        node.parentNode.classList.add("selected");
        this._selectedItem = node;
      } else {
        node.classList.remove("selected");
        node.parentNode.classList.remove("selected");
      }
    }

    this.ensureElementIsVisible(this.selectedItem);
  },

  





  ensureElementIsVisible: function(aElement) {
    if (!aElement) {
      return;
    }

    
    this._boxObject.ensureElementIsVisible(aElement);
    this._boxObject.scrollBy(-aElement.clientWidth, 0);
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

  







  getAttribute: function(aName) {
    return this._parent.getAttribute(aName);
  },

  







  setAttribute: function(aName, aValue) {
    this._parent.setAttribute(aName, aValue);

    if (aName == "notice") {
      this.notice = aValue;
    }
  },

  





  removeAttribute: function(aName) {
    this._parent.removeAttribute(aName);

    if (aName == "notice") {
      this._removeNotice();
    }
  },

  







  checkItem: function(aNode, aCheckState) {
    const widgetItem = this._itemsByElement.get(aNode);
    if (!widgetItem) {
      throw new Error("No item for " + aNode);
    }
    widgetItem.check(aCheckState);
  },

  



  set notice(aValue) {
    if (this._noticeTextNode) {
      this._noticeTextNode.setAttribute("value", aValue);
    }
    this._noticeTextValue = aValue;
    this._appendNotice();
  },

  


  _appendNotice: function() {
    if (this._noticeTextNode || !this._noticeTextValue) {
      return;
    }

    let container = this.document.createElement("vbox");
    container.className = "side-menu-widget-empty-notice-container";
    container.setAttribute("align", "center");

    let label = this.document.createElement("label");
    label.className = "plain side-menu-widget-empty-notice";
    label.setAttribute("value", this._noticeTextValue);
    container.appendChild(label);

    this._parent.insertBefore(container, this._list);
    this._noticeTextContainer = container;
    this._noticeTextNode = label;
  },

  


  _removeNotice: function() {
    if (!this._noticeTextNode) {
      return;
    }

    this._parent.removeChild(this._noticeTextContainer);
    this._noticeTextContainer = null;
    this._noticeTextNode = null;
  },

  








  _getMenuGroupForName: function(aName) {
    let cachedGroup = this._groupsByName.get(aName);
    if (cachedGroup) {
      return cachedGroup;
    }

    let group = new SideMenuGroup(this, aName);
    this._groupsByName.set(aName, group);
    group.insertSelfAt(this.sortedGroups ? group.findExpectedIndexForSelf() : -1);
    return group;
  },

  












  _getMenuItemForGroup: function(aGroup, aContents, aTooltip, aAttachment) {
    return new SideMenuItem(aGroup, aContents, aTooltip, this._showArrows, this._showCheckboxes, aAttachment);
  },

  window: null,
  document: null,
  _showArrows: false,
  _showCheckboxes: false,
  _parent: null,
  _list: null,
  _boxObject: null,
  _selectedItem: null,
  _groupsByName: null,
  _orderedGroupElementsArray: null,
  _orderedMenuElementsArray: null,
  _itemsByElement: null,
  _ensureVisibleTimeout: null,
  _noticeTextContainer: null,
  _noticeTextNode: null,
  _noticeTextValue: ""
};










function SideMenuGroup(aWidget, aName) {
  this.document = aWidget.document;
  this.window = aWidget.window;
  this.ownerView = aWidget;
  this.identifier = aName;

  
  if (aName) {
    let target = this._target = this.document.createElement("vbox");
    target.className = "side-menu-widget-group";
    target.setAttribute("name", aName);
    target.setAttribute("tooltiptext", aName);

    let list = this._list = this.document.createElement("vbox");
    list.className = "side-menu-widget-group-list";

    let title = this._title = this.document.createElement("hbox");
    title.className = "side-menu-widget-group-title";

    let name = this._name = this.document.createElement("label");
    name.className = "plain name";
    name.setAttribute("value", aName);
    name.setAttribute("crop", "end");
    name.setAttribute("flex", "1");

    title.appendChild(name);
    target.appendChild(title);
    target.appendChild(list);
  }
  
  else {
    let target = this._target = this._list = this.document.createElement("vbox");
    target.className = "side-menu-widget-group side-menu-widget-group-list";
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

  





  findExpectedIndexForSelf: function() {
    let identifier = this.identifier;
    let groupsArray = this._orderedGroupElementsArray;

    for (let group of groupsArray) {
      let name = group.getAttribute("name");
      if (name > identifier && 
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
  _title: null,
  _name: null,
  _list: null
};

















function SideMenuItem(aGroup, aContents, aTooltip, aArrowFlag, aCheckboxFlag, aAttachment={}) {
  this.document = aGroup.document;
  this.window = aGroup.window;
  this.ownerView = aGroup;

  if (aArrowFlag || aCheckboxFlag) {
    let container = this._container = this.document.createElement("hbox");
    container.className = "side-menu-widget-item";
    container.setAttribute("tooltiptext", aTooltip);

    let target = this._target = this.document.createElement("vbox");
    target.className = "side-menu-widget-item-contents";

    
    if (aCheckboxFlag) {
      let checkbox = this._checkbox = this._makeCheckbox(aAttachment);
      checkbox.setAttribute("align", "start");
      container.appendChild(checkbox);
    }

    container.appendChild(target);

    
    if (aArrowFlag) {
      let arrow = this._arrow = this.document.createElement("hbox");
      arrow.className = "side-menu-widget-item-arrow";
      container.appendChild(arrow);
    }
  }
  
  else {
    let target = this._target = this._container = this.document.createElement("hbox");
    target.className = "side-menu-widget-item side-menu-widget-item-contents";
  }

  this._target.setAttribute("flex", "1");
  this.contents = aContents;
}

SideMenuItem.prototype = {
  get _orderedGroupElementsArray() this.ownerView._orderedGroupElementsArray,
  get _orderedMenuElementsArray() this.ownerView._orderedMenuElementsArray,
  get _itemsByElement() { return this.ownerView._itemsByElement; },

  








  _makeCheckbox: function (aAttachment) {
    let checkbox = this.document.createElement("checkbox");
    checkbox.className = "side-menu-widget-item-checkbox";
    checkbox.setAttribute("tooltiptext", aAttachment.checkboxTooltip);

    if (aAttachment.checkboxState) {
      checkbox.setAttribute("checked", true);
    } else {
      checkbox.removeAttribute("checked");
    }

    
    checkbox.addEventListener("mousedown", function (event) {
      event.stopPropagation();
    }, false);

    checkbox.addEventListener("command", function (event) {
      ViewHelpers.dispatchEvent(checkbox, "check", {
        checked: checkbox.checked,
      });
    }, false);

    return checkbox;
  },

  







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
    if (aCheckState) {
      this._checkbox.setAttribute("checked", true);
    } else {
      this._checkbox.removeAttribute("checked");
    }
  },

  





  set contents(aContents) {
    
    
    if (typeof aContents == "string") {
      let label = this.document.createElement("label");
      label.className = "side-menu-widget-item-label";
      label.setAttribute("value", aContents);
      label.setAttribute("crop", "start");
      label.setAttribute("flex", "1");
      this.contents = label;
      return;
    }
    
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
