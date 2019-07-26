



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

this.EXPORTED_SYMBOLS = [ "TranslationDocument" ];

const SHOW_ELEMENT = Ci.nsIDOMNodeFilter.SHOW_ELEMENT;
const SHOW_TEXT = Ci.nsIDOMNodeFilter.SHOW_TEXT;
const TEXT_NODE = Ci.nsIDOMNode.TEXT_NODE;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");











this.TranslationDocument = function(document) {
  this.itemsMap = new Map();
  this.roots = [];
  this._init(document);
};

this.TranslationDocument.prototype = {
  





  _init: function(document) {
    let window = document.defaultView;
    let winUtils = window.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindowUtils);

    
    
    
    
    let nodeList = winUtils.getTranslationNodes(document.body);

    let length = nodeList.length;

    for (let i = 0; i < length; i++) {
      let node = nodeList.item(i);
      let isRoot = nodeList.isTranslationRootAtIndex(i);

      
      
      this._createItemForNode(node, i, isRoot);
    }

    
    
    

    
    
    

    
    
    

    for (let root of this.roots) {
      if (root.children.length == 0 &&
          root.nodeRef.childElementCount == 0) {
        root.isSimpleRoot = true;
      }
    }
  },

  









  _createItemForNode: function(node, id, isRoot) {
    if (this.itemsMap.has(node)) {
      return this.itemsMap.get(node);
    }

    let item = new TranslationItem(node, id, isRoot);

    if (isRoot) {
      
      this.roots.push(item);
    } else {
      let parentItem = this.itemsMap.get(node.parentNode);
      if (parentItem) {
        parentItem.children.push(item);
      }
    }

    this.itemsMap.set(node, item);
    return item;
  },

  









  generateTextForItem: function(item) {
    if (item.isSimpleRoot) {
      let text = item.nodeRef.firstChild.nodeValue.trim();
      item.original = [text];
      return text;
    }

    let localName = item.isRoot ? "div" : "b";
    let str = '<' + localName + ' id="n' + item.id + '">';

    item.original = [];

    for (let child of item.nodeRef.childNodes) {
      if (child.nodeType == TEXT_NODE) {
        let x = child.nodeValue.trim();
        str += x;
        item.original.push(x);
        continue;
      }

      let objInMap = this.itemsMap.get(child);
      if (objInMap) {
        
        
        
        item.original.push(objInMap);
        str += this.generateTextForItem(objInMap);
      } else {
        
        
        
        
        
        str += '<br/>';
      }
    }

    str += '</' + localName + '>';
    return str;
  }
};






function TranslationItem(node, id, isRoot) {
  this.nodeRef = node;
  this.id = id;
  this.isRoot = isRoot;
  this.children = [];
}

TranslationItem.prototype = {
  isRoot: false,
  isSimpleRoot: false,

  toString: function() {
    let rootType = this._isRoot
                   ? (this._isSimpleRoot ? ' (simple root)' : ' (non simple root)')
                   : '';
    return "[object TranslationItem: <" + this.nodeRef.localName + ">"
           + rootType + "]";
  }
}
