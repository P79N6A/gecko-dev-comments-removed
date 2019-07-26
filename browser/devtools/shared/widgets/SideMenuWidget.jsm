




"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

const ENSURE_SELECTION_VISIBLE_DELAY = 50; 

Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

this.EXPORTED_SYMBOLS = ["SideMenuWidget"];





















this.SideMenuWidget = function SideMenuWidget(aNode) {
  this._parent = aNode;

  
  this._list = this.document.createElement("scrollbox");
  this._list.className = "side-menu-widget-container";
  this._list.setAttribute("flex", "1");
  this._list.setAttribute("orient", "vertical");
  this._parent.appendChild(this._list);
  this._boxObject = this._list.boxObject.QueryInterface(Ci.nsIScrollBoxObject);

  
  this._groupsByName = new Map(); 
  this._orderedGroupElementsArray = [];
  this._orderedMenuElementsArray = [];

  
  
  ViewHelpers.delegateWidgetEventMethods(this, aNode);
};

SideMenuWidget.prototype = {
  get document() this._parent.ownerDocument,
  get window() this.document.defaultView,

  


  sortedGroups: true,

  














  insertItemAt: function SMW_insertItemAt(aIndex, aContents, aTooltip = "", aGroup = "") {
    this.ensureSelectionIsVisible(true, true); 
    let group = this._getGroupForName(aGroup);
    return group.insertItemAt(aIndex, aContents, aTooltip);
  },

  







  getItemAtIndex: function SMW_getItemAtIndex(aIndex) {
    return this._orderedMenuElementsArray[aIndex];
  },

  





  removeChild: function SMW_removeChild(aChild) {
    aChild.parentNode.removeChild(aChild);
    this._orderedMenuElementsArray.splice(
      this._orderedMenuElementsArray.indexOf(aChild), 1);

    if (this._selectedItem == aChild) {
      this._selectedItem = null;
    }
  },

  


  removeAllItems: function SMW_removeAllItems() {
    let parent = this._parent;
    let list = this._list;
    let firstChild;

    while (firstChild = list.firstChild) {
      list.removeChild(firstChild);
    }
    this._selectedItem = null;

    this._groupsByName = new Map();
    this._orderedGroupElementsArray.length = 0;
    this._orderedMenuElementsArray.length = 0;
  },

  



  get selectedItem() this._selectedItem,

  



  set selectedItem(aChild) {
    let menuElementsArray = this._orderedMenuElementsArray;

    if (!aChild) {
      this._selectedItem = null;
    }
    for (let node of menuElementsArray) {
      if (node == aChild) {
        node.parentNode.classList.add("selected");
        this._selectedItem = node;
      } else {
        node.parentNode.classList.remove("selected");
      }
    }
    
    
    this.ensureSelectionIsVisible(false, true);
  },

  



  ensureSelectionIsVisible:
  function SMW_ensureSelectionIsVisible(aGroupFlag, aDelayedFlag) {
    this.ensureElementIsVisible(this.selectedItem, aGroupFlag, aDelayedFlag);
  },

  









  ensureElementIsVisible:
  function SMW_ensureElementIsVisible(aElement, aGroupFlag, aDelayedFlag) {
    if (!aElement) {
      return;
    }
    if (aDelayedFlag) {
      this.window.clearTimeout(this._ensureVisibleTimeout);
      this._ensureVisibleTimeout = this.window.setTimeout(function() {
        this.ensureElementIsVisible(aElement, aGroupFlag, false);
      }.bind(this), ENSURE_SELECTION_VISIBLE_DELAY);
      return;
    }
    if (aGroupFlag) {
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
  this.ownerView = aWidget;
  this.identifier = aName;

  let document = this.document;
  let title = this._title = document.createElement("hbox");
  title.className = "side-menu-widget-group-title";

  let name = this._name = document.createElement("label");
  name.className = "plain name";
  name.setAttribute("value", aName);
  name.setAttribute("crop", "end");
  name.setAttribute("flex", "1");

  let list = this._list = document.createElement("vbox");
  list.className = "side-menu-widget-group-list";

  let target = this._target = document.createElement("vbox");
  target.className = "side-menu-widget-group side-menu-widget-item-or-group";
  target.setAttribute("name", aName);
  target.setAttribute("tooltiptext", aName);

  title.appendChild(name);
  target.appendChild(title);
  target.appendChild(list);
}

SideMenuGroup.prototype = {
  get document() this.ownerView.document,
  get window() this.document.defaultView,
  get _groupElementsArray() this.ownerView._orderedGroupElementsArray,
  get _menuElementsArray() this.ownerView._orderedMenuElementsArray,

  











  insertItemAt: function SMG_insertItemAt(aIndex, aContents, aTooltip) {
    let list = this._list;
    let menuArray = this._menuElementsArray;
    let item = new SideMenuItem(this, aContents, aTooltip);

    if (aIndex >= 0) {
      list.insertBefore(item._container, list.childNodes[aIndex]);
      menuArray.splice(aIndex, 0, item._target);
    } else {
      list.appendChild(item._container);
      menuArray.push(item._target);
    }
    return item._target;
  },

  





  insertSelfAt: function SMG_insertSelfAt(aIndex) {
    let ownerList = this.ownerView._list;
    let groupsArray = this._groupElementsArray;

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
    let groupsArray = this._groupElementsArray;

    for (let group of groupsArray) {
      let name = group.getAttribute("name");
      if (name > identifier && 
         !name.contains(identifier)) { 
        return groupsArray.indexOf(group);
      }
    }
    return -1;
  },

  ownerView: null,
  identifier: "",
  _target: null,
  _title: null,
  _name: null,
  _list: null
};











function SideMenuItem(aGroup, aContents, aTooltip = "") {
  this.ownerView = aGroup;

  let document = this.document;
  let target = this._target = document.createElement("vbox");
  target.className = "side-menu-widget-item-contents";
  target.setAttribute("flex", "1");
  this.contents = aContents;

  let arrow = this._arrow = document.createElement("hbox");
  arrow.className = "side-menu-widget-item-arrow";

  let container = this._container = document.createElement("hbox");
  container.className = "side-menu-widget-item side-menu-widget-item-or-group";
  container.setAttribute("tooltiptext", aTooltip);
  container.appendChild(target);
  container.appendChild(arrow);
}

SideMenuItem.prototype = {
  get document() this.ownerView.document,
  get window() this.document.defaultView,

  





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

  ownerView: null,
  _target: null,
  _container: null,
  _arrow: null
};
