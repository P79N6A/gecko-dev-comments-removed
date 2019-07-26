const EventEmitter = require("devtools/shared/event-emitter");
const { Cu, Ci } = require("chrome");
const { ViewHelpers } = Cu.import("resource:///modules/devtools/ViewHelpers.jsm", {});










const FastListWidget = module.exports = function FastListWidget(aNode) {
  this.document = aNode.ownerDocument;
  this.window = this.document.defaultView;
  this._parent = aNode;
  this._fragment = this.document.createDocumentFragment();

  
  this._templateElement = this.document.createElement("hbox");

  
  this._list = this.document.createElement("scrollbox");
  this._list.setAttribute("flex", "1");
  this._list.setAttribute("orient", "vertical");
  this._list.setAttribute("theme", "dark");
  this._list.setAttribute("tabindex", "0");
  this._list.addEventListener("keypress", e => this.emit("keyPress", e), false);
  this._list.addEventListener("mousedown", e => this.emit("mousePress", e), false);
  this._parent.appendChild(this._list);

  this._orderedMenuElementsArray = [];
  this._itemsByElement = new Map();

  
  EventEmitter.decorate(this);

  
  
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

  



  get selectedItem() this._selectedItem,

  



  set selectedItem(child) {
    let menuArray = this._orderedMenuElementsArray;

    if (!child) {
      this._selectedItem = null;
    }
    for (let node of menuArray) {
      if (node == child) {
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

  







  getItemAtIndex: function(index) {
    return this._orderedMenuElementsArray[index];
  },

  







  getAttribute: function(name) {
    return this._parent.getAttribute(name);
  },

  







  setAttribute: function(name, value) {
    this._parent.setAttribute(name, value);
  },

  





  removeAttribute: function(name) {
    this._parent.removeAttribute(name);
  },

  





  ensureElementIsVisible: function(element) {
    if (!element) {
      return;
    }

    
    let boxObject = this._list.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    boxObject.ensureElementIsVisible(element);
    boxObject.scrollBy(-element.clientWidth, 0);
  },

  window: null,
  document: null,
  _parent: null,
  _list: null,
  _selectedItem: null,
  _orderedMenuElementsArray: null,
  _itemsByElement: null
};
