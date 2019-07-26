




"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

const ENSURE_SELECTION_VISIBLE_DELAY = 50; 

Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

this.EXPORTED_SYMBOLS = ["SideMenuWidget"];























this.SideMenuWidget = function SideMenuWidget(aNode, aShowArrows = true) {
  this.document = aNode.ownerDocument;
  this.window = this.document.defaultView;
  this._parent = aNode;
  this._showArrows = aShowArrows;

  
  this._list = this.document.createElement("scrollbox");
  this._list.className = "side-menu-widget-container";
  this._list.setAttribute("flex", "1");
  this._list.setAttribute("orient", "vertical");
  this._list.setAttribute("with-arrow", aShowArrows);
  this._parent.appendChild(this._list);
  this._boxObject = this._list.boxObject.QueryInterface(Ci.nsIScrollBoxObject);

  
  this._groupsByName = new Map(); 
  this._orderedGroupElementsArray = [];
  this._orderedMenuElementsArray = [];

  
  
  ViewHelpers.delegateWidgetEventMethods(this, aNode);
};

SideMenuWidget.prototype = {
  



  maintainSelectionVisible: true,

  


  sortedGroups: true,

  














  insertItemAt: function SMW_insertItemAt(aIndex, aContents, aTooltip = "", aGroup = "") {
    
    this.removeAttribute("notice");

    if (this.maintainSelectionVisible) {
      this.ensureSelectionIsVisible({ withGroup: true, delayed: true });
    }

    let group = this._getGroupForName(aGroup);
    let item = this._getItemForGroup(group, aContents, aTooltip);
    return item.insertSelfAt(aIndex);
  },

  







  getItemAtIndex: function SMW_getItemAtIndex(aIndex) {
    return this._orderedMenuElementsArray[aIndex];
  },

  





  removeChild: function SMW_removeChild(aChild) {
    if (aChild.className == "side-menu-widget-item-contents") {
      
      aChild.parentNode.remove();
    } else {
      
      aChild.remove();
    }

    this._orderedMenuElementsArray.splice(
      this._orderedMenuElementsArray.indexOf(aChild), 1);

    if (this._selectedItem == aChild) {
      this._selectedItem = null;
    }
  },

  


  removeAllItems: function SMW_removeAllItems() {
    let parent = this._parent;
    let list = this._list;

    while (list.hasChildNodes()) {
      list.firstChild.remove();
    }

    this._selectedItem = null;

    this._groupsByName.clear();
    this._orderedGroupElementsArray.length = 0;
    this._orderedMenuElementsArray.length = 0;
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

    
    
    this.ensureSelectionIsVisible({ delayed: true });
  },

  



  ensureSelectionIsVisible: function SMW_ensureSelectionIsVisible(aFlags) {
    this.ensureElementIsVisible(this.selectedItem, aFlags);
  },

  









  ensureElementIsVisible: function SMW_ensureElementIsVisible(aElement, aFlags = {}) {
    if (!aElement) {
      return;
    }

    if (aFlags.delayed) {
      delete aFlags.delayed;
      this.window.clearTimeout(this._ensureVisibleTimeout);
      this._ensureVisibleTimeout = this.window.setTimeout(() => {
        this.ensureElementIsVisible(aElement, aFlags);
      }, ENSURE_SELECTION_VISIBLE_DELAY);
      return;
    }

    if (aFlags.withGroup) {
      let groupList = aElement.parentNode;
      let groupContainer = groupList.parentNode;
      groupContainer.scrollIntoView(true); 
    }

    this._boxObject.ensureElementIsVisible(aElement);
  },

  


  showEmptyGroups: function SMW_showEmptyGroups() {
    for (let group of this._orderedGroupElementsArray) {
      group.hidden = false;
    }
  },

  


  hideEmptyGroups: function SMW_hideEmptyGroups() {
    let visibleChildNodes = ".side-menu-widget-item-contents:not([hidden=true])";

    for (let group of this._orderedGroupElementsArray) {
      group.hidden = group.querySelectorAll(visibleChildNodes).length == 0;
    }
    for (let menuItem of this._orderedMenuElementsArray) {
      menuItem.parentNode.hidden = menuItem.hidden;
    }
  },

  







  getAttribute: function SMW_getAttribute(aName) {
    return this._parent.getAttribute(aName);
  },

  







  setAttribute: function SMW_setAttribute(aName, aValue) {
    this._parent.setAttribute(aName, aValue);

    if (aName == "notice") {
      this.notice = aValue;
    }
  },

  





  removeAttribute: function SMW_removeAttribute(aName) {
    this._parent.removeAttribute(aName);

    if (aName == "notice") {
      this._removeNotice();
    }
  },

  



  set notice(aValue) {
    if (this._noticeTextNode) {
      this._noticeTextNode.setAttribute("value", aValue);
    }
    this._noticeTextValue = aValue;
    this._appendNotice();
  },

  


  _appendNotice: function DVSL__appendNotice() {
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

  


  _removeNotice: function DVSL__removeNotice() {
    if (!this._noticeTextNode) {
      return;
    }

    this._parent.removeChild(this._noticeTextContainer);
    this._noticeTextContainer = null;
    this._noticeTextNode = null;
  },

  








  _getGroupForName: function SMW__getGroupForName(aName) {
    let cachedGroup = this._groupsByName.get(aName);
    if (cachedGroup) {
      return cachedGroup;
    }

    let group = new SideMenuGroup(this, aName);
    this._groupsByName.set(aName, group);
    group.insertSelfAt(this.sortedGroups ? group.findExpectedIndexForSelf() : -1);
    return group;
  },

  










  _getItemForGroup: function SMW__getItemForGroup(aGroup, aContents, aTooltip) {
    return new SideMenuItem(aGroup, aContents, aTooltip, this._showArrows);
  },

  window: null,
  document: null,
  _showArrows: false,
  _parent: null,
  _list: null,
  _boxObject: null,
  _selectedItem: null,
  _groupsByName: null,
  _orderedGroupElementsArray: null,
  _orderedMenuElementsArray: null,
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

  





  insertSelfAt: function SMG_insertSelfAt(aIndex) {
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

  





  findExpectedIndexForSelf: function SMG_findExpectedIndexForSelf() {
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













function SideMenuItem(aGroup, aContents, aTooltip, aArrowFlag) {
  this.document = aGroup.document;
  this.window = aGroup.window;
  this.ownerView = aGroup;

  
  if (aArrowFlag) {
    let container = this._container = this.document.createElement("hbox");
    container.className = "side-menu-widget-item";
    container.setAttribute("tooltiptext", aTooltip);

    let target = this._target = this.document.createElement("vbox");
    target.className = "side-menu-widget-item-contents";

    let arrow = this._arrow = this.document.createElement("hbox");
    arrow.className = "side-menu-widget-item-arrow";

    container.appendChild(target);
    container.appendChild(arrow);
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

  







  insertSelfAt: function SMI_insertSelfAt(aIndex) {
    let ownerList = this.ownerView._list;
    let menuArray = this._orderedMenuElementsArray;

    if (aIndex >= 0) {
      ownerList.insertBefore(this._container, ownerList.childNodes[aIndex]);
      menuArray.splice(aIndex, 0, this._target);
    } else {
      ownerList.appendChild(this._container);
      menuArray.push(this._target);
    }

    return this._target;
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
  _arrow: null
};
