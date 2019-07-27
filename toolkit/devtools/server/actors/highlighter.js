



"use strict";

const {Cu, Cc, Ci} = require("chrome");
const Services = require("Services");
const protocol = require("devtools/server/protocol");
const {Arg, Option, method} = protocol;
const events = require("sdk/event/core");
const Heritage = require("sdk/core/heritage");
const {CssLogic} = require("devtools/styleinspector/css-logic");
const EventEmitter = require("devtools/toolkit/event-emitter");


require("devtools/server/actors/inspector");

Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");


const PSEUDO_CLASSES = [":hover", ":active", ":focus"];
const BOX_MODEL_REGIONS = ["margin", "border", "padding", "content"];
const BOX_MODEL_SIDES = ["top", "right", "bottom", "left"];
const SVG_NS = "http://www.w3.org/2000/svg";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const HIGHLIGHTER_STYLESHEET_URI = "chrome://browser/skin/devtools/highlighter.css";
const HIGHLIGHTER_PICKED_TIMER = 1000;

const NODE_INFOBAR_HEIGHT = 40; 
const NODE_INFOBAR_ARROW_SIZE = 15; 

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


let HIGHLIGHTER_CLASSES = exports.HIGHLIGHTER_CLASSES = {
  "BoxModelHighlighter": BoxModelHighlighter,
  "CssTransformHighlighter": CssTransformHighlighter,
  "SelectorHighlighter": SelectorHighlighter
};



























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

    this._createHighlighter();

    
    
    events.on(this._tabActor, "navigate", this._onNavigate);
  },

  get conn() this._inspector && this._inspector.conn,

  _createHighlighter: function() {
    this._isPreviousWindowXUL = isXUL(this._tabActor);

    if (!this._isPreviousWindowXUL) {
      this._boxModelHighlighter = new BoxModelHighlighter(this._tabActor,
                                                          this._inspector);
      this._boxModelHighlighter.on("ready", this._highlighterReady);
      this._boxModelHighlighter.on("hide", this._highlighterHidden);
    } else {
      this._boxModelHighlighter = new SimpleOutlineHighlighter(this._tabActor);
    }
  },

  _destroyHighlighter: function() {
    if (this._boxModelHighlighter) {
      if (!this._isPreviousWindowXUL) {
        this._boxModelHighlighter.off("ready", this._highlighterReady);
        this._boxModelHighlighter.off("hide", this._highlighterHidden);
      }
      this._boxModelHighlighter.destroy();
      this._boxModelHighlighter = null;
    }
  },

  _onNavigate: function({isTopLevel}) {
    if (!isTopLevel) {
      return;
    }

    
    if (isXUL(this._tabActor) !== this._isPreviousWindowXUL) {
      this._destroyHighlighter();
      this._createHighlighter();
    }
  },

  destroy: function() {
    protocol.Actor.prototype.destroy.call(this);

    this._destroyHighlighter();
    events.off(this._tabActor, "navigate", this._onNavigate);
    this._autohide = null;
    this._inspector = null;
    this._walker = null;
    this._tabActor = null;
  },

  









  showBoxModel: method(function(node, options={}) {
    if (node && isNodeValid(node.rawNode)) {
      this._boxModelHighlighter.show(node.rawNode, options);
    } else {
      this._boxModelHighlighter.hide();
    }
  }, {
    request: {
      node: Arg(0, "domnode"),
      region: Option(1)
    }
  }),

  


  hideBoxModel: method(function() {
    this._boxModelHighlighter.hide();
  }, {
    request: {}
  }),

  









  _isPicking: false,
  _hoveredNode: null,

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
          this._boxModelHighlighter.hide();
        }, HIGHLIGHTER_PICKED_TIMER);
      }
      events.emit(this._walker, "picker-node-picked", this._findAndAttachElement(event));
    };

    this._onHovered = event => {
      this._preventContentEvent(event);
      let res = this._findAndAttachElement(event);
      if (this._hoveredNode !== res.node) {
        this._boxModelHighlighter.show(res.node.rawNode);
        events.emit(this._walker, "picker-node-hovered", res);
        this._hoveredNode = res.node;
      }
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

  











  _getPickerListenerTarget: function() {
    let actor = this._tabActor;
    return actor.isRootActor ? actor.window : actor.chromeEventHandler;
  },

  _startPickerListeners: function() {
    let target = this._getPickerListenerTarget();
    target.addEventListener("mousemove", this._onHovered, true);
    target.addEventListener("click", this._onPick, true);
    target.addEventListener("mousedown", this._preventContentEvent, true);
    target.addEventListener("mouseup", this._preventContentEvent, true);
    target.addEventListener("dblclick", this._preventContentEvent, true);
  },

  _stopPickerListeners: function() {
    let target = this._getPickerListenerTarget();
    target.removeEventListener("mousemove", this._onHovered, true);
    target.removeEventListener("click", this._onPick, true);
    target.removeEventListener("mousedown", this._preventContentEvent, true);
    target.removeEventListener("mouseup", this._preventContentEvent, true);
    target.removeEventListener("dblclick", this._preventContentEvent, true);
  },

  _highlighterReady: function() {
    events.emit(this._inspector.walker, "highlighter-ready");
  },

  _highlighterHidden: function() {
    events.emit(this._inspector.walker, "highlighter-hide");
  },

  cancelPick: method(function() {
    if (this._isPicking) {
      this._boxModelHighlighter.hide();
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

    let constructor = HIGHLIGHTER_CLASSES[typeName];
    if (!constructor) {
      throw new Error(typeName + " isn't a valid highlighter class (" +
        Object.keys(HIGHLIGHTER_CLASSES) + ")");
      return;
    }

    
    
    if (!isXUL(this._inspector.tabActor)) {
      this._highlighter = new constructor(inspector.tabActor);
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

  this._insert();

  this._onNavigate = this._onNavigate.bind(this);
  events.on(this.tabActor, "navigate", this._onNavigate);
}

CanvasFrameAnonymousContentHelper.prototype = {
  destroy: function() {
    
    
    try {
      let doc = this.tabActor.window.document;
      doc.removeAnonymousContent(this._content);
    } catch (e) {}
    events.off(this.tabActor, "navigate", this._onNavigate);
    this.tabActor = this.nodeBuilder = this._content = null;
  },

  _insert: function() {
    
    
    if (!isXUL(this.tabActor)) {
      
      
      
      
      installHelperSheet(this.tabActor.window,
        "@import url('" + HIGHLIGHTER_STYLESHEET_URI + "');");
      let node = this.nodeBuilder();
      let doc = this.tabActor.window.document;
      this._content = doc.insertAnonymousContent(node);
    }
  },

  _onNavigate: function({isTopLevel}) {
    if (isTopLevel) {
      this._insert();
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

  get content() {
    if (Cu.isDeadWrapper(this._content)) {
      return null;
    }
    return this._content;
  }
};











function AutoRefreshHighlighter(tabActor) {
  this.tabActor = tabActor;
  this.browser = tabActor.browser;
  this.win = tabActor.window;
  this.currentNode = null;

  this.update = this.update.bind(this);
}

AutoRefreshHighlighter.prototype = {
  





  show: function(node, options={}) {
    if (!isNodeValid(node) || node === this.currentNode) {
      return;
    }

    this.options = options;

    this._detachPageListeners();
    this.currentNode = node;
    this._attachPageListeners();
    this._show();
  },

  


  hide: function() {
    if (!isNodeValid(this.currentNode)) {
      return;
    }

    this._hide();
    this._detachPageListeners();
    this.currentNode = null;
    this.options = null;
  },

  


  update: function(e) {
    if (!isNodeValid(this.currentNode)) {
      return;
    }
    this._update();
  },

  _show: function() {
    
    
    
  },

  _update: function() {
    
    
    
    
  },

  _hide: function() {
    
    
  },

  


  _attachPageListeners: function() {
    if (isNodeValid(this.currentNode)) {
      
      
      
      let target = this.browser || this.win;
      target.addEventListener("MozAfterPaint", this.update);
    }
  },

  


  _detachPageListeners: function() {
    if (isNodeValid(this.currentNode)) {
      let target = this.browser || this.win;
      target.removeEventListener("MozAfterPaint", this.update);
    }
  },

  destroy: function() {
    this.hide();

    this.tabActor = null;
    this.win = null;
    this.browser = null;
    this.currentNode = null;
  }
};






















































function BoxModelHighlighter(tabActor) {
  AutoRefreshHighlighter.call(this, tabActor);
  this.layoutHelpers = new LayoutHelpers(this.win);
  EventEmitter.decorate(this);

  this.markup = new CanvasFrameAnonymousContentHelper(this.tabActor,
    this._buildMarkup.bind(this));

  



  this.regionFill = {};

  this._currentNode = null;

  
  this._currentQuads = {};
}

BoxModelHighlighter.prototype = Heritage.extend(AutoRefreshHighlighter.prototype, {
  ID_CLASS_PREFIX: "box-model-",

  get zoom() {
    return this.win.QueryInterface(Ci.nsIInterfaceRequestor)
               .getInterface(Ci.nsIDOMWindowUtils).fullZoom;
  },

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

    

    let svgRoot = this._createSVGNode("svg", highlighterContainer, {
      "id": "root",
      "class": "root",
      "width": "100%",
      "height": "100%",
      "style": "width:100%;height:100%;",
      "hidden": "true"
    });

    let boxModelContainer = this._createSVGNode("g", svgRoot, {
      "class": "container"
    });

    for (let region of BOX_MODEL_REGIONS) {
      this._createSVGNode("polygon", boxModelContainer, {
        "class": region,
        "id": region
      });
    }

    for (let side of BOX_MODEL_SIDES) {
      this._createSVGNode("line", svgRoot, {
        "class": "guide-" + side,
        "id": "guide-" + side,
        "stroke-width": GUIDE_STROKE_WIDTH
      });
    }

    highlighterContainer.appendChild(svgRoot);

    

    let infobarContainer = this._createNode("div", highlighterContainer, {
      "class": "nodeinfobar-container",
      "id": "nodeinfobar-container",
      "position": "top",
      "hidden": "true"
    });

    let nodeInfobar = this._createNode("div", infobarContainer, {
      "class": "nodeinfobar"
    });

    let texthbox = this._createNode("div", nodeInfobar, {
      "class": "nodeinfobar-text"
    });
    this._createNode("span", texthbox, {
      "class": "nodeinfobar-tagname",
      "id": "nodeinfobar-tagname"
    });
    this._createNode("span", texthbox, {
      "class": "nodeinfobar-id",
      "id": "nodeinfobar-id"
    });
    this._createNode("span", texthbox, {
      "class": "nodeinfobar-classes",
      "id": "nodeinfobar-classes"
    });
    this._createNode("span", texthbox, {
      "class": "nodeinfobar-pseudo-classes",
      "id": "nodeinfobar-pseudo-classes"
    });
    this._createNode("span", texthbox, {
      "class": "nodeinfobar-dimensions",
      "id": "nodeinfobar-dimensions"
    });

    return highlighterContainer;
  },

  _createSVGNode: function(nodeType, parent, attributes={}) {
    return this._createNode(nodeType, parent, attributes, SVG_NS);
  },

  _createNode: function(nodeType, parent, attributes={}, namespace=null) {
    let node;
    if (namespace) {
      node = this.win.document.createElementNS(namespace, nodeType);
    } else {
      node = this.win.document.createElement(nodeType);
    }

    for (let name in attributes) {
      let value = attributes[name];
      if (name === "class" || name === "id") {
        value = this.ID_CLASS_PREFIX + value
      }
      node.setAttribute(name, value);
    }

    parent.appendChild(node);
    return node;
  },

  


  destroy: function() {
    AutoRefreshHighlighter.prototype.destroy.call(this);

    this.markup.destroy();

    this._currentNode = null;
  },

  








  show: function(node, options={}) {
    if (!isNodeValid(node)) {
      return;
    }

    this.options = options;

    if (BOX_MODEL_REGIONS.indexOf(this.options.region) == -1)  {
      this.options.region = "content";
    }

    this._detachPageListeners();
    this.currentNode = node;
    this._attachPageListeners();
    this._show();
  },

  


  _show: function() {
    this._update();
    this._trackMutations();
    this.emit("ready");
  },

  



  _trackMutations: function() {
    if (isNodeValid(this.currentNode)) {
      let win = this.currentNode.ownerDocument.defaultView;
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
    if (this._updateBoxModel()) {
      if (!this.options.hideInfoBar) {
        this._showInfobar();
      }
      this._showBoxModel();
    } else {
      
      this._hide();
    }
  },

  


  _hide: function() {
    this._untrackMutations();
    this._hideBoxModel();
    this._hideInfobar();
  },

  


  _hideInfobar: function() {
    this.markup.setAttributeForElement(
      this.ID_CLASS_PREFIX + "nodeinfobar-container", "hidden", "true");
  },

  


  _showInfobar: function() {
    this.markup.removeAttributeForElement(
      this.ID_CLASS_PREFIX + "nodeinfobar-container", "hidden");
    this._updateInfobar();
  },

  


  _hideBoxModel: function() {
    this.markup.setAttributeForElement(this.ID_CLASS_PREFIX + "root", "hidden",
      "true");
  },

  


  _showBoxModel: function() {
    this.markup.removeAttributeForElement(this.ID_CLASS_PREFIX + "root",
      "hidden");
  },

  





  _updateBoxModel: function() {
    this.options.region = this.options.region || "content";

    if (this._nodeNeedsHighlighting()) {
      for (let boxType of BOX_MODEL_REGIONS) {

        this._currentQuads[boxType] = this.layoutHelpers.getAdjustedQuads(
          this.currentNode, boxType);
        if (!this._currentQuads[boxType]) {
          continue;
        }
        let {p1, p2, p3, p4} = this._currentQuads[boxType];

        if (this.regionFill[boxType]) {
          this.markup.setAttributeForElement(this.ID_CLASS_PREFIX + boxType,
            "style", "fill:" + this.regionFill[boxType]);
        } else {
          this.markup.setAttributeForElement(this.ID_CLASS_PREFIX + boxType,
            "style", "");
        }

        if (!this.options.showOnly || this.options.showOnly === boxType) {
          this.markup.setAttributeForElement(this.ID_CLASS_PREFIX + boxType,
            "points", p1.x + "," + p1.y + " " +
                      p2.x + "," + p2.y + " " +
                      p3.x + "," + p3.y + " " +
                      p4.x + "," + p4.y);
        } else {
          this.markup.removeAttributeForElement(this.ID_CLASS_PREFIX + boxType, "points");
        }

        if (boxType === this.options.region && !this.options.hideGuides) {
          this._showGuides(p1, p2, p3, p4);
        } else if (this.options.hideGuides) {
          this._hideGuides();
        }
      }

      return true;
    }

    this._hideBoxModel();
    return false;
  },

  _nodeNeedsHighlighting: function() {
    if (!this.currentNode ||
        Cu.isDeadWrapper(this.currentNode) ||
        this.currentNode.nodeType !== Ci.nsIDOMNode.ELEMENT_NODE ||
        !this.currentNode.ownerDocument ||
        !this.currentNode.ownerDocument.defaultView) {
      return false;
    }

    if (!this._computedStyle) {
      this._computedStyle = CssLogic.getComputedStyle(this.currentNode);
    }

    return this._computedStyle.getPropertyValue("display") !== "none";
  },

  _getOuterBounds: function() {
    for (let region of ["margin", "border", "padding", "content"]) {
      let quads = this._currentQuads[region];

      if (!quads) {
        
        break;
      }

      let {bottom, height, left, right, top, width, x, y} = quads.bounds;

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

  








  _showGuides: function(p1, p2, p3, p4) {
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
      this.markup.setAttributeForElement(
        this.ID_CLASS_PREFIX + "guide-" + side, "hidden", "true");
    }
  },

  








  _updateGuide: function(side, point=-1) {
    let guideId = this.ID_CLASS_PREFIX + "guide-" + side;

    if (point <= 0) {
      this.markup.setAttributeForElement(guideId, "hidden", "true");
      return false;
    }

    let offset = GUIDE_STROKE_WIDTH / 2;

    if (side === "top" || side === "left") {
      point -= offset;
    } else {
      point += offset;
    }

    if (side === "top" || side === "bottom") {
      this.markup.setAttributeForElement(guideId, "x1", "0");
      this.markup.setAttributeForElement(guideId, "y1", point + "");
      this.markup.setAttributeForElement(guideId, "x2", "100%");
      this.markup.setAttributeForElement(guideId, "y2", point + "");
    } else {
      this.markup.setAttributeForElement(guideId, "x1", point + "");
      this.markup.setAttributeForElement(guideId, "y1", "0");
      this.markup.setAttributeForElement(guideId, "x2", point + "");
      this.markup.setAttributeForElement(guideId, "y2", "100%");
    }

    this.markup.removeAttributeForElement(guideId, "hidden");

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

    let rect = node.getBoundingClientRect();
    let dim = Math.ceil(rect.width) + " x " + Math.ceil(rect.height);

    let elementId = this.ID_CLASS_PREFIX + "nodeinfobar-";
    this.markup.setTextContentForElement(elementId + "tagname", tagName);
    this.markup.setTextContentForElement(elementId + "id", id);
    this.markup.setTextContentForElement(elementId + "classes", classList);
    this.markup.setTextContentForElement(elementId + "pseudo-classes", pseudos);
    this.markup.setTextContentForElement(elementId + "dimensions", dim);

    this._moveInfobar();
  },

  


  _moveInfobar: function() {
    let bounds = this._getOuterBounds();
    let winHeight = this.win.innerHeight * this.zoom;
    let winWidth = this.win.innerWidth * this.zoom;

    
    
    let containerBottom = Math.max(0, bounds.bottom) + NODE_INFOBAR_ARROW_SIZE;
    let containerTop = Math.min(winHeight, bounds.top);
    let containerId = this.ID_CLASS_PREFIX + "nodeinfobar-container";

    
    let top;
    if (containerTop < NODE_INFOBAR_HEIGHT) {
      
      if (containerBottom + NODE_INFOBAR_HEIGHT > winHeight) {
        
        top = containerTop;
        this.markup.setAttributeForElement(containerId, "position", "overlap");
      } else {
        
        top = containerBottom;
        this.markup.setAttributeForElement(containerId, "position", "bottom");
      }
    } else {
      
      top = containerTop - NODE_INFOBAR_HEIGHT;
      this.markup.setAttributeForElement(containerId, "position", "top");
    }

    
    let left = bounds.right - bounds.width / 2;
    
    let buffer = 100;
    if (left < buffer) {
      left = buffer;
      this.markup.setAttributeForElement(containerId, "hide-arrow", "true");
    } else if (left > winWidth - buffer) {
      left = winWidth - buffer;
      this.markup.setAttributeForElement(containerId, "hide-arrow", "true");
    } else {
      this.markup.removeAttributeForElement(containerId, "hide-arrow");
    }

    let style = "top:" + top + "px;left:" + left + "px;";
    this.markup.setAttributeForElement(containerId, "style", style);
  }
});






function CssTransformHighlighter(tabActor) {
  AutoRefreshHighlighter.call(this, tabActor);

  this.layoutHelpers = new LayoutHelpers(tabActor.window);

  this.markup = new CanvasFrameAnonymousContentHelper(this.tabActor,
    this._buildMarkup.bind(this));
}

let MARKER_COUNTER = 1;

CssTransformHighlighter.prototype = Heritage.extend(AutoRefreshHighlighter.prototype, {
  ID_CLASS_PREFIX: "css-transform-",

  _buildMarkup: function() {
    let doc = this.win.document;

    let container = doc.createElement("div");
    container.className = "highlighter-container";

    let svgRoot = this._createSVGNode("svg", container, {
      "class": "root",
      "id": "root",
      "hidden": "true",
      "width": "100%",
      "height": "100%"
    });

    
    this.markerId = "arrow-marker-" + MARKER_COUNTER;
    MARKER_COUNTER ++;
    let marker = this._createSVGNode("marker", svgRoot, {
      "id": this.markerId,
      "markerWidth": "10",
      "markerHeight": "5",
      "orient": "auto",
      "markerUnits": "strokeWidth",
      "refX": "10",
      "refY": "5",
      "viewBox": "0 0 10 10",
    });
    this._createSVGNode("path", marker, {
      "d": "M 0 0 L 10 5 L 0 10 z",
      "fill": "#08C"
    });

    let shapesGroup = this._createSVGNode("g", svgRoot);

    
    this._createSVGNode("polygon", shapesGroup, {
      "id": "untransformed",
      "class": "untransformed"
    });
    this._createSVGNode("polygon", shapesGroup, {
      "id": "transformed",
      "class": "transformed"
    });

    
    for (let nb of ["1", "2", "3", "4"]) {
      this._createSVGNode("line", shapesGroup, {
        "id": "line" + nb,
        "class": "line",
        "marker-end": "url(#" + this.markerId + ")"
      });
    }

    container.appendChild(svgRoot);

    return container;
  },

  _createSVGNode: function(nodeType, parent, attributes={}) {
    let node = this.win.document.createElementNS(SVG_NS, nodeType);

    for (let name in attributes) {
      let value = attributes[name];
      if (name === "class" || name === "id") {
        value = this.ID_CLASS_PREFIX + value
      }
      node.setAttribute(name, value);
    }

    parent.appendChild(node);
    return node;
  },

  


  destroy: function() {
    AutoRefreshHighlighter.prototype.destroy.call(this);
    this.markup.destroy();
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
    this.markup.setAttributeForElement(this.ID_CLASS_PREFIX + id,
                                       "points",
                                       points.join(" "));
  },

  _setLinePoints: function(p1, p2, id) {
    this.markup.setAttributeForElement(this.ID_CLASS_PREFIX + id, "x1", p1.x);
    this.markup.setAttributeForElement(this.ID_CLASS_PREFIX + id, "y1", p1.y);
    this.markup.setAttributeForElement(this.ID_CLASS_PREFIX + id, "x2", p2.x);
    this.markup.setAttributeForElement(this.ID_CLASS_PREFIX + id, "y2", p2.y);

    let dist = Math.sqrt(Math.pow(p2.x - p1.x, 2) + Math.pow(p2.y - p1.y, 2));
    if (dist < ARROW_LINE_MIN_DISTANCE) {
      this.markup.removeAttributeForElement(this.ID_CLASS_PREFIX + id, "marker-end");
    } else {
      this.markup.setAttributeForElement(this.ID_CLASS_PREFIX + id, "marker-end",
                                         "url(#" + this.markerId + ")");
    }
  },

  




  _update: function() {
    
    let quad = this.layoutHelpers.getAdjustedQuads(this.currentNode, "border");
    if (!quad || quad.bounds.width <= 0 || quad.bounds.height <= 0) {
      this._hideShapes();
      return null;
    }

    
    let untransformedQuad = this.layoutHelpers.getNodeBounds(this.currentNode);

    this._setPolygonPoints(quad, "transformed");
    this._setPolygonPoints(untransformedQuad, "untransformed");
    for (let nb of ["1", "2", "3", "4"]) {
      this._setLinePoints(untransformedQuad["p" + nb], quad["p" + nb], "line" + nb);
    }

    this._showShapes();
  },

  


  _hide: function() {
    this._hideShapes();
  },

  _hideShapes: function() {
    this.markup.setAttributeForElement(this.ID_CLASS_PREFIX + "root", "hidden", "true");
  },

  _showShapes: function() {
    this.markup.removeAttributeForElement(this.ID_CLASS_PREFIX + "root", "hidden");
  }
});






function SelectorHighlighter(tabActor) {
  this.tabActor = tabActor;
  this._highlighters = [];
}

SelectorHighlighter.prototype = {
  








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
      installHelperSheet(node.ownerDocument.defaultView, SIMPLE_OUTLINE_SHEET);
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

XPCOMUtils.defineLazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils)
});
