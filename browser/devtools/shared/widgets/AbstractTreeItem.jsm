




"use strict";

const Cu = Components.utils;

Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
Cu.import("resource://gre/modules/devtools/event-emitter.js");

this.EXPORTED_SYMBOLS = ["AbstractTreeItem"];



































































































function AbstractTreeItem({ parent, level }) {
  this._rootItem = parent ? parent._rootItem : this;
  this._parentItem = parent;
  this._level = level || 0;
  this._childTreeItems = [];
  this._onArrowClick = this._onArrowClick.bind(this);
  this._onClick = this._onClick.bind(this);
  this._onDoubleClick = this._onDoubleClick.bind(this);
  this._onKeyPress = this._onKeyPress.bind(this);
  this._onFocus = this._onFocus.bind(this);

  EventEmitter.decorate(this);
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
    throw "This method needs to be implemented by inheriting classes.";
  },

  







  _populateSelf: function(children) {
    throw "This method needs to be implemented by inheriting classes.";
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

  







  attachTo: function(containerNode, beforeNode = null) {
    this._containerNode = containerNode;
    this._constructTargetNode();
    containerNode.insertBefore(this._targetNode, beforeNode);

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
    this._toggleChildren(true);
    this._rootItem.emit("expand", this);
  },

  


  collapse: function() {
    if (!this._expanded) {
      return;
    }
    this._expanded = false;
    this._arrowNode.removeAttribute("open");
    this._toggleChildren(false);
    this._rootItem.emit("collapse", this);
  },

  





  getChild: function(index = 0) {
    return this._childTreeItems[index];
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
    let childTreeItems = this._childTreeItems;
    let expandedChildTreeItems = childTreeItems.filter(e => e._expanded);
    let nextNode = this._getSiblingAtDelta(1);

    
    
    for (let item of childTreeItems) {
      item.attachTo(this._containerNode, nextNode);
    }
    for (let item of expandedChildTreeItems) {
      item._showChildren();
    }
  },

  


  _hideChildren: function() {
    for (let item of this._childTreeItems) {
      item._targetNode.remove();
      item._hideChildren();
    }
  },

  


  _constructTargetNode: function() {
    if (this._constructed) {
      return;
    }
    let document = this._containerNode.ownerDocument;

    let arrowNode = this._arrowNode = document.createElement("hbox");
    arrowNode.className = "arrow theme-twisty";
    arrowNode.addEventListener("mousedown", this._onArrowClick);

    let targetNode = this._targetNode = this._displaySelf(document, arrowNode);
    targetNode.style.MozUserFocus = "normal";

    targetNode.addEventListener("mousedown", this._onClick);
    targetNode.addEventListener("dblclick", this._onDoubleClick);
    targetNode.addEventListener("keypress", this._onKeyPress);
    targetNode.addEventListener("focus", this._onFocus);

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
  }
};
