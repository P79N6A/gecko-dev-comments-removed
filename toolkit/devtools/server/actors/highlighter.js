



"use strict";

const {Cu, Cc, Ci} = require("chrome");
const Services = require("Services");
const protocol = require("devtools/server/protocol");
const {Arg, Option, method} = protocol;
const events = require("sdk/event/core");
const Heritage = require("sdk/core/heritage");
const {CssLogic} = require("devtools/styleinspector/css-logic");
const EventEmitter = require("devtools/toolkit/event-emitter");
const {setIgnoreLayoutChanges} = require("devtools/server/actors/layout");

Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");


const PSEUDO_CLASSES = [":hover", ":active", ":focus"];
const BOX_MODEL_REGIONS = ["margin", "border", "padding", "content"];
const BOX_MODEL_SIDES = ["top", "right", "bottom", "left"];
const SVG_NS = "http://www.w3.org/2000/svg";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const HIGHLIGHTER_STYLESHEET_URI = "resource://gre/modules/devtools/server/actors/highlighter.css";
const HIGHLIGHTER_PICKED_TIMER = 1000;

const NODE_INFOBAR_HEIGHT = 34; 
const NODE_INFOBAR_ARROW_SIZE = 9; 

const GUIDE_STROKE_WIDTH = 1;

const ARROW_LINE_MIN_DISTANCE = 10;


const MAX_HIGHLIGHTED_ELEMENTS = 100;

const HIGHLIGHTED_PSEUDO_CLASS = ":-moz-devtools-highlighted";
const SIMPLE_OUTLINE_SHEET = ".__fx-devtools-hide-shortcut__ {" +
                             "  visibility: hidden !important" +
                             "}" +
                             HIGHLIGHTED_PSEUDO_CLASS + " {" +
                             "  outline: 2px dashed #F06!important;" +
                             "  outline-offset: -2px!important;" +
                             "}";

const GEOMETRY_SIZE_ARROW_OFFSET = .25; 
const GEOMETRY_LABEL_SIZE = 6;



const RULERS_MAX_X_AXIS = 10000;
const RULERS_MAX_Y_AXIS = 15000;


const RULERS_GRADUATION_STEP = 5;
const RULERS_MARKER_STEP = 50;
const RULERS_TEXT_STEP = 100;










const highlighterTypes = new Map();





const isTypeRegistered = (typeName) => highlighterTypes.has(typeName);
exports.isTypeRegistered = isTypeRegistered;






const register = (constructor, typeName=constructor.prototype.typeName) => {
  if (!typeName) {
    throw Error("No type's name found, or provided.")
  }

  if (highlighterTypes.has(typeName)) {
    throw Error(`${typeName} is already registered.`)
  }

  highlighterTypes.set(typeName, constructor);
};
exports.register = register;



























let HighlighterActor = exports.HighlighterActor = protocol.ActorClass({
  typeName: "highlighter",

  initialize: function(inspector, autohide) {
    protocol.Actor.prototype.initialize.call(this, null);

    this._autohide = autohide;
    this._inspector = inspector;
    this._walker = this._inspector.walker;
    this._tabActor = this._inspector.tabActor;

    this._highlighterReady = this._highlighterReady.bind(this);
    this._highlighterHidden = this._highlighterHidden.bind(this);
    this._onNavigate = this._onNavigate.bind(this);

    this._layoutHelpers = new LayoutHelpers(this._tabActor.window);
    this._createHighlighter();

    
    
    events.on(this._tabActor, "navigate", this._onNavigate);
  },

  get conn() this._inspector && this._inspector.conn,

  _createHighlighter: function() {
    this._isPreviousWindowXUL = isXUL(this._tabActor);

    if (!this._isPreviousWindowXUL) {
      this._highlighter = new BoxModelHighlighter(this._tabActor,
                                                          this._inspector);
      this._highlighter.on("ready", this._highlighterReady);
      this._highlighter.on("hide", this._highlighterHidden);
    } else {
      this._highlighter = new SimpleOutlineHighlighter(this._tabActor);
    }
  },

  _destroyHighlighter: function() {
    if (this._highlighter) {
      if (!this._isPreviousWindowXUL) {
        this._highlighter.off("ready", this._highlighterReady);
        this._highlighter.off("hide", this._highlighterHidden);
      }
      this._highlighter.destroy();
      this._highlighter = null;
    }
  },

  _onNavigate: function({isTopLevel}) {
    
    
    if (!isTopLevel || !this._tabActor.window.document.documentElement) {
      return;
    }

    
    if (isXUL(this._tabActor) !== this._isPreviousWindowXUL) {
      this._destroyHighlighter();
      this._createHighlighter();
    }
  },

  destroy: function() {
    if (!this._inspector) {
      return;
    }
    protocol.Actor.prototype.destroy.call(this);

    this._destroyHighlighter();
    events.off(this._tabActor, "navigate", this._onNavigate);
    this._autohide = null;
    this._inspector = null;
    this._walker = null;
    this._tabActor = null;
    this._layoutHelpers = null;
  },

  disconnect: function () {
    this.destroy();
  },

  









  showBoxModel: method(function(node, options={}) {
    if (node && isNodeValid(node.rawNode)) {
      this._highlighter.show(node.rawNode, options);
    } else {
      this._highlighter.hide();
    }
  }, {
    request: {
      node: Arg(0, "domnode"),
      region: Option(1),
      hideInfoBar: Option(1),
      hideGuides: Option(1),
      showOnly: Option(1)
    }
  }),

  


  hideBoxModel: method(function() {
    this._highlighter.hide();
  }, {
    request: {}
  }),

  









  _isPicking: false,
  _hoveredNode: null,
  _currentNode: null,

  pick: method(function() {
    if (this._isPicking) {
      return null;
    }
    this._isPicking = true;

    this._preventContentEvent = event => {
      event.stopPropagation();
      event.preventDefault();
    };

    this._onPick = event => {
      this._preventContentEvent(event);
      this._stopPickerListeners();
      this._isPicking = false;
      if (this._autohide) {
        this._tabActor.window.setTimeout(() => {
          this._highlighter.hide();
        }, HIGHLIGHTER_PICKED_TIMER);
      }
      if (!this._currentNode) {
        this._currentNode = this._findAndAttachElement(event);
      }
      events.emit(this._walker, "picker-node-picked", this._currentNode);
    };

    this._onHovered = event => {
      this._preventContentEvent(event);
      this._currentNode = this._findAndAttachElement(event);
      if (this._hoveredNode !== this._currentNode.node) {
        this._highlighter.show( this._currentNode.node.rawNode);
        events.emit(this._walker, "picker-node-hovered", this._currentNode);
        this._hoveredNode = this._currentNode.node;
      }
    };

    this._onKey = event => {
      if (!this._currentNode || !this._isPicking) {
        return;
      }

      this._preventContentEvent(event);
      let currentNode = this._currentNode.node.rawNode;

      






      switch(event.keyCode) {
        case Ci.nsIDOMKeyEvent.DOM_VK_LEFT: 
          if (!currentNode.parentElement) {
            return;
          }
          currentNode = currentNode.parentElement;
          break;

        case Ci.nsIDOMKeyEvent.DOM_VK_RIGHT: 
          if (!currentNode.children.length) {
            return;
          }

          
          let child = currentNode.firstElementChild;
          
          
          let hoveredNode = this._hoveredNode.rawNode;
          for (let sibling of currentNode.children) {
            if (sibling.contains(hoveredNode) || sibling === hoveredNode) {
              child = sibling;
            }
          }

          currentNode = child;
          break;

        case Ci.nsIDOMKeyEvent.DOM_VK_RETURN: 
          this._onPick(event);
          return;

        case Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE: 
          this.cancelPick();
          events.emit(this._walker, "picker-node-canceled");
          return;

        default: return;
      }

      
      this._currentNode = this._walker.attachElement(currentNode);
      this._highlighter.show(this._currentNode.node.rawNode);
      events.emit(this._walker, "picker-node-hovered", this._currentNode);
    };

    this._tabActor.window.focus();
    this._startPickerListeners();

    return null;
  }),

  _findAndAttachElement: function(event) {
    let doc = event.target.ownerDocument;

    let x = event.clientX;
    let y = event.clientY;

    let node = doc.elementFromPoint(x, y);
    return this._walker.attachElement(node);
  },

  _startPickerListeners: function() {
    let target = getPageListenerTarget(this._tabActor);
    target.addEventListener("mousemove", this._onHovered, true);
    target.addEventListener("click", this._onPick, true);
    target.addEventListener("mousedown", this._preventContentEvent, true);
    target.addEventListener("mouseup", this._preventContentEvent, true);
    target.addEventListener("dblclick", this._preventContentEvent, true);
    target.addEventListener("keydown", this._onKey, true);
    target.addEventListener("keyup", this._preventContentEvent, true);
  },

  _stopPickerListeners: function() {
    let target = getPageListenerTarget(this._tabActor);
    target.removeEventListener("mousemove", this._onHovered, true);
    target.removeEventListener("click", this._onPick, true);
    target.removeEventListener("mousedown", this._preventContentEvent, true);
    target.removeEventListener("mouseup", this._preventContentEvent, true);
    target.removeEventListener("dblclick", this._preventContentEvent, true);
    target.removeEventListener("keydown", this._onKey, true);
    target.removeEventListener("keyup", this._preventContentEvent, true);
  },

  _highlighterReady: function() {
    events.emit(this._inspector.walker, "highlighter-ready");
  },

  _highlighterHidden: function() {
    events.emit(this._inspector.walker, "highlighter-hide");
  },

  cancelPick: method(function() {
    if (this._isPicking) {
      this._highlighter.hide();
      this._stopPickerListeners();
      this._isPicking = false;
      this._hoveredNode = null;
    }
  })
});

let HighlighterFront = protocol.FrontClass(HighlighterActor, {});





let CustomHighlighterActor = exports.CustomHighlighterActor = protocol.ActorClass({
  typeName: "customhighlighter",

  




  initialize: function(inspector, typeName) {
    protocol.Actor.prototype.initialize.call(this, null);

    this._inspector = inspector;

    let constructor = highlighterTypes.get(typeName);
    if (!constructor) {
      let list = [...highlighterTypes.keys()];

      throw new Error(`${typeName} isn't a valid highlighter class (${list})`);
      return;
    }

    
    
    if (!isXUL(this._inspector.tabActor)) {
      this._highlighter = new constructor(inspector.tabActor);
    } else {
      throw new Error("Custom " + typeName +
        "highlighter cannot be created in a XUL window");
      return;
    }
  },

  get conn() this._inspector && this._inspector.conn,

  destroy: function() {
    protocol.Actor.prototype.destroy.call(this);
    this.finalize();
  },

  














  show: method(function(node, options) {
    if (!node || !isNodeValid(node.rawNode) || !this._highlighter) {
      return;
    }

    this._highlighter.show(node.rawNode, options);
  }, {
    request: {
      node: Arg(0, "domnode"),
      options: Arg(1, "nullable:json")
    }
  }),

  


  hide: method(function() {
    if (this._highlighter) {
      this._highlighter.hide();
    }
  }, {
    request: {}
  }),

  



  finalize: method(function() {
    if (this._highlighter) {
      this._highlighter.destroy();
      this._highlighter = null;
    }
  }, {
    oneway: true
  })
});

let CustomHighlighterFront = protocol.FrontClass(CustomHighlighterActor, {});




















function CanvasFrameAnonymousContentHelper(tabActor, nodeBuilder) {
  this.tabActor = tabActor;
  this.nodeBuilder = nodeBuilder;
  this.anonymousContentDocument = this.tabActor.window.document;
  
  this.anonymousContentGlobal = Cu.getGlobalForObject(this.anonymousContentDocument);

  this._insert();

  this._onNavigate = this._onNavigate.bind(this);
  events.on(this.tabActor, "navigate", this._onNavigate);

  this.listeners = new Map();
}

exports.CanvasFrameAnonymousContentHelper = CanvasFrameAnonymousContentHelper;

CanvasFrameAnonymousContentHelper.prototype = {
  destroy: function() {
    
    
    try {
      let doc = this.anonymousContentDocument;
      doc.removeAnonymousContent(this._content);
    } catch (e) {}
    events.off(this.tabActor, "navigate", this._onNavigate);
    this.tabActor = this.nodeBuilder = this._content = null;
    this.anonymousContentDocument = null;
    this.anonymousContentGlobal = null;

    this._removeAllListeners();
  },

  _insert: function() {
    
    
    if (!this.tabActor.window.document.documentElement ||
        isXUL(this.tabActor)) {
      return;
    }
    let doc = this.tabActor.window.document;

    
    
    
    if (doc.hidden) {
      
      
      let onVisibilityChange = () => {
        doc.removeEventListener("visibilitychange", onVisibilityChange);
        this._insert();
      };
      doc.addEventListener("visibilitychange", onVisibilityChange);
      return;
    }

    
    
    
    
    installHelperSheet(this.tabActor.window,
      "@import url('" + HIGHLIGHTER_STYLESHEET_URI + "');");
    let node = this.nodeBuilder();
    this._content = doc.insertAnonymousContent(node);
  },

  _onNavigate: function({isTopLevel}) {
    if (isTopLevel) {
      this._removeAllListeners();
      this._insert();
      this.anonymousContentDocument = this.tabActor.window.document;
    }
  },

  getTextContentForElement: function(id) {
    if (!this.content) {
      return null;
    }
    return this.content.getTextContentForElement(id);
  },

  setTextContentForElement: function(id, text) {
    if (this.content) {
      this.content.setTextContentForElement(id, text);
    }
  },

  setAttributeForElement: function(id, name, value) {
    if (this.content) {
      this.content.setAttributeForElement(id, name, value);
    }
  },

  getAttributeForElement: function(id, name) {
    if (!this.content) {
      return null;
    }
    return this.content.getAttributeForElement(id, name);
  },

  removeAttributeForElement: function(id, name) {
    if (this.content) {
      this.content.removeAttributeForElement(id, name);
    }
  },

  



































  addEventListenerForElement: function(id, type, handler) {
    if (typeof id !== "string") {
      throw new Error("Expected a string ID in addEventListenerForElement but" +
        " got: " + id);
    }

    
    if (!this.listeners.has(type)) {
      let target = getPageListenerTarget(this.tabActor);
      target.addEventListener(type, this, true);
      
      this.listeners.set(type, new Map);
    }

    let listeners = this.listeners.get(type);
    listeners.set(id, handler);
  },

  






  removeEventListenerForElement: function(id, type, handler) {
    let listeners = this.listeners.get(type);
    if (!listeners) {
      return;
    }
    listeners.delete(id);

    
    if (!this.listeners.has(type)) {
      let target = getPageListenerTarget(this.tabActor);
      target.removeEventListener(type, this, true);
    }
  },

  handleEvent: function(event) {
    let listeners = this.listeners.get(event.type);
    if (!listeners) {
      return;
    }

    
    
    let isPropagationStopped = false;
    let eventProxy = new Proxy(event, {
      get: (obj, name) => {
        if (name === "originalTarget") {
          return null;
        } else if (name === "stopPropagation") {
          return () => {
            isPropagationStopped = true;
          };
        } else {
          return obj[name];
        }
      }
    });

    
    
    let node = event.originalTarget;
    while (node) {
      let handler = listeners.get(node.id);
      if (handler) {
        handler(eventProxy, node.id);
        if (isPropagationStopped) {
          break;
        }
      }
      node = node.parentNode;
    }
  },

  _removeAllListeners: function() {
    if (this.tabActor) {
      let target = getPageListenerTarget(this.tabActor);
      for (let [type] of this.listeners) {
        target.removeEventListener(type, this, true);
      }
    }
    this.listeners.clear();
  },

  getElement: function(id) {
    let self = this;
    return {
      getTextContent: () => self.getTextContentForElement(id),
      setTextContent: text => self.setTextContentForElement(id, text),
      setAttribute: (name, value) => self.setAttributeForElement(id, name, value),
      getAttribute: name => self.getAttributeForElement(id, name),
      removeAttribute: name => self.removeAttributeForElement(id, name),
      addEventListener: (type, handler) => {
        return self.addEventListenerForElement(id, type, handler);
      },
      removeEventListener: (type, handler) => {
        return self.removeEventListenerForElement(id, type, handler);
      }
    };
  },

  get content() {
    if (!this._content || Cu.isDeadWrapper(this._content)) {
      return null;
    }
    return this._content;
  },

  




















  scaleRootElement: function(node, id) {
    let zoom = LayoutHelpers.getCurrentZoom(node);
    let value = "position:absolute;width:100%;height:100%;";

    if (zoom !== 1) {
      value = "position:absolute;";
      value += "transform-origin:top left;transform:scale(" + (1/zoom) + ");";
      value += "width:" + (100*zoom) + "%;height:" + (100*zoom) + "%;";
    }

    this.setAttributeForElement(id, "style", value);
  }
};





















function AutoRefreshHighlighter(tabActor) {
  EventEmitter.decorate(this);

  this.tabActor = tabActor;
  this.win = tabActor.window;

  this.currentNode = null;
  this.currentQuads = {};

  this.layoutHelpers = new LayoutHelpers(this.win);

  this.update = this.update.bind(this);
}

AutoRefreshHighlighter.prototype = {
  





  show: function(node, options={}) {
    let isSameNode = node === this.currentNode;
    let isSameOptions = this._isSameOptions(options);

    if (!isNodeValid(node) || (isSameNode && isSameOptions)) {
      return;
    }

    this.options = options;

    this._stopRefreshLoop();
    this.currentNode = node;
    this._updateAdjustedQuads();
    this._startRefreshLoop();
    this._show();

    this.emit("shown");
  },

  


  hide: function() {
    if (!isNodeValid(this.currentNode)) {
      return;
    }

    this._hide();
    this._stopRefreshLoop();
    this.currentNode = null;
    this.currentQuads = {};
    this.options = null;

    this.emit("hidden");
  },

  



  _isSameOptions: function(options) {
    if (!this.options) {
      return false;
    }

    let keys = Object.keys(options);

    if (keys.length !== Object.keys(this.options).length) {
      return false;
    }

    for (let key of keys) {
      if (this.options[key] !== options[key]) {
        return false;
      }
    }

    return true;
  },

  


  _updateAdjustedQuads: function() {
    for (let region of BOX_MODEL_REGIONS) {
      this.currentQuads[region] = this.layoutHelpers.getAdjustedQuads(
        this.currentNode, region);
    }
  },

  




  _hasMoved: function() {
    let oldQuads = JSON.stringify(this.currentQuads);
    this._updateAdjustedQuads();
    let newQuads = JSON.stringify(this.currentQuads);
    return oldQuads !== newQuads;
  },

  


  update: function(e) {
    if (!isNodeValid(this.currentNode) || !this._hasMoved()) {
      return;
    }

    this._update();
    this.emit("updated");
  },

  _show: function() {
    
    
    
  },

  _update: function() {
    
    
    
    
  },

  _hide: function() {
    
    
  },

  _startRefreshLoop: function() {
    let win = getWindow(this.currentNode);
    this.rafID = win.requestAnimationFrame(this._startRefreshLoop.bind(this));
    this.rafWin = win;
    this.update();
  },

  _stopRefreshLoop: function() {
    if (this.rafID && !Cu.isDeadWrapper(this.rafWin)) {
      this.rafWin.cancelAnimationFrame(this.rafID);
    }
    this.rafID = this.rafWin = null;
  },

  destroy: function() {
    this.hide();

    this.tabActor = null;
    this.win = null;
    this.currentNode = null;
    this.layoutHelpers = null;
  }
};

























































function BoxModelHighlighter(tabActor) {
  AutoRefreshHighlighter.call(this, tabActor);

  this.markup = new CanvasFrameAnonymousContentHelper(this.tabActor,
    this._buildMarkup.bind(this));

  



  this.regionFill = {};

  this._currentNode = null;
}

BoxModelHighlighter.prototype = Heritage.extend(AutoRefreshHighlighter.prototype, {
  typeName: "BoxModelHighlighter",

  ID_CLASS_PREFIX: "box-model-",

  get currentNode() {
    return this._currentNode;
  },

  set currentNode(node) {
    this._currentNode = node;
    this._computedStyle = null;
  },

  _buildMarkup: function() {
    let doc = this.win.document;

    let highlighterContainer = doc.createElement("div");
    highlighterContainer.className = "highlighter-container";

    
    let rootWrapper = createNode(this.win, {
      parent: highlighterContainer,
      attributes: {
        "id": "root",
        "class": "root"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    

    let svg = createSVGNode(this.win, {
      nodeType: "svg",
      parent: rootWrapper,
      attributes: {
        "id": "elements",
        "width": "100%",
        "height": "100%",
        "hidden": "true"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    let regions = createSVGNode(this.win, {
      nodeType: "g",
      parent: svg,
      attributes: {
        "class": "regions"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    for (let region of BOX_MODEL_REGIONS) {
      createSVGNode(this.win, {
        nodeType: "path",
        parent: regions,
        attributes: {
          "class": region,
          "id": region
        },
        prefix: this.ID_CLASS_PREFIX
      });
    }

    for (let side of BOX_MODEL_SIDES) {
      createSVGNode(this.win, {
        nodeType: "line",
        parent: svg,
        attributes: {
          "class": "guide-" + side,
          "id": "guide-" + side,
          "stroke-width": GUIDE_STROKE_WIDTH
        },
        prefix: this.ID_CLASS_PREFIX
      });
    }

    

    let infobarContainer = createNode(this.win, {
      parent: rootWrapper,
      attributes: {
        "class": "nodeinfobar-container",
        "id": "nodeinfobar-container",
        "position": "top",
        "hidden": "true"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    let nodeInfobar = createNode(this.win, {
      parent: infobarContainer,
      attributes: {
        "class": "nodeinfobar"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    let texthbox = createNode(this.win, {
      parent: nodeInfobar,
      attributes: {
        "class": "nodeinfobar-text"
      },
      prefix: this.ID_CLASS_PREFIX
    });
    createNode(this.win, {
      nodeType: "span",
      parent: texthbox,
      attributes: {
        "class": "nodeinfobar-tagname",
        "id": "nodeinfobar-tagname"
      },
      prefix: this.ID_CLASS_PREFIX
    });
    createNode(this.win, {
      nodeType: "span",
      parent: texthbox,
      attributes: {
        "class": "nodeinfobar-id",
        "id": "nodeinfobar-id"
      },
      prefix: this.ID_CLASS_PREFIX
    });
    createNode(this.win, {
      nodeType: "span",
      parent: texthbox,
      attributes: {
        "class": "nodeinfobar-classes",
        "id": "nodeinfobar-classes"
      },
      prefix: this.ID_CLASS_PREFIX
    });
    createNode(this.win, {
      nodeType: "span",
      parent: texthbox,
      attributes: {
        "class": "nodeinfobar-pseudo-classes",
        "id": "nodeinfobar-pseudo-classes"
      },
      prefix: this.ID_CLASS_PREFIX
    });
    createNode(this.win, {
      nodeType: "span",
      parent: texthbox,
      attributes: {
        "class": "nodeinfobar-dimensions",
        "id": "nodeinfobar-dimensions"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    return highlighterContainer;
  },

  


  destroy: function() {
    AutoRefreshHighlighter.prototype.destroy.call(this);

    this.markup.destroy();

    this._currentNode = null;
  },

  getElement: function(id) {
    return this.markup.getElement(this.ID_CLASS_PREFIX + id);
  },

  


  _show: function() {
    if (BOX_MODEL_REGIONS.indexOf(this.options.region) == -1)  {
      this.options.region = "content";
    }

    this._update();
    this._trackMutations();
    this.emit("ready");
  },

  



  _trackMutations: function() {
    if (isNodeValid(this.currentNode)) {
      let win = getWindow(this.currentNode);
      this.currentNodeObserver = new win.MutationObserver(this.update);
      this.currentNodeObserver.observe(this.currentNode, {attributes: true});
    }
  },

  _untrackMutations: function() {
    if (isNodeValid(this.currentNode) && this.currentNodeObserver) {
      this.currentNodeObserver.disconnect();
      this.currentNodeObserver = null;
    }
  },

  




  _update: function() {
    setIgnoreLayoutChanges(true);

    if (this._updateBoxModel()) {
      if (!this.options.hideInfoBar) {
        this._showInfobar();
      } else {
        this._hideInfobar();
      }
      this._showBoxModel();
    } else {
      
      this._hide();
    }

    setIgnoreLayoutChanges(false, this.currentNode.ownerDocument.documentElement);
  },

  


  _hide: function() {
    setIgnoreLayoutChanges(true);

    this._untrackMutations();
    this._hideBoxModel();
    this._hideInfobar();

    setIgnoreLayoutChanges(false, this.currentNode.ownerDocument.documentElement);
  },

  


  _hideInfobar: function() {
    this.getElement("nodeinfobar-container").setAttribute("hidden", "true");
  },

  


  _showInfobar: function() {
    this.getElement("nodeinfobar-container").removeAttribute("hidden");
    this._updateInfobar();
  },

  


  _hideBoxModel: function() {
    this.getElement("elements").setAttribute("hidden", "true");
  },

  


  _showBoxModel: function() {
    this.getElement("elements").removeAttribute("hidden");
  },

  









  _getOuterQuad: function(region) {
    let quads = this.currentQuads[region];
    if (!quads.length) {
      return null;
    }

    let quad = {
      p1: {x: Infinity, y: Infinity},
      p2: {x: -Infinity, y: Infinity},
      p3: {x: -Infinity, y: -Infinity},
      p4: {x: Infinity, y: -Infinity},
      bounds: {
        bottom: -Infinity,
        height: 0,
        left: Infinity,
        right: -Infinity,
        top: Infinity,
        width: 0,
        x: 0,
        y: 0,
      }
    };

    for (let q of quads) {
      quad.p1.x = Math.min(quad.p1.x, q.p1.x);
      quad.p1.y = Math.min(quad.p1.y, q.p1.y);
      quad.p2.x = Math.max(quad.p2.x, q.p2.x);
      quad.p2.y = Math.min(quad.p2.y, q.p2.y);
      quad.p3.x = Math.max(quad.p3.x, q.p3.x);
      quad.p3.y = Math.max(quad.p3.y, q.p3.y);
      quad.p4.x = Math.min(quad.p4.x, q.p4.x);
      quad.p4.y = Math.max(quad.p4.y, q.p4.y);

      quad.bounds.bottom = Math.max(quad.bounds.bottom, q.bounds.bottom);
      quad.bounds.top = Math.min(quad.bounds.top, q.bounds.top);
      quad.bounds.left = Math.min(quad.bounds.left, q.bounds.left);
      quad.bounds.right = Math.max(quad.bounds.right, q.bounds.right);
    }
    quad.bounds.x = quad.bounds.left;
    quad.bounds.y = quad.bounds.top;
    quad.bounds.width = quad.bounds.right - quad.bounds.left;
    quad.bounds.height = quad.bounds.bottom - quad.bounds.top;

    return quad;
  },

  





  _updateBoxModel: function() {
    this.options.region = this.options.region || "content";

    if (this._nodeNeedsHighlighting()) {
      for (let boxType of BOX_MODEL_REGIONS) {
        let box = this.getElement(boxType);

        if (this.regionFill[boxType]) {
          box.setAttribute("style", "fill:" + this.regionFill[boxType]);
        } else {
          box.setAttribute("style", "");
        }

        if (!this.options.showOnly || this.options.showOnly === boxType) {
          
          let path = [];
          for (let {p1, p2, p3, p4} of this.currentQuads[boxType]) {
            path.push("M" + p1.x + "," + p1.y + " " +
                      "L" + p2.x + "," + p2.y + " " +
                      "L" + p3.x + "," + p3.y + " " +
                      "L" + p4.x + "," + p4.y);
          }

          box.setAttribute("d", path.join(" "));
        } else {
          box.removeAttribute("d");
        }

        if (boxType === this.options.region && !this.options.hideGuides) {
          this._showGuides(boxType);
        } else if (this.options.hideGuides) {
          this._hideGuides();
        }
      }

      
      let rootId = this.ID_CLASS_PREFIX + "root";
      this.markup.scaleRootElement(this.currentNode, rootId);

      return true;
    }

    this._hideBoxModel();
    return false;
  },

  _nodeNeedsHighlighting: function() {
    let hasNoQuads = !this.currentQuads.margin.length &&
                     !this.currentQuads.border.length &&
                     !this.currentQuads.padding.length &&
                     !this.currentQuads.content.length;
    if (!this.currentNode ||
        Cu.isDeadWrapper(this.currentNode) ||
        this.currentNode.nodeType !== Ci.nsIDOMNode.ELEMENT_NODE ||
        !this.currentNode.ownerDocument ||
        !getWindow(this.currentNode) ||
        hasNoQuads) {
      return false;
    }

    if (!this._computedStyle) {
      this._computedStyle = CssLogic.getComputedStyle(this.currentNode);
    }

    return this._computedStyle.getPropertyValue("display") !== "none";
  },

  _getOuterBounds: function() {
    for (let region of ["margin", "border", "padding", "content"]) {
      let quad = this._getOuterQuad(region);

      if (!quad) {
        
        break;
      }

      let {bottom, height, left, right, top, width, x, y} = quad.bounds;

      if (width > 0 || height > 0) {
        return {bottom, height, left, right, top, width, x, y};
      }
    }

    return {
      bottom: 0,
      height: 0,
      left: 0,
      right: 0,
      top: 0,
      width: 0,
      x: 0,
      y: 0
    };
  },

  




  _showGuides: function(region) {
    let {p1, p2, p3, p4} = this._getOuterQuad(region);

    let allX = [p1.x, p2.x, p3.x, p4.x].sort((a, b) => a - b);
    let allY = [p1.y, p2.y, p3.y, p4.y].sort((a, b) => a - b);
    let toShowX = [];
    let toShowY = [];

    for (let arr of [allX, allY]) {
      for (let i = 0; i < arr.length; i++) {
        let val = arr[i];

        if (i !== arr.lastIndexOf(val)) {
          if (arr === allX) {
            toShowX.push(val);
          } else {
            toShowY.push(val);
          }
          arr.splice(arr.lastIndexOf(val), 1);
        }
      }
    }

    
    this._updateGuide("top", toShowY[0]);
    this._updateGuide("right", toShowX[1]);
    this._updateGuide("bottom", toShowY[1]);
    this._updateGuide("left", toShowX[0]);
  },

  _hideGuides: function() {
    for (let side of BOX_MODEL_SIDES) {
      this.getElement("guide-" + side).setAttribute("hidden", "true");
    }
  },

  








  _updateGuide: function(side, point=-1) {
    let guide = this.getElement("guide-" + side);

    if (point <= 0) {
      guide.setAttribute("hidden", "true");
      return false;
    }

    if (side === "top" || side === "bottom") {
      guide.setAttribute("x1", "0");
      guide.setAttribute("y1", point + "");
      guide.setAttribute("x2", "100%");
      guide.setAttribute("y2", point + "");
    } else {
      guide.setAttribute("x1", point + "");
      guide.setAttribute("y1", "0");
      guide.setAttribute("x2", point + "");
      guide.setAttribute("y2", "100%");
    }

    guide.removeAttribute("hidden");

    return true;
  },

  


  _updateInfobar: function() {
    if (!this.currentNode) {
      return;
    }

    let {bindingElement:node, pseudo} =
      CssLogic.getBindingElementAndPseudo(this.currentNode);

    
    let tagName = node.tagName;

    let id = node.id ? "#" + node.id : "";

    let classList = (node.classList || []).length ? "." + [...node.classList].join(".") : "";

    let pseudos = PSEUDO_CLASSES.filter(pseudo => {
      return DOMUtils.hasPseudoClassLock(node, pseudo);
    }, this).join("");
    if (pseudo) {
      
      pseudos += ":" + pseudo;
    }

    let rect = this._getOuterQuad("border").bounds;
    let dim = parseFloat(rect.width.toPrecision(6)) + " \u00D7 " + parseFloat(rect.height.toPrecision(6));

    this.getElement("nodeinfobar-tagname").setTextContent(tagName);
    this.getElement("nodeinfobar-id").setTextContent(id);
    this.getElement("nodeinfobar-classes").setTextContent(classList);
    this.getElement("nodeinfobar-pseudo-classes").setTextContent(pseudos);
    this.getElement("nodeinfobar-dimensions").setTextContent(dim);

    this._moveInfobar();
  },

  


  _moveInfobar: function() {
    let bounds = this._getOuterBounds();
    let winHeight = this.win.innerHeight * LayoutHelpers.getCurrentZoom(this.win);
    let winWidth = this.win.innerWidth * LayoutHelpers.getCurrentZoom(this.win);

    
    
    let containerBottom = Math.max(0, bounds.bottom) + NODE_INFOBAR_ARROW_SIZE;
    let containerTop = Math.min(winHeight, bounds.top);
    let container = this.getElement("nodeinfobar-container");

    
    let top;
    if (containerTop < NODE_INFOBAR_HEIGHT) {
      
      if (containerBottom + NODE_INFOBAR_HEIGHT > winHeight) {
        
        top = containerTop;
        container.setAttribute("position", "overlap");
      } else {
        
        top = containerBottom;
        container.setAttribute("position", "bottom");
      }
    } else {
      
      top = containerTop - NODE_INFOBAR_HEIGHT;
      container.setAttribute("position", "top");
    }

    
    let left = bounds.right - bounds.width / 2;
    
    let buffer = 100;
    if (left < buffer) {
      left = buffer;
      container.setAttribute("hide-arrow", "true");
    } else if (left > winWidth - buffer) {
      left = winWidth - buffer;
      container.setAttribute("hide-arrow", "true");
    } else {
      container.removeAttribute("hide-arrow");
    }

    let style = "top:" + top + "px;left:" + left + "px;";
    container.setAttribute("style", style);
  }
});
register(BoxModelHighlighter);
exports.BoxModelHighlighter = BoxModelHighlighter;






function CssTransformHighlighter(tabActor) {
  AutoRefreshHighlighter.call(this, tabActor);

  this.markup = new CanvasFrameAnonymousContentHelper(this.tabActor,
    this._buildMarkup.bind(this));
}

let MARKER_COUNTER = 1;

CssTransformHighlighter.prototype = Heritage.extend(AutoRefreshHighlighter.prototype, {
  typeName: "CssTransformHighlighter",

  ID_CLASS_PREFIX: "css-transform-",

  _buildMarkup: function() {
    let doc = this.win.document;

    let container = createNode(this.win, {
      attributes: {
        "class": "highlighter-container"
      }
    });

    
    let rootWrapper = createNode(this.win, {
      parent: container,
      attributes: {
        "id": "root",
        "class": "root"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    let svg = createSVGNode(this.win, {
      nodeType: "svg",
      parent: rootWrapper,
      attributes: {
        "id": "elements",
        "hidden": "true",
        "width": "100%",
        "height": "100%"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    
    this.markerId = "arrow-marker-" + MARKER_COUNTER;
    MARKER_COUNTER ++;
    let marker = createSVGNode(this.win, {
      nodeType: "marker",
      parent: svg,
      attributes: {
        "id": this.markerId,
        "markerWidth": "10",
        "markerHeight": "5",
        "orient": "auto",
        "markerUnits": "strokeWidth",
        "refX": "10",
        "refY": "5",
        "viewBox": "0 0 10 10"
      },
      prefix: this.ID_CLASS_PREFIX
    });
    createSVGNode(this.win, {
      nodeType: "path",
      parent: marker,
      attributes: {
        "d": "M 0 0 L 10 5 L 0 10 z",
        "fill": "#08C"
      }
    });

    let shapesGroup = createSVGNode(this.win, {
      nodeType: "g",
      parent: svg
    });

    
    createSVGNode(this.win, {
      nodeType: "polygon",
      parent: shapesGroup,
      attributes: {
        "id": "untransformed",
        "class": "untransformed"
      },
      prefix: this.ID_CLASS_PREFIX
    });
    createSVGNode(this.win, {
      nodeType: "polygon",
      parent: shapesGroup,
      attributes: {
        "id": "transformed",
        "class": "transformed"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    
    for (let nb of ["1", "2", "3", "4"]) {
      createSVGNode(this.win, {
        nodeType: "line",
        parent: shapesGroup,
        attributes: {
          "id": "line" + nb,
          "class": "line",
          "marker-end": "url(#" + this.markerId + ")"
        },
        prefix: this.ID_CLASS_PREFIX
      });
    }

    return container;
  },

  


  destroy: function() {
    AutoRefreshHighlighter.prototype.destroy.call(this);
    this.markup.destroy();
  },

  getElement: function(id) {
    return this.markup.getElement(this.ID_CLASS_PREFIX + id);
  },

  



  _show: function() {
    if (!this._isTransformed(this.currentNode)) {
      this.hide();
      return;
    }

    this._update();
  },

  


  _isTransformed: function(node) {
    let style = CssLogic.getComputedStyle(node);
    return style && (style.transform !== "none" && style.display !== "inline");
  },

  _setPolygonPoints: function(quad, id) {
    let points = [];
    for (let point of ["p1","p2", "p3", "p4"]) {
      points.push(quad[point].x + "," + quad[point].y);
    }
    this.getElement(id).setAttribute("points", points.join(" "));
  },

  _setLinePoints: function(p1, p2, id) {
    let line = this.getElement(id);
    line.setAttribute("x1", p1.x);
    line.setAttribute("y1", p1.y);
    line.setAttribute("x2", p2.x);
    line.setAttribute("y2", p2.y);

    let dist = Math.sqrt(Math.pow(p2.x - p1.x, 2) + Math.pow(p2.y - p1.y, 2));
    if (dist < ARROW_LINE_MIN_DISTANCE) {
      line.removeAttribute("marker-end");
    } else {
      line.setAttribute("marker-end", "url(#" + this.markerId + ")");
    }
  },

  




  _update: function() {
    setIgnoreLayoutChanges(true);

    
    let quads = this.currentQuads.border;
    if (!quads.length ||
        quads[0].bounds.width <= 0 || quads[0].bounds.height <= 0) {
      this._hideShapes();
      return null;
    }

    let [quad] = quads;

    
    let untransformedQuad = this.layoutHelpers.getNodeBounds(this.currentNode);

    this._setPolygonPoints(quad, "transformed");
    this._setPolygonPoints(untransformedQuad, "untransformed");
    for (let nb of ["1", "2", "3", "4"]) {
      this._setLinePoints(untransformedQuad["p" + nb], quad["p" + nb], "line" + nb);
    }

    
    this.markup.scaleRootElement(this.currentNode, this.ID_CLASS_PREFIX + "root");

    this._showShapes();

    setIgnoreLayoutChanges(false, this.currentNode.ownerDocument.documentElement);
  },

  


  _hide: function() {
    setIgnoreLayoutChanges(true);
    this._hideShapes();
    setIgnoreLayoutChanges(false, this.currentNode.ownerDocument.documentElement);
  },

  _hideShapes: function() {
    this.getElement("elements").setAttribute("hidden", "true");
  },

  _showShapes: function() {
    this.getElement("elements").removeAttribute("hidden");
  }
});
register(CssTransformHighlighter);
exports.CssTransformHighlighter = CssTransformHighlighter;






function SelectorHighlighter(tabActor) {
  this.tabActor = tabActor;
  this._highlighters = [];
}

SelectorHighlighter.prototype = {
  typeName: "SelectorHighlighter",

  








  show: function(node, options={}) {
    this.hide();

    if (!isNodeValid(node) || !options.selector) {
      return;
    }

    let nodes = [];
    try {
      nodes = [...node.ownerDocument.querySelectorAll(options.selector)];
    } catch (e) {}

    delete options.selector;

    let i = 0;
    for (let matchingNode of nodes) {
      if (i >= MAX_HIGHLIGHTED_ELEMENTS) {
        break;
      }

      let highlighter = new BoxModelHighlighter(this.tabActor);
      if (options.fill) {
        highlighter.regionFill[options.region || "border"] = options.fill;
      }
      highlighter.show(matchingNode, options);
      this._highlighters.push(highlighter);
      i ++;
    }
  },

  hide: function() {
    for (let highlighter of this._highlighters) {
      highlighter.destroy();
    }
    this._highlighters = [];
  },

  destroy: function() {
    this.hide();
    this.tabActor = null;
  }
};
register(SelectorHighlighter);
exports.SelectorHighlighter = SelectorHighlighter;








function RectHighlighter(tabActor) {
  this.win = tabActor.window;
  this.layoutHelpers = new LayoutHelpers(this.win);
  this.markup = new CanvasFrameAnonymousContentHelper(tabActor,
    this._buildMarkup.bind(this));
}

RectHighlighter.prototype = {
  typeName: "RectHighlighter",

  _buildMarkup: function() {
    let doc = this.win.document;

    let container = doc.createElement("div");
    container.className = "highlighter-container";
    container.innerHTML = '<div id="highlighted-rect" ' +
                          'class="highlighted-rect" hidden="true">';

    return container;
  },

  destroy: function() {
    this.win = null;
    this.layoutHelpers = null;
    this.markup.destroy();
  },

  getElement: function(id) {
    return this.markup.getElement(id);
  },

  _hasValidOptions: function(options) {
    let isValidNb = n => typeof n === "number" && n >= 0 && isFinite(n);
    return options && options.rect &&
           isValidNb(options.rect.x) &&
           isValidNb(options.rect.y) &&
           options.rect.width && isValidNb(options.rect.width) &&
           options.rect.height && isValidNb(options.rect.height);
  },

  








  show: function(node, options) {
    if (!this._hasValidOptions(options) || !node || !node.ownerDocument) {
      this.hide();
      return;
    }

    let contextNode = node.ownerDocument.documentElement;

    
    let quads = this.layoutHelpers.getAdjustedQuads(contextNode);
    if (!quads.length) {
      this.hide();
      return;
    }

    let {bounds} = quads[0];
    let x = "left:" + (bounds.x + options.rect.x) + "px;";
    let y = "top:" + (bounds.y + options.rect.y) + "px;";
    let width = "width:" + options.rect.width + "px;";
    let height = "height:" + options.rect.height + "px;";

    let style = x + y + width + height;
    if (options.fill) {
      style += "background:" + options.fill + ";";
    }

    
    let rect = this.getElement("highlighted-rect");
    rect.setAttribute("style", style);
    rect.removeAttribute("hidden");
  },

  hide: function() {
    this.getElement("highlighted-rect").setAttribute("hidden", "true");
  }
};
register(RectHighlighter);
exports.RectHighlighter = RectHighlighter;





let GeoProp = {
  SIDES: ["top", "right", "bottom", "left"],
  SIZES: ["width", "height"],

  allProps: function() {
    return [...this.SIDES, ...this.SIZES];
  },

  isSide: function(name) {
    return this.SIDES.indexOf(name) !== -1;
  },

  isSize: function(name) {
    return this.SIZES.indexOf(name) !== -1;
  },

  containsSide: function(names) {
    return names.some(name => this.SIDES.indexOf(name) !== -1);
  },

  containsSize: function(names) {
    return names.some(name => this.SIZES.indexOf(name) !== -1);
  },

  isHorizontal: function(name) {
    return name === "left" || name === "right" || name === "width";
  },

  isInverted: function(name) {
    return name === "right" || name === "bottom";
  },

  mainAxisStart: function(name) {
    return this.isHorizontal(name) ? "left" : "top";
  },

  crossAxisStart: function(name) {
    return this.isHorizontal(name) ? "top" : "left";
  },

  mainAxisSize: function(name) {
    return this.isHorizontal(name) ? "width" : "height";
  },

  crossAxisSize: function(name) {
    return this.isHorizontal(name) ? "height" : "width";
  },

  axis: function(name) {
    return this.isHorizontal(name) ? "x" : "y";
  },

  crossAxis: function(name) {
    return this.isHorizontal(name) ? "y" : "x";
  }
};



















function GeometryEditorHighlighter(tabActor) {
  AutoRefreshHighlighter.call(this, tabActor);

  
  this.definedProperties = new Map();

  this.markup = new CanvasFrameAnonymousContentHelper(tabActor,
    this._buildMarkup.bind(this));
}

GeometryEditorHighlighter.prototype = Heritage.extend(AutoRefreshHighlighter.prototype, {
  typeName: "GeometryEditorHighlighter",

  ID_CLASS_PREFIX: "geometry-editor-",

  _buildMarkup: function() {
    let container = createNode(this.win, {
      attributes: {"class": "highlighter-container"}
    });

    let root = createNode(this.win, {
      parent: container,
      attributes: {
        "id": "root",
        "class": "root"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    let svg = createSVGNode(this.win, {
      nodeType: "svg",
      parent: root,
      attributes: {
        "id": "elements",
        "width": "100%",
        "height": "100%"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    
    createSVGNode(this.win, {
      nodeType: "polygon",
      parent: svg,
      attributes: {
        "class": "offset-parent",
        "id": "offset-parent",
        "hidden": "true"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    
    createSVGNode(this.win, {
      nodeType: "polygon",
      parent: svg,
      attributes: {
        "class": "current-node",
        "id": "current-node",
        "hidden": "true"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    
    for (let name of GeoProp.SIDES) {
      createSVGNode(this.win, {
        nodeType: "line",
        parent: svg,
        attributes: {
          "class": "arrow " + name,
          "id": "arrow-" + name,
          "hidden": "true"
        },
        prefix: this.ID_CLASS_PREFIX
      });

      
      
      
      
      let labelG = createSVGNode(this.win, {
        nodeType: "g",
        parent: svg,
        attributes: {
          "id": "label-" + name,
          "hidden": "true"
        },
        prefix: this.ID_CLASS_PREFIX
      });

      let subG = createSVGNode(this.win, {
        nodeType: "g",
        parent: labelG,
        attributes: {
          "transform": GeoProp.isHorizontal(name)
                       ? "translate(-30 -30)"
                       : "translate(5 -10)"
        }
      });

      createSVGNode(this.win, {
        nodeType: "path",
        parent: subG,
        attributes: {
          "class": "label-bubble",
          "d": GeoProp.isHorizontal(name)
               ? "M0 0 L60 0 L60 20 L35 20 L30 25 L25 20 L0 20z"
               : "M5 0 L65 0 L65 20 L5 20 L5 15 L0 10 L5 5z"
        },
        prefix: this.ID_CLASS_PREFIX
      });

      createSVGNode(this.win, {
        nodeType: "text",
        parent: subG,
        attributes: {
          "class": "label-text",
          "id": "label-text-" + name,
          "x": GeoProp.isHorizontal(name) ? "30" : "35",
          "y": "10"
        },
        prefix: this.ID_CLASS_PREFIX
      });
    }

    
    let labelSizeG = createSVGNode(this.win, {
      nodeType: "g",
      parent: svg,
      attributes: {
        "id": "label-size",
        "hidden": "true"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    let subSizeG = createSVGNode(this.win, {
      nodeType: "g",
      parent: labelSizeG,
      attributes: {
        "transform": "translate(-50 -10)"
      }
    });

    createSVGNode(this.win, {
      nodeType: "path",
      parent: subSizeG,
      attributes: {
        "class": "label-bubble",
        "d": "M0 0 L100 0 L100 20 L0 20z"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    createSVGNode(this.win, {
      nodeType: "text",
      parent: subSizeG,
      attributes: {
        "class": "label-text",
        "id": "label-text-size",
        "x": "50",
        "y": "10"
      },
      prefix: this.ID_CLASS_PREFIX
    });

    return container;
  },

  destroy: function() {
    AutoRefreshHighlighter.prototype.destroy.call(this);

    this.markup.destroy();
    this.definedProperties.clear();
    this.definedProperties = null;
    this.offsetParent = null;
  },

  getElement: function(id) {
    return this.markup.getElement(this.ID_CLASS_PREFIX + id);
  },

  





  getDefinedGeometryProperties: function() {
    let props = new Map();
    if (!this.currentNode) {
      return props;
    }

    
    let cssRules = DOMUtils.getCSSStyleRules(this.currentNode);
    for (let i = 0; i < cssRules.Count(); i++) {
      let rule = cssRules.GetElementAt(i);
      for (let name of GeoProp.allProps()) {
        let value = rule.style.getPropertyValue(name);
        if (value && value !== "auto") {
          
          
          props.set(name, {
            cssRule: rule
          });
        }
      }
    }

    
    for (let name of GeoProp.allProps()) {
      let value = this.currentNode.style.getPropertyValue(name);
      if (value && value !== "auto") {
        props.set(name, {
          
          
          cssRule: this.currentNode
        });
      }
    }

    
    
    
    
    for (let [name] of props) {
      let pos = this.computedStyle.position;

      
      if (pos === "static" && GeoProp.SIDES.indexOf(name) !== -1) {
        props.delete(name);
      }

      
      
      let hasRightAndLeft = name === "right" && props.has("left");
      let hasBottomAndTop = name === "bottom" && props.has("top");
      if (pos === "relative" && (hasRightAndLeft || hasBottomAndTop)) {
        props.delete(name);
      }
    }

    return props;
  },

  _show: function() {
    this.computedStyle = CssLogic.getComputedStyle(this.currentNode);
    let pos = this.computedStyle.position;
    
    if (pos === "sticky") {
      this.hide();
      return;
    }

    let hasUpdated = this._update();
    if (!hasUpdated) {
      this.hide();
    }
  },

  _update: function() {
    
    
    this.definedProperties = this.getDefinedGeometryProperties();

    let isStatic = this.computedStyle.position === "static";
    let hasSizes = GeoProp.containsSize([...this.definedProperties.keys()]);

    if (!this.definedProperties.size) {
      console.warn("The element does not have editable geometry properties");
      return false;
    }

    setIgnoreLayoutChanges(true);

    
    this.updateOffsetParent();
    this.updateCurrentNode();
    this.updateArrows();
    this.updateSize();

    
    this.markup.scaleRootElement(this.currentNode, this.ID_CLASS_PREFIX + "root");

    setIgnoreLayoutChanges(false, this.currentNode.ownerDocument.documentElement);
    return true;
  },

  











  updateOffsetParent: function() {
    
    this.offsetParent = getOffsetParent(this.currentNode);
    
    this.parentQuads = this.layoutHelpers
                      .getAdjustedQuads(this.offsetParent.element, "padding");

    let el = this.getElement("offset-parent");

    let isPositioned = this.computedStyle.position === "absolute" ||
                       this.computedStyle.position === "fixed";
    let isRelative = this.computedStyle.position === "relative";
    let isHighlighted = false;

    if (this.offsetParent.element && isPositioned) {
      let {p1, p2, p3, p4} = this.parentQuads[0];
      let points = p1.x + "," + p1.y + " " +
                   p2.x + "," + p2.y + " " +
                   p3.x + "," + p3.y + " " +
                   p4.x + "," + p4.y;
      el.setAttribute("points", points);
      isHighlighted = true;
    } else if (isRelative) {
      let xDelta = parseFloat(this.computedStyle.left);
      let yDelta = parseFloat(this.computedStyle.top);
      if (xDelta || yDelta) {
        let {p1, p2, p3, p4} = this.currentQuads.margin[0];
        let points = (p1.x - xDelta) + "," + (p1.y - yDelta) + " " +
                     (p2.x - xDelta) + "," + (p2.y - yDelta) + " " +
                     (p3.x - xDelta) + "," + (p3.y - yDelta) + " " +
                     (p4.x - xDelta) + "," + (p4.y - yDelta);
        el.setAttribute("points", points);
        isHighlighted = true;
      }
    }

    if (isHighlighted) {
      el.removeAttribute("hidden");
    } else {
      el.setAttribute("hidden", "true");
    }
  },

  updateCurrentNode: function() {
    let box = this.getElement("current-node");
    let {p1, p2, p3, p4} = this.currentQuads.margin[0];
    let attr = p1.x + "," + p1.y + " " +
               p2.x + "," + p2.y + " " +
               p3.x + "," + p3.y + " " +
               p4.x + "," + p4.y;
    box.setAttribute("points", attr);
    box.removeAttribute("hidden");
  },

  _hide: function() {
    setIgnoreLayoutChanges(true);

    this.getElement("current-node").setAttribute("hidden", "true");
    this.getElement("offset-parent").setAttribute("hidden", "true");
    this.hideArrows();
    this.hideSize();

    this.definedProperties.clear();

    setIgnoreLayoutChanges(false, this.currentNode.ownerDocument.documentElement);
  },

  hideArrows: function() {
    for (let side of GeoProp.SIDES) {
      this.getElement("arrow-" + side).setAttribute("hidden", "true");
      this.getElement("label-" + side).setAttribute("hidden", "true");
    }
  },

  hideSize: function() {
    this.getElement("label-size").setAttribute("hidden", "true");
  },

  updateSize: function() {
    this.hideSize();

    let labels = [];
    let width = this.definedProperties.get("width");
    let height = this.definedProperties.get("height");

    if (width) {
      labels.push("↔ " + width.cssRule.style.getPropertyValue("width"));
    }
    if (height) {
      labels.push("↕ " + height.cssRule.style.getPropertyValue("height"));
    }

    if (labels.length) {
      let labelEl = this.getElement("label-size");
      let labelTextEl = this.getElement("label-text-size");

      let {bounds} = this.currentQuads.margin[0];

      labelEl.setAttribute("transform", "translate(" +
        (bounds.left + bounds.width/2) + " " +
        (bounds.top + bounds.height/2) + ")");
      labelEl.removeAttribute("hidden");
      labelTextEl.setTextContent(labels.join(" "));
    }
  },

  updateArrows: function() {
    this.hideArrows();

    
    let marginBox = this.currentQuads.margin[0].bounds;
    
    
    let boxSizing = this.computedStyle.boxSizing.split("-")[0];
    let box = this.currentQuads[boxSizing][0].bounds;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    let getSideArrowStartPos = side => {
      
      if (this.parentQuads && this.parentQuads.length) {
        return this.parentQuads[0].bounds[side];
      }

      
      if (this.computedStyle.position === "relative") {
        if (GeoProp.isInverted(side)) {
          return marginBox[side] + parseFloat(this.computedStyle[side]);
        } else {
          return marginBox[side] - parseFloat(this.computedStyle[side]);
        }
      }

      
      if (GeoProp.isInverted(side)) {
        return this.offsetParent.dimension[GeoProp.mainAxisSize(side)];
      } else {
        return -1 * getWindow(this.currentNode)["scroll" +
                                                GeoProp.axis(side).toUpperCase()];
      }
    };

    for (let side of GeoProp.SIDES) {
      let sideProp = this.definedProperties.get(side);
      if (!sideProp) {
        continue;
      }

      let mainAxisStartPos = getSideArrowStartPos(side);
      let mainAxisEndPos = marginBox[side];
      let crossAxisPos = marginBox[GeoProp.crossAxisStart(side)] +
                         marginBox[GeoProp.crossAxisSize(side)] / 2;

      this.updateArrow(side, mainAxisStartPos, mainAxisEndPos, crossAxisPos,
                       sideProp.cssRule.style.getPropertyValue(side));
    }
  },

  updateArrow: function(side, mainStart, mainEnd, crossPos, labelValue) {
    let arrowEl = this.getElement("arrow-" + side);
    let labelEl = this.getElement("label-" + side);
    let labelTextEl = this.getElement("label-text-" + side);

    
    arrowEl.setAttribute(GeoProp.axis(side) + "1", mainStart);
    arrowEl.setAttribute(GeoProp.crossAxis(side) + "1", crossPos);
    arrowEl.setAttribute(GeoProp.axis(side) + "2", mainEnd);
    arrowEl.setAttribute(GeoProp.crossAxis(side) + "2", crossPos);
    arrowEl.removeAttribute("hidden");

    
    
    let capitalize = str => str.substring(0, 1).toUpperCase() + str.substring(1);
    let winMain = this.win["inner" + capitalize(GeoProp.mainAxisSize(side))]
    let labelMain = mainStart + (mainEnd - mainStart) / 2;
    if ((mainStart > 0 && mainStart < winMain) ||
        (mainEnd > 0 && mainEnd < winMain)) {
      if (labelMain < GEOMETRY_LABEL_SIZE) {
        labelMain = GEOMETRY_LABEL_SIZE;
      } else if (labelMain > winMain - GEOMETRY_LABEL_SIZE) {
        labelMain = winMain - GEOMETRY_LABEL_SIZE;
      }
    }
    let labelCross = crossPos;
    labelEl.setAttribute("transform", GeoProp.isHorizontal(side)
                         ? "translate(" + labelMain + " " + labelCross + ")"
                         : "translate(" + labelCross + " " + labelMain + ")");
    labelEl.removeAttribute("hidden");
    labelTextEl.setTextContent(labelValue);
  }
});
register(GeometryEditorHighlighter);
exports.GeometryEditorHighlighter = GeometryEditorHighlighter;






function RulersHighlighter(tabActor) {
  this.win = tabActor.window;
  this.markup = new CanvasFrameAnonymousContentHelper(tabActor,
    this._buildMarkup.bind(this));

  this.win.addEventListener("scroll", this, true);
  this.win.addEventListener("pagehide", this, true);
}

RulersHighlighter.prototype =  {
  typeName: "RulersHighlighter",

  ID_CLASS_PREFIX: "rulers-highlighter-",

  _buildMarkup: function() {
    let prefix = this.ID_CLASS_PREFIX;
    let window = this.win;

    function createRuler(axis, size) {
      let width, height;
      let isHorizontal = true;

      if (axis === "x") {
        width = size;
        height = 16;
      } else if (axis === "y") {
        width = 16;
        height = size;
        isHorizontal = false;
      } else {
        throw new Error(`Invalid type of axis given; expected "x" or "y" but got "${axis}"`);
      }

      let g = createSVGNode(window, {
        nodeType: "g",
        attributes: {
          id: `${axis}-axis`
        },
        parent: svg,
        prefix
      });

      createSVGNode(window, {
        nodeType: "rect",
        attributes: {
          y: isHorizontal ? 0 : 16,
          width,
          height
        },
        parent: g
      });

      let gRule = createSVGNode(window, {
        nodeType: "g",
        attributes: {
          id: `${axis}-axis-ruler`
        },
        parent: g,
        prefix
      });

      let pathGraduations = createSVGNode(window, {
        nodeType: "path",
        attributes: {
          "class": "ruler-graduations",
          width,
          height
        },
        parent: gRule,
        prefix
      });

      let pathMarkers = createSVGNode(window, {
        nodeType: "path",
        attributes: {
          "class": "ruler-markers",
          width,
          height
        },
        parent: gRule,
        prefix
      });

      let gText = createSVGNode(window, {
        nodeType: "g",
        attributes: {
          id: `${axis}-axis-text`,
          "class": (isHorizontal ? "horizontal" : "vertical") + "-labels"
        },
        parent: g,
        prefix
      });

      let dGraduations = "";
      let dMarkers = "";
      let graduationLength;

      for (let i = 0; i < size; i+=RULERS_GRADUATION_STEP) {
        if (i === 0) continue;

        graduationLength = (i % 2 === 0) ? 6 : 4;

        if (i % RULERS_TEXT_STEP === 0) {
          graduationLength = 8;
          createSVGNode(window, {
            nodeType: "text",
            parent: gText,
            attributes: {
              x: isHorizontal ? 2 + i : -i - 1,
              y: 5
            }
          }).textContent = i;
        }

        if (isHorizontal) {
          if (i % RULERS_MARKER_STEP === 0)
            dMarkers += `M${i} 0 L${i} ${graduationLength}`;
          else
            dGraduations += `M${i} 0 L${i} ${graduationLength} `;

        } else {
          if (i % 50 === 0)
            dMarkers += `M0 ${i} L${graduationLength} ${i}`;
          else
            dGraduations += `M0 ${i} L${graduationLength} ${i}`;
        }
      }

      pathGraduations.setAttribute("d", dGraduations);
      pathMarkers.setAttribute("d", dMarkers);

      return g;
    }

    let container = createNode(window, {
      attributes: {"class": "highlighter-container"}
    });

    let root = createNode(window, {
      parent: container,
      attributes: {
        "id": "root",
        "class": "root"
      },
      prefix
    });

    let svg = createSVGNode(window, {
      nodeType: "svg",
      parent: root,
      attributes: {
        id: "elements",
        "class": "elements",
        width: "100%",
        height: "100%",
        hidden: "true"
      },
      prefix
    });

    createRuler("x", RULERS_MAX_X_AXIS);
    createRuler("y", RULERS_MAX_Y_AXIS);

    return container;
  },

  handleEvent: function(event) {
    switch (event.type) {
      case "scroll":
        this._onScroll(event);
        break;
      case "pagehide":
        this.destroy();
        break;
    }
  },

  _onScroll: function(event) {
    let prefix = this.ID_CLASS_PREFIX;
    let { scrollX, scrollY } = event.view;

    this.markup.getElement(`${prefix}x-axis-ruler`)
                        .setAttribute("transform", `translate(${-scrollX})`);
    this.markup.getElement(`${prefix}x-axis-text`)
                        .setAttribute("transform", `translate(${-scrollX})`);
    this.markup.getElement(`${prefix}y-axis-ruler`)
                        .setAttribute("transform", `translate(0, ${-scrollY})`);
    this.markup.getElement(`${prefix}y-axis-text`)
                        .setAttribute("transform", `translate(0, ${-scrollY})`);
  },

  destroy: function() {
    this.hide();

    this.win.removeEventListener("scroll", this, true);
    this.win.removeEventListener("pagehide", this, true);

    this.markup.destroy();

    events.emit(this, "destroy");
  },

  show: function() {
    this.markup.removeAttributeForElement(this.ID_CLASS_PREFIX + "elements",
      "hidden");
  },

  hide: function() {
    this.markup.setAttributeForElement(this.ID_CLASS_PREFIX + "elements",
      "hidden", "true");
  }
};

register(RulersHighlighter);
exports.RulersHighlighter = RulersHighlighter;








function SimpleOutlineHighlighter(tabActor) {
  this.chromeDoc = tabActor.window.document;
}

SimpleOutlineHighlighter.prototype = {
  


  destroy: function() {
    this.hide();
    this.chromeDoc = null;
  },

  



  show: function(node) {
    if (!this.currentNode || node !== this.currentNode) {
      this.hide();
      this.currentNode = node;
      installHelperSheet(getWindow(node), SIMPLE_OUTLINE_SHEET);
      DOMUtils.addPseudoClassLock(node, HIGHLIGHTED_PSEUDO_CLASS);
    }
  },

  


  hide: function() {
    if (this.currentNode) {
      DOMUtils.removePseudoClassLock(this.currentNode, HIGHLIGHTED_PSEUDO_CLASS);
      this.currentNode = null;
    }
  }
};

function isNodeValid(node) {
  
  if(!node || Cu.isDeadWrapper(node)) {
    return false;
  }

  
  if (node.nodeType !== Ci.nsIDOMNode.ELEMENT_NODE) {
    return false;
  }

  
  let doc = node.ownerDocument;
  if (!doc || !doc.defaultView) {
    return false;
  }

  
  
  let bindingParent = LayoutHelpers.getRootBindingParent(node);
  if (!doc.documentElement.contains(bindingParent)) {
    return false;
  }

  return true;
}




let installedHelperSheets = new WeakMap;
function installHelperSheet(win, source, type="agent") {
  if (installedHelperSheets.has(win.document)) {
    return;
  }
  let {Style} = require("sdk/stylesheet/style");
  let {attach} = require("sdk/content/mod");
  let style = Style({source, type});
  attach(style, win);
  installedHelperSheets.set(win.document, style);
}




function isXUL(tabActor) {
  return tabActor.window.document.documentElement.namespaceURI === XUL_NS;
}












function createSVGNode(win, options) {
  if (!options.nodeType) {
    options.nodeType = "box";
  }
  options.namespace = SVG_NS;
  return createNode(win, options);
}














function createNode(win, options) {
  let type = options.nodeType || "div";

  let node;
  if (options.namespace) {
    node = win.document.createElementNS(options.namespace, type);
  } else {
    node = win.document.createElement(type);
  }

  for (let name in options.attributes || {}) {
    let value = options.attributes[name];
    if (options.prefix && (name === "class" || name === "id")) {
      value = options.prefix + value
    }
    node.setAttribute(name, value);
  }

  if (options.parent) {
    options.parent.appendChild(node);
  }

  return node;
}














function getPageListenerTarget(tabActor) {
  return tabActor.isRootActor ? tabActor.window : tabActor.chromeEventHandler;
}




function getWindow(node) {
  return node.ownerDocument.defaultView;
}













function getOffsetParent(node) {
  let offsetParent = node.offsetParent;
  if (offsetParent &&
      CssLogic.getComputedStyle(offsetParent).position === "static") {
    offsetParent = null;
  }

  let width, height;
  if (!offsetParent) {
    height = getWindow(node).innerHeight;
    width = getWindow(node).innerWidth;
  } else {
    height = offsetParent.offsetHeight;
    width = offsetParent.offsetWidth;
  }

  return {
    element: offsetParent,
    dimension: {width, height}
  };
}

XPCOMUtils.defineLazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils)
});
