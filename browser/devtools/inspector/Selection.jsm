





const Cu = Components.utils;
Cu.import("resource:///modules/devtools/EventEmitter.jsm");

this.EXPORTED_SYMBOLS = ["Selection"];


















































this.Selection = function Selection(node=null, track={attributes:true,detached:true}) {
  EventEmitter.decorate(this);
  this._onMutations = this._onMutations.bind(this);
  this.track = track;
  this.setNode(node);
}

Selection.prototype = {
  _node: null,

  _onMutations: function(mutations) {
    let attributeChange = false;
    let detached = false;
    let parentNode = null;
    for (let m of mutations) {
      if (!attributeChange && m.type == "attributes") {
        attributeChange = true;
      }
      if (m.type == "childList") {
        if (!detached && !this.isConnected()) {
          parentNode = m.target;
          detached = true;
        }
      }
    }

    if (attributeChange)
      this.emit("attribute-changed");
    if (detached)
      this.emit("detached", parentNode);
  },

  _attachEvents: function SN__attachEvents() {
    if (!this.window || !this.isNode() || !this.track) {
      return;
    }

    if (this.track.attributes) {
      this._nodeObserver = new this.window.MutationObserver(this._onMutations);
      this._nodeObserver.observe(this.node, {attributes: true});
    }

    if (this.track.detached) {
      this._docObserver = new this.window.MutationObserver(this._onMutations);
      this._docObserver.observe(this.document.documentElement, {childList: true, subtree: true});
    }
  },

  _detachEvents: function SN__detachEvents() {
    
    
    try {
      if (this._nodeObserver)
        this._nodeObserver.disconnect();
    } catch(e) {}
    try {
      if (this._docObserver)
        this._docObserver.disconnect();
    } catch(e) {}
  },

  destroy: function SN_destroy() {
    this._detachEvents();
    this.setNode(null);
  },

  setNode: function SN_setNode(value, reason="unknown") {
    this.reason = reason;
    if (value !== this._node) {
      this.emit("before-new-node", value, reason);
      let previousNode = this._node;
      this._detachEvents();
      this._node = value;
      this._attachEvents();
      this.emit("new-node", previousNode, this.reason);
    }
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

  isRoot: function SN_isRootNode() {
    return this.isNode() &&
           this.isConnected() &&
           this.node.ownerDocument.documentElement === this.node;
  },

  isNode: function SN_isNode() {
    return (this.node &&
            this.node.ownerDocument &&
            this.node.ownerDocument.defaultView &&
            this.node instanceof this.node.ownerDocument.defaultView.Node);
  },

  isConnected: function SN_isConnected() {
    try {
      let doc = this.document;
      return doc && doc.defaultView && doc.documentElement.contains(this.node);
    } catch (e) {
      
      return false;
    }
  },

  isHTMLNode: function SN_isHTMLNode() {
    let xhtml_ns = "http://www.w3.org/1999/xhtml";
    return this.isNode() && this.node.namespaceURI == xhtml_ns;
  },

  

  isElementNode: function SN_isElementNode() {
    return this.isNode() && this.node.nodeType == this.window.Node.ELEMENT_NODE;
  },

  isAttributeNode: function SN_isAttributeNode() {
    return this.isNode() && this.node.nodeType == this.window.Node.ATTRIBUTE_NODE;
  },

  isTextNode: function SN_isTextNode() {
    return this.isNode() && this.node.nodeType == this.window.Node.TEXT_NODE;
  },

  isCDATANode: function SN_isCDATANode() {
    return this.isNode() && this.node.nodeType == this.window.Node.CDATA_SECTION_NODE;
  },

  isEntityRefNode: function SN_isEntityRefNode() {
    return this.isNode() && this.node.nodeType == this.window.Node.ENTITY_REFERENCE_NODE;
  },

  isEntityNode: function SN_isEntityNode() {
    return this.isNode() && this.node.nodeType == this.window.Node.ENTITY_NODE;
  },

  isProcessingInstructionNode: function SN_isProcessingInstructionNode() {
    return this.isNode() && this.node.nodeType == this.window.Node.PROCESSING_INSTRUCTION_NODE;
  },

  isCommentNode: function SN_isCommentNode() {
    return this.isNode() && this.node.nodeType == this.window.Node.PROCESSING_INSTRUCTION_NODE;
  },

  isDocumentNode: function SN_isDocumentNode() {
    return this.isNode() && this.node.nodeType == this.window.Node.DOCUMENT_NODE;
  },

  isDocumentTypeNode: function SN_isDocumentTypeNode() {
    return this.isNode() && this.node.nodeType ==this.window. Node.DOCUMENT_TYPE_NODE;
  },

  isDocumentFragmentNode: function SN_isDocumentFragmentNode() {
    return this.isNode() && this.node.nodeType == this.window.Node.DOCUMENT_FRAGMENT_NODE;
  },

  isNotationNode: function SN_isNotationNode() {
    return this.isNode() && this.node.nodeType == this.window.Node.NOTATION_NODE;
  },
}
