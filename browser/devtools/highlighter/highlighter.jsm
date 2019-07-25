





const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/LayoutHelpers.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

var EXPORTED_SYMBOLS = ["Highlighter"];

const INSPECTOR_INVISIBLE_ELEMENTS = {
  "head": true,
  "base": true,
  "basefont": true,
  "isindex": true,
  "link": true,
  "meta": true,
  "script": true,
  "style": true,
  "title": true,
};

const PSEUDO_CLASSES = [":hover", ":active", ":focus"];
  





















































































function Highlighter(aWindow)
{
  this.chromeWin = aWindow;
  this.tabbrowser = aWindow.gBrowser;
  this.chromeDoc = aWindow.document;
  this.browser = aWindow.gBrowser.selectedBrowser;
  this.events = {};

  this._init();
}

Highlighter.prototype = {
  _init: function Highlighter__init()
  {
    let stack = this.browser.parentNode;
    this.win = this.browser.contentWindow;
    this._highlighting = false;

    this.highlighterContainer = this.chromeDoc.createElement("stack");
    this.highlighterContainer.id = "highlighter-container";

    this.outline = this.chromeDoc.createElement("box");
    this.outline.id = "highlighter-outline";

    let outlineContainer = this.chromeDoc.createElement("box");
    outlineContainer.appendChild(this.outline);
    outlineContainer.id = "highlighter-outline-container";

    
    
    let controlsBox = this.chromeDoc.createElement("box");
    controlsBox.id = "highlighter-controls";
    this.highlighterContainer.appendChild(outlineContainer);
    this.highlighterContainer.appendChild(controlsBox);

    stack.appendChild(this.highlighterContainer);

    this.buildInfobar(controlsBox);

    this.transitionDisabler = null;
    this.pageEventsMuter = null;

    this.unlock();

    this.hidden = true;
    this.show();
  },

  


  destroy: function Highlighter_destroy()
  {
    this.detachMouseListeners();
    this.detachPageListeners();

    this.chromeWin.clearTimeout(this.transitionDisabler);
    this.chromeWin.clearTimeout(this.pageEventsMuter);
    this.boundCloseEventHandler = null;
    this._contentRect = null;
    this._highlightRect = null;
    this._highlighting = false;
    this.outline = null;
    this.node = null;
    this.nodeInfo = null;
    this.highlighterContainer.parentNode.removeChild(this.highlighterContainer);
    this.highlighterContainer = null;
    this.win = null
    this.browser = null;
    this.chromeDoc = null;
    this.chromeWin = null;
    this.tabbrowser = null;

    this.emitEvent("closed");
    this.removeAllListeners();
  },

  








  highlight: function Highlighter_highlight(aNode, aScroll)
  {
    if (this.hidden)
      this.show();

    let oldNode = this.node;

    if (!aNode) {
      if (!this.node)
        this.node = this.win.document.documentElement;
    } else {
      this.node = aNode;
    }

    if (oldNode !== this.node) {
      this.updateInfobar();
    }

    this.invalidateSize(!!aScroll);

    if (this._highlighting) {
      this.showOutline();
    }

    if (oldNode !== this.node) {
      this.emitEvent("nodeselected");
    }
  },

  




  pseudoClassLockToggled: function Highlighter_pseudoClassLockToggled(aPseudo)
  {
    this.emitEvent("pseudoclasstoggled", [aPseudo]);
    this.updateInfobar();
    this.moveInfobar();
  },

  


  invalidateSize: function Highlighter_invalidateSize(aScroll)
  {
    let rect = null;

    if (this.node && this.isNodeHighlightable(this.node)) {

      if (aScroll &&
          this.node.scrollIntoView) { 
        this.node.scrollIntoView();
      }
      let clientRect = this.node.getBoundingClientRect();
      rect = LayoutHelpers.getDirtyRect(this.node);
    }

    this.highlightRectangle(rect);

    this.moveInfobar();

    if (this._highlighting) {
      this.emitEvent("highlighting");
    }
  },

  




  getNode: function() {
    return this.node;
  },

  


  show: function() {
    if (!this.hidden) return;
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
    this.emitEvent("locked");
  },

  



  unlock: function() {
    if (this.locked === false) return;
    this.outline.removeAttribute("locked");
    this.nodeInfo.container.removeAttribute("locked");
    this.attachMouseListeners();
    this.locked = false;
    this.showOutline();
    this.emitEvent("unlocked");
  },

  







  isNodeHighlightable: function Highlighter_isNodeHighlightable(aNode)
  {
    if (!LayoutHelpers.isNodeConnected(aNode)) {
      return false;
    }
    if (aNode.nodeType != aNode.ELEMENT_NODE) {
      return false;
    }
    let nodeName = aNode.nodeName.toLowerCase();
    return !INSPECTOR_INVISIBLE_ELEMENTS[nodeName];
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
    container.id = "highlighter-nodeinfobar-container";
    container.setAttribute("position", "top");
    container.setAttribute("disabled", "true");

    let nodeInfobar = this.chromeDoc.createElement("hbox");
    nodeInfobar.id = "highlighter-nodeinfobar";

    nodeInfobar.addEventListener("mousedown", function(aEvent) {
      this.emitEvent("nodeselected");
    }.bind(this), true);

    let arrowBoxTop = this.chromeDoc.createElement("box");
    arrowBoxTop.className = "highlighter-nodeinfobar-arrow";
    arrowBoxTop.id = "highlighter-nodeinfobar-arrow-top";

    let arrowBoxBottom = this.chromeDoc.createElement("box");
    arrowBoxBottom.className = "highlighter-nodeinfobar-arrow";
    arrowBoxBottom.id = "highlighter-nodeinfobar-arrow-bottom";

    let tagNameLabel = this.chromeDoc.createElementNS("http://www.w3.org/1999/xhtml", "span");
    tagNameLabel.id = "highlighter-nodeinfobar-tagname";

    let idLabel = this.chromeDoc.createElementNS("http://www.w3.org/1999/xhtml", "span");
    idLabel.id = "highlighter-nodeinfobar-id";

    let classesBox = this.chromeDoc.createElementNS("http://www.w3.org/1999/xhtml", "span");
    classesBox.id = "highlighter-nodeinfobar-classes";

    let pseudoClassesBox = this.chromeDoc.createElementNS("http://www.w3.org/1999/xhtml", "span");
    pseudoClassesBox.id = "highlighter-nodeinfobar-pseudo-classes";

    
    pseudoClassesBox.textContent = "&nbsp;";

    

    let inspect = this.chromeDoc.createElement("toolbarbutton");
    inspect.id = "highlighter-nodeinfobar-inspectbutton";
    inspect.className = "highlighter-nodeinfobar-button"
    let toolbarInspectButton =
      this.chromeDoc.getElementById("inspector-inspect-toolbutton");
    inspect.setAttribute("tooltiptext",
                         toolbarInspectButton.getAttribute("tooltiptext"));
    inspect.setAttribute("command", "Inspector:Inspect");

    let nodemenu = this.chromeDoc.createElement("toolbarbutton");
    nodemenu.setAttribute("type", "menu");
    nodemenu.id = "highlighter-nodeinfobar-menu";
    nodemenu.className = "highlighter-nodeinfobar-button"
    nodemenu.setAttribute("tooltiptext",
                          this.strings.GetStringFromName("nodeMenu.tooltiptext"));

    let menu = this.chromeDoc.getElementById("inspector-node-popup");
    menu = menu.cloneNode(true);
    menu.id = "highlighter-node-menu";

    let separator = this.chromeDoc.createElement("menuseparator");
    menu.appendChild(separator);

    menu.addEventListener("popupshowing", function() {
      let items = menu.getElementsByClassName("highlighter-pseudo-class-menuitem");
      let i = items.length;
      while (i--) {
        menu.removeChild(items[i]);
      }

      let fragment = this.buildPseudoClassMenu();
      menu.appendChild(fragment);
    }.bind(this), true);

    nodemenu.appendChild(menu);

    
    let texthbox = this.chromeDoc.createElement("hbox");
    texthbox.id = "highlighter-nodeinfobar-text";
    texthbox.setAttribute("align", "center");
    texthbox.setAttribute("flex", "1");

    texthbox.appendChild(tagNameLabel);
    texthbox.appendChild(idLabel);
    texthbox.appendChild(classesBox);
    texthbox.appendChild(pseudoClassesBox);

    nodeInfobar.appendChild(inspect);
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

  




  buildPseudoClassMenu: function IUI_buildPseudoClassesMenu()
  {
    let fragment = this.chromeDoc.createDocumentFragment();
    for (let i = 0; i < PSEUDO_CLASSES.length; i++) {
      let pseudo = PSEUDO_CLASSES[i];
      let item = this.chromeDoc.createElement("menuitem");
      item.id = "highlighter-pseudo-class-menuitem-" + pseudo;
      item.setAttribute("type", "checkbox");
      item.setAttribute("label", pseudo);
      item.className = "highlighter-pseudo-class-menuitem";
      item.setAttribute("checked", DOMUtils.hasPseudoClassLock(this.node,
                        pseudo));
      item.addEventListener("command",
                            this.pseudoClassLockToggled.bind(this, pseudo), false);
      fragment.appendChild(item);
    }
    return fragment;
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
    
    this.nodeInfo.tagNameLabel.textContent = this.node.tagName;

    
    this.nodeInfo.idLabel.textContent = this.node.id ? "#" + this.node.id : "";

    
    let classes = this.nodeInfo.classesBox;

    classes.textContent = this.node.classList.length ?
                            "." + Array.join(this.node.classList, ".") : "";

    
    let pseudos = PSEUDO_CLASSES.filter(function(pseudo) {
      return DOMUtils.hasPseudoClassLock(this.node, pseudo);
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
      .screenPixelsPerCSSPixel;
  },

  
  

  addListener: function Highlighter_addListener(aEvent, aListener)
  {
    if (!(aEvent in this.events))
      this.events[aEvent] = [];
    this.events[aEvent].push(aListener);
  },

  removeListener: function Highlighter_removeListener(aEvent, aListener)
  {
    if (!(aEvent in this.events))
      return;
    let idx = this.events[aEvent].indexOf(aListener);
    if (idx > -1)
      this.events[aEvent].splice(idx, 1);
  },

  emitEvent: function Highlighter_emitEvent(aEvent, aArgv)
  {
    if (!(aEvent in this.events))
      return;

    let listeners = this.events[aEvent];
    let highlighter = this;
    listeners.forEach(function(aListener) {
      try {
        aListener.apply(highlighter, aArgv);
      } catch(e) {}
    });
  },

  removeAllListeners: function Highlighter_removeAllIsteners()
  {
    for (let event in this.events) {
      delete this.events[event];
    }
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
      let win = aEvent.target.ownerDocument.defaultView;
      this.lock();
      win.focus();
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
      if (element && element != this.node) {
        this.highlight(element);
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
