



"use strict";

const {Cu, Cc, Ci} = require("chrome");
const Services = require("Services");
const protocol = require("devtools/server/protocol");
const {Arg, Option, method} = protocol;
const events = require("sdk/event/core");

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











let HighlighterActor = protocol.ActorClass({
  typeName: "highlighter",

  initialize: function(inspector, autohide) {
    protocol.Actor.prototype.initialize.call(this, null);

    this._autohide = autohide;
    this._inspector = inspector;
    this._walker = this._inspector.walker;
    this._tabActor = this._inspector.tabActor;

    this._highlighterReady = this._highlighterReady.bind(this);
    this._highlighterHidden = this._highlighterHidden.bind(this);

    if (this._supportsBoxModelHighlighter()) {
      this._boxModelHighlighter =
        new BoxModelHighlighter(this._tabActor, this._inspector);

        this._boxModelHighlighter.on("ready", this._highlighterReady);
        this._boxModelHighlighter.on("hide", this._highlighterHidden);
    } else {
      this._boxModelHighlighter = new SimpleOutlineHighlighter(this._tabActor);
    }
  },

  get conn() this._inspector && this._inspector.conn,

  



  _supportsBoxModelHighlighter: function() {
    
    
    return this._tabActor.browser &&
           !!this._tabActor.browser.parentNode &&
           Services.appinfo.ID !== "{aa3c5121-dab2-40e2-81ca-7ea25febc110}";
  },

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
    if (node && this._isNodeValidForHighlighting(node.rawNode)) {
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

  _isNodeValidForHighlighting: function(node) {
    
    let isNotDead = node && !Cu.isDeadWrapper(node);

    
    let isConnected = false;
    try {
      let doc = node.ownerDocument;
      isConnected = (doc && doc.defaultView && doc.documentElement.contains(node));
    } catch (e) {
      
    }

    
    let isElementNode = node.nodeType === Ci.nsIDOMNode.ELEMENT_NODE;

    return isNotDead && isConnected && isElementNode;
  },

  


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

exports.HighlighterActor = HighlighterActor;




let HighlighterFront = protocol.FrontClass(HighlighterActor, {});











































function BoxModelHighlighter(tabActor, inspector) {
  this.browser = tabActor.browser;
  this.win = tabActor.window;
  this.chromeDoc = this.browser.ownerDocument;
  this.chromeWin = this.chromeDoc.defaultView;
  this._inspector = inspector;

  this.layoutHelpers = new LayoutHelpers(this.win);
  this.chromeLayoutHelper = new LayoutHelpers(this.chromeWin);

  this.transitionDisabler = null;
  this.pageEventsMuter = null;
  this._update = this._update.bind(this);
  this.handleEvent = this.handleEvent.bind(this);
  this.currentNode = null;

  EventEmitter.decorate(this);
  this._initMarkup();
}

BoxModelHighlighter.prototype = {
  get zoom() {
    return this.win.QueryInterface(Ci.nsIInterfaceRequestor)
               .getInterface(Ci.nsIDOMWindowUtils).fullZoom;
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
    this.hide();

    this.chromeWin.clearTimeout(this.transitionDisabler);
    this.chromeWin.clearTimeout(this.pageEventsMuter);

    this.nodeInfo = null;

    this._highlighterContainer.remove();
    this._highlighterContainer = null;

    this.rect = null;
    this.win = null;
    this.browser = null;
    this.chromeDoc = null;
    this.chromeWin = null;
    this.currentNode = null;
  },

  






  show: function(node, options={}) {
    this.currentNode = node;

    this._showInfobar();
    this._detachPageListeners();
    this._attachPageListeners();
    this._update();
    this._trackMutations();
  },

  _trackMutations: function() {
    if (this.currentNode) {
      let win = this.currentNode.ownerDocument.defaultView;
      this.currentNodeObserver = new win.MutationObserver(() => {
        this._update();
      });
      this.currentNodeObserver.observe(this.currentNode, {attributes: true});
    }
  },

  _untrackMutations: function() {
    if (this.currentNode) {
      if (this.currentNodeObserver) {
        
        
        try {
          this.currentNodeObserver.disconnect();
        } catch (e) {}
        this.currentNodeObserver = null;
      }
    }
  },

  








  _update: function(options={}) {
    if (this.currentNode) {
      if (this._highlightBoxModel(options)) {
        this._showInfobar();
      } else {
        
        this.hide();
      }
      this.emit("ready");
    }
  },

  


  hide: function() {
    if (this.currentNode) {
      this._untrackMutations();
      this.currentNode = null;
      this._hideBoxModel();
      this._hideInfobar();
      this._detachPageListeners();
    }
    this.emit("hide");
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

  









  _highlightBoxModel: function(options) {
    let isShown = false;

    options.region = options.region || "content";

    this.rect = this.layoutHelpers.getAdjustedQuads(this.currentNode, "margin");

    if (!this.rect) {
      return null;
    }

    if (this.rect.bounds.width > 0 && this.rect.bounds.height > 0) {
      for (let boxType in this._boxModelNodes) {
        let {p1, p2, p3, p4} = boxType === "margin" ? this.rect :
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

      isShown = true;
      this._showBoxModel();
    } else {
      
      
      
      if (this.rect.width > 0 || this.rect.height > 0) {
        isShown = true;
        this._hideBoxModel();
      }
    }
    return isShown;
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

    
    this.nodeInfo.tagNameLabel.textContent = this.currentNode.tagName;

    
    this.nodeInfo.idLabel.textContent = this.currentNode.id ? "#" + this.currentNode.id : "";

    
    let classes = this.nodeInfo.classesBox;

    classes.textContent = this.currentNode.classList.length ?
                            "." + Array.join(this.currentNode.classList, ".") : "";

    
    let pseudos = PSEUDO_CLASSES.filter(pseudo => {
      return DOMUtils.hasPseudoClassLock(this.currentNode, pseudo);
    }, this);

    let pseudoBox = this.nodeInfo.pseudoClassesBox;
    pseudoBox.textContent = pseudos.join("");

    
    let dimensionBox = this.nodeInfo.dimensionBox;
    let rect = this.currentNode.getBoundingClientRect();
    dimensionBox.textContent = Math.ceil(rect.width) + " x " +
                               Math.ceil(rect.height);
    this._moveInfobar();
  },

  


  _moveInfobar: function() {
    if (this.rect) {
      let bounds = this.rect.bounds;
      let winHeight = this.win.innerHeight * this.zoom;
      let winWidth = this.win.innerWidth * this.zoom;

      this.nodeInfo.positioner.removeAttribute("disabled");
      
      if (bounds.top < this.nodeInfo.barHeight) {
        
        if (bounds.bottom + this.nodeInfo.barHeight > winHeight) {
          
          this.nodeInfo.positioner.style.top = bounds.top + "px";
          this.nodeInfo.positioner.setAttribute("position", "overlap");
        } else {
          
          this.nodeInfo.positioner.style.top = bounds.bottom - INFO_BAR_OFFSET + "px";
          this.nodeInfo.positioner.setAttribute("position", "bottom");
        }
      } else {
        
        this.nodeInfo.positioner.style.top =
          bounds.top + INFO_BAR_OFFSET - this.nodeInfo.barHeight + "px";
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
    } else {
      this.nodeInfo.positioner.style.left = "0";
      this.nodeInfo.positioner.style.top = "0";
      this.nodeInfo.positioner.setAttribute("position", "top");
      this.nodeInfo.positioner.setAttribute("hide-arrow", "true");
    }
  },

  _attachPageListeners: function() {
    if (this.currentNode) {
      let win = this.currentNode.ownerGlobal;

      win.addEventListener("scroll", this, false);
      win.addEventListener("resize", this, false);
      win.addEventListener("MozAfterPaint", this, false);
    }
  },

  _detachPageListeners: function() {
    if (this.currentNode) {
      let win = this.currentNode.ownerGlobal;

      win.removeEventListener("scroll", this, false);
      win.removeEventListener("resize", this, false);
      win.removeEventListener("MozAfterPaint", this, false);
    }
  },

  





  handleEvent: function(event) {
    switch (event.type) {
      case "resize":
      case "MozAfterPaint":
      case "scroll":
        this._update();
        break;
    }
  },
};









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

XPCOMUtils.defineLazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils)
});
