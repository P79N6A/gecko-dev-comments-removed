





const {Cu, Cc, Ci} = require("chrome");

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let EventEmitter = require("devtools/shared/event-emitter");

const PSEUDO_CLASSES = [":hover", ":active", ":focus"];
  

exports._forceBasic = {value: false};

exports.Highlighter = function Highlighter(aTarget, aInspector, aToolbox) {
  if (aTarget.isLocalTab && !exports._forceBasic.value) {
    return new LocalHighlighter(aTarget, aInspector, aToolbox);
  } else {
    return new BasicHighlighter(aTarget, aInspector, aToolbox);
  }
}

exports.LocalHighlighter = LocalHighlighter;
exports.BasicHighlighter = BasicHighlighter;

























































function LocalHighlighter(aTarget, aInspector, aToolbox)
{
  this.target = aTarget;
  this.tab = aTarget.tab;
  this.toolbox = aToolbox;
  this.browser = this.tab.linkedBrowser;
  this.chromeDoc = this.tab.ownerDocument;
  this.chromeWin = this.chromeDoc.defaultView;
  this.inspector = aInspector
  this.layoutHelpers = new LayoutHelpers(this.browser.contentWindow);

  EventEmitter.decorate(this);

  this._init();
}

LocalHighlighter.prototype = {
  get selection() {
    return this.inspector.selection;
  },

  _init: function LocalHighlighter__init()
  {
    this.toggleLockState = this.toggleLockState.bind(this);
    this.unlockAndFocus = this.unlockAndFocus.bind(this);
    this.updateInfobar = this.updateInfobar.bind(this);
    this.highlight = this.highlight.bind(this);

    let stack = this.browser.parentNode;
    this.win = this.browser.contentWindow;
    this._highlighting = false;

    this.highlighterContainer = this.chromeDoc.createElement("stack");
    this.highlighterContainer.className = "highlighter-container";

    this.outline = this.chromeDoc.createElement("box");
    this.outline.className = "highlighter-outline";

    let outlineContainer = this.chromeDoc.createElement("box");
    outlineContainer.appendChild(this.outline);
    outlineContainer.className = "highlighter-outline-container";

    
    
    let controlsBox = this.chromeDoc.createElement("box");
    controlsBox.className = "highlighter-controls";
    this.highlighterContainer.appendChild(outlineContainer);
    this.highlighterContainer.appendChild(controlsBox);

    
    stack.insertBefore(this.highlighterContainer, stack.childNodes[1]);

    this.buildInfobar(controlsBox);

    this.transitionDisabler = null;
    this.pageEventsMuter = null;

    this.selection.on("new-node", this.highlight);
    this.selection.on("new-node", this.updateInfobar);
    this.selection.on("pseudoclass", this.updateInfobar);
    this.selection.on("attribute-changed", this.updateInfobar);

    this.onToolSelected = function(event, id) {
      if (id != "inspector") {
        this.chromeWin.clearTimeout(this.pageEventsMuter);
        this.detachMouseListeners();
        this.disabled = true;
        this.hide();
      } else {
        if (!this.locked) {
          this.attachMouseListeners();
        }
        this.disabled = false;
        this.show();
      }
    }.bind(this);
    this.toolbox.on("select", this.onToolSelected);

    this.hidden = true;
    this.highlight();
  },

  


  destroy: function LocalHighlighter_destroy()
  {
    this.inspectButton.removeEventListener("command", this.unlockAndFocus);
    this.inspectButton = null;

    this.toolbox.off("select", this.onToolSelected);
    this.toolbox = null;

    this.selection.off("new-node", this.highlight);
    this.selection.off("new-node", this.updateInfobar);
    this.selection.off("pseudoclass", this.updateInfobar);
    this.selection.off("attribute-changed", this.updateInfobar);

    this.detachMouseListeners();
    this.detachPageListeners();

    this.chromeWin.clearTimeout(this.transitionDisabler);
    this.chromeWin.clearTimeout(this.pageEventsMuter);
    this.boundCloseEventHandler = null;
    this._contentRect = null;
    this._highlightRect = null;
    this._highlighting = false;
    this.outline = null;
    this.nodeInfo = null;
    this.highlighterContainer.parentNode.removeChild(this.highlighterContainer);
    this.highlighterContainer = null;
    this.win = null
    this.browser = null;
    this.chromeDoc = null;
    this.chromeWin = null;
    this.tabbrowser = null;

    this.emit("closed");
  },

  


  highlight: function LocalHighlighter_highlight()
  {
    if (this.selection.reason != "highlighter") {
      this.lock();
    }

    let canHighlightNode = this.selection.isNode() &&
                          this.selection.isConnected() &&
                          this.selection.isElementNode();

    if (canHighlightNode) {
      if (this.selection.reason != "navigateaway") {
        this.disabled = false;
      }
      this.show();
      this.updateInfobar();
      this.invalidateSize();
      if (!this._highlighting &&
          this.selection.reason != "highlighter") {
        this.layoutHelpers.scrollIntoViewIfNeeded(this.selection.node);
      }
    } else {
      this.disabled = true;
      this.hide();
    }
  },

  


  invalidateSize: function LocalHighlighter_invalidateSize()
  {
    let canHiglightNode = this.selection.isNode() &&
                          this.selection.isConnected() &&
                          this.selection.isElementNode();

    if (!canHiglightNode)
      return;

    
    
    
    if (!this.selection.node ||
        !this.selection.node.ownerDocument ||
        !this.selection.node.ownerDocument.defaultView) {
      return;
    }

    let clientRect = this.selection.node.getBoundingClientRect();
    let rect = this.layoutHelpers.getDirtyRect(this.selection.node);
    this.highlightRectangle(rect);

    this.moveInfobar();

    if (this._highlighting) {
      this.showOutline();
      this.emit("highlighting");
    }
  },

  


  show: function() {
    if (!this.hidden || this.disabled) return;
    this.showOutline();
    this.showInfobar();
    this.computeZoomFactor();
    this.attachPageListeners();
    this.invalidateSize();
    this.hidden = false;
  },

  


  hide: function() {
    if (this.hidden) return;
    this.hideOutline();
    this.hideInfobar();
    this.detachPageListeners();
    this.hidden = true;
  },

  




  isHidden: function() {
    return this.hidden;
  },

  


  lock: function() {
    if (this.locked === true) return;
    this.outline.setAttribute("locked", "true");
    this.nodeInfo.container.setAttribute("locked", "true");
    this.detachMouseListeners();
    this.locked = true;
    this.emit("locked");
  },

  



  unlock: function() {
    if (this.locked === false) return;
    this.outline.removeAttribute("locked");
    this.nodeInfo.container.removeAttribute("locked");
    this.attachMouseListeners();
    this.locked = false;
    if (this.selection.isElementNode() &&
        this.selection.isConnected()) {
      this.showOutline();
    }
    this.emit("unlocked");
  },

  


  toggleLockState: function() {
    if (this.locked) {
      this.startNode = this.selection.node;
      this.unlockAndFocus();
    } else {
      this.selection.setNode(this.startNode);
      this.lock();
    }
  },

  


  unlockAndFocus: function LocalHighlighter_unlockAndFocus() {
    if (this.locked === false) return;
    this.chromeWin.focus();
    this.unlock();
  },

  


   hideInfobar: function LocalHighlighter_hideInfobar() {
     this.nodeInfo.container.setAttribute("force-transitions", "true");
     this.nodeInfo.container.setAttribute("hidden", "true");
   },

  


   showInfobar: function LocalHighlighter_showInfobar() {
     this.nodeInfo.container.removeAttribute("hidden");
     this.moveInfobar();
     this.nodeInfo.container.removeAttribute("force-transitions");
   },

  


   hideOutline: function LocalHighlighter_hideOutline() {
     this.outline.setAttribute("hidden", "true");
   },

  


   showOutline: function LocalHighlighter_showOutline() {
     if (this._highlighting)
       this.outline.removeAttribute("hidden");
   },

  




















  buildInfobar: function LocalHighlighter_buildInfobar(aParent)
  {
    let container = this.chromeDoc.createElement("box");
    container.className = "highlighter-nodeinfobar-container";
    container.setAttribute("position", "top");
    container.setAttribute("disabled", "true");

    let nodeInfobar = this.chromeDoc.createElement("hbox");
    nodeInfobar.className = "highlighter-nodeinfobar";

    let arrowBoxTop = this.chromeDoc.createElement("box");
    arrowBoxTop.className = "highlighter-nodeinfobar-arrow highlighter-nodeinfobar-arrow-top";

    let arrowBoxBottom = this.chromeDoc.createElement("box");
    arrowBoxBottom.className = "highlighter-nodeinfobar-arrow highlighter-nodeinfobar-arrow-bottom";

    let tagNameLabel = this.chromeDoc.createElementNS("http://www.w3.org/1999/xhtml", "span");
    tagNameLabel.className = "highlighter-nodeinfobar-tagname";

    let idLabel = this.chromeDoc.createElementNS("http://www.w3.org/1999/xhtml", "span");
    idLabel.className = "highlighter-nodeinfobar-id";

    let classesBox = this.chromeDoc.createElementNS("http://www.w3.org/1999/xhtml", "span");
    classesBox.className = "highlighter-nodeinfobar-classes";

    let pseudoClassesBox = this.chromeDoc.createElementNS("http://www.w3.org/1999/xhtml", "span");
    pseudoClassesBox.className = "highlighter-nodeinfobar-pseudo-classes";

    
    pseudoClassesBox.textContent = "&nbsp;";

    

    this.inspectButton = this.chromeDoc.createElement("toolbarbutton");
    this.inspectButton.className = "highlighter-nodeinfobar-button highlighter-nodeinfobar-inspectbutton"
    let toolbarInspectButton = this.inspector.panelDoc.getElementById("inspector-inspect-toolbutton");
    this.inspectButton.setAttribute("tooltiptext", toolbarInspectButton.getAttribute("tooltiptext"));
    this.inspectButton.addEventListener("command", this.toggleLockState);

    let nodemenu = this.chromeDoc.createElement("toolbarbutton");
    nodemenu.setAttribute("type", "menu");
    nodemenu.className = "highlighter-nodeinfobar-button highlighter-nodeinfobar-menu"
    nodemenu.setAttribute("tooltiptext",
                          this.strings.GetStringFromName("nodeMenu.tooltiptext"));

    nodemenu.onclick = function() {
      this.inspector.showNodeMenu(nodemenu, "after_start");
    }.bind(this);

    
    let texthbox = this.chromeDoc.createElement("hbox");
    texthbox.className = "highlighter-nodeinfobar-text";
    texthbox.setAttribute("align", "center");
    texthbox.setAttribute("flex", "1");

    texthbox.addEventListener("mousedown", function(aEvent) {
      
      if (this.selection.isElementNode()) {
        this.layoutHelpers.scrollIntoViewIfNeeded(this.selection.node);
      }
    }.bind(this), true);

    texthbox.appendChild(tagNameLabel);
    texthbox.appendChild(idLabel);
    texthbox.appendChild(classesBox);
    texthbox.appendChild(pseudoClassesBox);

    nodeInfobar.appendChild(this.inspectButton);
    nodeInfobar.appendChild(texthbox);
    nodeInfobar.appendChild(nodemenu);

    container.appendChild(arrowBoxTop);
    container.appendChild(nodeInfobar);
    container.appendChild(arrowBoxBottom);

    aParent.appendChild(container);

    let barHeight = container.getBoundingClientRect().height;

    this.nodeInfo = {
      tagNameLabel: tagNameLabel,
      idLabel: idLabel,
      classesBox: classesBox,
      pseudoClassesBox: pseudoClassesBox,
      container: container,
      barHeight: barHeight,
    };
  },

  







  highlightRectangle: function LocalHighlighter_highlightRectangle(aRect)
  {
    if (!aRect) {
      this.unhighlight();
      return;
    }

    let oldRect = this._contentRect;

    if (oldRect && aRect.top == oldRect.top && aRect.left == oldRect.left &&
        aRect.width == oldRect.width && aRect.height == oldRect.height) {
      return; 
    }

    let aRectScaled = this.layoutHelpers.getZoomedRect(this.win, aRect);

    if (aRectScaled.left >= 0 && aRectScaled.top >= 0 &&
        aRectScaled.width > 0 && aRectScaled.height > 0) {

      this.showOutline();

      
      
      let top = "top:" + aRectScaled.top + "px;";
      let left = "left:" + aRectScaled.left + "px;";
      let width = "width:" + aRectScaled.width + "px;";
      let height = "height:" + aRectScaled.height + "px;";
      this.outline.setAttribute("style", top + left + width + height);

      this._highlighting = true;
    } else {
      this.unhighlight();
    }

    this._contentRect = aRect; 
    this._highlightRect = aRectScaled; 

    return;
  },

  


  unhighlight: function LocalHighlighter_unhighlight()
  {
    this._highlighting = false;
    this.hideOutline();
  },

  


  updateInfobar: function LocalHighlighter_updateInfobar()
  {
    if (!this.selection.isElementNode()) {
      this.nodeInfo.tagNameLabel.textContent = "";
      this.nodeInfo.idLabel.textContent = "";
      this.nodeInfo.classesBox.textContent = "";
      this.nodeInfo.pseudoClassesBox.textContent = "";
      return;
    }

    let node = this.selection.node;

    
    this.nodeInfo.tagNameLabel.textContent = node.tagName;

    
    this.nodeInfo.idLabel.textContent = node.id ? "#" + node.id : "";

    
    let classes = this.nodeInfo.classesBox;

    classes.textContent = node.classList.length ?
                            "." + Array.join(node.classList, ".") : "";

    
    let pseudos = PSEUDO_CLASSES.filter(function(pseudo) {
      return DOMUtils.hasPseudoClassLock(node, pseudo);
    }, this);

    let pseudoBox = this.nodeInfo.pseudoClassesBox;
    pseudoBox.textContent = pseudos.join("");
  },

  


  moveInfobar: function LocalHighlighter_moveInfobar()
  {
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

      this.nodeInfo.container.removeAttribute("disabled");
      
      if (rect.top < this.nodeInfo.barHeight) {
        
        if (rect.top + rect.height +
            this.nodeInfo.barHeight > winHeight) {
          
          this.nodeInfo.container.style.top = rect.top + "px";
          this.nodeInfo.container.setAttribute("position", "overlap");
        } else {
          
          this.nodeInfo.container.style.top = rect.top + rect.height + "px";
          this.nodeInfo.container.setAttribute("position", "bottom");
        }
      } else {
        
        this.nodeInfo.container.style.top =
          rect.top - this.nodeInfo.barHeight + "px";
        this.nodeInfo.container.setAttribute("position", "top");
      }

      let barWidth = this.nodeInfo.container.getBoundingClientRect().width;
      let left = rect.left + rect.width / 2 - barWidth / 2;

      
      if (left < 0) {
        left = 0;
        this.nodeInfo.container.setAttribute("hide-arrow", "true");
      } else {
        if (left + barWidth > winWidth) {
          left = winWidth - barWidth;
          this.nodeInfo.container.setAttribute("hide-arrow", "true");
        } else {
          this.nodeInfo.container.removeAttribute("hide-arrow");
        }
      }
      this.nodeInfo.container.style.left = left + "px";
    } else {
      this.nodeInfo.container.style.left = "0";
      this.nodeInfo.container.style.top = "0";
      this.nodeInfo.container.setAttribute("position", "top");
      this.nodeInfo.container.setAttribute("hide-arrow", "true");
    }
  },

  


  computeZoomFactor: function LocalHighlighter_computeZoomFactor() {
    this.zoom =
      this.win.QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDOMWindowUtils)
      .fullZoom;
  },

  
  

  attachMouseListeners: function LocalHighlighter_attachMouseListeners()
  {
    this.browser.addEventListener("mousemove", this, true);
    this.browser.addEventListener("click", this, true);
    this.browser.addEventListener("dblclick", this, true);
    this.browser.addEventListener("mousedown", this, true);
    this.browser.addEventListener("mouseup", this, true);
  },

  detachMouseListeners: function LocalHighlighter_detachMouseListeners()
  {
    this.browser.removeEventListener("mousemove", this, true);
    this.browser.removeEventListener("click", this, true);
    this.browser.removeEventListener("dblclick", this, true);
    this.browser.removeEventListener("mousedown", this, true);
    this.browser.removeEventListener("mouseup", this, true);
  },

  attachPageListeners: function LocalHighlighter_attachPageListeners()
  {
    this.browser.addEventListener("resize", this, true);
    this.browser.addEventListener("scroll", this, true);
    this.browser.addEventListener("MozAfterPaint", this, true);
  },

  detachPageListeners: function LocalHighlighter_detachPageListeners()
  {
    this.browser.removeEventListener("resize", this, true);
    this.browser.removeEventListener("scroll", this, true);
    this.browser.removeEventListener("MozAfterPaint", this, true);
  },

  





  handleEvent: function LocalHighlighter_handleEvent(aEvent)
  {
    switch (aEvent.type) {
      case "click":
        this.handleClick(aEvent);
        break;
      case "mousemove":
        this.brieflyIgnorePageEvents();
        this.handleMouseMove(aEvent);
        break;
      case "resize":
        this.computeZoomFactor();
        break;
      case "MozAfterPaint":
      case "scroll":
        this.brieflyDisableTransitions();
        this.invalidateSize();
        break;
      case "dblclick":
      case "mousedown":
      case "mouseup":
        aEvent.stopPropagation();
        aEvent.preventDefault();
        break;
    }
  },

  



  brieflyDisableTransitions: function LocalHighlighter_brieflyDisableTransitions()
  {
    if (this.transitionDisabler) {
      this.chromeWin.clearTimeout(this.transitionDisabler);
    } else {
      this.outline.setAttribute("disable-transitions", "true");
      this.nodeInfo.container.setAttribute("disable-transitions", "true");
    }
    this.transitionDisabler =
      this.chromeWin.setTimeout(function() {
        this.outline.removeAttribute("disable-transitions");
        this.nodeInfo.container.removeAttribute("disable-transitions");
        this.transitionDisabler = null;
      }.bind(this), 500);
  },

  


  brieflyIgnorePageEvents: function LocalHighlighter_brieflyIgnorePageEvents()
  {
    
    
    
    
    
    
    
    
    
    if (this.pageEventsMuter) {
      this.chromeWin.clearTimeout(this.pageEventsMuter);
    } else {
      this.detachPageListeners();
    }
    this.pageEventsMuter =
      this.chromeWin.setTimeout(function() {
        this.attachPageListeners();
        
        this.computeZoomFactor();
        this.pageEventsMuter = null;
      }.bind(this), 500);
  },

  





  handleClick: function LocalHighlighter_handleClick(aEvent)
  {
    
    if (aEvent.button == 0) {
      this.lock();
      let node = this.selection.node;
      this.selection.setNode(node, "highlighter-lock");
      aEvent.preventDefault();
      aEvent.stopPropagation();
    }
  },

  





  handleMouseMove: function LocalHighlighter_handleMouseMove(aEvent)
  {
    let doc = aEvent.target.ownerDocument;

    
    
    if (doc && doc != this.chromeDoc) {
      let element = this.layoutHelpers.getElementFromPoint(aEvent.target.ownerDocument,
        aEvent.clientX, aEvent.clientY);
      if (element && element != this.selection.node) {
        this.selection.setNode(element, "highlighter");
      }
    }
  },
};




function BasicHighlighter(aTarget, aInspector)
{
  this.walker = aInspector.walker;
  this.selection = aInspector.selection;
  this.highlight = this.highlight.bind(this);
  this.selection.on("new-node-front", this.highlight);
  EventEmitter.decorate(this);
  this.locked = true;
}

BasicHighlighter.prototype = {
  destroy: function() {
    this.walker.highlight(null);
    this.selection.off("new-node-front", this.highlight);
    this.walker = null;
    this.selection = null;
  },
  toggleLockState: function() {
    this.locked = !this.locked;
    if (this.locked) {
      this.walker.cancelPick();
    } else {
      this.emit("unlocked");
      this.walker.pick().then(
        (node) => this._onPick(node),
        () => this._onPick(null)
      );
    }
  },
  highlight: function() {
    this.walker.highlight(this.selection.nodeFront);
  },
  _onPick: function(node) {
    if (node) {
      this.selection.setNodeFront(node);
    }
    this.locked = true;
    this.emit("locked");
  },
  hide: function() {},
  show: function() {},
}



XPCOMUtils.defineLazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils)
});

XPCOMUtils.defineLazyGetter(LocalHighlighter.prototype, "strings", function () {
    return Services.strings.createBundle(
            "chrome://browser/locale/devtools/inspector.properties");
});
