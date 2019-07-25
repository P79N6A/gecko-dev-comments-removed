













function Highlighter(aInspector)
{
  this.IUI = aInspector;
  this._init();
}

Highlighter.prototype = {
  _init: function Highlighter__init()
  {
    this.browser = this.IUI.browser;
    this.chromeDoc = this.IUI.chromeDoc;

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

    if (!this.IUI.store.getValue(this.winID, "inspecting")) {
      this.veilContainer.setAttribute("locked", true);
      this.nodeInfo.container.setAttribute("locked", true);
    }

    this.browser.addEventListener("resize", this, true);
    this.browser.addEventListener("scroll", this, true);

    this.transitionDisabler = null;

    this.computeZoomFactor();
    this.handleResize();
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

  


  destroy: function Highlighter_destroy()
  {
    this.IUI.win.clearTimeout(this.transitionDisabler);
    this.browser.removeEventListener("scroll", this, true);
    this.browser.removeEventListener("resize", this, true);
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
    this.IUI = null;
  },

  



  get isHighlighting() {
    return this._highlighting;
  },

  





  highlight: function Highlighter_highlight(aScroll)
  {
    let rect = null;

    if (this.node && this.isNodeHighlightable(this.node)) {

      if (aScroll) {
        this.node.scrollIntoView();
      }

      let clientRect = this.node.getBoundingClientRect();

      
      
      rect = {top: clientRect.top,
              left: clientRect.left,
              width: clientRect.width,
              height: clientRect.height};

      let frameWin = this.node.ownerDocument.defaultView;

      
      while (true) {

        
        let diffx = frameWin.innerWidth - (rect.left + rect.width);
        if (diffx < 0) {
          rect.width += diffx;
        }

        
        let diffy = frameWin.innerHeight - (rect.top + rect.height);
        if (diffy < 0) {
          rect.height += diffy;
        }

        
        if (rect.left < 0) {
          rect.width += rect.left;
          rect.left = 0;
        }

        
        if (rect.top < 0) {
          rect.height += rect.top;
          rect.top = 0;
        }

        

        
        if (frameWin.parent === frameWin || !frameWin.frameElement) {
          break;
        }

        
        
        
        let frameRect = frameWin.frameElement.getBoundingClientRect();

        let [offsetTop, offsetLeft] =
          this.IUI.getIframeContentOffset(frameWin.frameElement);

        rect.top += frameRect.top + offsetTop;
        rect.left += frameRect.left + offsetLeft;

        frameWin = frameWin.parent;
      }
    }

    this.highlightRectangle(rect);

    this.moveInfobar();

    if (this._highlighting) {
      Services.obs.notifyObservers(null,
        INSPECTOR_NOTIFICATIONS.HIGHLIGHTING, null);
    }
  },

  







  highlightNode: function Highlighter_highlightNode(aNode, aParams)
  {
    this.node = aNode;
    this.updateInfobar();
    this.highlight(aParams && aParams.scroll);
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
      return this._highlighting; 
    }

    
    let aRectScaled = {};
    for (let prop in aRect) {
      aRectScaled[prop] = aRect[prop] * this.zoom;
    }

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

    return this._highlighting;
  },

  


  unhighlight: function Highlighter_unhighlight()
  {
    this._highlighting = false;
    this.veilMiddleBox.style.height = 0;
    this.veilTransparentBox.style.width = 0;
    this.veilTransparentBox.style.visibility = "hidden";
    Services.obs.notifyObservers(null,
      INSPECTOR_NOTIFICATIONS.UNHIGHLIGHTING, null);
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

  









  midPoint: function Highlighter_midPoint(aPointA, aPointB)
  {
    let pointC = { };
    pointC.x = (aPointB.x - aPointA.x) / 2 + aPointA.x;
    pointC.y = (aPointB.y - aPointA.y) / 2 + aPointA.y;
    return pointC;
  },

  








  get highlitNode()
  {
    
    if (!this._highlighting || !this._contentRect) {
      return null;
    }

    let a = {
      x: this._contentRect.left,
      y: this._contentRect.top
    };

    let b = {
      x: a.x + this._contentRect.width,
      y: a.y + this._contentRect.height
    };

    
    let midpoint = this.midPoint(a, b);

    return this.IUI.elementFromPoint(this.win.document, midpoint.x,
      midpoint.y);
  },

  







  isNodeHighlightable: function Highlighter_isNodeHighlightable(aNode)
  {
    if (aNode.nodeType != aNode.ELEMENT_NODE) {
      return false;
    }
    let nodeName = aNode.nodeName.toLowerCase();
    return !INSPECTOR_INVISIBLE_ELEMENTS[nodeName];
  },

  


  computeZoomFactor: function Highlighter_computeZoomFactor() {
    this.zoom =
      this.win.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
      .getInterface(Components.interfaces.nsIDOMWindowUtils)
      .screenPixelsPerCSSPixel;
  },

  
  

  attachInspectListeners: function Highlighter_attachInspectListeners()
  {
    this.browser.addEventListener("mousemove", this, true);
    this.browser.addEventListener("click", this, true);
    this.browser.addEventListener("dblclick", this, true);
    this.browser.addEventListener("mousedown", this, true);
    this.browser.addEventListener("mouseup", this, true);
  },

  detachInspectListeners: function Highlighter_detachInspectListeners()
  {
    this.browser.removeEventListener("mousemove", this, true);
    this.browser.removeEventListener("click", this, true);
    this.browser.removeEventListener("dblclick", this, true);
    this.browser.removeEventListener("mousedown", this, true);
    this.browser.removeEventListener("mouseup", this, true);
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
        this.computeZoomFactor();
        this.brieflyDisableTransitions();
        this.handleResize(aEvent);
        break;
      case "dblclick":
      case "mousedown":
      case "mouseup":
        aEvent.stopPropagation();
        aEvent.preventDefault();
        break;
      case "scroll":
        this.brieflyDisableTransitions();
        this.highlight();
        break;
    }
  },

  



  brieflyDisableTransitions: function Highlighter_brieflyDisableTransitions()
  {
   if (this.transitionDisabler) {
     this.IUI.win.clearTimeout(this.transitionDisabler);
   } else {
     this.veilContainer.setAttribute("disable-transitions", "true");
     this.nodeInfo.container.setAttribute("disable-transitions", "true");
   }
   this.transitionDisabler =
     this.IUI.win.setTimeout(function() {
       this.veilContainer.removeAttribute("disable-transitions");
       this.nodeInfo.container.removeAttribute("disable-transitions");
       this.transitionDisabler = null;
     }.bind(this), 500);
  },

  





  handleClick: function Highlighter_handleClick(aEvent)
  {
    
    if (aEvent.button == 0) {
      let win = aEvent.target.ownerDocument.defaultView;
      this.IUI.stopInspecting();
      win.focus();
    }
    aEvent.preventDefault();
    aEvent.stopPropagation();
  },

  





  handleMouseMove: function Highlighter_handleMouseMove(aEvent)
  {
    let element = this.IUI.elementFromPoint(aEvent.target.ownerDocument,
      aEvent.clientX, aEvent.clientY);
    if (element && element != this.node) {
      this.IUI.inspectNode(element);
    }
  },

  


  handleResize: function Highlighter_handleResize()
  {
    this.highlight();
  },
};



