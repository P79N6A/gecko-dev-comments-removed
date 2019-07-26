



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

this.EXPORTED_SYMBOLS = [ "TranslationDocument" ];

const SHOW_ELEMENT = Ci.nsIDOMNodeFilter.SHOW_ELEMENT;
const SHOW_TEXT = Ci.nsIDOMNodeFilter.SHOW_TEXT;
const TEXT_NODE = Ci.nsIDOMNode.TEXT_NODE;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/Task.jsm");










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
  },

  



  showTranslation: function() {
    this._swapDocumentContent("translation");
  },

  



  showOriginal: function() {
    this._swapDocumentContent("original");
  },

  






  _swapDocumentContent: function(target) {
    Task.spawn(function *() {
      
      
      const YIELD_INTERVAL = 100;
      let count = YIELD_INTERVAL;

      for (let root of this.roots) {
        root.swapText(target);
        if (count-- == 0) {
          count = YIELD_INTERVAL;
          yield CommonUtils.laterTickResolvingPromise();
        }
      }
    }.bind(this));
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
  },

  
















  parseResult: function(result) {
    if (this.isSimpleRoot) {
      this.translation = [result];
      return;
    }

    let domParser = Cc["@mozilla.org/xmlextras/domparser;1"]
                      .createInstance(Ci.nsIDOMParser);

    let doc = domParser.parseFromString(result, "text/html");
    parseResultNode(this, doc.body.firstChild);
  },

  






  getChildById: function(id) {
    for (let child of this.children) {
      if (("n" + child.id) == id) {
        return child;
      }
    }
    return null;
  },

  






  swapText: function(target) {
    swapTextForItem(this, target);
  }
};















function parseResultNode(item, node) {
  item.translation = [];
  for (let child of node.childNodes) {
    if (child.nodeType == TEXT_NODE) {
      item.translation.push(child.nodeValue);
    } else {
      let translationItemChild = item.getChildById(child.id);

      if (translationItemChild) {
        item.translation.push(translationItemChild);
        parseResultNode(translationItemChild, child);
      }
    }
  }
}









function swapTextForItem(item, target) {
  
  
  let visitStack = [ item ];
  let source = target == "translation" ? "original" : "translation";

  while (visitStack.length > 0) {
    let curItem = visitStack.shift();

    let domNode = curItem.nodeRef;
    if (!domNode) {
      
      continue;
    }

    let sourceNodeCount = 0;

    if (!curItem[target]) {
      
      
      
      
      
      continue;
    }

    
    
    
    for (let child of curItem[target]) {
      
      
      if (child instanceof TranslationItem) {
        
        visitStack.push(child);
        continue;
      }

      
      
      
      
      
      let targetTextNode = getNthNonEmptyTextNodeFromElement(sourceNodeCount++, domNode);

      
      let preSpace = targetTextNode.nodeValue.startsWith(" ") ? " " : "";
      let endSpace = targetTextNode.nodeValue.endsWith(" ") ? " " : "";
      targetTextNode.nodeValue = preSpace + child + endSpace;
    }

    
    
    if (sourceNodeCount > 0) {
      clearRemainingNonEmptyTextNodesFromElement(sourceNodeCount, domNode);
    }
  }
}

function getNthNonEmptyTextNodeFromElement(n, element) {
  for (let childNode of element.childNodes) {
    if (childNode.nodeType == Ci.nsIDOMNode.TEXT_NODE &&
        childNode.nodeValue.trim() != "") {
      if (n-- == 0)
        return childNode;
    }
  }

  
  return element.appendChild(element.ownerDocument.createTextNode(""));
}

function clearRemainingNonEmptyTextNodesFromElement(start, element) {
  let count = 0;
  for (let childNode of element.childNodes) {
    if (childNode.nodeType == Ci.nsIDOMNode.TEXT_NODE &&
        childNode.nodeValue.trim() != "") {
      if (count++ >= start) {
        childNode.nodeValue = "";
      }
    }
  }
}
