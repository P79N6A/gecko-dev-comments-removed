



"use strict";















































const {Cc, Ci, Cu, Cr} = require("chrome");

const protocol = require("devtools/server/protocol");
const {Arg, Option, method, RetVal, types} = protocol;
const {LongStringActor, ShortLongString} = require("devtools/server/actors/string");
const promise = require("sdk/core/promise");
const object = require("sdk/util/object");
const events = require("sdk/event/core");
const {setTimeout, clearTimeout} = require('sdk/timers');
const { Unknown } = require("sdk/platform/xpcom");
const { Class } = require("sdk/core/heritage");
const {PageStyleActor} = require("devtools/server/actors/styles");

const PSEUDO_CLASSES = [":hover", ":active", ":focus"];

const HIDDEN_CLASS = "__fx-devtools-hide-shortcut__";

const HIGHLIGHTED_PSEUDO_CLASS = ":-moz-devtools-highlighted";
const HIGHLIGHTED_TIMEOUT = 2000;

let HELPER_SHEET = ".__fx-devtools-hide-shortcut__ { visibility: hidden !important } ";
HELPER_SHEET += ":-moz-devtools-highlighted { outline: 2px dashed #F06!important; outline-offset: -2px!important } ";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm");

exports.register = function(handle) {
  handle.addTabActor(InspectorActor, "inspectorActor");
};

exports.unregister = function(handle) {
  handle.removeTabActor(InspectorActor);
};



function makeInfallible(handler) {
  return function(...args) {
    try {
      return handler.apply(this, args);
    } catch(ex) {
      console.error(ex);
    }
    return undefined;
  }
}


function delayedResolve(value) {
  let deferred = promise.defer();
  Services.tm.mainThread.dispatch(makeInfallible(function delayedResolveHandler() {
    deferred.resolve(value);
  }), 0);
  return deferred.promise;
}





exports.DEFAULT_VALUE_SUMMARY_LENGTH = 50;
var gValueSummaryLength = exports.DEFAULT_VALUE_SUMMARY_LENGTH;

exports.getValueSummaryLength = function() {
  return gValueSummaryLength;
};

exports.setValueSummaryLength = function(val) {
  gValueSummaryLength = val;
};




var NodeActor = protocol.ActorClass({
  typeName: "domnode",

  initialize: function(walker, node) {
    protocol.Actor.prototype.initialize.call(this, null);
    this.walker = walker;
    this.rawNode = node;
  },

  toString: function() {
    return "[NodeActor " + this.actorID + " for " + this.rawNode.toString() + "]";
  },

  



  get conn() this.walker.conn,

  
  form: function(detail) {
    if (detail === "actorid") {
      return this.actorID;
    }

    let parentNode = this.walker.parentNode(this);

    
    let numChildren = this.rawNode.childNodes.length;
    if (numChildren === 0 &&
        (this.rawNode.contentDocument || this.rawNode.getSVGDocument)) {
      
      numChildren = 1;
    }

    let form = {
      actor: this.actorID,
      parent: parentNode ? parentNode.actorID : undefined,
      nodeType: this.rawNode.nodeType,
      namespaceURI: this.rawNode.namespaceURI,
      nodeName: this.rawNode.nodeName,
      numChildren: numChildren,

      
      name: this.rawNode.name,
      publicId: this.rawNode.publicId,
      systemId: this.rawNode.systemId,

      attrs: this.writeAttrs(),

      pseudoClassLocks: this.writePseudoClassLocks(),
    };

    if (this.rawNode.ownerDocument &&
        this.rawNode.ownerDocument.documentElement === this.rawNode) {
      form.isDocumentElement = true;
    }

    if (this.rawNode.nodeValue) {
      
      
      if (this.rawNode.nodeValue.length > gValueSummaryLength) {
        form.shortValue = this.rawNode.nodeValue.substring(0, gValueSummaryLength);
        form.incompleteValue = true;
      } else {
        form.shortValue = this.rawNode.nodeValue;
      }
    }

    return form;
  },

  writeAttrs: function() {
    if (!this.rawNode.attributes) {
      return undefined;
    }
    return [{namespace: attr.namespace, name: attr.name, value: attr.value }
            for (attr of this.rawNode.attributes)];
  },

  writePseudoClassLocks: function() {
    if (this.rawNode.nodeType !== Ci.nsIDOMNode.ELEMENT_NODE) {
      return undefined;
    }
    let ret = undefined;
    for (let pseudo of PSEUDO_CLASSES) {
      if (DOMUtils.hasPseudoClassLock(this.rawNode, pseudo)) {
        ret = ret || [];
        ret.push(pseudo);
      }
    }
    return ret;
  },

  


  getNodeValue: method(function() {
    return new LongStringActor(this.conn, this.rawNode.nodeValue || "");
  }, {
    request: {},
    response: {
      value: RetVal("longstring")
    }
  }),

  


  setNodeValue: method(function(value) {
    this.rawNode.nodeValue = value;
  }, {
    request: { value: Arg(0) },
    response: {}
  }),

  












  modifyAttributes: method(function(modifications) {
    let rawNode = this.rawNode;
    for (let change of modifications) {
      if (change.newValue == null) {
        if (change.attributeNamespace) {
          rawNode.removeAttributeNS(change.attributeNamespace, change.attributeName);
        } else {
          rawNode.removeAttribute(change.attributeName);
        }
      } else {
        if (change.attributeNamespace) {
          rawNode.setAttributeNS(change.attributeNamespace, change.attributeName, change.newValue);
        } else {
          rawNode.setAttribute(change.attributeName, change.newValue);
        }
      }
    }
  }, {
    request: {
      modifications: Arg(0, "array:json")
    },
    response: {}
  }),

});















let NodeFront = protocol.FrontClass(NodeActor, {
  initialize: function(conn, form, detail, ctx) {
    this._parent = null; 
    this._child = null;  
    this._next = null;   
    this._prev = null;   
    protocol.Front.prototype.initialize.call(this, conn, form, detail, ctx);
  },

  




  destroy: function() {
    
    if (this.observer) {
      this._observer.disconnect();
      this._observer = null;
    }

    protocol.Front.prototype.destroy.call(this);
  },

  
  form: function(form, detail, ctx) {
    if (detail === "actorid") {
      this.actorID = form;
      return;
    }
    
    
    this._form = object.merge(form);
    this._form.attrs = this._form.attrs ? this._form.attrs.slice() : [];

    if (form.parent) {
      
      
      
      let parentNodeFront = ctx.marshallPool().ensureParentFront(form.parent);
      this.reparent(parentNodeFront);
    }
  },

  


  parentNode: function() {
    return this._parent;
  },

  





  updateMutation: function(change) {
    if (change.type === "attributes") {
      
      this._attrMap = undefined;

      
      let found = false;
      for (let i = 0; i < this.attributes.length; i++) {
        let attr = this.attributes[i];
        if (attr.name == change.attributeName &&
            attr.namespace == change.attributeNamespace) {
          if (change.newValue !== null) {
            attr.value = change.newValue;
          } else {
            this.attributes.splice(i, 1);
          }
          found = true;
          break;
        }
      }
      
      if (!found)  {
        this.attributes.push({
          name: change.attributeName,
          namespace: change.attributeNamespace,
          value: change.newValue
        });
      }
    } else if (change.type === "characterData") {
      this._form.shortValue = change.newValue;
      this._form.incompleteValue = change.incompleteValue;
    } else if (change.type === "pseudoClassLock") {
      this._form.pseudoClassLocks = change.pseudoClassLocks;
    }
  },

  

  get id() this.getAttribute("id"),

  get nodeType() this._form.nodeType,
  get namespaceURI() this._form.namespaceURI,
  get nodeName() this._form.nodeName,

  get className() {
    return this.getAttribute("class") || '';
  },

  get hasChildren() this._form.numChildren > 0,
  get numChildren() this._form.numChildren,

  get tagName() this.nodeType === Ci.nsIDOMNode.ELEMENT_NODE ? this.nodeName : null,
  get shortValue() this._form.shortValue,
  get incompleteValue() !!this._form.incompleteValue,

  get isDocumentElement() !!this._form.isDocumentElement,

  
  get name() this._form.name,
  get publicId() this._form.publicId,
  get systemId() this._form.systemId,

  getAttribute: function(name) {
    let attr = this._getAttribute(name);
    return attr ? attr.value : null;
  },
  hasAttribute: function(name) {
    this._cacheAttributes();
    return (name in this._attrMap);
  },

  get hidden() {
    let cls = this.getAttribute("class");
    return cls && cls.indexOf(HIDDEN_CLASS) > -1;
  },

  get attributes() this._form.attrs,

  get pseudoClassLocks() this._form.pseudoClassLocks || [],
  hasPseudoClassLock: function(pseudo) {
    return this.pseudoClassLocks.some(locked => locked === pseudo);
  },

  getNodeValue: protocol.custom(function() {
    if (!this.incompleteValue) {
      return delayedResolve(new ShortLongString(this.shortValue));
    } else {
      return this._getNodeValue();
    }
  }, {
    impl: "_getNodeValue"
  }),

  


  startModifyingAttributes: function() {
    return AttributeModificationList(this);
  },

  _cacheAttributes: function() {
    if (typeof(this._attrMap) != "undefined") {
      return;
    }
    this._attrMap = {};
    for (let attr of this.attributes) {
      this._attrMap[attr.name] = attr;
    }
  },

  _getAttribute: function(name) {
    this._cacheAttributes();
    return this._attrMap[name] || undefined;
  },

  




  reparent: function(parent) {
    if (this._parent === parent) {
      return;
    }

    if (this._parent && this._parent._child === this) {
      this._parent._child = this._next;
    }
    if (this._prev) {
      this._prev._next = this._next;
    }
    if (this._next) {
      this._next._prev = this._prev;
    }
    this._next = null;
    this._prev = null;
    this._parent = parent;
    if (!parent) {
      
      return;
    }
    this._next = parent._child;
    if (this._next) {
      this._next._prev = this;
    }
    parent._child = this;
  },

  


  treeChildren: function() {
    let ret = [];
    for (let child = this._child; child != null; child = child._next) {
      ret.push(child);
    }
    return ret;
  },

  




  rawNode: function(rawNode) {
    if (!this.conn._transport._serverConnection) {
      console.warn("Tried to use rawNode on a remote connection.");
      return null;
    }
    let actor = this.conn._transport._serverConnection.getActor(this.actorID);
    if (!actor) {
      
      
      return null;
    }
    return actor.rawNode;
  }
});





types.addDictType("disconnectedNode", {
  
  node: "domnode",

  
  newParents: "array:domnode"
});

types.addDictType("disconnectedNodeArray", {
  
  nodes: "array:domnode",

  
  newParents: "array:domnode"
});

types.addDictType("dommutation", {});




var NodeListActor = exports.NodeListActor = protocol.ActorClass({
  typeName: "domnodelist",

  initialize: function(walker, nodeList) {
    protocol.Actor.prototype.initialize.call(this);
    this.walker = walker;
    this.nodeList = nodeList;
  },

  destroy: function() {
    protocol.Actor.prototype.destroy.call(this);
  },

  



  get conn() {
    return this.walker.conn;
  },

  


  marshallPool: function() {
    return this.walker;
  },

  
  form: function() {
    return {
      actor: this.actorID,
      length: this.nodeList.length
    }
  },

  


  item: method(function(index) {
    let node = this.walker._ref(this.nodeList[index]);
    let newParents = [node for (node of this.walker.ensurePathToRoot(node))];
    return {
      node: node,
      newParents: newParents
    }
  }, {
    request: { item: Arg(0) },
    response: RetVal("disconnectedNode")
  }),

  


  items: method(function(start=0, end=this.nodeList.length) {
    let items = [this.walker._ref(item) for (item of Array.prototype.slice.call(this.nodeList, start, end))];
    let newParents = new Set();
    for (let item of items) {
      this.walker.ensurePathToRoot(item, newParents);
    }
    return {
      nodes: items,
      newParents: [node for (node of newParents)]
    }
  }, {
    request: {
      start: Arg(0, "nullable:number"),
      end: Arg(1, "nullable:number")
    },
    response: RetVal("disconnectedNodeArray")
  }),

  release: method(function() {}, { release: true })
});




var NodeListFront = exports.NodeLIstFront = protocol.FrontClass(NodeListActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
  },

  destroy: function() {
    protocol.Front.prototype.destroy.call(this);
  },

  marshallPool: function() {
    return this.parent();
  },

  
  form: function(json) {
    this.length = json.length;
  },

  item: protocol.custom(function(index) {
    return this._item(index).then(response => {
      return response.node;
    });
  }, {
    impl: "_item"
  }),

  items: protocol.custom(function(start, end) {
    return this._items(start, end).then(response => {
      return response.nodes;
    });
  }, {
    impl: "_items"
  })
});



let nodeArrayMethod = {
  request: {
    node: Arg(0, "domnode"),
    maxNodes: Option(1),
    center: Option(1, "domnode"),
    start: Option(1, "domnode"),
    whatToShow: Option(1)
  },
  response: RetVal(types.addDictType("domtraversalarray", {
    nodes: "array:domnode"
  }))
};

let traversalMethod = {
  request: {
    node: Arg(0, "domnode"),
    whatToShow: Option(1)
  },
  response: {
    node: RetVal("nullable:domnode")
  }
}












var ProgressListener = Class({
  extends: Unknown,
  interfaces: ["nsIWebProgressListener", "nsISupportsWeakReference"],

  initialize: function(tabActor) {
    Unknown.prototype.initialize.call(this);
    this.webProgress = tabActor.webProgress;
    this.webProgress.addProgressListener(
      this, Ci.nsIWebProgress.NOTIFY_ALL
    );
  },

  destroy: function() {
    try {
      this.webProgress.removeProgressListener(this);
    } catch(ex) {
      
    }
    this.webProgress = null;
  },

  onStateChange: makeInfallible(function stateChange(progress, request, flag, status) {
    if (!this.webProgress) {
      console.warn("got an onStateChange after destruction");
      return;
    }

    let isWindow = flag & Ci.nsIWebProgressListener.STATE_IS_WINDOW;
    let isDocument = flag & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT;
    if (!(isWindow || isDocument)) {
      return;
    }

    if (isDocument && (flag & Ci.nsIWebProgressListener.STATE_START)) {
      events.emit(this, "windowchange-start", progress.DOMWindow);
    }
    if (isWindow && (flag & Ci.nsIWebProgressListener.STATE_STOP)) {
      events.emit(this, "windowchange-stop", progress.DOMWindow);
    }
  }),
  onProgressChange: function() {},
  onSecurityChange: function() {},
  onStatusChange: function() {},
  onLocationChange: function() {},
});




var WalkerActor = protocol.ActorClass({
  typeName: "domwalker",

  events: {
    "new-mutations" : {
      type: "newMutations"
    }
  },

  




  initialize: function(conn, tabActor, options) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.rootWin = tabActor.window;
    this.rootDoc = this.rootWin.document;
    this._refMap = new Map();
    this._pendingMutations = [];
    this._activePseudoClassLocks = new Set();

    this.layoutHelpers = new LayoutHelpers(this.rootWin);

    
    
    
    this._orphaned = new Set();

    
    
    
    this._retainedOrphans = new Set();

    this.onMutations = this.onMutations.bind(this);
    this.onFrameLoad = this.onFrameLoad.bind(this);
    this.onFrameUnload = this.onFrameUnload.bind(this);

    this.progressListener = ProgressListener(tabActor);

    events.on(this.progressListener, "windowchange-start", this.onFrameUnload);
    events.on(this.progressListener, "windowchange-stop", this.onFrameLoad);

    
    
    this.rootNode = this.document();
  },

  
  form: function() {
    return {
      actor: this.actorID,
      root: this.rootNode.form()
    }
  },

  toString: function() {
    return "[WalkerActor " + this.actorID + "]";
  },

  destroy: function() {
    this.clearPseudoClassLocks();
    this._activePseudoClassLocks = null;
    this.progressListener.destroy();
    this.rootDoc = null;
    protocol.Actor.prototype.destroy.call(this);
  },

  release: method(function() {}, { release: true }),

  unmanage: function(actor) {
    if (actor instanceof NodeActor) {
      if (this._activePseudoClassLocks &&
          this._activePseudoClassLocks.has(actor)) {
        this.clearPsuedoClassLocks(actor);
      }
      this._refMap.delete(actor.rawNode);
    }
    protocol.Actor.prototype.unmanage.call(this, actor);
  },

  _ref: function(node) {
    let actor = this._refMap.get(node);
    if (actor) return actor;

    actor = new NodeActor(this, node);

    
    
    this.manage(actor);
    this._refMap.set(node, actor);

    if (node.nodeType === Ci.nsIDOMNode.DOCUMENT_NODE) {
      this._watchDocument(actor);
    }
    return actor;
  },

  


  _pickDeferred: null,
  pick: method(function() {
    if (this._pickDeferred) {
      return this._pickDeferred.promise;
    }

    this._pickDeferred = promise.defer();

    let window = this.rootDoc.defaultView;
    let isTouch = 'ontouchstart' in window;
    let event = isTouch ? 'touchstart' : 'click';

    this._onPick = function(e) {
      e.stopImmediatePropagation();
      e.preventDefault();
      window.removeEventListener(event, this._onPick, true);
      let u = window.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);

      let x, y;
      if (isTouch) {
        x = e.touches[0].clientX;
        y = e.touches[0].clientY;
      } else {
        x = e.clientX;
        y = e.clientY;
      }

      let node = u.elementFromPoint(x, y, false, false);
      node = this._ref(node);
      let newParents = this.ensurePathToRoot(node);

      this._pickDeferred.resolve({
        node: node,
        newParents: [parent for (parent of newParents)]
      });
      this._pickDeferred = null;

    }.bind(this);

    window.addEventListener(event, this._onPick, true);

    return this._pickDeferred.promise;
  }, { request: { }, response: RetVal("disconnectedNode") }),

  cancelPick: method(function() {
    if (this._pickDeferred) {
      let window = this.rootDoc.defaultView;
      let isTouch = 'ontouchstart' in window;
      let event = isTouch ? 'touchstart' : 'click';
      window.removeEventListener(event, this._onPick, true);
      this._pickDeferred.resolve(null);
      this._pickDeferred = null;
    }
  }),

  


  _unhighlight: function() {
    clearTimeout(this._highlightTimeout);
    if (!this.rootDoc) {
      return;
    }
    let nodes = this.rootDoc.querySelectorAll(HIGHLIGHTED_PSEUDO_CLASS);
    for (let node of nodes) {
      DOMUtils.removePseudoClassLock(node, HIGHLIGHTED_PSEUDO_CLASS);
    }
  },

  highlight: method(function(node) {
    this._installHelperSheet(node);
    this._unhighlight();

    if (!node ||
        !node.rawNode ||
         node.rawNode.nodeType !== Ci.nsIDOMNode.ELEMENT_NODE) {
      return;
    }

    this.layoutHelpers.scrollIntoViewIfNeeded(node.rawNode);
    DOMUtils.addPseudoClassLock(node.rawNode, HIGHLIGHTED_PSEUDO_CLASS);
    this._highlightTimeout = setTimeout(this._unhighlight.bind(this), HIGHLIGHTED_TIMEOUT);

  }, { request: { node: Arg(0, "nullable:domnode") }}),

  



  _watchDocument: function(actor) {
    let node = actor.rawNode;
    
    
    actor.observer = actor.rawNode.defaultView.MutationObserver(this.onMutations);
    actor.observer.observe(node, {
      attributes: true,
      characterData: true,
      childList: true,
      subtree: true
    });
  },

  






  document: method(function(node) {
    let doc = node ? nodeDocument(node.rawNode) : this.rootDoc;
    return this._ref(doc);
  }, {
    request: { node: Arg(0, "nullable:domnode") },
    response: { node: RetVal("domnode") },
  }),

  






  documentElement: method(function(node) {
    let elt = node ? nodeDocument(node.rawNode).documentElement : this.rootDoc.documentElement;
    return this._ref(elt);
  }, {
    request: { node: Arg(0, "nullable:domnode") },
    response: { node: RetVal("domnode") },
  }),

  









  parents: method(function(node, options={}) {
    let walker = documentWalker(node.rawNode, this.rootWin);
    let parents = [];
    let cur;
    while((cur = walker.parentNode())) {
      if (options.sameDocument && cur.ownerDocument != node.rawNode.ownerDocument) {
        break;
      }
      parents.push(this._ref(cur));
    }
    return parents;
  }, {
    request: {
      node: Arg(0, "domnode"),
      sameDocument: Option(1)
    },
    response: {
      nodes: RetVal("array:domnode")
    },
  }),

  parentNode: function(node) {
    let walker = documentWalker(node.rawNode, this.rootWin);
    let parent = walker.parentNode();
    if (parent) {
      return this._ref(parent);
    }
    return null;
  },

  
















  retainNode: method(function(node) {
    node.retained = true;
  }, {
    request: { node: Arg(0, "domnode") },
    response: {}
  }),

  



  unretainNode: method(function(node) {
    node.retained = false;
    if (this._retainedOrphans.has(node)) {
      this._retainedOrphans.delete(node);
      this.releaseNode(node);
    }
  }, {
    request: { node: Arg(0, "domnode") },
    response: {},
  }),

  


  releaseNode: method(function(node, options={}) {
    if (node.retained && !options.force) {
      this._retainedOrphans.add(node);
      return;
    }

    if (node.retained) {
      
      this._retainedOrphans.delete(node);
    }

    let walker = documentWalker(node.rawNode, this.rootWin);

    let child = walker.firstChild();
    while (child) {
      let childActor = this._refMap.get(child);
      if (childActor) {
        this.releaseNode(childActor, options);
      }
      child = walker.nextSibling();
    }

    node.destroy();
  }, {
    request: {
      node: Arg(0, "domnode"),
      force: Option(1)
    }
  }),

  



  ensurePathToRoot: function(node, newParents=new Set()) {
    if (!node) {
      return newParents;
    }
    let walker = documentWalker(node.rawNode, this.rootWin);
    let cur;
    while ((cur = walker.parentNode())) {
      let parent = this._refMap.get(cur);
      if (!parent) {
        
        newParents.add(this._ref(cur));
      } else {
        
        return newParents;
      }
    }
    return newParents;
  },

  























  children: method(function(node, options={}) {
    if (options.center && options.start) {
      throw Error("Can't specify both 'center' and 'start' options.");
    }
    let maxNodes = options.maxNodes || -1;
    if (maxNodes == -1) {
      maxNodes = Number.MAX_VALUE;
    }

    
    
    let filteredWalker = (node) => {
      return documentWalker(node, this.rootWin, options.whatToShow);
    }

    
    let rawNode = node.rawNode;
    let firstChild = filteredWalker(rawNode).firstChild();
    let lastChild = filteredWalker(rawNode).lastChild();

    if (!firstChild) {
      
      return { hasFirst: true, hasLast: true, nodes: [] };
    }

    let start;
    if (options.center) {
      start = options.center.rawNode;
    } else if (options.start) {
      start = options.start.rawNode;
    } else {
      start = firstChild;
    }

    let nodes = [];

    
    let backwardWalker = filteredWalker(start);
    if (start != firstChild && options.center) {
      backwardWalker.previousSibling();
      let backwardCount = Math.floor(maxNodes / 2);
      let backwardNodes = this._readBackward(backwardWalker, backwardCount);
      nodes = backwardNodes;
    }

    
    let forwardWalker = filteredWalker(start);
    let forwardCount = maxNodes - nodes.length;
    nodes = nodes.concat(this._readForward(forwardWalker, forwardCount));

    
    
    let remaining = maxNodes - nodes.length;
    if (options.center && remaining > 0 && nodes[0].rawNode != firstChild) {
      let firstNodes = this._readBackward(backwardWalker, remaining);

      
      nodes = firstNodes.concat(nodes);
    }

    return {
      hasFirst: nodes[0].rawNode == firstChild,
      hasLast: nodes[nodes.length - 1].rawNode == lastChild,
      nodes: nodes
    };
  }, nodeArrayMethod),

  


























  siblings: method(function(node, options={}) {
    let parentNode = documentWalker(node.rawNode, this.rootWin).parentNode();
    if (!parentNode) {
      return {
        hasFirst: true,
        hasLast: true,
        nodes: [node]
      };
    }

    if (!(options.start || options.center)) {
      options.center = node;
    }

    return this.children(this._ref(parentNode), options);
  }, nodeArrayMethod),

  








  nextSibling: method(function(node, options={}) {
    let walker = documentWalker(node.rawNode, this.rootWin, options.whatToShow || Ci.nsIDOMNodeFilter.SHOW_ALL);
    let sibling = walker.nextSibling();
    return sibling ? this._ref(sibling) : null;
  }, traversalMethod),

  








  previousSibling: method(function(node, options={}) {
    let walker = documentWalker(node.rawNode, this.rootWin, options.whatToShow || Ci.nsIDOMNodeFilter.SHOW_ALL);
    let sibling = walker.previousSibling();
    return sibling ? this._ref(sibling) : null;
  }, traversalMethod),

  



  _readForward: function(walker, count)
  {
    let ret = [];
    let node = walker.currentNode;
    do {
      ret.push(this._ref(node));
      node = walker.nextSibling();
    } while (node && --count);
    return ret;
  },

  



  _readBackward: function(walker, count)
  {
    let ret = [];
    let node = walker.currentNode;
    do {
      ret.push(this._ref(node));
      node = walker.previousSibling();
    } while(node && --count);
    ret.reverse();
    return ret;
  },

  






  querySelector: method(function(baseNode, selector) {
    let node = baseNode.rawNode.querySelector(selector);

    if (!node) {
      return {
      }
    };

    let node = this._ref(node);
    let newParents = this.ensurePathToRoot(node);
    return {
      node: node,
      newParents: [parent for (parent of newParents)]
    }
  }, {
    request: {
      node: Arg(0, "domnode"),
      selector: Arg(1)
    },
    response: RetVal("disconnectedNode")
  }),

  






  querySelectorAll: method(function(baseNode, selector) {
    return new NodeListActor(this, baseNode.rawNode.querySelectorAll(selector));
  }, {
    request: {
      node: Arg(0, "domnode"),
      selector: Arg(1)
    },
    response: {
      list: RetVal("domnodelist")
    }
  }),

  













  addPseudoClassLock: method(function(node, pseudo, options={}) {
    this._addPseudoClassLock(node, pseudo);

    if (!options.parents) {
      return;
    }

    let walker = documentWalker(node.rawNode, this.rootWin);
    let cur;
    while ((cur = walker.parentNode())) {
      let curNode = this._ref(cur);
      this._addPseudoClassLock(curNode, pseudo);
    }
  }, {
    request: {
      node: Arg(0, "domnode"),
      pseudoClass: Arg(1),
      parents: Option(2)
    },
    response: {}
  }),

  _queuePseudoClassMutation: function(node) {
    this.queueMutation({
      target: node.actorID,
      type: "pseudoClassLock",
      pseudoClassLocks: node.writePseudoClassLocks()
    });
  },

  _addPseudoClassLock: function(node, pseudo) {
    if (node.rawNode.nodeType !== Ci.nsIDOMNode.ELEMENT_NODE) {
      return false;
    }
    DOMUtils.addPseudoClassLock(node.rawNode, pseudo);
    this._activePseudoClassLocks.add(node);
    this._queuePseudoClassMutation(node);
    return true;
  },

  _installHelperSheet: function(node) {
    if (!this.installedHelpers) {
      this.installedHelpers = new WeakMap;
    }
    let win = node.rawNode.ownerDocument.defaultView;
    if (!this.installedHelpers.has(win)) {
      let { Style } = require("sdk/stylesheet/style");
      let { attach } = require("sdk/content/mod");
      let style = Style({source: HELPER_SHEET, type: "agent" });
      attach(style, win);
      this.installedHelpers.set(win, style);
    }
  },

  hideNode: method(function(node) {
    this._installHelperSheet(node);
    node.rawNode.classList.add(HIDDEN_CLASS);
  }, {
    request: { node: Arg(0, "domnode") }
  }),

  unhideNode: method(function(node) {
    node.rawNode.classList.remove(HIDDEN_CLASS);
  }, {
    request: { node: Arg(0, "domnode") }
  }),

  













  removePseudoClassLock: method(function(node, pseudo, options={}) {
    this._removePseudoClassLock(node, pseudo);

    if (!options.parents) {
      return;
    }

    let walker = documentWalker(node.rawNode, this.rootWin);
    let cur;
    while ((cur = walker.parentNode())) {
      let curNode = this._ref(cur);
      this._removePseudoClassLock(curNode, pseudo);
    }
  }, {
    request: {
      node: Arg(0, "domnode"),
      pseudoClass: Arg(1),
      parents: Option(2)
    },
    response: {}
  }),

  _removePseudoClassLock: function(node, pseudo) {
    if (node.rawNode.nodeType != Ci.nsIDOMNode.ELEMENT_NODE) {
      return false;
    }
    DOMUtils.removePseudoClassLock(node.rawNode, pseudo);
    if (!node.writePseudoClassLocks()) {
      this._activePseudoClassLocks.delete(node);
    }
    this._queuePseudoClassMutation(node);
    return true;
  },

  



  clearPseudoClassLocks: method(function(node) {
    if (node) {
      DOMUtils.clearPseudoClassLocks(node.rawNode);
      this._activePseudoClassLocks.delete(node);
      this._queuePseudoClassMutation(node);
    } else {
      for (let locked of this._activePseudoClassLocks) {
        DOMUtils.clearPseudoClassLocks(locked.rawNode);
        this._activePseudoClassLocks.delete(locked);
        this._queuePseudoClassMutation(locked);
      }
    }
  }, {
    request: {
      node: Arg(0, "nullable:domnode")
    },
    response: {}
  }),

  


  innerHTML: method(function(node) {
    return LongStringActor(this.conn, node.rawNode.innerHTML);
  }, {
    request: {
      node: Arg(0, "domnode")
    },
    response: {
      value: RetVal("longstring")
    }
  }),

  


  outerHTML: method(function(node) {
    return LongStringActor(this.conn, node.rawNode.outerHTML);
  }, {
    request: {
      node: Arg(0, "domnode")
    },
    response: {
      value: RetVal("longstring")
    }
  }),

  




  removeNode: method(function(node) {
    if ((node.rawNode.ownerDocument &&
         node.rawNode.ownerDocument.documentElement === this.rawNode) ||
         node.rawNode.nodeType === Ci.nsIDOMNode.DOCUMENT_NODE) {
      throw Error("Cannot remove document or document elements.");
    }
    let nextSibling = this.nextSibling(node);
    if (node.rawNode.parentNode) {
      node.rawNode.parentNode.removeChild(node.rawNode);
      
    }
    return nextSibling;
  }, {
    request: {
      node: Arg(0, "domnode")
    },
    response: {
      nextSibling: RetVal("nullable:domnode")
    }
  }),

  


  insertBefore: method(function(node, parent, sibling) {
    parent.rawNode.insertBefore(node.rawNode, sibling ? sibling.rawNode : null);
  }, {
    request: {
      node: Arg(0, "domnode"),
      parent: Arg(1, "domnode"),
      sibling: Arg(2, "nullable:domnode")
    },
    response: {}
  }),

  










































  getMutations: method(function(options={}) {
    let pending = this._pendingMutations || [];
    this._pendingMutations = [];

    if (options.cleanup) {
      for (let node of this._orphaned) {
        
        
        this.releaseNode(node);
      }
      this._orphaned = new Set();
    }

    return pending;
  }, {
    request: {
      cleanup: Option(0)
    },
    response: {
      mutations: RetVal("array:dommutation")
    }
  }),

  queueMutation: function(mutation) {
    if (!this.actorID) {
      
      return;
    }
    
    
    let needEvent = this._pendingMutations.length === 0;

    this._pendingMutations.push(mutation);

    if (needEvent) {
      events.emit(this, "new-mutations");
    }
  },

  





  onMutations: function(mutations) {
    for (let change of mutations) {
      let targetActor = this._refMap.get(change.target);
      if (!targetActor) {
        continue;
      }
      let targetNode = change.target;
      let mutation = {
        type: change.type,
        target: targetActor.actorID,
      }

      if (mutation.type === "attributes") {
        mutation.attributeName = change.attributeName;
        mutation.attributeNamespace = change.attributeNamespace || undefined;
        mutation.newValue = targetNode.getAttribute(mutation.attributeName);
      } else if (mutation.type === "characterData") {
        if (targetNode.nodeValue.length > gValueSummaryLength) {
          mutation.newValue = targetNode.nodeValue.substring(0, gValueSummaryLength);
          mutation.incompleteValue = true;
        } else {
          mutation.newValue = targetNode.nodeValue;
        }
      } else if (mutation.type === "childList") {
        
        
        let removedActors = [];
        let addedActors = [];
        for (let removed of change.removedNodes) {
          let removedActor = this._refMap.get(removed);
          if (!removedActor) {
            
            
            continue;
          }
          
          this._orphaned.add(removedActor);
          removedActors.push(removedActor.actorID);
        }
        for (let added of change.addedNodes) {
          let addedActor = this._refMap.get(added);
          if (!addedActor) {
            
            
            
            continue;
          }
          
          
          
          this._orphaned.delete(addedActor);
          addedActors.push(addedActor.actorID);
        }
        mutation.numChildren = change.target.childNodes.length;
        mutation.removed = removedActors;
        mutation.added = addedActors;
      }
      this.queueMutation(mutation);
    }
  },

  onFrameLoad: function(window) {
    let frame = this.layoutHelpers.getFrameElement(window);
    if (!frame && !this.rootDoc) {
      this.rootDoc = window.document;
      this.rootNode = this.document();
      this.queueMutation({
        type: "newRoot",
        target: this.rootNode.form()
      });
    }
    let frameActor = this._refMap.get(frame);
    if (!frameActor) {
      return;
    }

    this.queueMutation({
      type: "frameLoad",
      target: frameActor.actorID,
    });

    
    this.queueMutation({
      type: "childList",
      target: frameActor.actorID,
      added: [],
      removed: []
    })
  },

  
  _childOfWindow: function(window, domNode) {
    let win = nodeDocument(domNode).defaultView;
    while (win) {
      if (win === window) {
        return true;
      }
      win = this.layoutHelpers.getFrameElement(win);
    }
    return false;
  },

  onFrameUnload: function(window) {
    
    
    
    let releasedOrphans = [];

    for (let retained of this._retainedOrphans) {
      if (Cu.isDeadWrapper(retained.rawNode) ||
          this._childOfWindow(window, retained.rawNode)) {
        this._retainedOrphans.delete(retained);
        releasedOrphans.push(retained.actorID);
        this.releaseNode(retained, { force: true });
      }
    }

    if (releasedOrphans.length > 0) {
      this.queueMutation({
        target: this.rootNode.actorID,
        type: "unretained",
        nodes: releasedOrphans
      });
    }

    let doc = window.document;
    let documentActor = this._refMap.get(doc);
    if (!documentActor) {
      return;
    }

    if (this.rootDoc === doc) {
      this.rootDoc = null;
      this.rootNode = null;
    }

    this.queueMutation({
      type: "documentUnload",
      target: documentActor.actorID
    });

    let walker = documentWalker(doc, this.rootWin);
    let parentNode = walker.parentNode();
    if (parentNode) {
      
      
      this.queueMutation({
        type: "childList",
        target: this._refMap.get(parentNode).actorID,
        added: [],
        removed: []
      });
    }

    
    
    this.releaseNode(documentActor, { force: true });
  }
});




var WalkerFront = exports.WalkerFront = protocol.FrontClass(WalkerActor, {
  
  autoCleanup: true,

  pick: protocol.custom(function() {
    return this._pick().then(response => {
      return response.node;
    });
  }, {
    impl: "_pick"
  }),

  initialize: function(client, form) {
    this._createRootNodePromise();
    protocol.Front.prototype.initialize.call(this, client, form);
    this._orphaned = new Set();
    this._retainedOrphans = new Set();
  },

  destroy: function() {
    protocol.Front.prototype.destroy.call(this);
  },

  
  form: function(json) {
    this.actorID = json.actor;
    this.rootNode = types.getType("domnode").read(json.root, this);
    this._rootNodeDeferred.resolve(this.rootNode);
  },

  





  getRootNode: function() {
    return this._rootNodeDeferred.promise;
  },

  



  _createRootNodePromise: function() {
    this._rootNodeDeferred = promise.defer();
    this._rootNodeDeferred.promise.then(() => {
      events.emit(this, "new-root");
    });
  },

  







  ensureParentFront: function(id) {
    let front = this.get(id);
    if (front) {
      return front;
    }

    return types.getType("domnode").read({ actor: id }, this, "standin");
  },

  



















  retainNode: protocol.custom(function(node) {
    return this._retainNode(node).then(() => {
      node.retained = true;
    });
  }, {
    impl: "_retainNode",
  }),

  unretainNode: protocol.custom(function(node) {
    return this._unretainNode(node).then(() => {
      node.retained = false;
      if (this._retainedOrphans.has(node)) {
        this._retainedOrphans.delete(node);
        this._releaseFront(node);
      }
    });
  }, {
    impl: "_unretainNode"
  }),

  releaseNode: protocol.custom(function(node, options={}) {
    
    
    let actorID = node.actorID;
    this._releaseFront(node, !!options.force);
    return this._releaseNode({ actorID: actorID });
  }, {
    impl: "_releaseNode"
  }),

  querySelector: protocol.custom(function(queryNode, selector) {
    return this._querySelector(queryNode, selector).then(response => {
      return response.node;
    });
  }, {
    impl: "_querySelector"
  }),

  _releaseFront: function(node, force) {
    if (node.retained && !force) {
      node.reparent(null);
      this._retainedOrphans.add(node);
      return;
    }

    if (node.retained) {
      
      this._retainedOrphans.delete(node);
    }

    
    for (let child of node.treeChildren()) {
      this._releaseFront(child, force);
    }

    
    node.reparent(null);
    node.destroy();
  },

  


  getMutations: protocol.custom(function(options={}) {
    return this._getMutations(options).then(mutations => {
      let emitMutations = [];
      for (let change of mutations) {
        
        let targetID;
        let targetFront;

        if (change.type === "newRoot") {
          this.rootNode = types.getType("domnode").read(change.target, this);
          this._rootNodeDeferred.resolve(this.rootNode);
          targetID = this.rootNode.actorID;
          targetFront = this.rootNode;
        } else {
          targetID = change.target;
          targetFront = this.get(targetID);
        }

        if (!targetFront) {
          console.trace("Got a mutation for an unexpected actor: " + targetID + ", please file a bug on bugzilla.mozilla.org!");
          continue;
        }

        let emittedMutation = object.merge(change, { target: targetFront });

        if (change.type === "childList") {
          
          let addedFronts = [];
          let removedFronts = [];
          for (let removed of change.removed) {
            let removedFront = this.get(removed);
            if (!removedFront) {
              console.error("Got a removal of an actor we didn't know about: " + removed);
              continue;
            }
            
            removedFront.reparent(null);

            
            
            this._orphaned.add(removedFront);
            removedFronts.push(removedFront);
          }
          for (let added of change.added) {
            let addedFront = this.get(added);
            if (!addedFront) {
              console.error("Got an addition of an actor we didn't know about: " + added);
              continue;
            }
            addedFront.reparent(targetFront)

            
            
            this._orphaned.delete(addedFront);
            addedFronts.push(addedFront);
          }
          
          
          emittedMutation.added = addedFronts;
          emittedMutation.removed = removedFronts;
          targetFront._form.numChildren = change.numChildren;
        } else if (change.type === "frameLoad") {
          
          
          
          for (let child of targetFront.treeChildren()) {
            if (child.nodeType === Ci.nsIDOMNode.DOCUMENT_NODE) {
              console.trace("Got an unexpected frameLoad in the inspector, please file a bug on bugzilla.mozilla.org!");
            }
          }
        } else if (change.type === "documentUnload") {
          if (targetFront === this.rootNode) {
            this._createRootNodePromise();
          }

          
          
          emittedMutation.target = targetFront.actorID;
          emittedMutation.targetParent = targetFront.parentNode();

          
          this._releaseFront(targetFront, true);
        } else if (change.type === "unretained") {
          
          
          for (let released of change.nodes) {
            let releasedFront = this.get(released);
            this._retainedOrphans.delete(released);
            this._releaseFront(releasedFront, true);
          }
        } else {
          targetFront.updateMutation(change);
        }

        emitMutations.push(emittedMutation);
      }

      if (options.cleanup) {
        for (let node of this._orphaned) {
          
          this._releaseFront(node);
        }
        this._orphaned = new Set();
      }

      events.emit(this, "mutations", emitMutations);
    });
  }, {
    impl: "_getMutations"
  }),

  



  onMutations: protocol.preEvent("new-mutations", function() {
    
    this.getMutations({cleanup: this.autoCleanup}).then(null, console.error);
  }),

  isLocal: function() {
    return !!this.conn._transport._serverConnection;
  },

  
  
  frontForRawNode: function(rawNode){
    if (!this.isLocal()) {
      console.warn("Tried to use frontForRawNode on a remote connection.");
      return null;
    }
    let walkerActor = this.conn._transport._serverConnection.getActor(this.actorID);
    if (!walkerActor) {
      throw Error("Could not find client side for actor " + this.actorID);
    }
    let nodeActor = walkerActor._ref(rawNode);

    
    let nodeType = types.getType("domnode");
    let returnNode = nodeType.read(nodeType.write(nodeActor, walkerActor), this);
    let top = returnNode;
    let extras = walkerActor.parents(nodeActor);
    for (let extraActor of extras) {
      top = nodeType.read(nodeType.write(extraActor, walkerActor), this);
    }

    if (top !== this.rootNode) {
      
      this._orphaned.add(top);
      walkerActor._orphaned.add(this.conn._transport._serverConnection.getActor(top.actorID));
    }
    return returnNode;
  }
});





var AttributeModificationList = Class({
  initialize: function(node) {
    this.node = node;
    this.modifications = [];
  },

  apply: function() {
    let ret = this.node.modifyAttributes(this.modifications);
    return ret;
  },

  destroy: function() {
    this.node = null;
    this.modification = null;
  },

  setAttributeNS: function(ns, name, value) {
    this.modifications.push({
      attributeNamespace: ns,
      attributeName: name,
      newValue: value
    });
  },

  setAttribute: function(name, value) {
    this.setAttributeNS(undefined, name, value);
  },

  removeAttributeNS: function(ns, name) {
    this.setAttributeNS(ns, name, undefined);
  },

  removeAttribute: function(name) {
    this.setAttributeNS(undefined, name, undefined);
  }
})





var InspectorActor = protocol.ActorClass({
  typeName: "inspector",
  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;
  },

  get window() this.tabActor.window,

  getWalker: method(function(options={}) {
    if (this._walkerPromise) {
      return this._walkerPromise;
    }

    let deferred = promise.defer();
    this._walkerPromise = deferred.promise;

    let window = this.window;
    var domReady = () => {
      let tabActor = this.tabActor;
      window.removeEventListener("DOMContentLoaded", domReady, true);
      this.walker = WalkerActor(this.conn, tabActor, options);
      deferred.resolve(this.walker);
    };

    if (window.document.readyState === "loading") {
      window.addEventListener("DOMContentLoaded", domReady, true);
    } else {
      domReady();
    }

    return this._walkerPromise;
  }, {
    request: {},
    response: {
      walker: RetVal("domwalker")
    }
  }),

  getPageStyle: method(function() {
    if (this._pageStylePromise) {
      return this._pageStylePromise;
    }

    this._pageStylePromise = this.getWalker().then(walker => {
      return PageStyleActor(this);
    });
    return this._pageStylePromise;
  }, {
    request: {},
    response: { pageStyle: RetVal("pagestyle") }
  })
});





var InspectorFront = exports.InspectorFront = protocol.FrontClass(InspectorActor, {
  initialize: function(client, tabForm) {
    protocol.Front.prototype.initialize.call(this, client);
    this.actorID = tabForm.inspectorActor;

    
    
    client.addActorPool(this);
    this.manage(this);
  },

  getWalker: protocol.custom(function() {
    return this._getWalker().then(walker => {
      this.walker = walker;
      return walker;
    });
  }, {
    impl: "_getWalker"
  }),

  getPageStyle: protocol.custom(function() {
    return this._getPageStyle().then(pageStyle => {
      
      
      if (this.walker) {
        return pageStyle;
      }
      return this.getWalker().then(() => {
        return pageStyle;
      });
    });
  }, {
    impl: "_getPageStyle"
  })
});

function documentWalker(node, rootWin, whatToShow=Ci.nsIDOMNodeFilter.SHOW_ALL) {
  return new DocumentWalker(node, rootWin, whatToShow, whitespaceTextFilter, false);
}


exports._documentWalker = documentWalker;

function nodeDocument(node) {
  return node.ownerDocument || (node.nodeType == Ci.nsIDOMNode.DOCUMENT_NODE ? node : null);
}







function DocumentWalker(aNode, aRootWin, aShow, aFilter, aExpandEntityReferences)
{
  let doc = nodeDocument(aNode);
  this.layoutHelpers = new LayoutHelpers(aRootWin);
  this.walker = doc.createTreeWalker(doc,
    aShow, aFilter, aExpandEntityReferences);
  this.walker.currentNode = aNode;
  this.filter = aFilter;
}

DocumentWalker.prototype = {
  get node() this.walker.node,
  get whatToShow() this.walker.whatToShow,
  get expandEntityReferences() this.walker.expandEntityReferences,
  get currentNode() this.walker.currentNode,
  set currentNode(aVal) this.walker.currentNode = aVal,

  




  _reparentWalker: function DW_reparentWalker(aNewNode) {
    if (!aNewNode) {
      return null;
    }
    let doc = nodeDocument(aNewNode);
    let walker = doc.createTreeWalker(doc,
      this.whatToShow, this.filter, this.expandEntityReferences);
    walker.currentNode = aNewNode;
    this.walker = walker;
    return aNewNode;
  },

  parentNode: function DW_parentNode()
  {
    let currentNode = this.walker.currentNode;
    let parentNode = this.walker.parentNode();

    if (!parentNode) {
      if (currentNode && currentNode.nodeType == Ci.nsIDOMNode.DOCUMENT_NODE
          && currentNode.defaultView) {

        let window = currentNode.defaultView;
        let frame = this.layoutHelpers.getFrameElement(window);
        if (frame) {
          return this._reparentWalker(frame);
        }
      }
      return null;
    }

    return parentNode;
  },

  firstChild: function DW_firstChild()
  {
    let node = this.walker.currentNode;
    if (!node)
      return null;
    if (node.contentDocument) {
      return this._reparentWalker(node.contentDocument);
    } else if (node.getSVGDocument) {
      return this._reparentWalker(node.getSVGDocument());
    }
    return this.walker.firstChild();
  },

  lastChild: function DW_lastChild()
  {
    let node = this.walker.currentNode;
    if (!node)
      return null;
    if (node.contentDocument) {
      return this._reparentWalker(node.contentDocument);
    } else if (node.getSVGDocument) {
      return this._reparentWalker(node.getSVGDocument());
    }
    return this.walker.lastChild();
  },

  previousSibling: function DW_previousSibling() this.walker.previousSibling(),
  nextSibling: function DW_nextSibling() this.walker.nextSibling()
}




function whitespaceTextFilter(aNode)
{
    if (aNode.nodeType == Ci.nsIDOMNode.TEXT_NODE &&
        !/[^\s]/.exec(aNode.nodeValue)) {
      return Ci.nsIDOMNodeFilter.FILTER_SKIP;
    } else {
      return Ci.nsIDOMNodeFilter.FILTER_ACCEPT;
    }
}

loader.lazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});
