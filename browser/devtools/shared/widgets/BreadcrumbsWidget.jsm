




"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

const ENSURE_SELECTION_VISIBLE_DELAY = 50; 

Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
Cu.import("resource://gre/modules/devtools/event-emitter.js");

this.EXPORTED_SYMBOLS = ["BreadcrumbsWidget"];












this.BreadcrumbsWidget = function BreadcrumbsWidget(aNode, aOptions={}) {
  this.document = aNode.ownerDocument;
  this.window = this.document.defaultView;
  this._parent = aNode;

  
  this._list = this.document.createElement("arrowscrollbox");
  this._list.className = "breadcrumbs-widget-container";
  this._list.setAttribute("flex", "1");
  this._list.setAttribute("orient", "horizontal");
  this._list.setAttribute("clicktoscroll", "true");
  this._list.setAttribute("smoothscroll", !!aOptions.smoothScroll);
  this._list.addEventListener("keypress", e => this.emit("keyPress", e), false);
  this._list.addEventListener("mousedown", e => this.emit("mousePress", e), false);
  this._parent.appendChild(this._list);

  
  
  this._list._scrollButtonUp.collapsed = true;
  this._list._scrollButtonDown.collapsed = true;
  this._list.addEventListener("underflow", this._onUnderflow.bind(this), false);
  this._list.addEventListener("overflow", this._onOverflow.bind(this), false);

  
  
  this._separators = this.document.createElement("box");
  this._separators.className = "breadcrumb-separator-container";
  this._separators.innerHTML =
                    "<box id='breadcrumb-separator-before'></box>" +
                    "<box id='breadcrumb-separator-after'></box>" +
                    "<box id='breadcrumb-separator-normal'></box>";
  this._parent.appendChild(this._separators);

  
  EventEmitter.decorate(this);

  
  
  ViewHelpers.delegateWidgetAttributeMethods(this, aNode);
  ViewHelpers.delegateWidgetEventMethods(this, aNode);
};

BreadcrumbsWidget.prototype = {
  









  insertItemAt: function(aIndex, aContents) {
    let list = this._list;
    let breadcrumb = new Breadcrumb(this, aContents);
    return list.insertBefore(breadcrumb._target, list.childNodes[aIndex]);
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

    while (list.hasChildNodes()) {
      list.firstChild.remove();
    }

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
        node.setAttribute("checked", "");
        this._selectedItem = node;
      } else {
        node.removeAttribute("checked");
      }
    }
  },

  







  getAttribute: function(aName) {
    if (aName == "scrollPosition") return this._list.scrollPosition;
    if (aName == "scrollWidth") return this._list.scrollWidth;
    return this._parent.getAttribute(aName);
  },

  





  ensureElementIsVisible: function(aElement) {
    if (!aElement) {
      return;
    }

    
    
    setNamedTimeout("breadcrumb-select", ENSURE_SELECTION_VISIBLE_DELAY, () => {
      if (this._list.ensureElementIsVisible) {
        this._list.ensureElementIsVisible(aElement);
      }
    });
  },

  


  _onUnderflow: function({ target }) {
    if (target != this._list) {
      return;
    }
    target._scrollButtonUp.collapsed = true;
    target._scrollButtonDown.collapsed = true;
    target.removeAttribute("overflows");
  },

  


  _onOverflow: function({ target }) {
    if (target != this._list) {
      return;
    }
    target._scrollButtonUp.collapsed = false;
    target._scrollButtonDown.collapsed = false;
    target.setAttribute("overflows", "");
  },

  window: null,
  document: null,
  _parent: null,
  _list: null,
  _selectedItem: null
};









function Breadcrumb(aWidget, aContents) {
  this.document = aWidget.document;
  this.window = aWidget.window;
  this.ownerView = aWidget;

  this._target = this.document.createElement("hbox");
  this._target.className = "breadcrumbs-widget-item";
  this._target.setAttribute("align", "center");
  this.contents = aContents;
}

Breadcrumb.prototype = {
  





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
  _target: null
};
