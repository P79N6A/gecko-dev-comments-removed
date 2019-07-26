



"use strict";

const {Cu, Cc, Ci} = require("chrome");
const protocol = require("devtools/server/protocol");
const {Arg, Option, method} = protocol;
const events = require("sdk/event/core");

require("devtools/server/actors/inspector");

Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");


const PSEUDO_CLASSES = [":hover", ":active", ":focus"];
const HIGHLIGHTED_PSEUDO_CLASS = ":-moz-devtools-highlighted";
let HELPER_SHEET = ".__fx-devtools-hide-shortcut__ { visibility: hidden !important } ";
HELPER_SHEET += ":-moz-devtools-highlighted { outline: 2px dashed #F06!important; outline-offset: -2px!important } ";
const XHTML_NS = "http://www.w3.org/1999/xhtml";
const HIGHLIGHTER_PICKED_TIMER = 1000;











let HighlighterActor = protocol.ActorClass({
  typeName: "highlighter",

  initialize: function(inspector) {
    protocol.Actor.prototype.initialize.call(this, null);

    this._inspector = inspector;
    this._walker = this._inspector.walker;
    this._tabActor = this._inspector.tabActor;

    if (this._supportsBoxModelHighlighter()) {
      this._boxModelHighlighter = new BoxModelHighlighter(this._tabActor);
    } else {
      this._boxModelHighlighter = new SimpleOutlineHighlighter(this._tabActor);
    }
  },

  get conn() this._inspector && this._inspector.conn,

  



  _supportsBoxModelHighlighter: function() {
    return this._tabActor.browser && !!this._tabActor.browser.parentNode;
  },

  destroy: function() {
    protocol.Actor.prototype.destroy.call(this);
    if (this._boxModelHighlighter) {
      this._boxModelHighlighter.destroy();
      this._boxModelHighlighter = null;
    }
    this._inspector = null;
    this._walker = null;
    this._tabActor = null;
  },

  










  showBoxModel: method(function(node, options={}) {
    if (this._isNodeValidForHighlighting(node.rawNode)) {
      this._boxModelHighlighter.show(node.rawNode, options);
    } else {
      this._boxModelHighlighter.hide();
    }
  }, {
    request: {
      node: Arg(0, "domnode"),
      scrollIntoView: Option(1)
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
      this._tabActor.window.setTimeout(() => {
        this._boxModelHighlighter.hide();
      }, HIGHLIGHTER_PICKED_TIMER);
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
    return actor.isRootActor ? actor.window : actor.browser;
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





























function BoxModelHighlighter(tabActor) {
  this.browser = tabActor.browser;
  this.win = tabActor.window;
  this.chromeDoc = this.browser.ownerDocument;
  this.chromeWin = this.chromeDoc.defaultView;

  this.layoutHelpers = new LayoutHelpers(this.win);
  this.chromeLayoutHelper = new LayoutHelpers(this.chromeWin);

  this.transitionDisabler = null;
  this.pageEventsMuter = null;
  this._update = this._update.bind(this);
  this.currentNode = null;

  this._initMarkup();
}

BoxModelHighlighter.prototype = {
  _initMarkup: function() {
    let stack = this.browser.parentNode;

    this.highlighterContainer = this.chromeDoc.createElement("stack");
    this.highlighterContainer.className = "highlighter-container";

    this.outline = this.chromeDoc.createElement("box");
    this.outline.className = "highlighter-outline";

    let outlineContainer = this.chromeDoc.createElement("box");
    outlineContainer.appendChild(this.outline);
    outlineContainer.className = "highlighter-outline-container";
    this.highlighterContainer.appendChild(outlineContainer);

    let infobarContainer = this.chromeDoc.createElement("box");
    infobarContainer.className = "highlighter-nodeinfobar-container";
    this.highlighterContainer.appendChild(infobarContainer);

    
    stack.insertBefore(this.highlighterContainer, stack.childNodes[1]);

    
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

    
    pseudoClassesBox.textContent = "&nbsp;";

    
    let texthbox = this.chromeDoc.createElement("hbox");
    texthbox.className = "highlighter-nodeinfobar-text";
    texthbox.setAttribute("align", "center");
    texthbox.setAttribute("flex", "1");

    texthbox.appendChild(tagNameLabel);
    texthbox.appendChild(idLabel);
    texthbox.appendChild(classesBox);
    texthbox.appendChild(pseudoClassesBox);

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
      positioner: infobarPositioner,
      barHeight: barHeight,
    };
  },

  


  destroy: function() {
    this.hide();

    this.chromeWin.clearTimeout(this.transitionDisabler);
    this.chromeWin.clearTimeout(this.pageEventsMuter);

    this._contentRect = null;
    this._highlightRect = null;
    this.outline = null;
    this.nodeInfo = null;

    this.highlighterContainer.remove();
    this.highlighterContainer = null;

    this.win = null
    this.browser = null;
    this.chromeDoc = null;
    this.chromeWin = null;
    this.currentNode = null;
  },

  




  show: function(node, options={}) {
    if (!this.currentNode || node !== this.currentNode) {
      this.currentNode = node;

      this._showInfobar();
      this._computeZoomFactor();
      this._detachPageListeners();
      this._attachPageListeners();
      this._update();
      this._trackMutations();

      if (options.scrollIntoView) {
        this.chromeLayoutHelper.scrollIntoViewIfNeeded(node);
      }
    }
  },

  _trackMutations: function() {
    if (this.currentNode) {
      let win = this.currentNode.ownerDocument.defaultView;
      this.currentNodeObserver = win.MutationObserver(this._update);
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

  







  _update: function(brieflyDisableTransitions) {
    if (this.currentNode) {
      let rect = this.layoutHelpers.getDirtyRect(this.currentNode);

      if (this._highlightRectangle(rect, brieflyDisableTransitions)) {
        this._moveInfobar();
        this._updateInfobar();
      } else {
        
        this.hide();
      }
    }
  },

  


  hide: function() {
    if (this.currentNode) {
      this._untrackMutations();
      this.currentNode = null;
      this._hideOutline();
      this._hideInfobar();
      this._detachPageListeners();
    }
  },

  


  _hideInfobar: function() {
    this.nodeInfo.positioner.setAttribute("force-transitions", "true");
    this.nodeInfo.positioner.setAttribute("hidden", "true");
  },

  


  _showInfobar: function() {
    this.nodeInfo.positioner.removeAttribute("hidden");
    this._moveInfobar();
    this.nodeInfo.positioner.removeAttribute("force-transitions");
  },

  


  _hideOutline: function() {
    this.outline.setAttribute("hidden", "true");
  },

  


  _showOutline: function() {
    this.outline.removeAttribute("hidden");
  },

  









  _highlightRectangle: function(aRect, brieflyDisableTransitions) {
    if (!aRect) {
      return false;
    }

    let oldRect = this._contentRect;

    if (oldRect && aRect.top == oldRect.top && aRect.left == oldRect.left &&
        aRect.width == oldRect.width && aRect.height == oldRect.height) {
      this._showOutline();
      return true; 
    }

    let aRectScaled = this.layoutHelpers.getZoomedRect(this.win, aRect);
    let isShown = false;

    if (aRectScaled.left >= 0 && aRectScaled.top >= 0 &&
        aRectScaled.width > 0 && aRectScaled.height > 0) {

      
      
      let top = "top:" + aRectScaled.top + "px;";
      let left = "left:" + aRectScaled.left + "px;";
      let width = "width:" + aRectScaled.width + "px;";
      let height = "height:" + aRectScaled.height + "px;";

      if (brieflyDisableTransitions) {
        this._brieflyDisableTransitions();
      }

      this.outline.setAttribute("style", top + left + width + height);

      isShown = true;
      this._showOutline();
    } else {
      
      
      
      if (aRectScaled.width > 0 || aRectScaled.height > 0) {
        isShown = true;
        this._hideOutline();
      }
    }

    this._contentRect = aRect; 
    this._highlightRect = aRectScaled; 

    return isShown;
  },

  


  _updateInfobar: function() {
    if (this.currentNode) {
      
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
    }
  },

  


  _moveInfobar: function() {
    if (this._highlightRect) {
      let winHeight = this.win.innerHeight * this.zoom;
      let winWidth = this.win.innerWidth * this.zoom;

      let rect = {top: this._highlightRect.top,
                  left: this._highlightRect.left,
                  width: this._highlightRect.width,
                  height: this._highlightRect.height};

      rect.top = Math.max(rect.top, 0);
      rect.left = Math.max(rect.left, 0);
      rect.width = Math.max(rect.width, 0);
      rect.height = Math.max(rect.height, 0);

      rect.top = Math.min(rect.top, winHeight);
      rect.left = Math.min(rect.left, winWidth);

      this.nodeInfo.positioner.removeAttribute("disabled");
      
      if (rect.top < this.nodeInfo.barHeight) {
        
        if (rect.top + rect.height +
            this.nodeInfo.barHeight > winHeight) {
          
          this.nodeInfo.positioner.style.top = rect.top + "px";
          this.nodeInfo.positioner.setAttribute("position", "overlap");
        } else {
          
          this.nodeInfo.positioner.style.top = rect.top + rect.height + "px";
          this.nodeInfo.positioner.setAttribute("position", "bottom");
        }
      } else {
        
        this.nodeInfo.positioner.style.top =
          rect.top - this.nodeInfo.barHeight + "px";
        this.nodeInfo.positioner.setAttribute("position", "top");
      }

      let barWidth = this.nodeInfo.positioner.getBoundingClientRect().width;
      let left = rect.left + rect.width / 2 - barWidth / 2;

      
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

  


  _computeZoomFactor: function() {
    this.zoom =
      this.win.QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDOMWindowUtils)
      .fullZoom;
  },

  _attachPageListeners: function() {
    this.browser.addEventListener("resize", this, true);
    this.browser.addEventListener("scroll", this, true);
    this.browser.addEventListener("MozAfterPaint", this, true);
  },

  _detachPageListeners: function() {
    this.browser.removeEventListener("resize", this, true);
    this.browser.removeEventListener("scroll", this, true);
    this.browser.removeEventListener("MozAfterPaint", this, true);
  },

  





  handleEvent: function(event) {
    switch (event.type) {
      case "resize":
        this._computeZoomFactor();
        break;
      case "MozAfterPaint":
      case "scroll":
        this._update(true);
        break;
    }
  },

  



  _brieflyDisableTransitions: function() {
    if (this.transitionDisabler) {
      this.chromeWin.clearTimeout(this.transitionDisabler);
    } else {
      this.outline.setAttribute("disable-transitions", "true");
      this.nodeInfo.positioner.setAttribute("disable-transitions", "true");
    }
    this.transitionDisabler =
      this.chromeWin.setTimeout(() => {
        this.outline.removeAttribute("disable-transitions");
        this.nodeInfo.positioner.removeAttribute("disable-transitions");
        this.transitionDisabler = null;
      }, 500);
  }
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
