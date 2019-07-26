



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

this.EXPORTED_SYMBOLS = [ "TranslationDocument" ];

const SHOW_ELEMENT = Ci.nsIDOMNodeFilter.SHOW_ELEMENT;
const SHOW_TEXT = Ci.nsIDOMNodeFilter.SHOW_TEXT;
const TEXT_NODE = Ci.nsIDOMNode.TEXT_NODE;

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
    if (item.original) {
      return regenerateTextFromOriginalHelper(item);
    }

    if (item.isSimpleRoot) {
      let text = item.nodeRef.firstChild.nodeValue.trim();
      item.original = [text];
      return text;
    }

    let str = "";
    item.original = [];
    let wasLastItemPlaceholder = false;

    for (let child of item.nodeRef.childNodes) {
      if (child.nodeType == TEXT_NODE) {
        let x = child.nodeValue.trim();
        if (x != "") {
          item.original.push(x);
          str += x;
          wasLastItemPlaceholder = false;
        }
        continue;
      }

      let objInMap = this.itemsMap.get(child);
      if (objInMap && !objInMap.isRoot) {
        
        
        
        
        
        
        item.original.push(objInMap);
        str += this.generateTextForItem(objInMap);
        wasLastItemPlaceholder = false;
      } else {
        
        
        
        
        
        
        
        if (!wasLastItemPlaceholder) {
          item.original.push(TranslationItem_NodePlaceholder);
          str += '<br>';
          wasLastItemPlaceholder = true;
        }
      }
    }

    return generateTranslationHtmlForItem(item, str);
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
    let rootType = this.isRoot
                   ? (this.isSimpleRoot ? ' (simple root)' : ' (non simple root)')
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








const TranslationItem_NodePlaceholder = {
  toString: function() {
    return "[object TranslationItem_NodePlaceholder]";
  }
};









function generateTranslationHtmlForItem(item, content) {
  let localName = item.isRoot ? "div" : "b";
  return '<' + localName + ' id=n' + item.id + '>' +
         content +
         "</" + localName + ">";
}

 








function regenerateTextFromOriginalHelper(item) {
  if (item.isSimpleRoot) {
    return item.original[0];
  }

  let str = "";
  for (let child of item.original) {
    if (child instanceof TranslationItem) {
      str += regenerateTextFromOriginalHelper(child);
    } else if (child === TranslationItem_NodePlaceholder) {
      str += "<br>";
    } else {
      str += child;
    }
  }

  return generateTranslationHtmlForItem(item, str);
}















function parseResultNode(item, node) {
  item.translation = [];
  for (let child of node.childNodes) {
    if (child.nodeType == TEXT_NODE) {
      item.translation.push(child.nodeValue);
    } else if (child.localName == "br") {
      item.translation.push(TranslationItem_NodePlaceholder);
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

    if (!curItem[target]) {
      
      
      
      
      
      continue;
    }

    domNode.normalize();

    
    
    
    
    
    
    
    
    
    
    let curNode = domNode.firstChild;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    while (curNode &&
           curNode.nodeType == TEXT_NODE &&
           curNode.nodeValue.trim() == "") {
      curNode = curNode.nextSibling;
    }

    
    
    
    for (let targetItem of curItem[target]) {

      if (targetItem instanceof TranslationItem) {
        
        
        visitStack.push(targetItem);

        let targetNode = targetItem.nodeRef;

            
            
        if (curNode != targetNode &&
            
            
            
            
            targetNode.parentNode == domNode) {

          
          
          
          domNode.insertBefore(targetNode, curNode);
          curNode = targetNode;
        }

        
        
        
        
        if (curNode) {
          curNode = getNextSiblingSkippingEmptyTextNodes(curNode);
        }

      } else if (targetItem === TranslationItem_NodePlaceholder) {
        
        
        
        
        
        

        while (curNode &&
               (curNode.nodeType != TEXT_NODE ||
                curNode.nodeValue.trim() == "")) {
          curNode = curNode.nextSibling;
        }

      } else {
        
        
        
        while (curNode && curNode.nodeType != TEXT_NODE) {
          curNode = curNode.nextSibling;
        }

        
        
        if (!curNode) {
          
          
          
          curNode = domNode.appendChild(domNode.ownerDocument.createTextNode(" "));
        }

        
        
        let preSpace = /^\s/.test(curNode.nodeValue) ? " " : "";
        let endSpace = /\s$/.test(curNode.nodeValue) ? " " : "";

        curNode.nodeValue = preSpace + targetItem + endSpace;
        curNode = getNextSiblingSkippingEmptyTextNodes(curNode);
      }
    }

    
    
    if (curNode) {
      clearRemainingNonEmptyTextNodesFromElement(curNode);
    }

    
    domNode.normalize();
  }
}

function getNextSiblingSkippingEmptyTextNodes(startSibling) {
  let item = startSibling.nextSibling;
  while (item &&
         item.nodeType == TEXT_NODE &&
         item.nodeValue.trim() == "") {
    item = item.nextSibling;
  }
  return item;
}

function clearRemainingNonEmptyTextNodesFromElement(startSibling) {
  let item = startSibling;
  while (item) {
    if (item.nodeType == TEXT_NODE &&
        item.nodeValue != "") {
      item.nodeValue = "";
    }
    item = item.nextSibling;
  }
}
