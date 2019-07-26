



"use strict";






































const {Cc, Ci, Cu} = require("chrome");

const protocol = require("devtools/server/protocol");
const {Arg, Option, method, RetVal, types} = protocol;
const {LongStringActor, ShortLongString} = require("devtools/server/actors/string");
const promise = require("sdk/core/promise");
const object = require("sdk/util/object");
const events = require("sdk/event/core");

Cu.import("resource://gre/modules/Services.jsm");

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
    let parentNode = this.walker.parentNode(this);

    let form = {
      actor: this.actorID,
      parent: parentNode ? parentNode.actorID : undefined,
      nodeType: this.rawNode.nodeType,
      namespaceURI: this.namespaceURI,
      nodeName: this.rawNode.nodeName,
      numChildren: this.rawNode.childNodes.length,

      
      name: this.rawNode.name,
      publicId: this.rawNode.publicId,
      systemId: this.rawNode.systemId,

      attrs: this.writeAttrs()
    };

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

    
    
    this.reparent(null);
    for (let child of this.treeChildren()) {
      child.destroy();
    }
    protocol.Front.prototype.destroy.call(this);
  },

  
  form: function(form, detail, ctx) {
    
    
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

  get attributes() this._form.attrs,

  getNodeValue: protocol.custom(function() {
    if (!this.incompleteValue) {
      return delayedResolve(new ShortLongString(this.shortValue));
    } else {
      return this._getNodeValue();
    }
  }, {
    impl: "_getNodeValue"
  }),

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
      throw new Error("Tried to use rawNode on a remote connection.");
    }
    let actor = this.conn._transport._serverConnection.getActor(this.actorID);
    if (!actor) {
      throw new Error("Could not find client side for actor " + this.actorID);
    }
    return actor.rawNode;
  }
});





types.addDictType("disconnectedNode", {
  
  node: "domnode",

  
  newNodes: "array:domnode"
});

types.addDictType("disconnectedNodeArray", {
  
  nodes: "array:domnode",

  
  newNodes: "array:domnode"
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
    let newNodes = [node for (node of this.walker.ensurePathToRoot(node))];
    return {
      node: node,
      newNodes: newNodes
    }
  }, {
    request: { item: Arg(0) },
    response: RetVal("disconnectedNode")
  }),

  


  items: method(function(start=0, end=this.nodeList.length) {
    let items = [this.walker._ref(item) for (item of Array.prototype.slice.call(this.nodeList, start, end))];
    let newNodes = new Set();
    for (let item of items) {
      this.walker.ensurePathToRoot(item, newNodes);
    }
    return {
      nodes: items,
      newNodes: [node for (node of newNodes)]
    }
  }, {
    request: {
      start: Arg(0, "number", { optional: true }),
      end: Arg(1, "number", { optional: true })
    },
    response: { nodes: RetVal("disconnectedNodeArray") }
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
    node: RetVal("domnode")
  }
}




var WalkerActor = protocol.ActorClass({
  typeName: "domwalker",

  events: {
    "new-mutations" : {
      type: "newMutations"
    }
  },

  




  initialize: function(conn, document, options) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.rootDoc = document;
    this._refMap = new Map();
    this._pendingMutations = [];

    
    
    
    this._orphaned = new Set();

    this.onMutations = this.onMutations.bind(this);

    
    
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
    protocol.Actor.prototype.destroy.call(this);
    this.rootDoc = null;
  },

  release: method(function() {}, { release: true }),

  unmanage: function(actor) {
    if (actor instanceof NodeActor) {
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
    request: { node: Arg(0, "domnode", {optional: true}) },
    response: { node: RetVal("domnode") },
  }),

  






  documentElement: method(function(node) {
    let elt = node ? nodeDocument(node.rawNode).documentElement : this.rootDoc.documentElement;
    return this._ref(elt);
  }, {
    request: { node: Arg(0, "domnode", {optional: true}) },
    response: { node: RetVal("domnode") },
  }),

  









  parents: method(function(node, options={}) {
    let walker = documentWalker(node.rawNode);
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
    let walker = documentWalker(node.rawNode);
    let parent = walker.parentNode();
    if (parent) {
      return this._ref(parent);
    }
    return null;
  },

  


  releaseNode: method(function(node) {
    let walker = documentWalker(node.rawNode);

    let child = walker.firstChild();
    while (child) {
      let childActor = this._refMap.get(child);
      if (childActor) {
        this.releaseNode(childActor);
      }
      child = walker.nextSibling();
    }

    node.destroy();
  }, {
    request: { node: Arg(0, "domnode") }
  }),

  



  ensurePathToRoot: function(node, newParents=new Set()) {
    if (!node) {
      return newParents;
    }
    let walker = documentWalker(node.rawNode);
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

    
    
    let filteredWalker = function(node) {
      return documentWalker(node, options.whatToShow);
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
    let parentNode = documentWalker(node.rawNode).parentNode();
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
    let walker = documentWalker(node.rawNode, options.whatToShow || Ci.nsIDOMNodeFilter.SHOW_ALL);
    return this._ref(walker.nextSibling());
  }, traversalMethod),

  








  previousSibling: method(function(node, options={}) {
    let walker = documentWalker(node.rawNode, options.whatToShow || Ci.nsIDOMNodeFilter.SHOW_ALL);
    return this._ref(walker.previousSibling());
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
      newNodes: [parent for (parent of newParents)]
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

  










































  getMutations: method(function() {
    let pending = this._pendingMutations || [];
    this._pendingMutations = [];
    return pending;
  }, {
    request: {},
    response: {
      mutations: RetVal("array:dommutation")
    }
  }),

  





  onMutations: function(mutations) {
    
    
    let needEvent = this._pendingMutations.length === 0;

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
      this._pendingMutations.push(mutation);
    }
    if (needEvent) {
      events.emit(this, "new-mutations");
    }
  }

});




var WalkerFront = exports.WalkerFront = protocol.FrontClass(WalkerActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
    this._orphaned = new Set();
  },

  destroy: function() {
    protocol.Front.prototype.destroy.call(this);
  },

  
  form: function(json) {
    this.actorID = json.actorID;
    this.rootNode = types.getType("domnode").read(json.root, this);
  },

  







  ensureParentFront: function(id) {
    let front = this.get(id);
    if (front) {
      return front;
    }

    return types.getType("domnode").read({ actor: id }, this, "standin");
  },

  releaseNode: protocol.custom(function(node) {
    
    
    let actorID = node.actorID;
    node.destroy();
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

  


  getMutations: protocol.custom(function() {
    return this._getMutations().then(mutations => {
      let emitMutations = [];
      for (let change of mutations) {
        
        let targetID = change.target;
        let targetFront = this.get(targetID);
        if (!targetFront) {
          console.error("Got a mutation for an unexpected actor: " + targetID);
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
        } else {
          targetFront.updateMutation(change);
        }

        emitMutations.push(emittedMutation);
      }
      events.emit(this, "mutations", emitMutations);
    });
  }, {
    impl: "_getMutations"
  }),

  



  onMutations: protocol.preEvent("new-mutations", function() {
    
    this.getMutations().then(null, console.error);
  }),

  
  
  frontForRawNode: function(rawNode){
    if (!this.conn._transport._serverConnection) {
      throw Error("Tried to use frontForRawNode on a remote connection.");
    }
    let actor = this.conn._transport._serverConnection.getActor(this.actorID);
    if (!actor) {
      throw Error("Could not find client side for actor " + this.actorID);
    }
    let nodeActor = actor._ref(rawNode);

    
    let nodeType = types.getType("domnode");
    return nodeType.read(nodeType.write(nodeActor, actor), this);
  }
});





var InspectorActor = protocol.ActorClass({
  typeName: "inspector",
  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;
    if (tabActor.browser instanceof Ci.nsIDOMWindow) {
      this.window = tabActor.browser;
    } else if (tabActor.browser instanceof Ci.nsIDOMElement) {
      this.window = tabActor.browser.contentWindow;
    }
  },

  getWalker: method(function(options={}) {
    return WalkerActor(this.conn, this.window.document, options);
  }, {
    request: {},
    response: {
      walker: RetVal("domwalker")
    }
  })
});





var InspectorFront = exports.InspectorFront = protocol.FrontClass(InspectorActor, {
  initialize: function(client, tabForm) {
    protocol.Front.prototype.initialize.call(this, client);
    this.actorID = tabForm.inspectorActor;

    
    
    client.addActorPool(this);
    this.manage(this);
  }
});

function documentWalker(node, whatToShow=Ci.nsIDOMNodeFilter.SHOW_ALL) {
  return new DocumentWalker(node, whatToShow, whitespaceTextFilter, false);
}


exports._documentWalker = documentWalker;

function nodeDocument(node) {
  return node.ownerDocument || (node.nodeType == Ci.nsIDOMNode.DOCUMENT_NODE ? node : null);
}







function DocumentWalker(aNode, aShow, aFilter, aExpandEntityReferences)
{
  let doc = nodeDocument(aNode);
  this.walker = doc.createTreeWalker(nodeDocument(aNode),
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
        let embeddingFrame = currentNode.defaultView.frameElement;
        if (embeddingFrame) {
          return this._reparentWalker(embeddingFrame);
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
