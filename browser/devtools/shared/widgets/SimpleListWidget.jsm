




"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

this.EXPORTED_SYMBOLS = ["SimpleListWidget"];










function SimpleListWidget(aNode) {
  this.document = aNode.ownerDocument;
  this.window = this.document.defaultView;
  this._parent = aNode;

  
  this._list = this.document.createElement("scrollbox");
  this._list.className = "simple-list-widget-container theme-body";
  this._list.setAttribute("flex", "1");
  this._list.setAttribute("orient", "vertical");
  this._parent.appendChild(this._list);

  
  
  ViewHelpers.delegateWidgetAttributeMethods(this, aNode);
  ViewHelpers.delegateWidgetEventMethods(this, aNode);
}

SimpleListWidget.prototype = {
  









  insertItemAt: function(aIndex, aContents) {
    aContents.classList.add("simple-list-widget-item");

    let list = this._list;
    return list.insertBefore(aContents, list.childNodes[aIndex]);
  },

  







  getItemAtIndex: function(aIndex) {
    return this._list.childNodes[aIndex];
  },

  





  removeChild: function(aChild) {
    this._list.removeChild(aChild);

    if (this._selectedItem == aChild) {
      this._selectedItem = null;
    }
  },

  


  removeAllItems: function() {
    let list = this._list;
    let parent = this._parent;

    while (list.hasChildNodes()) {
      list.firstChild.remove();
    }

    parent.scrollTop = 0;
    parent.scrollLeft = 0;
    this._selectedItem = null;
  },

  



  get selectedItem() {
    return this._selectedItem;
  },

  



  set selectedItem(aChild) {
    let childNodes = this._list.childNodes;

    if (!aChild) {
      this._selectedItem = null;
    }
    for (let node of childNodes) {
      if (node == aChild) {
        node.classList.add("selected");
        this._selectedItem = node;
      } else {
        node.classList.remove("selected");
      }
    }
  },

  







  setAttribute: function(aName, aValue) {
    this._parent.setAttribute(aName, aValue);

    if (aName == "emptyText") {
      this._textWhenEmpty = aValue;
    } else if (aName == "headerText") {
      this._textAsHeader = aValue;
    }
  },

  





  removeAttribute: function(aName) {
    this._parent.removeAttribute(aName);

    if (aName == "emptyText") {
      this._removeEmptyText();
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

  



  set _textAsHeader(aValue) {
    if (this._headerTextNode) {
      this._headerTextNode.setAttribute("value", aValue);
    }
    this._headerTextValue = aValue;
    this._showHeaderText();
  },

  



  set _textWhenEmpty(aValue) {
    if (this._emptyTextNode) {
      this._emptyTextNode.setAttribute("value", aValue);
    }
    this._emptyTextValue = aValue;
    this._showEmptyText();
  },

  


  _showHeaderText: function() {
    if (this._headerTextNode || !this._headerTextValue) {
      return;
    }
    let label = this.document.createElement("label");
    label.className = "plain simple-list-widget-perma-text";
    label.setAttribute("value", this._headerTextValue);

    this._parent.insertBefore(label, this._list);
    this._headerTextNode = label;
  },

  


  _showEmptyText: function() {
    if (this._emptyTextNode || !this._emptyTextValue) {
      return;
    }
    let label = this.document.createElement("label");
    label.className = "plain simple-list-widget-empty-text";
    label.setAttribute("value", this._emptyTextValue);

    this._parent.appendChild(label);
    this._emptyTextNode = label;
  },

  


  _removeEmptyText: function() {
    if (!this._emptyTextNode) {
      return;
    }
    this._parent.removeChild(this._emptyTextNode);
    this._emptyTextNode = null;
  },

  window: null,
  document: null,
  _parent: null,
  _list: null,
  _selectedItem: null,
  _headerTextNode: null,
  _headerTextValue: "",
  _emptyTextNode: null,
  _emptyTextValue: ""
};
