



"use strict";















































const {Cc, Ci, Cu, Cr} = require("chrome");
const Services = require("Services");
const protocol = require("devtools/server/protocol");
const {Arg, Option, method, RetVal, types} = protocol;
const {LongStringActor, ShortLongString} = require("devtools/server/actors/string");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const {Task} = Cu.import("resource://gre/modules/Task.jsm", {});
const object = require("sdk/util/object");
const events = require("sdk/event/core");
const {Unknown} = require("sdk/platform/xpcom");
const {Class} = require("sdk/core/heritage");
const {PageStyleActor, getFontPreviewData} = require("devtools/server/actors/styles");
const {
  HighlighterActor,
  CustomHighlighterActor,
  isTypeRegistered,
} = require("devtools/server/actors/highlighter");
const {getLayoutChangesObserver, releaseLayoutChangesObserver} =
  require("devtools/server/actors/layout");

const {EventParsers} = require("devtools/toolkit/event-parsers");

const FONT_FAMILY_PREVIEW_TEXT = "The quick brown fox jumps over the lazy dog";
const FONT_FAMILY_PREVIEW_TEXT_SIZE = 20;
const PSEUDO_CLASSES = [":hover", ":active", ":focus"];
const HIDDEN_CLASS = "__fx-devtools-hide-shortcut__";
const XHTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = 'http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul';
const IMAGE_FETCHING_TIMEOUT = 500;


const PSEUDO_SELECTORS = [
  [":active", 1],
  [":hover", 1],
  [":focus", 1],
  [":visited", 0],
  [":link", 0],
  [":first-letter", 0],
  [":first-child", 2],
  [":before", 2],
  [":after", 2],
  [":lang(", 0],
  [":not(", 3],
  [":first-of-type", 0],
  [":last-of-type", 0],
  [":only-of-type", 0],
  [":only-child", 2],
  [":nth-child(", 3],
  [":nth-last-child(", 0],
  [":nth-of-type(", 0],
  [":nth-last-of-type(", 0],
  [":last-child", 2],
  [":root", 0],
  [":empty", 0],
  [":target", 0],
  [":enabled", 0],
  [":disabled", 0],
  [":checked", 1],
  ["::selection", 0]
];


let HELPER_SHEET = ".__fx-devtools-hide-shortcut__ { visibility: hidden !important } ";
HELPER_SHEET += ":-moz-devtools-highlighted { outline: 2px dashed #F06!important; outline-offset: -2px!important } ";

Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm");

loader.lazyImporter(this, "gDevTools", "resource:///modules/devtools/gDevTools.jsm");

loader.lazyGetter(this, "DOMParser", function() {
  return Cc["@mozilla.org/xmlextras/domparser;1"].createInstance(Ci.nsIDOMParser);
});

loader.lazyGetter(this, "eventListenerService", function() {
  return Cc["@mozilla.org/eventlistenerservice;1"]
           .getService(Ci.nsIEventListenerService);
});

loader.lazyGetter(this, "CssLogic", () => require("devtools/styleinspector/css-logic").CssLogic);



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

types.addDictType("imageData", {
  
  data: "nullable:longstring",
  
  size: "json"
});





exports.DEFAULT_VALUE_SUMMARY_LENGTH = 50;
var gValueSummaryLength = exports.DEFAULT_VALUE_SUMMARY_LENGTH;

exports.getValueSummaryLength = function() {
  return gValueSummaryLength;
};

exports.setValueSummaryLength = function(val) {
  gValueSummaryLength = val;
};







var gInspectingNode = null;



exports.setInspectingNode = function(val) {
  gInspectingNode = val;
};




var NodeActor = exports.NodeActor = protocol.ActorClass({
  typeName: "domnode",

  initialize: function(walker, node) {
    protocol.Actor.prototype.initialize.call(this, null);
    this.walker = walker;
    this.rawNode = node;
    this._eventParsers = new EventParsers().parsers;

    
    
    this.wasDisplayed = this.isDisplayed;
  },

  toString: function() {
    return "[NodeActor " + this.actorID + " for " + this.rawNode.toString() + "]";
  },

  



  get conn() this.walker.conn,

  isDocumentElement: function() {
    return this.rawNode.ownerDocument &&
           this.rawNode.ownerDocument.documentElement === this.rawNode;
  },

  
  form: function(detail) {
    if (detail === "actorid") {
      return this.actorID;
    }

    let parentNode = this.walker.parentNode(this);

    let form = {
      actor: this.actorID,
      baseURI: this.rawNode.baseURI,
      parent: parentNode ? parentNode.actorID : undefined,
      nodeType: this.rawNode.nodeType,
      namespaceURI: this.rawNode.namespaceURI,
      nodeName: this.rawNode.nodeName,
      numChildren: this.numChildren,

      
      name: this.rawNode.name,
      publicId: this.rawNode.publicId,
      systemId: this.rawNode.systemId,

      attrs: this.writeAttrs(),
      isBeforePseudoElement: this.isBeforePseudoElement,
      isAfterPseudoElement: this.isAfterPseudoElement,
      isAnonymous: LayoutHelpers.isAnonymous(this.rawNode),
      isNativeAnonymous: LayoutHelpers.isNativeAnonymous(this.rawNode),
      isXBLAnonymous: LayoutHelpers.isXBLAnonymous(this.rawNode),
      isShadowAnonymous: LayoutHelpers.isShadowAnonymous(this.rawNode),
      pseudoClassLocks: this.writePseudoClassLocks(),

      isDisplayed: this.isDisplayed,

      hasEventListeners: this._hasEventListeners,
    };

    if (this.isDocumentElement()) {
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

  get isBeforePseudoElement() {
    return this.rawNode.nodeName === "_moz_generated_content_before"
  },

  get isAfterPseudoElement() {
    return this.rawNode.nodeName === "_moz_generated_content_after"
  },

  
  
  get numChildren() {
    
    
    if (this.isBeforePseudoElement || this.isAfterPseudoElement) {
      return 0;
    }

    let numChildren = this.rawNode.childNodes.length;
    if (numChildren === 0 &&
        (this.rawNode.contentDocument || this.rawNode.getSVGDocument)) {
      
      numChildren = 1;
    }

    
    if (this.rawNode.nodeType === Ci.nsIDOMNode.ELEMENT_NODE) {
      let anonChildren = this.rawNode.ownerDocument.getAnonymousNodes(this.rawNode);
      if (anonChildren) {
        numChildren += anonChildren.length;
      }
    }

    
    
    if (numChildren === 0) {
      numChildren = this.walker.children(this).nodes.length;
    }

    return numChildren;
  },

  get computedStyle() {
    return CssLogic.getComputedStyle(this.rawNode);
  },

  


  get isDisplayed() {
    
    if (isNodeDead(this) ||
        this.rawNode.nodeType !== Ci.nsIDOMNode.ELEMENT_NODE ||
        this.isAfterPseudoElement ||
        this.isBeforePseudoElement) {
      return true;
    }

    let style = this.computedStyle;
    if (!style) {
      return true;
    } else {
      return style.display !== "none";
    }
  },

  




  get _hasEventListeners() {
    let parsers = this._eventParsers;
    for (let [,{hasListeners}] of parsers) {
      try {
        if (hasListeners && hasListeners(this.rawNode)) {
          return true;
        }
      } catch(e) {
        
        
      }
    }
    return false;
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

  





  getEventListeners: function(node) {
    let parsers = this._eventParsers;
    let dbg = this.parent().tabActor.makeDebugger();
    let events = [];

    for (let [,{getListeners, normalizeHandler}] of parsers) {
      try {
        let eventInfos = getListeners(node);

        if (!eventInfos) {
          continue;
        }

        for (let eventInfo of eventInfos) {
          if (normalizeHandler) {
            eventInfo.normalizeHandler = normalizeHandler;
          }

          this.processHandlerForEvent(node, events, dbg, eventInfo);
        }
      } catch(e) {
        
        
      }
    }

    events.sort((a, b) => {
      return a.type.localeCompare(b.type);
    });

    return events;
  },

  




























  processHandlerForEvent: function(node, events, dbg, eventInfo) {
    let type = eventInfo.type || "";
    let handler = eventInfo.handler;
    let tags = eventInfo.tags || "";
    let hide = eventInfo.hide || {};
    let override = eventInfo.override || {};
    let global = Cu.getGlobalForObject(handler);
    let globalDO = dbg.addDebuggee(global);
    let listenerDO = globalDO.makeDebuggeeValue(handler);

    if (eventInfo.normalizeHandler) {
      listenerDO = eventInfo.normalizeHandler(listenerDO);
    }

    
    if (listenerDO.class === "Object" || listenerDO.class === "XULElement") {
      let desc;

      while (!desc && listenerDO) {
        desc = listenerDO.getOwnPropertyDescriptor("handleEvent");
        listenerDO = listenerDO.proto;
      }

      if (desc && desc.value) {
        listenerDO = desc.value;
      }
    }

    if (listenerDO.isBoundFunction) {
      listenerDO = listenerDO.boundTargetFunction;
    }

    let script = listenerDO.script;
    let scriptSource = script.source.text;
    let functionSource =
      scriptSource.substr(script.sourceStart, script.sourceLength);

    











    let scriptBeforeFunc = scriptSource.substr(0, script.sourceStart);
    let lastEnding = Math.max(
      scriptBeforeFunc.lastIndexOf(";"),
      scriptBeforeFunc.lastIndexOf("}"),
      scriptBeforeFunc.lastIndexOf("{"),
      scriptBeforeFunc.lastIndexOf("("),
      scriptBeforeFunc.lastIndexOf(","),
      scriptBeforeFunc.lastIndexOf("!")
    );

    if (lastEnding !== -1) {
      let functionPrefix = scriptBeforeFunc.substr(lastEnding + 1);
      functionSource = functionPrefix + functionSource;
    }

    let dom0 = false;

    if (typeof node.hasAttribute !== "undefined") {
      dom0 = !!node.hasAttribute("on" + type);
    } else {
      dom0 = !!node["on" + type];
    }

    let line = script.startLine;
    let url = script.url;
    let origin = url + (dom0 ? "" : ":" + line);
    let searchString;

    if (dom0) {
      searchString = "on" + type + "=\"" + script.source.text + "\"";
    } else {
      scriptSource = "    " + scriptSource;
    }

    let eventObj = {
      type: typeof override.type !== "undefined" ? override.type : type,
      handler: functionSource.trim(),
      origin: typeof override.origin !== "undefined" ?
                     override.origin : origin,
      searchString: typeof override.searchString !== "undefined" ?
                           override.searchString : searchString,
      tags: tags,
      DOM0: typeof override.dom0 !== "undefined" ? override.dom0 : dom0,
      capturing: typeof override.capturing !== "undefined" ?
                        override.capturing : eventInfo.capturing,
      hide: hide
    };

    events.push(eventObj);

    dbg.removeDebuggee(globalDO);
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

  


  getUniqueSelector: method(function() {
    return CssLogic.findCssSelector(this.rawNode);
  }, {
    request: {},
    response: {
      value: RetVal("string")
    }
  }),

  


  scrollIntoView: method(function() {
    this.rawNode.scrollIntoView(true);
  }, {
    request: {},
    response: {}
  }),

  










  getImageData: method(function(maxDim) {
    
    try {
      let imageData = imageToImageData(this.rawNode, maxDim);
      return promise.resolve({
        data: LongStringActor(this.conn, imageData.data),
        size: imageData.size
      });
    } catch(e) {
      return promise.reject(new Error("Image not available"));
    }
  }, {
    request: {maxDim: Arg(0, "nullable:number")},
    response: RetVal("imageData")
  }),

  


  getEventListenerInfo: method(function() {
    if (this.rawNode.nodeName.toLowerCase() === "html") {
      return this.getEventListeners(this.rawNode.ownerGlobal);
    }
    return this.getEventListeners(this.rawNode);
  }, {
    request: {},
    response: {
      events: RetVal("json")
    }
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

  






  getFontFamilyDataURL: method(function(font, fillStyle="black") {
    let doc = this.rawNode.ownerDocument;
    let options = {
      previewText: FONT_FAMILY_PREVIEW_TEXT,
      previewFontSize: FONT_FAMILY_PREVIEW_TEXT_SIZE,
      fillStyle: fillStyle
    }
    let { dataURL, size } = getFontPreviewData(font, doc, options);

    return { data: LongStringActor(this.conn, dataURL), size: size };
  }, {
    request: {font: Arg(0, "string"), fillStyle: Arg(1, "nullable:string")},
    response: RetVal("imageData")
  })
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
      this.observer.disconnect();
      this.observer = null;
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

  get baseURI() this._form.baseURI,

  get className() {
    return this.getAttribute("class") || '';
  },

  get hasChildren() this._form.numChildren > 0,
  get numChildren() this._form.numChildren,
  get hasEventListeners() this._form.hasEventListeners,

  get isBeforePseudoElement() this._form.isBeforePseudoElement,
  get isAfterPseudoElement() this._form.isAfterPseudoElement,
  get isPseudoElement() this.isBeforePseudoElement || this.isAfterPseudoElement,
  get isAnonymous() this._form.isAnonymous,

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

  get isDisplayed() {
    
    
    return "isDisplayed" in this._form ? this._form.isDisplayed : true;
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

  






  isLocal_toBeDeprecated: function() {
    return !!this.conn._transport._serverConnection;
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
    this.nodeList = nodeList || [];
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
      length: this.nodeList ? this.nodeList.length : 0
    }
  },

  


  item: method(function(index) {
    return this.walker.attachElement(this.nodeList[index]);
  }, {
    request: { item: Arg(0) },
    response: RetVal("disconnectedNode")
  }),

  


  items: method(function(start=0, end=this.nodeList.length) {
    let items = [this.walker._ref(item) for (item of Array.prototype.slice.call(this.nodeList, start, end))];
    return this.walker.attachElements(items);
  }, {
    request: {
      start: Arg(0, "nullable:number"),
      end: Arg(1, "nullable:number")
    },
    response: RetVal("disconnectedNodeArray")
  }),

  release: method(function() {}, { release: true })
});




var NodeListFront = exports.NodeListFront = protocol.FrontClass(NodeListActor, {
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




var WalkerActor = protocol.ActorClass({
  typeName: "domwalker",

  events: {
    "new-mutations" : {
      type: "newMutations"
    },
    "picker-node-picked" : {
      type: "pickerNodePicked",
      node: Arg(0, "disconnectedNode")
    },
    "picker-node-hovered" : {
      type: "pickerNodeHovered",
      node: Arg(0, "disconnectedNode")
    },
    "picker-node-canceled" : {
      type: "pickerNodeCanceled"
    },
    "highlighter-ready" : {
      type: "highlighter-ready"
    },
    "highlighter-hide" : {
      type: "highlighter-hide"
    },
    "display-change" : {
      type: "display-change",
      nodes: Arg(0, "array:domnode")
    }
  },

  




  initialize: function(conn, tabActor, options) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;
    this.rootWin = tabActor.window;
    this.rootDoc = this.rootWin.document;
    this._refMap = new Map();
    this._pendingMutations = [];
    this._activePseudoClassLocks = new Set();
    this.showAllAnonymousContent = options.showAllAnonymousContent;

    this.layoutHelpers = new LayoutHelpers(this.rootWin);

    
    
    
    this._orphaned = new Set();

    
    
    
    this._retainedOrphans = new Set();

    this.onMutations = this.onMutations.bind(this);
    this.onFrameLoad = this.onFrameLoad.bind(this);
    this.onFrameUnload = this.onFrameUnload.bind(this);

    events.on(tabActor, "will-navigate", this.onFrameUnload);
    events.on(tabActor, "navigate", this.onFrameLoad);

    
    
    this.rootNode = this.document();

    this.reflowObserver = getLayoutChangesObserver(this.tabActor);
    this._onReflows = this._onReflows.bind(this);
    this.reflowObserver.on("reflows", this._onReflows);
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

  getDocumentWalker: function(node, whatToShow) {
    
    let nodeFilter = this.showAllAnonymousContent ? allAnonymousContentTreeWalkerFilter : standardTreeWalkerFilter;
    return new DocumentWalker(node, this.rootWin, whatToShow, nodeFilter);
  },

  destroy: function() {
    try {
      this._destroyed = true;

      this.clearPseudoClassLocks();
      this._activePseudoClassLocks = null;

      this._hoveredNode = null;
      this.rootDoc = null;

      this.reflowObserver.off("reflows", this._onReflows);
      this.reflowObserver = null;
      releaseLayoutChangesObserver(this.tabActor);

      events.emit(this, "destroyed");
    } catch(e) {
      console.error(e);
    }
    protocol.Actor.prototype.destroy.call(this);
  },

  release: method(function() {}, { release: true }),

  unmanage: function(actor) {
    if (actor instanceof NodeActor) {
      if (this._activePseudoClassLocks &&
          this._activePseudoClassLocks.has(actor)) {
        this.clearPseudoClassLocks(actor);
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

  _onReflows: function(reflows) {
    
    
    let changes = [];
    for (let [node, actor] of this._refMap) {
      if (Cu.isDeadWrapper(node)) {
        continue;
      }

      let isDisplayed = actor.isDisplayed;
      if (isDisplayed !== actor.wasDisplayed) {
        changes.push(actor);
        
        actor.wasDisplayed = isDisplayed;
      }
    }

    if (changes.length) {
      events.emit(this, "display-change", changes);
    }
  },

  

















  pick: method(function() {}, {request: {}, response: RetVal("disconnectedNode")}),
  cancelPick: method(function() {}),
  highlight: method(function(node) {}, {request: {node: Arg(0, "nullable:domnode")}}),

  





  attachElement: function(node) {
    let { nodes, newParents } = this.attachElements([node]);
    return {
      node: nodes[0],
      newParents: newParents
    };
  },

  





  attachElements: function(nodes) {
    let nodeActors = [];
    let newParents = new Set();
    for (let node of nodes) {
      
      if (!(node instanceof NodeActor))
        node = this._ref(node);

      this.ensurePathToRoot(node, newParents);
      
      
      nodeActors.push(node);
    }

    return {
      nodes: nodeActors,
      newParents: [...newParents]
    };
  },

  



  _watchDocument: function(actor) {
    let node = actor.rawNode;
    
    
    actor.observer = new actor.rawNode.defaultView.MutationObserver(this.onMutations);
    actor.observer.observe(node, {
      attributes: true,
      characterData: true,
      childList: true,
      subtree: true
    });
  },

  






  document: method(function(node) {
    let doc = isNodeDead(node) ? this.rootDoc : nodeDocument(node.rawNode);
    return this._ref(doc);
  }, {
    request: { node: Arg(0, "nullable:domnode") },
    response: { node: RetVal("domnode") },
  }),

  






  documentElement: method(function(node) {
    let elt = isNodeDead(node)
              ? this.rootDoc.documentElement
              : nodeDocument(node.rawNode).documentElement;
    return this._ref(elt);
  }, {
    request: { node: Arg(0, "nullable:domnode") },
    response: { node: RetVal("domnode") },
  }),

  











  parents: method(function(node, options={}) {
    if (isNodeDead(node)) {
      return [];
    }

    let walker = this.getDocumentWalker(node.rawNode);
    let parents = [];
    let cur;
    while((cur = walker.parentNode())) {
      if (options.sameDocument && nodeDocument(cur) != nodeDocument(node.rawNode)) {
        break;
      }

      if (options.sameTypeRootTreeItem &&
          nodeDocshell(cur).sameTypeRootTreeItem != nodeDocshell(node.rawNode).sameTypeRootTreeItem) {
        break;
      }

      parents.push(this._ref(cur));
    }
    return parents;
  }, {
    request: {
      node: Arg(0, "domnode"),
      sameDocument: Option(1),
      sameTypeRootTreeItem: Option(1)
    },
    response: {
      nodes: RetVal("array:domnode")
    },
  }),

  parentNode: function(node) {
    let walker = this.getDocumentWalker(node.rawNode);
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
    if (isNodeDead(node)) {
      return;
    }

    if (node.retained && !options.force) {
      this._retainedOrphans.add(node);
      return;
    }

    if (node.retained) {
      
      this._retainedOrphans.delete(node);
    }

    let walker = this.getDocumentWalker(node.rawNode);

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
    let walker = this.getDocumentWalker(node.rawNode);
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
    if (isNodeDead(node)) {
      return { hasFirst: true, hasLast: true, nodes: [] };
    }

    if (options.center && options.start) {
      throw Error("Can't specify both 'center' and 'start' options.");
    }
    let maxNodes = options.maxNodes || -1;
    if (maxNodes == -1) {
      maxNodes = Number.MAX_VALUE;
    }

    
    
    let getFilteredWalker = (node) => {
      return this.getDocumentWalker(node, options.whatToShow);
    }

    
    let rawNode = node.rawNode;
    let firstChild = getFilteredWalker(rawNode).firstChild();
    let lastChild = getFilteredWalker(rawNode).lastChild();

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

    
    let backwardWalker = getFilteredWalker(start);
    if (start != firstChild && options.center) {
      backwardWalker.previousSibling();
      let backwardCount = Math.floor(maxNodes / 2);
      let backwardNodes = this._readBackward(backwardWalker, backwardCount);
      nodes = backwardNodes;
    }

    
    let forwardWalker = getFilteredWalker(start);
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
    if (isNodeDead(node)) {
      return { hasFirst: true, hasLast: true, nodes: [] };
    }

    let parentNode = this.getDocumentWalker(node.rawNode, options.whatToShow).parentNode();
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
    if (isNodeDead(node)) {
      return null;
    }

    let walker = this.getDocumentWalker(node.rawNode, options.whatToShow);
    let sibling = walker.nextSibling();
    return sibling ? this._ref(sibling) : null;
  }, traversalMethod),

  








  previousSibling: method(function(node, options={}) {
    if (isNodeDead(node)) {
      return null;
    }

    let walker = this.getDocumentWalker(node.rawNode, options.whatToShow);
    let sibling = walker.previousSibling();
    return sibling ? this._ref(sibling) : null;
  }, traversalMethod),

  



  _readForward: function(walker, count) {
    let ret = [];
    let node = walker.currentNode;
    do {
      ret.push(this._ref(node));
      node = walker.nextSibling();
    } while (node && --count);
    return ret;
  },

  



  _readBackward: function(walker, count) {
    let ret = [];
    let node = walker.currentNode;
    do {
      ret.push(this._ref(node));
      node = walker.previousSibling();
    } while(node && --count);
    ret.reverse();
    return ret;
  },

  






  findInspectingNode: method(function() {
    let node = gInspectingNode;
    if (!node) {
      return {}
    };

    return this.attachElement(node);
  }, {
    request: {},
    response: RetVal("disconnectedNode")
  }),

  






  querySelector: method(function(baseNode, selector) {
    if (isNodeDead(baseNode)) {
      return {};
    }

    let node = baseNode.rawNode.querySelector(selector);
    if (!node) {
      return {}
    };

    return this.attachElement(node);
  }, {
    request: {
      node: Arg(0, "domnode"),
      selector: Arg(1)
    },
    response: RetVal("disconnectedNode")
  }),

  






  querySelectorAll: method(function(baseNode, selector) {
    let nodeList = null;

    try {
      nodeList = baseNode.rawNode.querySelectorAll(selector);
    } catch(e) {
      
    }

    return new NodeListActor(this, nodeList);
  }, {
    request: {
      node: Arg(0, "domnode"),
      selector: Arg(1)
    },
    response: {
      list: RetVal("domnodelist")
    }
  }),

  





  _multiFrameQuerySelectorAll: function(selector) {
    let nodes = [];

    for (let {document} of this.tabActor.windows) {
      try {
        nodes = [...nodes, ...document.querySelectorAll(selector)];
      } catch(e) {
        
      }
    }

    return nodes;
  },

  




  multiFrameQuerySelectorAll: method(function(selector) {
    return new NodeListActor(this, this._multiFrameQuerySelectorAll(selector));
  }, {
    request: {
      selector: Arg(0)
    },
    response: {
      list: RetVal("domnodelist")
    }
  }),

  









  getSuggestionsForQuery: method(function(query, completing, selectorState) {
    let sugs = {
      classes: new Map,
      tags: new Map,
      ids: new Map
    };
    let result = [];
    let nodes = null;
    
    switch (selectorState) {
      case "pseudo":
        result = PSEUDO_SELECTORS.filter(item => {
          return item[0].startsWith(":" + completing);
        });
        break;

      case "class":
        if (!query) {
          nodes = this._multiFrameQuerySelectorAll("[class]");
        }
        else {
          nodes = this._multiFrameQuerySelectorAll(query);
        }
        for (let node of nodes) {
          for (let className of node.className.split(" ")) {
            sugs.classes.set(className, (sugs.classes.get(className)|0) + 1);
          }
        }
        sugs.classes.delete("");
        
        
        
        sugs.classes.delete("moz-styleeditor-transitioning");
        sugs.classes.delete(HIDDEN_CLASS);
        for (let [className, count] of sugs.classes) {
          if (className.startsWith(completing)) {
            result.push(["." + CSS.escape(className), count, selectorState]);
          }
        }
        break;

      case "id":
        if (!query) {
          nodes = this._multiFrameQuerySelectorAll("[id]");
        }
        else {
          nodes = this._multiFrameQuerySelectorAll(query);
        }
        for (let node of nodes) {
          sugs.ids.set(node.id, (sugs.ids.get(node.id)|0) + 1);
        }
        for (let [id, count] of sugs.ids) {
          if (id.startsWith(completing)) {
            result.push(["#" + CSS.escape(id), count, selectorState]);
          }
        }
        break;

      case "tag":
        if (!query) {
          nodes = this._multiFrameQuerySelectorAll("*");
        }
        else {
          nodes = this._multiFrameQuerySelectorAll(query);
        }
        for (let node of nodes) {
          let tag = node.tagName.toLowerCase();
          sugs.tags.set(tag, (sugs.tags.get(tag)|0) + 1);
        }
        for (let [tag, count] of sugs.tags) {
          if ((new RegExp("^" + completing + ".*", "i")).test(tag)) {
            result.push([tag, count, selectorState]);
          }
        }

        
        
        if (!query) {
          result = [
            ...result,
            ...this.getSuggestionsForQuery(null, completing, "class").suggestions,
            ...this.getSuggestionsForQuery(null, completing, "id").suggestions
          ];
        }

        break;

      case "null":
        nodes = this._multiFrameQuerySelectorAll(query);
        for (let node of nodes) {
          sugs.ids.set(node.id, (sugs.ids.get(node.id)|0) + 1);
          let tag = node.tagName.toLowerCase();
          sugs.tags.set(tag, (sugs.tags.get(tag)|0) + 1);
          for (let className of node.className.split(" ")) {
            sugs.classes.set(className, (sugs.classes.get(className)|0) + 1);
          }
        }
        for (let [tag, count] of sugs.tags) {
          tag && result.push([tag, count]);
        }
        for (let [id, count] of sugs.ids) {
          id && result.push(["#" + id, count]);
        }
        sugs.classes.delete("");
        
        
        
        sugs.classes.delete("moz-styleeditor-transitioning");
        sugs.classes.delete(HIDDEN_CLASS);
        for (let [className, count] of sugs.classes) {
          className && result.push(["." + className, count]);
        }
    }

    
    result = result.sort((a, b) => {
      
      let sortA = (10000-a[1]) + a[0];
      let sortB = (10000-b[1]) + b[0];

      
      let firstA = a[0].substring(0, 1);
      let firstB = b[0].substring(0, 1);

      if (firstA === "#") {
        sortA = "2" + sortA;
      }
      else if (firstA === ".") {
        sortA = "1" + sortA;
      }
      else {
        sortA = "0" + sortA;
      }

      if (firstB === "#") {
        sortB = "2" + sortB;
      }
      else if (firstB === ".") {
        sortB = "1" + sortB;
      }
      else {
        sortB = "0" + sortB;
      }

      
      return sortA.localeCompare(sortB);
    });

    result.slice(0, 25);

    return {
      query: query,
      suggestions: result
    };
  }, {
    request: {
      query: Arg(0),
      completing: Arg(1),
      selectorState: Arg(2)
    },
    response: {
      list: RetVal("array:array:string")
    }
  }),

  













  addPseudoClassLock: method(function(node, pseudo, options={}) {
    if (isNodeDead(node)) {
      return;
    }

    this._addPseudoClassLock(node, pseudo);

    if (!options.parents) {
      return;
    }

    let walker = this.getDocumentWalker(node.rawNode);
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
    if (isNodeDead(node)) {
      return;
    }

    this._installHelperSheet(node);
    node.rawNode.classList.add(HIDDEN_CLASS);
  }, {
    request: { node: Arg(0, "domnode") }
  }),

  unhideNode: method(function(node) {
    if (isNodeDead(node)) {
      return;
    }

    node.rawNode.classList.remove(HIDDEN_CLASS);
  }, {
    request: { node: Arg(0, "domnode") }
  }),

  













  removePseudoClassLock: method(function(node, pseudo, options={}) {
    if (isNodeDead(node)) {
      return;
    }

    this._removePseudoClassLock(node, pseudo);

    if (!options.parents) {
      return;
    }

    let walker = this.getDocumentWalker(node.rawNode);
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
    if (node && isNodeDead(node)) {
      return;
    }

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
    let html = "";
    if (!isNodeDead(node)) {
      html = node.rawNode.innerHTML;
    }
    return LongStringActor(this.conn, html);
  }, {
    request: {
      node: Arg(0, "domnode")
    },
    response: {
      value: RetVal("longstring")
    }
  }),

  





  setInnerHTML: method(function(node, value) {
    if (isNodeDead(node)) {
      return;
    }

    let rawNode = node.rawNode;
    if (rawNode.nodeType !== rawNode.ownerDocument.ELEMENT_NODE)
      throw new Error("Can only change innerHTML to element nodes");
    rawNode.innerHTML = value;
  }, {
    request: {
      node: Arg(0, "domnode"),
      value: Arg(1, "string"),
    },
    response: {}
  }),

  




  outerHTML: method(function(node) {
    let html = "";
    if (!isNodeDead(node)) {
      html = node.rawNode.outerHTML;
    }
    return LongStringActor(this.conn, html);
  }, {
    request: {
      node: Arg(0, "domnode")
    },
    response: {
      value: RetVal("longstring")
    }
  }),

  





  setOuterHTML: method(function(node, value) {
    if (isNodeDead(node)) {
      return;
    }

    let parsedDOM = DOMParser.parseFromString(value, "text/html");
    let rawNode = node.rawNode;
    let parentNode = rawNode.parentNode;

    
    
    
    
    if (rawNode.tagName === "BODY") {
      if (parsedDOM.head.innerHTML === "") {
        parentNode.replaceChild(parsedDOM.body, rawNode);
      } else {
        rawNode.outerHTML = value;
      }
    } else if (rawNode.tagName === "HEAD") {
      if (parsedDOM.body.innerHTML === "") {
        parentNode.replaceChild(parsedDOM.head, rawNode);
      } else {
        rawNode.outerHTML = value;
      }
    } else if (node.isDocumentElement()) {
      
      
      let finalAttributeModifications = [];
      let attributeModifications = {};
      for (let attribute of rawNode.attributes) {
        attributeModifications[attribute.name] = null;
      }
      for (let attribute of parsedDOM.documentElement.attributes) {
        attributeModifications[attribute.name] = attribute.value;
      }
      for (let key in attributeModifications) {
        finalAttributeModifications.push({
          attributeName: key,
          newValue: attributeModifications[key]
        });
      }
      node.modifyAttributes(finalAttributeModifications);
      rawNode.replaceChild(parsedDOM.head, rawNode.querySelector("head"));
      rawNode.replaceChild(parsedDOM.body, rawNode.querySelector("body"));
    } else {
      rawNode.outerHTML = value;
    }
  }, {
    request: {
      node: Arg(0, "domnode"),
      value: Arg(1, "string"),
    },
    response: {}
  }),

  







  insertAdjacentHTML: method(function(node, position, value) {
    if (isNodeDead(node)) {
      return {node: [], newParents: []}
    }

    let rawNode = node.rawNode;
    
    
    if (node.isDocumentElement()) {
      throw new Error("Can't insert adjacent element to the root.");
    }

    let isInsertAsSibling = position === "beforeBegin" ||
      position === "afterEnd";
    if ((rawNode.tagName === "BODY" || rawNode.tagName === "HEAD") &&
      isInsertAsSibling) {
      throw new Error("Can't insert element before or after the body " +
        "or the head.");
    }

    let rawParentNode = rawNode.parentNode;
    if (!rawParentNode && isInsertAsSibling) {
      throw new Error("Can't insert as sibling without parent node.");
    }

    
    
    
    let range = rawNode.ownerDocument.createRange();
    if (position === "beforeBegin" || position === "afterEnd") {
      range.selectNode(rawNode);
    } else {
      range.selectNodeContents(rawNode);
    }
    let docFrag = range.createContextualFragment(value);
    let newRawNodes = Array.from(docFrag.childNodes);
    switch (position) {
      case "beforeBegin":
        rawParentNode.insertBefore(docFrag, rawNode);
        break;
      case "afterEnd":
        
        
        rawParentNode.insertBefore(docFrag, rawNode.nextSibling);
      case "afterBegin":
        rawNode.insertBefore(docFrag, rawNode.firstChild);
        break;
      case "beforeEnd":
        rawNode.appendChild(docFrag);
        break;
      default:
        throw new Error('Invalid position value. Must be either ' +
          '"beforeBegin", "beforeEnd", "afterBegin" or "afterEnd".');
    }

    return this.attachElements(newRawNodes);
  }, {
    request: {
      node: Arg(0, "domnode"),
      position: Arg(1, "string"),
      value: Arg(2, "string")
    },
    response: RetVal("disconnectedNodeArray")
  }),

  





  isDocumentOrDocumentElementNode: function(node) {
      return ((node.rawNode.ownerDocument &&
        node.rawNode.ownerDocument.documentElement === this.rawNode) ||
        node.rawNode.nodeType === Ci.nsIDOMNode.DOCUMENT_NODE);
  },

  





  removeNode: method(function(node) {
    if (isNodeDead(node) || this.isDocumentOrDocumentElementNode(node)) {
      throw Error("Cannot remove document, document elements or dead nodes.");
    }

    let nextSibling = this.nextSibling(node);
    node.rawNode.remove();
    
    return nextSibling;
  }, {
    request: {
      node: Arg(0, "domnode")
    },
    response: {
      nextSibling: RetVal("nullable:domnode")
    }
  }),

  




  removeNodes: method(function(nodes) {
    
    for (let node of nodes) {
      if (isNodeDead(node) || this.isDocumentOrDocumentElementNode(node)) {
        throw Error("Cannot remove document, document elements or dead nodes");
      }
    }

    for (let node of nodes) {
      node.rawNode.remove();
      
    }
  }, {
    request: {
      node: Arg(0, "array:domnode")
    },
    response: {}
  }),

  


  insertBefore: method(function(node, parent, sibling) {
    if (isNodeDead(node) ||
        isNodeDead(parent) ||
        (sibling && isNodeDead(sibling))) {
      return null;
    }

    parent.rawNode.insertBefore(node.rawNode, sibling ? sibling.rawNode : null);
  }, {
    request: {
      node: Arg(0, "domnode"),
      parent: Arg(1, "domnode"),
      sibling: Arg(2, "nullable:domnode")
    },
    response: {}
  }),

  





  editTagName: method(function(node, tagName) {
    if (isNodeDead(node)) {
      return;
    }

    let oldNode = node.rawNode;

    
    
    let newNode;
    try {
      newNode = nodeDocument(oldNode).createElement(tagName);
    } catch(x) {
      
      
      return Promise.reject(new Error("Could not change node's tagName to " + tagName));
    }

    let attrs = oldNode.attributes;
    for (let i = 0; i < attrs.length; i ++) {
      newNode.setAttribute(attrs[i].name, attrs[i].value);
    }

    
    oldNode.parentNode.insertBefore(newNode, oldNode);
    while (oldNode.firstChild) {
      newNode.appendChild(oldNode.firstChild);
    }

    oldNode.remove();
  }, {
    request: {
      node: Arg(0, "domnode"),
      tagName: Arg(1, "string")
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


    
    
    let targetMap = {};
    let filtered = pending.reverse().filter(mutation => {
      if (mutation.type === "attributes") {
        if (!targetMap[mutation.target]) {
          targetMap[mutation.target] = {};
        }
        let attributesForTarget = targetMap[mutation.target];

        if (attributesForTarget[mutation.attributeName]) {
          
          
          return false;
        }

        attributesForTarget[mutation.attributeName] = true;
      }
      return true;
    }).reverse();

    return filtered;
  }, {
    request: {
      cleanup: Option(0)
    },
    response: {
      mutations: RetVal("array:dommutation")
    }
  }),

  queueMutation: function(mutation) {
    if (!this.actorID || this._destroyed) {
      
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
        numChildren: targetActor.numChildren
      };

      if (mutation.type === "attributes") {
        mutation.attributeName = change.attributeName;
        mutation.attributeNamespace = change.attributeNamespace || undefined;
        mutation.newValue = targetNode.hasAttribute(mutation.attributeName) ?
                            targetNode.getAttribute(mutation.attributeName)
                            : null;
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

        mutation.removed = removedActors;
        mutation.added = addedActors;
      }
      this.queueMutation(mutation);
    }
  },

  onFrameLoad: function({ window, isTopLevel }) {
    if (!this.rootDoc && isTopLevel) {
      this.rootDoc = window.document;
      this.rootNode = this.document();
      this.queueMutation({
        type: "newRoot",
        target: this.rootNode.form()
      });
      return;
    }
    let frame = this.layoutHelpers.getFrameElement(window);
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

  onFrameUnload: function({ window }) {
    
    
    
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

    let walker = this.getDocumentWalker(doc);
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
  },

  





  _isInDOMTree: function(rawNode) {
    let walker = this.getDocumentWalker(rawNode);
    let current = walker.currentNode;

    
    while (walker.parentNode()) {
      current = walker.currentNode;
    }

    
    
    if (current.nodeType === Ci.nsIDOMNode.DOCUMENT_FRAGMENT_NODE ||
        current !== this.rootDoc) {
      return false;
    }

    
    return true;
  },

  


  isInDOMTree: method(function(node) {
    if (isNodeDead(node)) {
      return false;
    }
    return this._isInDOMTree(node.rawNode);
  }, {
    request: { node: Arg(0, "domnode") },
    response: { attached: RetVal("boolean") }
  }),

  



  getNodeActorFromObjectActor: method(function(objectActorID) {
    let actor = this.conn.getActor(objectActorID);
    if (!actor) {
      return null;
    }

    let debuggerObject = this.conn.getActor(objectActorID).obj;
    let rawNode = debuggerObject.unsafeDereference();

    if (!this._isInDOMTree(rawNode)) {
      return null;
    }

    
    
    if (rawNode.defaultView && rawNode === rawNode.defaultView.document) {
      rawNode = rawNode.documentElement;
    }

    return this.attachElement(rawNode);
  }, {
    request: {
      objectActorID: Arg(0, "string")
    },
    response: {
      nodeFront: RetVal("nullable:disconnectedNode")
    }
  }),

  




  getStyleSheetOwnerNode: method(function(styleSheetActorID) {
    let styleSheetActor = this.conn.getActor(styleSheetActorID);
    let ownerNode = styleSheetActor.ownerNode;

    if (!styleSheetActor || !ownerNode) {
      return null;
    }

    return this.attachElement(ownerNode);
  }, {
    request: {
      styleSheetActorID: Arg(0, "string")
    },
    response: {
      ownerNode: RetVal("nullable:disconnectedNode")
    }
  }),
});




var WalkerFront = exports.WalkerFront = protocol.FrontClass(WalkerActor, {
  
  autoCleanup: true,

  



  pick: protocol.custom(function() {
    return this._pick().then(response => {
      return response.node;
    });
  }, {impl: "_pick"}),

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

  findInspectingNode: protocol.custom(function() {
    return this._findInspectingNode().then(response => {
      return response.node;
    });
  }, {
    impl: "_findInspectingNode"
  }),

  querySelector: protocol.custom(function(queryNode, selector) {
    return this._querySelector(queryNode, selector).then(response => {
      return response.node;
    });
  }, {
    impl: "_querySelector"
  }),

  getNodeActorFromObjectActor: protocol.custom(function(objectActorID) {
    return this._getNodeActorFromObjectActor(objectActorID).then(response => {
      return response ? response.node : null;
    });
  }, {
    impl: "_getNodeActorFromObjectActor"
  }),

  getStyleSheetOwnerNode: protocol.custom(function(styleSheetActorID) {
    return this._getStyleSheetOwnerNode(styleSheetActorID).then(response => {
      return response ? response.node : null;
    });
  }, {
    impl: "_getStyleSheetOwnerNode"
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

          
          
          
          if ('numChildren' in change) {
            targetFront._form.numChildren = change.numChildren;
          }
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
    
    this.getMutations({cleanup: this.autoCleanup}).catch(() => {});
  }),

  isLocal: function() {
    return !!this.conn._transport._serverConnection;
  },

  
  
  frontForRawNode: function(rawNode) {
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
    let extras = walkerActor.parents(nodeActor, {sameTypeRootTreeItem: true});
    for (let extraActor of extras) {
      top = nodeType.read(nodeType.write(extraActor, walkerActor), this);
    }

    if (top !== this.rootNode) {
      
      this._orphaned.add(top);
      walkerActor._orphaned.add(this.conn._transport._serverConnection.getActor(top.actorID));
    }
    return returnNode;
  },

  removeNode: protocol.custom(Task.async(function* (node) {
    let previousSibling = yield this.previousSibling(node);
    let nextSibling = yield this._removeNode(node);
    return {
      previousSibling: previousSibling,
      nextSibling: nextSibling,
    };
  }), {
    impl: "_removeNode"
  }),
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





var InspectorActor = exports.InspectorActor = protocol.ActorClass({
  typeName: "inspector",
  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;
  },

  destroy: function () {
    protocol.Actor.prototype.destroy.call(this);
    this._highlighterPromise = null;
    this._pageStylePromise = null;
    this._walkerPromise = null;
    this.tabActor = null;
  },

  disconnect: function () {
    this.destroy();
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
      events.once(this.walker, "destroyed", () => {
        this._walkerPromise = null;
        this._pageStylePromise = null;
      });
      deferred.resolve(this.walker);
    };

    if (window.document.readyState === "loading") {
      window.addEventListener("DOMContentLoaded", domReady, true);
    } else {
      domReady();
    }

    return this._walkerPromise;
  }, {
    request: {
      options: Arg(0, "nullable:json")
    },
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
    response: {
      pageStyle: RetVal("pagestyle")
    }
  }),

  












  getHighlighter: method(function (autohide) {
    if (this._highlighterPromise) {
      return this._highlighterPromise;
    }

    this._highlighterPromise = this.getWalker().then(walker => {
      return HighlighterActor(this, autohide);
    });
    return this._highlighterPromise;
  }, {
    request: {
      autohide: Arg(0, "boolean")
    },
    response: {
      highligter: RetVal("highlighter")
    }
  }),

  










  getHighlighterByType: method(function (typeName) {
    if (isTypeRegistered(typeName)) {
      return CustomHighlighterActor(this, typeName);
    } else {
      return null;
    }
  }, {
    request: {
      typeName: Arg(0)
    },
    response: {
      highlighter: RetVal("nullable:customhighlighter")
    }
  }),

  










  getImageDataFromURL: method(function(url, maxDim) {
    let deferred = promise.defer();
    let img = new this.window.Image();

    
    img.onload = () => {
      
      try {
        let imageData = imageToImageData(img, maxDim);
        deferred.resolve({
          data: LongStringActor(this.conn, imageData.data),
          size: imageData.size
        });
      } catch (e) {
        deferred.reject(new Error("Image " + url+ " not available"));
      }
    }

    
    img.onerror = () => {
      deferred.reject(new Error("Image " + url+ " not available"));
    }

    
    
    if (!gDevTools.testing) {
      this.window.setTimeout(() => {
        deferred.reject(new Error("Image " + url + " could not be retrieved in time"));
      }, IMAGE_FETCHING_TIMEOUT);
    }

    img.src = url;

    return deferred.promise;
  }, {
    request: {url: Arg(0), maxDim: Arg(1, "nullable:number")},
    response: RetVal("imageData")
  })
});





var InspectorFront = exports.InspectorFront = protocol.FrontClass(InspectorActor, {
  initialize: function(client, tabForm) {
    protocol.Front.prototype.initialize.call(this, client);
    this.actorID = tabForm.inspectorActor;

    
    
    this.manage(this);
  },

  destroy: function() {
    delete this.walker;
    protocol.Front.prototype.destroy.call(this);
  },

  getWalker: protocol.custom(function(options = {}) {
    return this._getWalker(options).then(walker => {
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


exports._documentWalker = DocumentWalker;

function nodeDocument(node) {
  if (Cu.isDeadWrapper(node)) {
    return null;
  }
  return node.ownerDocument || (node.nodeType == Ci.nsIDOMNode.DOCUMENT_NODE ? node : null);
}

function nodeDocshell(node) {
  let doc = node ? nodeDocument(node) : null;
  let win = doc ? doc.defaultView : null;
  if (win) {
    return win.
           QueryInterface(Ci.nsIInterfaceRequestor).
           getInterface(Ci.nsIDocShell);
  }
}

function isNodeDead(node) {
  return !node || !node.rawNode || Cu.isDeadWrapper(node.rawNode);
}











function DocumentWalker(node, rootWin, whatToShow=Ci.nsIDOMNodeFilter.SHOW_ALL, filter=standardTreeWalkerFilter) {
  if (!rootWin.location) {
    throw new Error("Got an invalid root window in DocumentWalker");
  }

  this.walker = Cc["@mozilla.org/inspector/deep-tree-walker;1"].createInstance(Ci.inIDeepTreeWalker);
  this.walker.showAnonymousContent = true;
  this.walker.showSubDocuments = true;
  this.walker.showDocumentsAsNodes = true;
  this.walker.init(rootWin.document, whatToShow);
  this.walker.currentNode = node;
  this.filter = filter;
}

DocumentWalker.prototype = {
  get node() this.walker.node,
  get whatToShow() this.walker.whatToShow,
  get currentNode() this.walker.currentNode,
  set currentNode(aVal) this.walker.currentNode = aVal,

  parentNode: function() {
    return this.walker.parentNode();
  },

  firstChild: function() {
    let node = this.walker.currentNode;
    if (!node)
      return null;

    let firstChild = this.walker.firstChild();
    while (firstChild && this.filter(firstChild) === Ci.nsIDOMNodeFilter.FILTER_SKIP) {
      firstChild = this.walker.nextSibling();
    }

    return firstChild;
  },

  lastChild: function() {
    let node = this.walker.currentNode;
    if (!node)
      return null;

    let lastChild = this.walker.lastChild();
    while (lastChild && this.filter(lastChild) === Ci.nsIDOMNodeFilter.FILTER_SKIP) {
      lastChild = this.walker.previousSibling();
    }

    return lastChild;
  },

  previousSibling: function() {
    let node = this.walker.previousSibling();
    while (node && this.filter(node) === Ci.nsIDOMNodeFilter.FILTER_SKIP) {
      node = this.walker.previousSibling();
    }
    return node;
  },

  nextSibling: function() {
    let node = this.walker.nextSibling();
    while (node && this.filter(node) === Ci.nsIDOMNodeFilter.FILTER_SKIP) {
      node = this.walker.nextSibling();
    }
    return node;
  }
};

function isXULElement(el) {
  return el &&
         el.namespaceURI === XUL_NS;
}






function standardTreeWalkerFilter(aNode) {
  
  if (aNode.nodeType == Ci.nsIDOMNode.TEXT_NODE &&
      !/[^\s]/.exec(aNode.nodeValue)) {
    return Ci.nsIDOMNodeFilter.FILTER_SKIP;
  }

  
  
  
  
  
  
  if (LayoutHelpers.isNativeAnonymous(aNode) &&
      !isXULElement(aNode.parentNode) &&
      (
        aNode.nodeName !== "_moz_generated_content_before" &&
        aNode.nodeName !== "_moz_generated_content_after")
      ) {
    return Ci.nsIDOMNodeFilter.FILTER_SKIP;
  }

  return Ci.nsIDOMNodeFilter.FILTER_ACCEPT;
}





function allAnonymousContentTreeWalkerFilter(aNode) {
  
  if (aNode.nodeType == Ci.nsIDOMNode.TEXT_NODE &&
      !/[^\s]/.exec(aNode.nodeValue)) {
    return Ci.nsIDOMNodeFilter.FILTER_SKIP;
  }
  return Ci.nsIDOMNodeFilter.FILTER_ACCEPT
}










function imageToImageData(node, maxDim) {
  let isImg = node.tagName.toLowerCase() === "img";
  let isCanvas = node.tagName.toLowerCase() === "canvas";

  if (!isImg && !isCanvas) {
    return null;
  }

  
  let resizeRatio = 1;
  let imgWidth = node.naturalWidth || node.width;
  let imgHeight = node.naturalHeight || node.height;
  let imgMax = Math.max(imgWidth, imgHeight);
  if (maxDim && imgMax > maxDim) {
    resizeRatio = maxDim / imgMax;
  }

  
  let imageData;
  
  
  if (isImg && node.src.startsWith("data:")) {
    imageData = node.src;
  } else {
    
    let canvas = node.ownerDocument.createElementNS(XHTML_NS, "canvas");
    canvas.width = imgWidth * resizeRatio;
    canvas.height = imgHeight * resizeRatio;
    let ctx = canvas.getContext("2d");

    
    ctx.drawImage(node, 0, 0, canvas.width, canvas.height);
    imageData = canvas.toDataURL("image/png");
  }

  return {
    data: imageData,
    size: {
      naturalWidth: imgWidth,
      naturalHeight: imgHeight,
      resized: resizeRatio !== 1
    }
  }
}

loader.lazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});
