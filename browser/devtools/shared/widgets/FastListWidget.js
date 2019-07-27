




"use strict";

const EventEmitter = require("devtools/toolkit/event-emitter");
const { Cu, Ci } = require("chrome");
const { ViewHelpers } = Cu.import("resource:///modules/devtools/ViewHelpers.jsm", {});










const FastListWidget = module.exports = function FastListWidget(aNode) {
  this.document = aNode.ownerDocument;
  this.window = this.document.defaultView;
  this._parent = aNode;
  this._fragment = this.document.createDocumentFragment();

  
  this._templateElement = this.document.createElement("hbox");

  
  this._list = this.document.createElement("scrollbox");
  this._list.className = "fast-list-widget-container theme-body";
  this._list.setAttribute("flex", "1");
  this._list.setAttribute("orient", "vertical");
  this._list.setAttribute("tabindex", "0");
  this._list.addEventListener("keypress", e => this.emit("keyPress", e), false);
  this._list.addEventListener("mousedown", e => this.emit("mousePress", e), false);
  this._parent.appendChild(this._list);

  this._orderedMenuElementsArray = [];
  this._itemsByElement = new Map();

  
  EventEmitter.decorate(this);

  
  
  ViewHelpers.delegateWidgetAttributeMethods(this, aNode);
  ViewHelpers.delegateWidgetEventMethods(this, aNode);
}

FastListWidget.prototype = {
  












  insertItemAt: function(aIndex, aContents, aAttachment={}) {
    let element = this._templateElement.cloneNode();
    element.appendChild(aContents);

    if (aIndex >= 0) {
      throw new Error("FastListWidget only supports appending items.");
    }

    this._fragment.appendChild(element);
    this._orderedMenuElementsArray.push(element);
    this._itemsByElement.set(element, this);

    return element;
  },

  




  flush: function() {
    this._list.appendChild(this._fragment);
  },

  


  removeAllItems: function() {
    let parent = this._parent;
    let list = this._list;

    while (list.hasChildNodes()) {
      list.firstChild.remove();
    }

    this._selectedItem = null;

    this._orderedMenuElementsArray.length = 0;
    this._itemsByElement.clear();
  },

  


  removeChild: function(child) {
    throw new Error("Not yet implemented");
  },

  



  get selectedItem() {
    return this._selectedItem;
  },

  



  set selectedItem(child) {
    let menuArray = this._orderedMenuElementsArray;

    if (!child) {
      this._selectedItem = null;
    }
    for (let node of menuArray) {
      if (node == child) {
        node.classList.add("selected");
        this._selectedItem = node;
      } else {
        node.classList.remove("selected");
      }
    }

    this.ensureElementIsVisible(this.selectedItem);
  },

  







  getItemAtIndex: function(index) {
    return this._orderedMenuElementsArray[index];
  },

  







  setAttribute: function(name, value) {
    this._parent.setAttribute(name, value);

    if (name == "emptyText") {
      this._textWhenEmpty = value;
    }
  },

  





  removeAttribute: function(name) {
    this._parent.removeAttribute(name);

    if (name == "emptyText") {
      this._removeEmptyText();
    }
  },

  





  ensureElementIsVisible: function(element) {
    if (!element) {
      return;
    }

    
    let boxObject = this._list.boxObject;
    boxObject.ensureElementIsVisible(element);
    boxObject.scrollBy(-this._list.clientWidth, 0);
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
    label.className = "plain fast-list-widget-empty-text";
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

  window: null,
  document: null,
  _parent: null,
  _list: null,
  _selectedItem: null,
  _orderedMenuElementsArray: null,
  _itemsByElement: null,
  _emptyTextNode: null,
  _emptyTextValue: ""
};
