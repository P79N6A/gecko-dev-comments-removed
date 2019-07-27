




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "EventEmitter",
  "resource://gre/modules/devtools/event-emitter.js");

XPCOMUtils.defineLazyModuleGetter(this, "console",
  "resource://gre/modules/devtools/Console.jsm");

this.EXPORTED_SYMBOLS = ["AbstractTreeItem"];



































































































function AbstractTreeItem({ parent, level }) {
  this._rootItem = parent ? parent._rootItem : this;
  this._parentItem = parent;
  this._level = level || 0;
  this._childTreeItems = [];

  
  
  if (this == this._rootItem) {
    EventEmitter.decorate(this);
  }
}

AbstractTreeItem.prototype = {
  _containerNode: null,
  _targetNode: null,
  _arrowNode: null,
  _constructed: false,
  _populated: false,
  _expanded: false,

  



  autoExpandDepth: 0,

  








  _displaySelf: function(document, arrowNode) {
    throw new Error(
      "The `_displaySelf` method needs to be implemented by inheriting classes.");
  },

  







  _populateSelf: function(children) {
    throw new Error(
      "The `_populateSelf` method needs to be implemented by inheriting classes.");
  },

  



  get document() {
    return this._containerNode.ownerDocument;
  },

  



  get root() {
    return this._rootItem;
  },

  



  get parent() {
    return this._parentItem;
  },

  


  get level() {
    return this._level;
  },

  


  get target() {
    return this._targetNode;
  },

  



  get container() {
    return this._containerNode;
  },

  




  get populated() {
    return this._populated;
  },

  




  get expanded() {
    return this._expanded;
  },

  



  get bounds() {
    let win = this.document.defaultView;
    let utils = win.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    return utils.getBoundsWithoutFlushing(this._containerNode);
  },

  










  attachTo: function(containerNode, fragmentNode = containerNode, beforeNode = null) {
    this._containerNode = containerNode;
    this._constructTargetNode();

    if (beforeNode) {
      fragmentNode.insertBefore(this._targetNode, beforeNode);
    } else {
      fragmentNode.appendChild(this._targetNode);
    }

    if (this._level < this.autoExpandDepth) {
      this.expand();
    }
  },

  



  remove: function() {
    this._targetNode.remove();
    this._hideChildren();
    this._childTreeItems.length = 0;
  },

  


  focus: function() {
    this._targetNode.focus();
  },

  


  expand: function() {
    if (this._expanded) {
      return;
    }
    this._expanded = true;
    this._arrowNode.setAttribute("open", "");
    this._targetNode.setAttribute("expanded", "");
    this._toggleChildren(true);
    this._rootItem.emit("expand", this);
  },

  


  collapse: function() {
    if (!this._expanded) {
      return;
    }
    this._expanded = false;
    this._arrowNode.removeAttribute("open");
    this._targetNode.removeAttribute("expanded", "");
    this._toggleChildren(false);
    this._rootItem.emit("collapse", this);
  },

  





  getChild: function(index = 0) {
    return this._childTreeItems[index];
  },

  




  traverse: function(cb) {
    for (let child of this._childTreeItems) {
      cb(child);
      child.bfs();
    }
  },

  





  find: function(predicate) {
    for (let child of this._childTreeItems) {
      if (predicate(child) || child.find(predicate)) {
        return child;
      }
    }
    return null;
  },

  






  _toggleChildren: function(visible) {
    if (visible) {
      if (!this._populated) {
        this._populateSelf(this._childTreeItems);
        this._populated = this._childTreeItems.length > 0;
      }
      this._showChildren();
    } else {
      this._hideChildren();
    }
  },

  


  _showChildren: function() {
    
    
    if (this == this._rootItem && this.autoExpandDepth == 0) {
      this._appendChildrenBatch();
    }
    
    
    
    else {
      this._appendChildrenSuccessive();
    }
  },

  


  _hideChildren: function() {
    for (let item of this._childTreeItems) {
      item._targetNode.remove();
      item._hideChildren();
    }
  },

  



  _appendChildrenBatch: function() {
    if (this._fragment === undefined) {
      this._fragment = this.document.createDocumentFragment();
    }

    let childTreeItems = this._childTreeItems;

    for (let i = 0, len = childTreeItems.length; i < len; i++) {
      childTreeItems[i].attachTo(this._containerNode, this._fragment);
    }

    this._containerNode.appendChild(this._fragment);
  },

  


  _appendChildrenSuccessive: function() {
    let childTreeItems = this._childTreeItems;
    let expandedChildTreeItems = childTreeItems.filter(e => e._expanded);
    let nextNode = this._getSiblingAtDelta(1);

    for (let i = 0, len = childTreeItems.length; i < len; i++) {
      childTreeItems[i].attachTo(this._containerNode, undefined, nextNode);
    }
    for (let i = 0, len = expandedChildTreeItems.length; i < len; i++) {
      expandedChildTreeItems[i]._showChildren();
    }
  },

  


  _constructTargetNode: function() {
    if (this._constructed) {
      return;
    }
    this._onArrowClick = this._onArrowClick.bind(this);
    this._onClick = this._onClick.bind(this);
    this._onDoubleClick = this._onDoubleClick.bind(this);
    this._onKeyPress = this._onKeyPress.bind(this);
    this._onFocus = this._onFocus.bind(this);
    this._onBlur = this._onBlur.bind(this);

    let document = this.document;

    let arrowNode = this._arrowNode = document.createElement("hbox");
    arrowNode.className = "arrow theme-twisty";
    arrowNode.addEventListener("mousedown", this._onArrowClick);

    let targetNode = this._targetNode = this._displaySelf(document, arrowNode);
    targetNode.style.MozUserFocus = "normal";

    targetNode.addEventListener("mousedown", this._onClick);
    targetNode.addEventListener("dblclick", this._onDoubleClick);
    targetNode.addEventListener("keypress", this._onKeyPress);
    targetNode.addEventListener("focus", this._onFocus);
    targetNode.addEventListener("blur", this._onBlur);

    this._constructed = true;
  },

  








  _getSiblingAtDelta: function(delta) {
    let childNodes = this._containerNode.childNodes;
    let indexOfSelf = Array.indexOf(childNodes, this._targetNode);
    return childNodes[indexOfSelf + delta];
  },

  


  _focusNextNode: function() {
    let nextElement = this._getSiblingAtDelta(1);
    if (nextElement) nextElement.focus(); 
  },

  


  _focusPrevNode: function() {
    let prevElement = this._getSiblingAtDelta(-1);
    if (prevElement) prevElement.focus(); 
  },

  





  _focusParentNode: function() {
    let parentItem = this._parentItem;
    if (parentItem) parentItem.focus(); 
  },

  


  _onArrowClick: function(e) {
    if (!this._expanded) {
      this.expand();
    } else {
      this.collapse();
    }
  },

  


  _onClick: function(e) {
    e.preventDefault();
    e.stopPropagation();
    this.focus();
  },

  


  _onDoubleClick: function(e) {
    
    
    if (!e.target.classList.contains("arrow")) {
      this._onArrowClick(e);
    }
    this.focus();
  },

  


  _onKeyPress: function(e) {
    
    ViewHelpers.preventScrolling(e);

    switch (e.keyCode) {
      case e.DOM_VK_UP:
        this._focusPrevNode();
        return;

      case e.DOM_VK_DOWN:
        this._focusNextNode();
        return;

      case e.DOM_VK_LEFT:
        if (this._expanded && this._populated) {
          this.collapse();
        } else {
          this._focusParentNode();
        }
        return;

      case e.DOM_VK_RIGHT:
        if (!this._expanded) {
          this.expand();
        } else {
          this._focusNextNode();
        }
        return;
    }
  },

  


  _onFocus: function(e) {
    this._rootItem.emit("focus", this);
  },

  


  _onBlur: function(e) {
    this._rootItem.emit("blur", this);
  }
};
