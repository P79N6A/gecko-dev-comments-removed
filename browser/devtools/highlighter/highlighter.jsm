











































const Cu = Components.utils;
Cu.import("resource:///modules/devtools/LayoutHelpers.jsm");

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

    this.veilContainer = this.chromeDoc.createElement("vbox");
    this.veilContainer.id = "highlighter-veil-container";

    
    
    let controlsBox = this.chromeDoc.createElement("box");
    controlsBox.id = "highlighter-controls";
    this.highlighterContainer.appendChild(this.veilContainer);
    this.highlighterContainer.appendChild(controlsBox);

    stack.appendChild(this.highlighterContainer);

    
    
    this.buildVeil(this.veilContainer);

    this.buildInfobar(controlsBox);

    this.transitionDisabler = null;

    this.computeZoomFactor();
    this.unlock();
    this.hide();
  },

  


  destroy: function Highlighter_destroy()
  {
    this.detachKeysListeners();
    this.detachMouseListeners();
    this.detachPageListeners();

    this.chromeWin.clearTimeout(this.transitionDisabler);
    this.boundCloseEventHandler = null;
    this._contentRect = null;
    this._highlightRect = null;
    this._highlighting = false;
    this.veilTopBox = null;
    this.veilLeftBox = null;
    this.veilMiddleBox = null;
    this.veilTransparentBox = null;
    this.veilContainer = null;
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

    if (oldNode !== this.node) {
      this.emitEvent("nodeselected");
    }
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
    this.veilContainer.removeAttribute("hidden");
    this.nodeInfo.container.removeAttribute("hidden");
    this.attachKeysListeners();
    this.attachPageListeners();
    this.invalidateSize();
    this.hidden = false;
  },

  


  hide: function() {
    if (this.hidden) return;
    this.veilContainer.setAttribute("hidden", "true");
    this.nodeInfo.container.setAttribute("hidden", "true");
    this.detachKeysListeners();
    this.detachPageListeners();
    this.hidden = true;
  },

  




  isHidden: function() {
    return this.hidden;
  },

  


  lock: function() {
    if (this.locked === true) return;
    this.veilContainer.setAttribute("locked", "true");
    this.nodeInfo.container.setAttribute("locked", "true");
    this.detachMouseListeners();
    this.locked = true;
    this.emitEvent("locked");
  },

  



  unlock: function() {
    if (this.locked === false) return;
    this.veilContainer.removeAttribute("locked");
    this.nodeInfo.container.removeAttribute("locked");
    this.attachMouseListeners();
    this.locked = false;
    this.emitEvent("unlocked");
  },

  







  isNodeHighlightable: function Highlighter_isNodeHighlightable(aNode)
  {
    if (aNode.nodeType != aNode.ELEMENT_NODE) {
      return false;
    }
    let nodeName = aNode.nodeName.toLowerCase();
    return !INSPECTOR_INVISIBLE_ELEMENTS[nodeName];
  },
  
















  buildVeil: function Highlighter_buildVeil(aParent)
  {
    
    

    this.veilTopBox = this.chromeDoc.createElement("box");
    this.veilTopBox.id = "highlighter-veil-topbox";
    this.veilTopBox.className = "highlighter-veil";

    this.veilMiddleBox = this.chromeDoc.createElement("hbox");
    this.veilMiddleBox.id = "highlighter-veil-middlebox";

    this.veilLeftBox = this.chromeDoc.createElement("box");
    this.veilLeftBox.id = "highlighter-veil-leftbox";
    this.veilLeftBox.className = "highlighter-veil";

    this.veilTransparentBox = this.chromeDoc.createElement("box");
    this.veilTransparentBox.id = "highlighter-veil-transparentbox";

    
    

    let veilRightBox = this.chromeDoc.createElement("box");
    veilRightBox.id = "highlighter-veil-rightbox";
    veilRightBox.className = "highlighter-veil";

    let veilBottomBox = this.chromeDoc.createElement("box");
    veilBottomBox.id = "highlighter-veil-bottombox";
    veilBottomBox.className = "highlighter-veil";

    this.veilMiddleBox.appendChild(this.veilLeftBox);
    this.veilMiddleBox.appendChild(this.veilTransparentBox);
    this.veilMiddleBox.appendChild(veilRightBox);

    aParent.appendChild(this.veilTopBox);
    aParent.appendChild(this.veilMiddleBox);
    aParent.appendChild(veilBottomBox);
  },

  















  buildInfobar: function Highlighter_buildInfobar(aParent)
  {
    let container = this.chromeDoc.createElement("box");
    container.id = "highlighter-nodeinfobar-container";
    container.setAttribute("position", "top");
    container.setAttribute("disabled", "true");

    let nodeInfobar = this.chromeDoc.createElement("hbox");
    nodeInfobar.id = "highlighter-nodeinfobar";

    let arrowBoxTop = this.chromeDoc.createElement("box");
    arrowBoxTop.className = "highlighter-nodeinfobar-arrow";
    arrowBoxTop.id = "highlighter-nodeinfobar-arrow-top";

    let arrowBoxBottom = this.chromeDoc.createElement("box");
    arrowBoxBottom.className = "highlighter-nodeinfobar-arrow";
    arrowBoxBottom.id = "highlighter-nodeinfobar-arrow-bottom";

    let tagNameLabel = this.chromeDoc.createElement("label");
    tagNameLabel.id = "highlighter-nodeinfobar-tagname";
    tagNameLabel.className = "plain";

    let idLabel = this.chromeDoc.createElement("label");
    idLabel.id = "highlighter-nodeinfobar-id";
    idLabel.className = "plain";

    let classesBox = this.chromeDoc.createElement("hbox");
    classesBox.id = "highlighter-nodeinfobar-classes";

    nodeInfobar.appendChild(tagNameLabel);
    nodeInfobar.appendChild(idLabel);
    nodeInfobar.appendChild(classesBox);
    container.appendChild(arrowBoxTop);
    container.appendChild(nodeInfobar);
    container.appendChild(arrowBoxBottom);

    aParent.appendChild(container);

    let barHeight = container.getBoundingClientRect().height;

    this.nodeInfo = {
      tagNameLabel: tagNameLabel,
      idLabel: idLabel,
      classesBox: classesBox,
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

      this.veilTransparentBox.style.visibility = "visible";

      
      
      this.veilTopBox.style.height = aRectScaled.top + "px";
      this.veilLeftBox.style.width = aRectScaled.left + "px";
      this.veilMiddleBox.style.height = aRectScaled.height + "px";
      this.veilTransparentBox.style.width = aRectScaled.width + "px";

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
    this.veilMiddleBox.style.height = 0;
    this.veilTransparentBox.style.width = 0;
    this.veilTransparentBox.style.visibility = "hidden";
  },

  


  updateInfobar: function Highlighter_updateInfobar()
  {
    
    this.nodeInfo.tagNameLabel.textContent = this.node.tagName;

    
    this.nodeInfo.idLabel.textContent = this.node.id ? "#" + this.node.id : "";

    
    let classes = this.nodeInfo.classesBox;
    while (classes.hasChildNodes()) {
      classes.removeChild(classes.firstChild);
    }

    if (this.node.className) {
      let fragment = this.chromeDoc.createDocumentFragment();
      for (let i = 0; i < this.node.classList.length; i++) {
        let classLabel = this.chromeDoc.createElement("label");
        classLabel.className = "highlighter-nodeinfobar-class plain";
        classLabel.textContent = "." + this.node.classList[i];
        fragment.appendChild(classLabel);
      }
      classes.appendChild(fragment);
    }
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
      this.win.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
      .getInterface(Components.interfaces.nsIDOMWindowUtils)
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
  },

  detachPageListeners: function Highlighter_detachPageListeners()
  {
    this.browser.removeEventListener("resize", this, true);
    this.browser.removeEventListener("scroll", this, true);
  },

  attachKeysListeners: function Highlighter_attachKeysListeners()
  {
    this.browser.addEventListener("keypress", this, true);
    this.highlighterContainer.addEventListener("keypress", this, true);
  },

  detachKeysListeners: function Highlighter_detachKeysListeners()
  {
    this.browser.removeEventListener("keypress", this, true);
    this.highlighterContainer.removeEventListener("keypress", this, true);
  },

  





  handleEvent: function Highlighter_handleEvent(aEvent)
  {
    switch (aEvent.type) {
      case "click":
        this.handleClick(aEvent);
        break;
      case "mousemove":
        this.handleMouseMove(aEvent);
        break;
      case "resize":
      case "scroll":
        this.computeZoomFactor();
        this.brieflyDisableTransitions();
        this.invalidateSize();
        break;
      case "dblclick":
      case "mousedown":
      case "mouseup":
        aEvent.stopPropagation();
        aEvent.preventDefault();
        break;
        break;
      case "keypress":
        switch (aEvent.keyCode) {
          case this.chromeWin.KeyEvent.DOM_VK_RETURN:
            this.locked ? this.unlock() : this.lock();
            aEvent.preventDefault();
            aEvent.stopPropagation();
            break;
          case this.chromeWin.KeyEvent.DOM_VK_LEFT:
            let node;
            if (this.node) {
              node = this.node.parentNode;
            } else {
              node = this.defaultSelection;
            }
            if (node && this.isNodeHighlightable(node)) {
              this.highlight(node);
            }
            aEvent.preventDefault();
            aEvent.stopPropagation();
            break;
          case this.chromeWin.KeyEvent.DOM_VK_RIGHT:
            if (this.node) {
              
              for (let i = 0; i < this.node.childNodes.length; i++) {
                node = this.node.childNodes[i];
                if (node && this.isNodeHighlightable(node)) {
                  break;
                }
              }
            } else {
              node = this.defaultSelection;
            }
            if (node && this.isNodeHighlightable(node)) {
              this.highlight(node, true);
            }
            aEvent.preventDefault();
            aEvent.stopPropagation();
            break;
          case this.chromeWin.KeyEvent.DOM_VK_UP:
            if (this.node) {
              
              node = this.node.previousSibling;
              while (node && !this.isNodeHighlightable(node)) {
                node = node.previousSibling;
              }
            } else {
              node = this.defaultSelection;
            }
            if (node && this.isNodeHighlightable(node)) {
              this.highlight(node, true);
            }
            aEvent.preventDefault();
            aEvent.stopPropagation();
            break;
          case this.chromeWin.KeyEvent.DOM_VK_DOWN:
            if (this.node) {
              
              node = this.node.nextSibling;
              while (node && !this.isNodeHighlightable(node)) {
                node = node.nextSibling;
              }
            } else {
              node = this.defaultSelection;
            }
            if (node && this.isNodeHighlightable(node)) {
              this.highlight(node, true);
            }
            aEvent.preventDefault();
            aEvent.stopPropagation();
            break;
        }
    }
  },

  



  brieflyDisableTransitions: function Highlighter_brieflyDisableTransitions()
  {
   if (this.transitionDisabler) {
     this.chromeWin.clearTimeout(this.transitionDisabler);
   } else {
     this.veilContainer.setAttribute("disable-transitions", "true");
     this.nodeInfo.container.setAttribute("disable-transitions", "true");
   }
   this.transitionDisabler =
     this.chromeWin.setTimeout(function() {
       this.veilContainer.removeAttribute("disable-transitions");
       this.nodeInfo.container.removeAttribute("disable-transitions");
       this.transitionDisabler = null;
     }.bind(this), 500);
  },

  





  handleClick: function Highlighter_handleClick(aEvent)
  {
    
    if (aEvent.button == 0) {
      let win = aEvent.target.ownerDocument.defaultView;
      this.lock();
      win.focus();
    }
    aEvent.preventDefault();
    aEvent.stopPropagation();
  },

  





  handleMouseMove: function Highlighter_handleMouseMove(aEvent)
  {
    let element = LayoutHelpers.getElementFromPoint(aEvent.target.ownerDocument,
      aEvent.clientX, aEvent.clientY);
    if (element && element != this.node) {
      this.highlight(element);
    }
  },
};



