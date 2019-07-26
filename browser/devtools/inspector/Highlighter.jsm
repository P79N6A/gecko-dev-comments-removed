





const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/LayoutHelpers.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/EventEmitter.jsm");

this.EXPORTED_SYMBOLS = ["Highlighter"];

const PSEUDO_CLASSES = [":hover", ":active", ":focus"];
  

























































this.Highlighter = function Highlighter(aTarget, aInspector, aToolbox)
{
  this.target = aTarget;
  this.tab = aTarget.tab;
  this.toolbox = aToolbox;
  this.browser = this.tab.linkedBrowser;
  this.chromeDoc = this.tab.ownerDocument;
  this.chromeWin = this.chromeDoc.defaultView;
  this.inspector = aInspector

  EventEmitter.decorate(this);

  this._init();
}

Highlighter.prototype = {
  get selection() {
    return this.inspector.selection;
  },

  _init: function Highlighter__init()
  {
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

    this.unlockAndFocus();

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

  


  destroy: function Highlighter_destroy()
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

  


  highlight: function Highlighter_highlight()
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
        LayoutHelpers.scrollIntoViewIfNeeded(this.selection.node);
      }
    } else {
      this.disabled = true;
      this.hide();
    }
  },

  


  invalidateSize: function Highlighter_invalidateSize()
  {
    let canHiglightNode = this.selection.isNode() &&
                          this.selection.isConnected() &&
                          this.selection.isElementNode();

    if (!canHiglightNode)
      return;

    let clientRect = this.selection.node.getBoundingClientRect();
    let rect = LayoutHelpers.getDirtyRect(this.selection.node);
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

  


  unlockAndFocus: function Highlighter_unlockAndFocus() {
    if (this.locked === false) return;
    this.chromeWin.focus();
    this.unlock();
  },

  


   hideInfobar: function Highlighter_hideInfobar() {
     this.nodeInfo.container.setAttribute("force-transitions", "true");
     this.nodeInfo.container.setAttribute("hidden", "true");
   },

  


   showInfobar: function Highlighter_showInfobar() {
     this.nodeInfo.container.removeAttribute("hidden");
     this.moveInfobar();
     this.nodeInfo.container.removeAttribute("force-transitions");
   },

  


   hideOutline: function Highlighter_hideOutline() {
     this.outline.setAttribute("hidden", "true");
   },

  


   showOutline: function Highlighter_showOutline() {
     if (this._highlighting)
       this.outline.removeAttribute("hidden");
   },

  




















  buildInfobar: function Highlighter_buildInfobar(aParent)
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
    this.inspectButton.addEventListener("command", this.unlockAndFocus);

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
        LayoutHelpers.scrollIntoViewIfNeeded(this.selection.node);
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

  







  highlightRectangle: function Highlighter_highlightRectangle(aRect)
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

    let aRectScaled = LayoutHelpers.getZoomedRect(this.win, aRect);

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

  


  unhighlight: function Highlighter_unhighlight()
  {
    this._highlighting = false;
    this.hideOutline();
  },

  


  updateInfobar: function Highlighter_updateInfobar()
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

  


  moveInfobar: function Highlighter_moveInfobar()
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

  


  computeZoomFactor: function Highlighter_computeZoomFactor() {
    this.zoom =
      this.win.QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDOMWindowUtils)
      .fullZoom;
  },

  
  

  attachMouseListeners: function Highlighter_attachMouseListeners()
  {
    this.browser.addEventListener("mousemove", this, true);
    this.browser.addEventListener("click", this, true);
    this.browser.addEventListener("dblclick", this, true);
    this.browser.addEventListener("mousedown", this, true);
    this.browser.addEventListener("mouseup", this, true);
  },

  detachMouseListeners: function Highlighter_detachMouseListeners()
  {
    this.browser.removeEventListener("mousemove", this, true);
    this.browser.removeEventListener("click", this, true);
    this.browser.removeEventListener("dblclick", this, true);
    this.browser.removeEventListener("mousedown", this, true);
    this.browser.removeEventListener("mouseup", this, true);
  },

  attachPageListeners: function Highlighter_attachPageListeners()
  {
    this.browser.addEventListener("resize", this, true);
    this.browser.addEventListener("scroll", this, true);
    this.browser.addEventListener("MozAfterPaint", this, true);
  },

  detachPageListeners: function Highlighter_detachPageListeners()
  {
    this.browser.removeEventListener("resize", this, true);
    this.browser.removeEventListener("scroll", this, true);
    this.browser.removeEventListener("MozAfterPaint", this, true);
  },

  





  handleEvent: function Highlighter_handleEvent(aEvent)
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

  



  brieflyDisableTransitions: function Highlighter_brieflyDisableTransitions()
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

  


  brieflyIgnorePageEvents: function Highlighter_brieflyIgnorePageEvents()
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

  





  handleClick: function Highlighter_handleClick(aEvent)
  {
    
    if (aEvent.button == 0) {
      this.lock();
      let node = this.selection.node;
      this.selection.setNode(node, "highlighter-lock");
      aEvent.preventDefault();
      aEvent.stopPropagation();
    }
  },

  





  handleMouseMove: function Highlighter_handleMouseMove(aEvent)
  {
    let doc = aEvent.target.ownerDocument;

    
    
    if (doc && doc != this.chromeDoc) {
      let element = LayoutHelpers.getElementFromPoint(aEvent.target.ownerDocument,
        aEvent.clientX, aEvent.clientY);
      if (element && element != this.selection.node) {
        this.selection.setNode(element, "highlighter");
      }
    }
  },
};



XPCOMUtils.defineLazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils)
});

XPCOMUtils.defineLazyGetter(Highlighter.prototype, "strings", function () {
    return Services.strings.createBundle(
            "chrome://browser/locale/devtools/inspector.properties");
});
