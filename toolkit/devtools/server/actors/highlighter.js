



"use strict";

const {Cu, Cc, Ci} = require("chrome");
const Services = require("Services");
const protocol = require("devtools/server/protocol");
const {Arg, Option, method} = protocol;
const events = require("sdk/event/core");
const Heritage = require("sdk/core/heritage");

const EventEmitter = require("devtools/toolkit/event-emitter");
const GUIDE_STROKE_WIDTH = 1;


require("devtools/server/actors/inspector");

Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");


const PSEUDO_CLASSES = [":hover", ":active", ":focus"];
const HIGHLIGHTED_PSEUDO_CLASS = ":-moz-devtools-highlighted";
let HELPER_SHEET = ".__fx-devtools-hide-shortcut__ { visibility: hidden !important } ";
HELPER_SHEET += ":-moz-devtools-highlighted { outline: 2px dashed #F06!important; outline-offset: -2px!important } ";
const XHTML_NS = "http://www.w3.org/1999/xhtml";
const SVG_NS = "http://www.w3.org/2000/svg";
const HIGHLIGHTER_PICKED_TIMER = 1000;
const INFO_BAR_OFFSET = 5;

const ARROW_LINE_MIN_DISTANCE = 10;


let HIGHLIGHTER_CLASSES = exports.HIGHLIGHTER_CLASSES = {
  "BoxModelHighlighter": BoxModelHighlighter,
  "CssTransformHighlighter": CssTransformHighlighter
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

    if (supportXULBasedHighlighter(this._tabActor)) {
      this._boxModelHighlighter =
        new BoxModelHighlighter(this._tabActor, this._inspector);

        this._boxModelHighlighter.on("ready", this._highlighterReady);
        this._boxModelHighlighter.on("hide", this._highlighterHidden);
    } else {
      this._boxModelHighlighter = new SimpleOutlineHighlighter(this._tabActor);
    }
  },

  get conn() this._inspector && this._inspector.conn,

  destroy: function() {
    protocol.Actor.prototype.destroy.call(this);
    if (this._boxModelHighlighter) {
      this._boxModelHighlighter.off("ready", this._highlighterReady);
      this._boxModelHighlighter.off("hide", this._highlighterHidden);
      this._boxModelHighlighter.destroy();
      this._boxModelHighlighter = null;
    }
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

    
    
    if (supportXULBasedHighlighter(inspector.tabActor)) {
      this._highlighter = new constructor(inspector.tabActor);
    }
  },

  get conn() this._inspector && this._inspector.conn,

  destroy: function() {
    protocol.Actor.prototype.destroy.call(this);
    this.finalize();
  },

  



  show: method(function(node) {
    if (!node || !isNodeValid(node.rawNode) || !this._highlighter) {
      return;
    }

    this._highlighter.show(node.rawNode);
  }, {
    request: {
      node: Arg(0, "domnode")
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





function XULBasedHighlighter(tabActor) {
  this.browser = tabActor.browser;
  this.win = tabActor.window;
  this.chromeDoc = this.browser.ownerDocument;
  this.currentNode = null;

  this.update = this.update.bind(this);
}

XULBasedHighlighter.prototype = {
  



  show: function(node) {
    if (!isNodeValid(node) || node === this.currentNode) {
      return;
    }

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
  },

  


  update: function() {
    if (isNodeValid(this.currentNode)) {
      this._update();
    }
  },

  _show: function() {
    
    
    
  },

  _update: function() {
    
    
    
    
  },

  _hide: function() {
    
    
  },

  


  _attachPageListeners: function() {
    if (isNodeValid(this.currentNode)) {
      let win = this.currentNode.ownerDocument.defaultView;
      this.browser.addEventListener("MozAfterPaint", this.update);
    }
  },

  


  _detachPageListeners: function() {
    if (isNodeValid(this.currentNode)) {
      let win = this.currentNode.ownerDocument.defaultView;
      this.browser.removeEventListener("MozAfterPaint", this.update);
    }
  },

  destroy: function() {
    this.hide();

    this.win = null;
    this.browser = null;
    this.chromeDoc = null;
    this.currentNode = null;
  }
};











































function BoxModelHighlighter(tabActor) {
  XULBasedHighlighter.call(this, tabActor);
  this.layoutHelpers = new LayoutHelpers(this.win);
  this._initMarkup();
  EventEmitter.decorate(this);

  this._currentNode = null;
}

BoxModelHighlighter.prototype = Heritage.extend(XULBasedHighlighter.prototype, {
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

  _initMarkup: function() {
    let stack = this.browser.parentNode;

    this._highlighterContainer = this.chromeDoc.createElement("stack");
    this._highlighterContainer.className = "highlighter-container";

    this._svgRoot = this._createSVGNode("root", "svg", this._highlighterContainer);

    this._boxModelContainer = this._createSVGNode("container", "g", this._svgRoot);

    this._boxModelNodes = {
      margin: this._createSVGNode("margin", "polygon", this._boxModelContainer),
      border: this._createSVGNode("border", "polygon", this._boxModelContainer),
      padding: this._createSVGNode("padding", "polygon", this._boxModelContainer),
      content: this._createSVGNode("content", "polygon", this._boxModelContainer)
    };

    this._guideNodes = {
      top: this._createSVGNode("guide-top", "line", this._svgRoot),
      right: this._createSVGNode("guide-right", "line", this._svgRoot),
      bottom: this._createSVGNode("guide-bottom", "line", this._svgRoot),
      left: this._createSVGNode("guide-left", "line", this._svgRoot)
    };

    this._guideNodes.top.setAttribute("stroke-width", GUIDE_STROKE_WIDTH);
    this._guideNodes.right.setAttribute("stroke-width", GUIDE_STROKE_WIDTH);
    this._guideNodes.bottom.setAttribute("stroke-width", GUIDE_STROKE_WIDTH);
    this._guideNodes.left.setAttribute("stroke-width", GUIDE_STROKE_WIDTH);

    this._highlighterContainer.appendChild(this._svgRoot);

    let infobarContainer = this.chromeDoc.createElement("box");
    infobarContainer.className = "highlighter-nodeinfobar-container";
    this._highlighterContainer.appendChild(infobarContainer);

    
    stack.insertBefore(this._highlighterContainer, stack.childNodes[1]);

    
    let infobarPositioner = this.chromeDoc.createElement("box");
    infobarPositioner.className = "highlighter-nodeinfobar-positioner";
    infobarPositioner.setAttribute("position", "top");
    infobarPositioner.setAttribute("disabled", "true");

    let nodeInfobar = this.chromeDoc.createElement("hbox");
    nodeInfobar.className = "highlighter-nodeinfobar";

    let arrowBoxTop = this.chromeDoc.createElement("box");
    arrowBoxTop.className = "highlighter-nodeinfobar-arrow highlighter-nodeinfobar-arrow-top";

    let arrowBoxBottom = this.chromeDoc.createElement("box");
    arrowBoxBottom.className = "highlighter-nodeinfobar-arrow highlighter-nodeinfobar-arrow-bottom";

    let tagNameLabel = this.chromeDoc.createElementNS(XHTML_NS, "span");
    tagNameLabel.className = "highlighter-nodeinfobar-tagname";

    let idLabel = this.chromeDoc.createElementNS(XHTML_NS, "span");
    idLabel.className = "highlighter-nodeinfobar-id";

    let classesBox = this.chromeDoc.createElementNS(XHTML_NS, "span");
    classesBox.className = "highlighter-nodeinfobar-classes";

    let pseudoClassesBox = this.chromeDoc.createElementNS(XHTML_NS, "span");
    pseudoClassesBox.className = "highlighter-nodeinfobar-pseudo-classes";

    let dimensionBox = this.chromeDoc.createElementNS(XHTML_NS, "span");
    dimensionBox.className = "highlighter-nodeinfobar-dimensions";

    
    pseudoClassesBox.textContent = "&nbsp;";

    
    let texthbox = this.chromeDoc.createElement("hbox");
    texthbox.className = "highlighter-nodeinfobar-text";
    texthbox.setAttribute("align", "center");
    texthbox.setAttribute("flex", "1");

    texthbox.appendChild(tagNameLabel);
    texthbox.appendChild(idLabel);
    texthbox.appendChild(classesBox);
    texthbox.appendChild(pseudoClassesBox);
    texthbox.appendChild(dimensionBox);

    nodeInfobar.appendChild(texthbox);

    infobarPositioner.appendChild(arrowBoxTop);
    infobarPositioner.appendChild(nodeInfobar);
    infobarPositioner.appendChild(arrowBoxBottom);

    infobarContainer.appendChild(infobarPositioner);

    let barHeight = infobarPositioner.getBoundingClientRect().height;

    this.nodeInfo = {
      tagNameLabel: tagNameLabel,
      idLabel: idLabel,
      classesBox: classesBox,
      pseudoClassesBox: pseudoClassesBox,
      dimensionBox: dimensionBox,
      positioner: infobarPositioner,
      barHeight: barHeight,
    };
  },

  _createSVGNode: function(classPostfix, nodeType, parent) {
    let node = this.chromeDoc.createElementNS(SVG_NS, nodeType);
    node.setAttribute("class", "box-model-" + classPostfix);

    parent.appendChild(node);

    return node;
  },

  


  destroy: function() {
    XULBasedHighlighter.prototype.destroy.call(this);

    this._highlighterContainer.remove();
    this._highlighterContainer = null;

    this.nodeInfo = null;
    this._currentNode = null;
  },

  




  _show: function(options={}) {
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

  








  _update: function(options={}) {
    if (this._updateBoxModel(options)) {
      this._showInfobar();
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
    this.nodeInfo.positioner.setAttribute("hidden", "true");
  },

  


  _showInfobar: function() {
    this.nodeInfo.positioner.removeAttribute("hidden");
    this._updateInfobar();
  },

  


  _hideBoxModel: function() {
    this._svgRoot.setAttribute("hidden", "true");
  },

  


  _showBoxModel: function() {
    this._svgRoot.removeAttribute("hidden");
  },

  









  _updateBoxModel: function(options) {
    options.region = options.region || "content";

    if (this._nodeNeedsHighlighting()) {
      for (let boxType in this._boxModelNodes) {
        let {p1, p2, p3, p4} =
          this.layoutHelpers.getAdjustedQuads(this.currentNode, boxType);

        let boxNode = this._boxModelNodes[boxType];
        boxNode.setAttribute("points",
                             p1.x + "," + p1.y + " " +
                             p2.x + "," + p2.y + " " +
                             p3.x + "," + p3.y + " " +
                             p4.x + "," + p4.y);

        if (boxType === options.region) {
          this._showGuides(p1, p2, p3, p4);
        }
      }

      return true;
    }

    this._hideBoxModel();
    return false;
  },

  _nodeNeedsHighlighting: function() {
    if (!this.currentNode) {
      return false;
    }

    if (!this._computedStyle) {
      this._computedStyle =
        this.currentNode.ownerDocument.defaultView.getComputedStyle(this.currentNode);
    }

    return this._computedStyle.getPropertyValue("display") !== "none";
  },

  _getOuterBounds: function() {
    for (let region of ["margin", "border", "padding", "content"]) {
      let quads = this.layoutHelpers.getAdjustedQuads(this.currentNode, region);

      if (!quads) {
        
        break;
      }

      let {bottom, height, left, right, top, width, x, y} = quads.bounds;

      if (width > 0 || height > 0) {
        return this._boundsHelper(bottom, height, left, right, top, width, x, y);
      }
    }

    return this._boundsHelper();
  },

  _boundsHelper: function(bottom=0, height=0, left=0, right=0,
                          top=0, width=0, x=0, y=0) {
    return {
      bottom: bottom,
      height: height,
      left: left,
      right: right,
      top: top,
      width: width,
      x: x,
      y: y
    };
  },

  












  _showGuides: function(p1, p2, p3, p4) {
    let allX = [p1.x, p2.x, p3.x, p4.x].sort();
    let allY = [p1.y, p2.y, p3.y, p4.y].sort();
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

    
    this._updateGuide(this._guideNodes.top, toShowY[0]);
    this._updateGuide(this._guideNodes.right, toShowX[1]);
    this._updateGuide(this._guideNodes.bottom, toShowY[1]);
    this._updateGuide(this._guideNodes.left, toShowX[0]);
  },

  








  _updateGuide: function(guide, point=-1) {
    if (point > 0) {
      let offset = GUIDE_STROKE_WIDTH / 2;

      if (guide === this._guideNodes.top || guide === this._guideNodes.left) {
        point -= offset;
      } else {
        point += offset;
      }

      if (guide === this._guideNodes.top || guide === this._guideNodes.bottom) {
        guide.setAttribute("x1", 0);
        guide.setAttribute("y1", point);
        guide.setAttribute("x2", "100%");
        guide.setAttribute("y2", point);
      } else {
        guide.setAttribute("x1", point);
        guide.setAttribute("y1", 0);
        guide.setAttribute("x2", point);
        guide.setAttribute("y2", "100%");
      }
      guide.removeAttribute("hidden");
      return true;
    } else {
      guide.setAttribute("hidden", "true");
      return false;
    }
  },

  


  _updateInfobar: function() {
    if (!this.currentNode) {
      return;
    }

    let node = this.currentNode;
    let info = this.nodeInfo;

    
    

    let tagName = node.tagName;
    if (info.tagNameLabel.textContent !== tagName) {
      info.tagNameLabel.textContent = tagName;
    }

    let id = node.id ? "#" + node.id : "";
    if (info.idLabel.textContent !== id) {
      info.idLabel.textContent = id;
    }

    let classList = node.classList.length ? "." + [...node.classList].join(".") : "";
    if (info.classesBox.textContent !== classList) {
      info.classesBox.textContent = classList;
    }

    let pseudos = PSEUDO_CLASSES.filter(pseudo => {
      return DOMUtils.hasPseudoClassLock(node, pseudo);
    }, this).join("");
    if (info.pseudoClassesBox.textContent !== pseudos) {
      info.pseudoClassesBox.textContent = pseudos;
    }

    let rect = node.getBoundingClientRect();
    let dim = Math.ceil(rect.width) + " x " + Math.ceil(rect.height);
    if (info.dimensionBox.textContent !== dim) {
      info.dimensionBox.textContent = dim;
    }

    this._moveInfobar();
  },

  


  _moveInfobar: function() {
    let bounds = this._getOuterBounds();
    let winHeight = this.win.innerHeight * this.zoom;
    let winWidth = this.win.innerWidth * this.zoom;

    
    
    let positionerBottom = Math.max(0, bounds.bottom);
    let positionerTop = Math.max(0, bounds.top);

    
    if (this.chromeDoc.defaultView.gBrowser) {
      
      let viewportTop = this.browser.getBoundingClientRect().top;

      
      let findbar = this.chromeDoc.defaultView.gBrowser.getFindBar();
      let findTop = findbar.getBoundingClientRect().top - viewportTop;

      
      positionerTop = Math.min(positionerTop, findTop);
    }

    this.nodeInfo.positioner.removeAttribute("disabled");
    
    if (positionerTop < this.nodeInfo.barHeight) {
      
      if (positionerBottom + this.nodeInfo.barHeight > winHeight) {
        
        this.nodeInfo.positioner.style.top = positionerTop + "px";
        this.nodeInfo.positioner.setAttribute("position", "overlap");
      } else {
        
        this.nodeInfo.positioner.style.top = positionerBottom - INFO_BAR_OFFSET + "px";
        this.nodeInfo.positioner.setAttribute("position", "bottom");
      }
    } else {
      
      this.nodeInfo.positioner.style.top =
        positionerTop + INFO_BAR_OFFSET - this.nodeInfo.barHeight + "px";
      this.nodeInfo.positioner.setAttribute("position", "top");
    }

    let barWidth = this.nodeInfo.positioner.getBoundingClientRect().width;
    let left = bounds.right - bounds.width / 2 - barWidth / 2;

    
    if (left < 0) {
      left = 0;
      this.nodeInfo.positioner.setAttribute("hide-arrow", "true");
    } else {
      if (left + barWidth > winWidth) {
        left = winWidth - barWidth;
        this.nodeInfo.positioner.setAttribute("hide-arrow", "true");
      } else {
        this.nodeInfo.positioner.removeAttribute("hide-arrow");
      }
    }
    this.nodeInfo.positioner.style.left = left + "px";
  }
});






function CssTransformHighlighter(tabActor) {
  XULBasedHighlighter.call(this, tabActor);

  this.layoutHelpers = new LayoutHelpers(tabActor.window);
  this._initMarkup();
}

let MARKER_COUNTER = 1;

CssTransformHighlighter.prototype = Heritage.extend(XULBasedHighlighter.prototype, {
  _initMarkup: function() {
    let stack = this.browser.parentNode;

    this._container = this.chromeDoc.createElement("stack");
    this._container.className = "highlighter-container";

    this._svgRoot = this._createSVGNode("root", "svg", this._container);
    this._svgRoot.setAttribute("hidden", "true");

    
    let marker = this.chromeDoc.createElementNS(SVG_NS, "marker");
    this.markerId = "css-transform-arrow-marker-" + MARKER_COUNTER;
    MARKER_COUNTER ++;
    marker.setAttribute("id", this.markerId);
    marker.setAttribute("markerWidth", "10");
    marker.setAttribute("markerHeight", "5");
    marker.setAttribute("orient", "auto");
    marker.setAttribute("markerUnits", "strokeWidth");
    marker.setAttribute("refX", "10");
    marker.setAttribute("refY", "5");
    marker.setAttribute("viewBox", "0 0 10 10");
    let path = this.chromeDoc.createElementNS(SVG_NS, "path");
    path.setAttribute("d", "M 0 0 L 10 5 L 0 10 z");
    path.setAttribute("fill", "#08C");
    marker.appendChild(path);
    this._svgRoot.appendChild(marker);

    
    let shapesGroup = this._createSVGNode("container", "g", this._svgRoot);
    this._shapes = {
      untransformed: this._createSVGNode("untransformed", "polygon", shapesGroup),
      transformed: this._createSVGNode("transformed", "polygon", shapesGroup)
    };

    
    for (let nb of ["1", "2", "3", "4"]) {
      let line = this._createSVGNode("line", "line", shapesGroup);
      line.setAttribute("marker-end", "url(#" + this.markerId + ")");
      this._shapes["line" + nb] = line;
    }

    this._container.appendChild(this._svgRoot);

    
    stack.insertBefore(this._container, stack.childNodes[1]);
  },

  _createSVGNode: function(classPostfix, nodeType, parent) {
    let node = this.chromeDoc.createElementNS(SVG_NS, nodeType);
    node.setAttribute("class", "css-transform-" + classPostfix);

    parent.appendChild(node);
    return node;
  },

  


  destroy: function() {
    XULBasedHighlighter.prototype.destroy.call(this);

    this._container.remove();
    this._container = null;
  },

  



  _show: function() {
    if (!this._isTransformed(this.currentNode)) {
      this.hide();
      return;
    }

    this._update();
  },

  


  _isTransformed: function(node) {
    let style = node.ownerDocument.defaultView.getComputedStyle(node);
    return style.transform !== "none" && style.display !== "inline";
  },

  _setPolygonPoints: function(quad, poly) {
    let points = [];
    for (let point of ["p1","p2", "p3", "p4"]) {
      points.push(quad[point].x + "," + quad[point].y);
    }
    poly.setAttribute("points", points.join(" "));
  },

  _setLinePoints: function(p1, p2, line) {
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
    
    let quad = this.layoutHelpers.getAdjustedQuads(this.currentNode, "border");
    if (!quad || quad.bounds.width <= 0 || quad.bounds.height <= 0) {
      this._hideShapes();
      return null;
    }

    
    let untransformedQuad = this.layoutHelpers.getNodeBounds(this.currentNode);

    this._setPolygonPoints(quad, this._shapes.transformed);
    this._setPolygonPoints(untransformedQuad, this._shapes.untransformed);
    for (let nb of ["1", "2", "3", "4"]) {
      this._setLinePoints(untransformedQuad["p" + nb], quad["p" + nb],
        this._shapes["line" + nb]);
    }

    this._showShapes();
  },

  


  _hide: function() {
    this._hideShapes();
  },

  _hideShapes: function() {
    this._svgRoot.setAttribute("hidden", "true");
  },

  _showShapes: function() {
    this._svgRoot.removeAttribute("hidden");
  }
});









function SimpleOutlineHighlighter(tabActor) {
  this.chromeDoc = tabActor.window.document;
}

SimpleOutlineHighlighter.prototype = {
  


  destroy: function() {
    this.hide();
    if (this.installedHelpers) {
      this.installedHelpers.clear();
    }
    this.chromeDoc = null;
  },

  _installHelperSheet: function(node) {
    if (!this.installedHelpers) {
      this.installedHelpers = new WeakMap;
    }
    let win = node.ownerDocument.defaultView;
    if (!this.installedHelpers.has(win)) {
      let {Style} = require("sdk/stylesheet/style");
      let {attach} = require("sdk/content/mod");
      let style = Style({source: HELPER_SHEET, type: "agent"});
      attach(style, win);
      this.installedHelpers.set(win, style);
    }
  },

  



  show: function(node) {
    if (!this.currentNode || node !== this.currentNode) {
      this.hide();
      this.currentNode = node;
      this._installHelperSheet(node);
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







function supportXULBasedHighlighter(tabActor) {
  
  
  return tabActor.browser &&
         !!tabActor.browser.parentNode &&
         Services.appinfo.ID !== "{aa3c5121-dab2-40e2-81ca-7ea25febc110}";
}

function isNodeValid(node) {
  
  if(!node || Cu.isDeadWrapper(node)) {
    return false;
  }

  
  if (node.nodeType !== Ci.nsIDOMNode.ELEMENT_NODE) {
    return false;
  }

  
  let doc = node.ownerDocument;
  if (!doc || !doc.defaultView || !doc.documentElement.contains(node)) {
    return false;
  }

  return true;
}

XPCOMUtils.defineLazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils)
});
