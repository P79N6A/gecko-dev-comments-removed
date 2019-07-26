





"use strict";

const {Cu, Ci} = require("chrome");
let EventEmitter = require("devtools/toolkit/event-emitter");


















































function Selection(walker, node=null, track={attributes:true,detached:true}) {
  EventEmitter.decorate(this);

  this._onMutations = this._onMutations.bind(this);
  this.track = track;
  this.setWalker(walker);
  this.setNode(node);
}

exports.Selection = Selection;

Selection.prototype = {
  _walker: null,
  _node: null,

  _onMutations: function(mutations) {
    let attributeChange = false;
    let pseudoChange = false;
    let detached = false;
    let parentNode = null;

    for (let m of mutations) {
      if (!attributeChange && m.type == "attributes") {
        attributeChange = true;
      }
      if (m.type == "childList") {
        if (!detached && !this.isConnected()) {
          if (this.isNode()) {
            parentNode = m.target;
          }
          detached = true;
        }
      }
      if (m.type == "pseudoClassLock") {
        pseudoChange = true;
      }
    }

    
    if (attributeChange) {
      this.emit("attribute-changed");
    }
    if (pseudoChange) {
      this.emit("pseudoclass");
    }
    if (detached) {
      let rawNode = null;
      if (parentNode && parentNode.isLocal_toBeDeprecated()) {
        rawNode = parentNode.rawNode();
      }

      this.emit("detached", rawNode, null);
      this.emit("detached-front", parentNode);
    }
  },

  destroy: function() {
    this.setNode(null);
    this.setWalker(null);
  },

  setWalker: function(walker) {
    if (this._walker) {
      this._walker.off("mutations", this._onMutations);
    }
    this._walker = walker;
    if (this._walker) {
      this._walker.on("mutations", this._onMutations);
    }
  },

  
  setNode: function(value, reason="unknown") {
    if (value) {
      value = this._walker.frontForRawNode(value);
    }
    this.setNodeFront(value, reason);
  },

  
  get node() {
    return this._node;
  },

  
  get window() {
    if (this.isNode()) {
      return this.node.ownerDocument.defaultView;
    }
    return null;
  },

  
  get document() {
    if (this.isNode()) {
      return this.node.ownerDocument;
    }
    return null;
  },

  setNodeFront: function(value, reason="unknown") {
    this.reason = reason;
    if (value !== this._nodeFront) {
      let rawValue = null;
      if (value && value.isLocal_toBeDeprecated()) {
        rawValue = value.rawNode();
      }
      this.emit("before-new-node", rawValue, reason);
      this.emit("before-new-node-front", value, reason);
      let previousNode = this._node;
      let previousFront = this._nodeFront;
      this._node = rawValue;
      this._nodeFront = value;
      this.emit("new-node", previousNode, this.reason);
      this.emit("new-node-front", value, this.reason);
    }
  },

  get documentFront() {
    return this._walker.document(this._nodeFront);
  },

  get nodeFront() {
    return this._nodeFront;
  },

  isRoot: function() {
    return this.isNode() &&
           this.isConnected() &&
           this._nodeFront.isDocumentElement;
  },

  isNode: function() {
    if (!this._nodeFront) {
      return false;
    }

    
    
    if (this._node && Cu.isDeadWrapper(this._node)) {
      return false;
    }

    return true;
  },

  isLocal: function() {
    return !!this._node;
  },

  isConnected: function() {
    let node = this._nodeFront;
    if (!node || !node.actorID) {
      return false;
    }

    
    
    let rawNode = null;
    if (node.isLocal_toBeDeprecated()) {
      rawNode = node.rawNode();
    }
    if (rawNode) {
      try {
        let doc = this.document;
        return (doc && doc.defaultView && doc.documentElement.contains(rawNode));
      } catch (e) {
        
        return false;
      }
    }

    while(node) {
      if (node === this._walker.rootNode) {
        return true;
      }
      node = node.parentNode();
    };
    return false;
  },

  isHTMLNode: function() {
    let xhtml_ns = "http://www.w3.org/1999/xhtml";
    return this.isNode() && this.node.namespaceURI == xhtml_ns;
  },

  

  isElementNode: function() {
    return this.isNode() && this.nodeFront.nodeType == Ci.nsIDOMNode.ELEMENT_NODE;
  },

  isAttributeNode: function() {
    return this.isNode() && this.nodeFront.nodeType == Ci.nsIDOMNode.ATTRIBUTE_NODE;
  },

  isTextNode: function() {
    return this.isNode() && this.nodeFront.nodeType == Ci.nsIDOMNode.TEXT_NODE;
  },

  isCDATANode: function() {
    return this.isNode() && this.nodeFront.nodeType == Ci.nsIDOMNode.CDATA_SECTION_NODE;
  },

  isEntityRefNode: function() {
    return this.isNode() && this.nodeFront.nodeType == Ci.nsIDOMNode.ENTITY_REFERENCE_NODE;
  },

  isEntityNode: function() {
    return this.isNode() && this.nodeFront.nodeType == Ci.nsIDOMNode.ENTITY_NODE;
  },

  isProcessingInstructionNode: function() {
    return this.isNode() && this.nodeFront.nodeType == Ci.nsIDOMNode.PROCESSING_INSTRUCTION_NODE;
  },

  isCommentNode: function() {
    return this.isNode() && this.nodeFront.nodeType == Ci.nsIDOMNode.PROCESSING_INSTRUCTION_NODE;
  },

  isDocumentNode: function() {
    return this.isNode() && this.nodeFront.nodeType == Ci.nsIDOMNode.DOCUMENT_NODE;
  },

  isDocumentTypeNode: function() {
    return this.isNode() && this.nodeFront.nodeType == Ci.nsIDOMNode.DOCUMENT_TYPE_NODE;
  },

  isDocumentFragmentNode: function() {
    return this.isNode() && this.nodeFront.nodeType == Ci.nsIDOMNode.DOCUMENT_FRAGMENT_NODE;
  },

  isNotationNode: function() {
    return this.isNode() && this.nodeFront.nodeType == Ci.nsIDOMNode.NOTATION_NODE;
  },
};
