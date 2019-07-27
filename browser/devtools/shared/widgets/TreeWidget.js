




"use strict";

const Services = require("Services")
const HTML_NS = "http://www.w3.org/1999/xhtml";

const EventEmitter = require("devtools/toolkit/event-emitter");












function TreeWidget(node, options={}) {
  EventEmitter.decorate(this);

  this.document = node.ownerDocument;
  this.window = this.document.defaultView;
  this._parent = node;

  this.emptyText = options.emptyText || "";
  this.defaultType = options.defaultType;
  this.sorted = options.sorted !== false;

  this.setupRoot();

  this.placeholder = this.document.createElementNS(HTML_NS, "label");
  this.placeholder.className = "tree-widget-empty-text";
  this._parent.appendChild(this.placeholder);

  if (this.emptyText) {
    this.setPlaceholderText(this.emptyText);
  }
  
  this.attachments = new Map();
};

TreeWidget.prototype = {

  _selectedLabel: null,
  _selectedItem: null,

  





  set selectedItem(id) {
    if (this._selectedLabel) {
      this._selectedLabel.classList.remove("theme-selected");
    }
    let currentSelected = this._selectedLabel;
    if (id == -1) {
      this._selectedLabel = this._selectedItem = null;
      return;
    }
    if (!Array.isArray(id)) {
      return;
    }
    this._selectedLabel = this.root.setSelectedItem(id);
    if (!this._selectedLabel) {
      this._selectedItem = null;
    } else {
      if (currentSelected != this._selectedLabel) {
        this.ensureSelectedVisible();
      }
      this._selectedItem =
      JSON.parse(this._selectedLabel.parentNode.getAttribute("data-id"));
    }
  },

  





  get selectedItem() {
    return this._selectedItem;
  },

  





  isSelected: function(item) {
    if (!this._selectedItem || this._selectedItem.length != item.length) {
      return false;
    }

    for (let i = 0; i < this._selectedItem.length; i++) {
      if (this._selectedItem[i] != item[i]) {
        return false;
      }
    }

    return true;
  },

  destroy: function() {
    this.root.remove();
    this.root = null;
  },

  


  setupRoot: function() {
    this.root = new TreeItem(this.document);
    this._parent.appendChild(this.root.children);

    this.root.children.addEventListener("click", e => this.onClick(e));
    this.root.children.addEventListener("keypress", e => this.onKeypress(e));
  },

  


  setPlaceholderText: function(text) {
    this.placeholder.textContent = text;
  },

  





  selectItem: function(id) {
    this.selectedItem = id;
  },

  


  selectNextItem: function() {
    let next = this.getNextVisibleItem();
    if (next) {
      this.selectedItem = next;
    }
  },

  


  selectPreviousItem: function() {
    let prev = this.getPreviousVisibleItem();
    if (prev) {
      this.selectedItem = prev;
    }
  },

  


  getNextVisibleItem: function() {
    let node = this._selectedLabel;
    if (node.hasAttribute("expanded") && node.nextSibling.firstChild) {
      return JSON.parse(node.nextSibling.firstChild.getAttribute("data-id"));
    }
    node = node.parentNode;
    if (node.nextSibling) {
      return JSON.parse(node.nextSibling.getAttribute("data-id"));
    }
    node = node.parentNode;
    while (node.parentNode && node != this.root.children) {
      if (node.parentNode && node.parentNode.nextSibling) {
        return JSON.parse(node.parentNode.nextSibling.getAttribute("data-id"));
      }
      node = node.parentNode;
    }
    return null;
  },

  


  getPreviousVisibleItem: function() {
    let node = this._selectedLabel.parentNode;
    if (node.previousSibling) {
      node = node.previousSibling.firstChild;
      while (node.hasAttribute("expanded") && !node.hasAttribute("empty")) {
        if (!node.nextSibling.lastChild) {
          break;
        }
        node = node.nextSibling.lastChild.firstChild;
      }
      return JSON.parse(node.parentNode.getAttribute("data-id"));
    }
    node = node.parentNode;
    if (node.parentNode && node != this.root.children) {
      node = node.parentNode;
      while (node.hasAttribute("expanded") && !node.hasAttribute("empty")) {
        if (!node.nextSibling.firstChild) {
          break;
        }
        node = node.nextSibling.firstChild.firstChild;
      }
      return JSON.parse(node.getAttribute("data-id"));
    }
    return null;
  },

  clearSelection: function() {
    this.selectedItem = -1;
  },

  



























  add: function(items) {
    this.root.add(items, this.defaultType, this.sorted);
    for (let i = 0; i < items.length; i++) {
      if (items[i].attachment) {
        this.attachments.set(JSON.stringify(
          items.slice(0, i + 1).map(item => item.id || item)
        ), items[i].attachment);
      }
    }
    
    this.setPlaceholderText("");
  },

  





  remove: function(item) {
    this.root.remove(item)
    this.attachments.delete(JSON.stringify(item));
    
    if (this.root.items.size == 0 && this.emptyText) {
      this.setPlaceholderText(this.emptyText);
    }
  },

  


  clear: function() {
    this.root.remove();
    this.setupRoot();
    this.attachments.clear();
    if (this.emptyText) {
      this.setPlaceholderText(this.emptyText);
    }
  },

  


  expandAll: function() {
    this.root.expandAll();
  },

  


  collapseAll: function() {
    this.root.collapseAll();
  },

  


  onClick: function(event) {
    let target = event.originalTarget;
    while (target && !target.classList.contains("tree-widget-item")) {
      if (target == this.root.children) {
        return;
      }
      target = target.parentNode;
    }
    if (!target) {
      return;
    }
    if (target.hasAttribute("expanded")) {
      target.removeAttribute("expanded");
    } else {
      target.setAttribute("expanded", "true");
    }
    if (this._selectedLabel) {
      this._selectedLabel.classList.remove("theme-selected");
    }
    if (this._selectedLabel != target) {
      let ids = target.parentNode.getAttribute("data-id");
      this._selectedItem = JSON.parse(ids);
      this.emit("select", this._selectedItem, this.attachments.get(ids));
      this._selectedLabel = target;
    }
    target.classList.add("theme-selected");
  },

  



  onKeypress: function(event) {
    let currentSelected = this._selectedLabel;
    switch(event.keyCode) {
      case event.DOM_VK_UP:
        this.selectPreviousItem();
        break;

      case event.DOM_VK_DOWN:
        this.selectNextItem();
        break;

      case event.DOM_VK_RIGHT:
        if (this._selectedLabel.hasAttribute("expanded")) {
          this.selectNextItem();
        } else {
          this._selectedLabel.setAttribute("expanded", "true");
        }
        break;

      case event.DOM_VK_LEFT:
        if (this._selectedLabel.hasAttribute("expanded") &&
            !this._selectedLabel.hasAttribute("empty")) {
          this._selectedLabel.removeAttribute("expanded");
        } else {
          this.selectPreviousItem();
        }
        break;

      default: return;
    }
    event.preventDefault();
    if (this._selectedLabel != currentSelected) {
      let ids = JSON.stringify(this._selectedItem);
      this.emit("select", this._selectedItem, this.attachments.get(ids));
      this.ensureSelectedVisible();
    }
  },

  



  ensureSelectedVisible: function() {
    let {top, bottom} = this._selectedLabel.getBoundingClientRect();
    let height = this.root.children.parentNode.clientHeight;
    if (top < 0) {
      this._selectedLabel.scrollIntoView();
    } else if (bottom > height) {
      this._selectedLabel.scrollIntoView(false);
    }
  }
};

module.exports.TreeWidget = TreeWidget;














function TreeItem(document, parent, label, type) {
  this.document = document
  this.node = this.document.createElementNS(HTML_NS, "li");
  this.node.setAttribute("tabindex", "0");
  this.isRoot = !parent;
  this.parent = parent;
  if (this.parent) {
    this.level = this.parent.level + 1;
  }
  if (!!label) {
    this.label = this.document.createElementNS(HTML_NS, "div");
    this.label.setAttribute("empty", "true");
    this.label.setAttribute("level", this.level);
    this.label.className = "tree-widget-item";
    if (type) {
      this.label.setAttribute("type", type);
    }
    if (typeof label == "string") {
      this.label.textContent = label
    } else {
      this.label.appendChild(label);
    }
    this.node.appendChild(this.label);
  }
  this.children = this.document.createElementNS(HTML_NS, "ul");
  if (this.isRoot) {
    this.children.className = "tree-widget-container";
  } else {
    this.children.className = "tree-widget-children";
  }
  this.node.appendChild(this.children);
  this.items = new Map();
}

TreeItem.prototype = {

  items: null,

  isSelected: false,

  expanded: false,

  isRoot: false,

  parent: null,

  children: null,

  level: 0,

  











  add: function(items, defaultType, sorted) {
    if (items.length == this.level) {
      
      return;
    }
    
    let id = items[this.level].id || items[this.level];
    if (this.items.has(id)) {
      
      
      this.items.get(id).add(items, defaultType, sorted);
      return;
    }
    
    
    
    
    let label = items[this.level].label || items[this.level].id || items[this.level];
    let node = items[this.level].node;
    if (node) {
      
      
      label = node.textContent;
    }
    let treeItem = new TreeItem(this.document, this, node || label,
                                items[this.level].type || defaultType);

    treeItem.add(items, defaultType, sorted);
    treeItem.node.setAttribute("data-id", JSON.stringify(
      items.slice(0, this.level + 1).map(item => item.id || item)
    ));

    if (sorted) {
      
      let nextSibling = [...this.items.values()].find(child => {
        return child.label.textContent >= label;
      });

      if (nextSibling) {
        this.children.insertBefore(treeItem.node, nextSibling.node);
      } else {
        this.children.appendChild(treeItem.node);
      }
    } else {
      this.children.appendChild(treeItem.node);
    }

    if (this.label) {
      this.label.removeAttribute("empty");
    }
    this.items.set(id, treeItem);
  },

  








  remove: function(items = []) {
    let id = items.shift();
    if (id && this.items.has(id)) {
      let deleted = this.items.get(id);
      if (!items.length) {
        this.items.delete(id);
      }
      deleted.remove(items);
    } else if (!id) {
      this.destroy();
    }
  },

  






  setSelectedItem: function(items) {
    if (!items[this.level]) {
      this.label.classList.add("theme-selected");
      this.label.setAttribute("expanded", "true");
      return this.label;
    }
    if (this.items.has(items[this.level])) {
      let label = this.items.get(items[this.level]).setSelectedItem(items);
      if (label && this.label) {
        this.label.setAttribute("expanded", true);
      }
      return label;
    }
    return null;
  },

  


  collapseAll: function() {
    if (this.label) {
      this.label.removeAttribute("expanded");
    }
    for (let child of this.items.values()) {
      child.collapseAll();
    }
  },

  


  expandAll: function() {
    if (this.label) {
      this.label.setAttribute("expanded", "true");
    }
    for (let child of this.items.values()) {
      child.expandAll();
    }
  },

  destroy: function() {
    this.children.remove();
    this.node.remove();
    this.label = null;
    this.items = null;
    this.children = null;
  }
};
