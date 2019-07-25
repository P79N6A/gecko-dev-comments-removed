

#ifdef 0







































#endif

#include insideOutBox.js

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


const INSPECTOR_NOTIFICATIONS = {
  
  HIGHLIGHTING: "inspector-highlighting",

  
  UNHIGHLIGHTING: "inspector-unhighlighting",

  
  
  OPENED: "inspector-opened",

  
  CLOSED: "inspector-closed",
};
















function Highlighter(aBrowser)
{
  this._init(aBrowser);
}

Highlighter.prototype = {

  _init: function Highlighter__init(aBrowser)
  {
    this.browser = aBrowser;
    let stack = this.browser.parentNode;
    this.win = this.browser.contentWindow;
    this._highlighting = false;

    this.highlighterContainer = document.createElement("stack");
    this.highlighterContainer.id = "highlighter-container";

    let veilBox = document.createElement("vbox");
    veilBox.id = "highlighter-veil-container";

    let controlsBox = document.createElement("box");
    controlsBox.id = "highlighter-controls";

    
    
    this.buildVeil(veilBox);

    
    
    this.buildControls(controlsBox);

    this.highlighterContainer.appendChild(veilBox);
    this.highlighterContainer.appendChild(controlsBox);

    stack.appendChild(this.highlighterContainer);

    this.browser.addEventListener("resize", this, true);
    this.browser.addEventListener("scroll", this, true);

    this.handleResize();
  },


  














  buildVeil: function Highlighter_buildVeil(aParent)
  {

    
    

    this.veilTopBox = document.createElement("box");
    this.veilTopBox.id = "highlighter-veil-topbox";
    this.veilTopBox.className = "highlighter-veil";

    this.veilMiddleBox = document.createElement("hbox");
    this.veilMiddleBox.id = "highlighter-veil-middlebox";

    this.veilLeftBox = document.createElement("box");
    this.veilLeftBox.id = "highlighter-veil-leftbox";
    this.veilLeftBox.className = "highlighter-veil";

    this.veilTransparentBox = document.createElement("box");
    this.veilTransparentBox.id = "highlighter-veil-transparentbox";

    
    

    let veilRightBox = document.createElement("box");
    veilRightBox.id = "highlighter-veil-rightbox";
    veilRightBox.className = "highlighter-veil";

    let veilBottomBox = document.createElement("box");
    veilBottomBox.id = "highlighter-veil-bottombox";
    veilBottomBox.className = "highlighter-veil";

    this.veilMiddleBox.appendChild(this.veilLeftBox);
    this.veilMiddleBox.appendChild(this.veilTransparentBox);
    this.veilMiddleBox.appendChild(veilRightBox);

    aParent.appendChild(this.veilTopBox);
    aParent.appendChild(this.veilMiddleBox);
    aParent.appendChild(veilBottomBox);
  },

  






  buildControls: function Highlighter_buildControls(aParent)
  {
    let closeButton = document.createElement("box");
    closeButton.id = "highlighter-close-button";
    closeButton.appendChild(document.createElement("image"));

    closeButton.setAttribute("onclick", "InspectorUI.closeInspectorUI(false);");

    aParent.appendChild(closeButton);
  },


  


  destroy: function Highlighter_destroy()
  {
    this.browser.removeEventListener("scroll", this, true);
    this.browser.removeEventListener("resize", this, true);
    this._highlightRect = null;
    this._highlighting = false;
    this.veilTopBox = null;
    this.veilLeftBox = null;
    this.veilMiddleBox = null;
    this.veilTransparentBox = null;
    this.node = null;
    this.highlighterContainer.parentNode.removeChild(this.highlighterContainer);
    this.highlighterContainer = null;
    this.win = null
    this.browser = null;
    this.toolbar = null;
  },

  



  get isHighlighting() {
    return this._highlighting;
  },

  





  highlight: function Highlighter_highlight(aScroll)
  {
    
    if (!this.node || !this.isNodeHighlightable()) {
      return;
    }

    let clientRect = this.node.getBoundingClientRect();

    
    let rect = {top: clientRect.top,
                left: clientRect.left,
                width: clientRect.width,
                height: clientRect.height};

    if (aScroll) {
      this.node.scrollIntoView();
    }

    
    
    let frameWin = this.node.ownerDocument.defaultView;
    do {
      let frameRect = frameWin.frameElement ?
                      frameWin.frameElement.getBoundingClientRect() :
                      {top: 0, left: 0};

      if (rect.top < 0) {
        rect.height += rect.top;
        rect.top = 0;
      }

      if (rect.left < 0) {
        rect.width += rect.left;
        rect.left = 0;
      }

      let diffx = frameWin.innerWidth - rect.left - rect.width;
      if (diffx < 0) {
        rect.width += diffx;
      }
      let diffy = frameWin.innerHeight - rect.top - rect.height;
      if (diffy < 0) {
        rect.height += diffy;
      }

      rect.left += frameRect.left;
      rect.top += frameRect.top;

      frameWin = frameWin.parent;
    } while (frameWin != this.win);

    this.highlightRectangle(rect);

    if (this._highlighting) {
      Services.obs.notifyObservers(null,
        INSPECTOR_NOTIFICATIONS.HIGHLIGHTING, null);
    }
  },

  







  highlightNode: function Highlighter_highlightNode(aNode, aParams)
  {
    this.node = aNode;
    this.highlight(aParams && aParams.scroll);
  },

  







  highlightRectangle: function Highlighter_highlightRectangle(aRect)
  {
    let oldRect = this._highlightRect;

    if (oldRect && aRect.top == oldRect.top && aRect.left == oldRect.left &&
        aRect.width == oldRect.width && aRect.height == oldRect.height) {
      return this._highlighting; 
    }

    if (aRect.left >= 0 && aRect.top >= 0 &&
        aRect.width > 0 && aRect.height > 0) {
      
      
      this.veilTopBox.style.height = aRect.top + "px";
      this.veilLeftBox.style.width = aRect.left + "px";
      this.veilMiddleBox.style.height = aRect.height + "px";
      this.veilTransparentBox.style.width = aRect.width + "px";

      this._highlighting = true;
    } else {
      this.unhighlight();
    }

    this._highlightRect = aRect;

    return this._highlighting;
  },

  


  unhighlight: function Highlighter_unhighlight()
  {
    this._highlighting = false;
    this.veilMiddleBox.style.height = 0;
    this.veilTransparentBox.style.width = 0;
    Services.obs.notifyObservers(null,
      INSPECTOR_NOTIFICATIONS.UNHIGHLIGHTING, null);
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
    
    if (!this._highlighting || !this._highlightRect) {
      return null;
    }

    let a = {
      x: this._highlightRect.left,
      y: this._highlightRect.top
    };

    let b = {
      x: a.x + this._highlightRect.width,
      y: a.y + this._highlightRect.height
    };

    
    let midpoint = this.midPoint(a, b);

    return InspectorUI.elementFromPoint(this.win.document, midpoint.x,
      midpoint.y);
  },

  





  isNodeHighlightable: function Highlighter_isNodeHighlightable()
  {
    if (!this.node || this.node.nodeType != Node.ELEMENT_NODE) {
      return false;
    }
    let nodeName = this.node.nodeName.toLowerCase();
    return !INSPECTOR_INVISIBLE_ELEMENTS[nodeName];
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
        this.handleResize(aEvent);
        break;
      case "dblclick":
      case "mousedown":
      case "mouseup":
        aEvent.stopPropagation();
        aEvent.preventDefault();
        break;
      case "scroll":
        this.highlight();
        break;
    }
  },

  





  handleClick: function Highlighter_handleClick(aEvent)
  {
    
    if (aEvent.button == 0) {
      let win = aEvent.target.ownerDocument.defaultView;
      InspectorUI.stopInspecting();
      win.focus();
    }
    aEvent.preventDefault();
    aEvent.stopPropagation();
  },

  





  handleMouseMove: function Highlighter_handleMouseMove(aEvent)
  {
    let element = InspectorUI.elementFromPoint(aEvent.target.ownerDocument,
      aEvent.clientX, aEvent.clientY);
    if (element && element != this.node) {
      InspectorUI.inspectNode(element);
    }
  },

  


  handleResize: function Highlighter_handleResize()
  {
    this.highlight();
  },
};







var InspectorUI = {
  browser: null,
  tools: {},
  showTextNodesWithWhitespace: false,
  inspecting: false,
  treeLoaded: false,
  prefEnabledName: "devtools.inspector.enabled",

  





  toggleInspectorUI: function IUI_toggleInspectorUI(aEvent)
  {
    if (this.isTreePanelOpen) {
      this.closeInspectorUI();
    } else {
      this.openInspectorUI();
    }
  },

  



  toggleInspection: function IUI_toggleInspection()
  {
    if (this.inspecting) {
      this.stopInspecting();
    } else {
      this.startInspecting();
    }
  },

  




  get isTreePanelOpen()
  {
    return this.treePanel && this.treePanel.state == "open";
  },

  


  get defaultSelection()
  {
    let doc = this.win.document;
    return doc.documentElement.lastElementChild;
  },

  initializeTreePanel: function IUI_initializeTreePanel()
  {
    this.treeBrowserDocument = this.treeIFrame.contentDocument;
    this.treePanelDiv = this.treeBrowserDocument.createElement("div");
    this.treeBrowserDocument.body.appendChild(this.treePanelDiv);
    this.treePanelDiv.ownerPanel = this;
    this.ioBox = new InsideOutBox(this, this.treePanelDiv);
    this.ioBox.createObjectBox(this.win.document.documentElement);
    this.treeLoaded = true;

    
    this.initializeHighlighter();
  },

  


  openTreePanel: function IUI_openTreePanel()
  {
    if (!this.treePanel) {
      this.treePanel = document.getElementById("inspector-tree-panel");
      this.treePanel.hidden = false;
    }

    this.treeIFrame = document.getElementById("inspector-tree-iframe");
    if (!this.treeIFrame) {
      let resizerBox = document.getElementById("tree-panel-resizer-box");
      this.treeIFrame = document.createElement("iframe");
      this.treeIFrame.setAttribute("id", "inspector-tree-iframe");
      this.treeIFrame.setAttribute("flex", "1");
      this.treeIFrame.setAttribute("type", "content");
      this.treeIFrame.setAttribute("onclick", "InspectorUI.onTreeClick(event)");
      this.treeIFrame = this.treePanel.insertBefore(this.treeIFrame, resizerBox);
    }

    this.treePanel.addEventListener("popupshown", function treePanelShown() {
      InspectorUI.treePanel.removeEventListener("popupshown",
        treePanelShown, false);

        InspectorUI.treeIFrame.addEventListener("load",
          function loadedInitializeTreePanel() {
            InspectorUI.treeIFrame.removeEventListener("load",
              loadedInitializeTreePanel, true);
            InspectorUI.initializeTreePanel();
          }, true);

      let src = InspectorUI.treeIFrame.getAttribute("src");
      if (src != "chrome://browser/content/inspector.html") {
        InspectorUI.treeIFrame.setAttribute("src",
          "chrome://browser/content/inspector.html");
      } else {
        InspectorUI.treeIFrame.contentWindow.location.reload();
      }

    }, false);

    const panelWidthRatio = 7 / 8;
    const panelHeightRatio = 1 / 5;

    let width = parseInt(this.win.outerWidth * panelWidthRatio);
    let height = parseInt(this.win.outerHeight * panelHeightRatio);
    let y = Math.min(window.screen.availHeight - height, this.win.innerHeight);

    this.treePanel.openPopup(this.browser, "overlap", 0, 0,
      false, false);

    this.treePanel.moveTo(80, y);
    this.treePanel.sizeTo(width, height);
  },

  createObjectBox: function IUI_createObjectBox(object, isRoot)
  {
    let tag = this.domplateUtils.getNodeTag(object);
    if (tag)
      return tag.replace({object: object}, this.treeBrowserDocument);
  },

  getParentObject: function IUI_getParentObject(node)
  {
    let parentNode = node ? node.parentNode : null;

    if (!parentNode) {
      
      
      if (node && node == Node.DOCUMENT_NODE) {
        
        if (node.defaultView) {
          let embeddingFrame = node.defaultView.frameElement;
          if (embeddingFrame)
            return embeddingFrame.parentNode;
        }
      }
      
      return null;  
    }

    if (parentNode.nodeType == Node.DOCUMENT_NODE) {
      if (parentNode.defaultView) {
        return parentNode.defaultView.frameElement;
      }
      
      return null;
    }
    if (!parentNode.localName) {
      return null;
    }
    return parentNode;
  },

  getChildObject: function IUI_getChildObject(node, index, previousSibling)
  {
    if (!node)
      return null;

    if (node.contentDocument) {
      
      if (index == 0) {
        return node.contentDocument.documentElement;  
      }
      return null;
    }

    if (node instanceof GetSVGDocument) {
      let svgDocument = node.getSVGDocument();
      if (svgDocument) {
        
        if (index == 0) {
          return svgDocument.documentElement;  
        }
        return null;
      }
    }

    let child = null;
    if (previousSibling)  
      child = this.getNextSibling(previousSibling);
    else
      child = this.getFirstChild(node);

    if (this.showTextNodesWithWhitespace)
      return child;

    for (; child; child = this.getNextSibling(child)) {
      if (!this.domplateUtils.isWhitespaceText(child))
        return child;
    }

    return null;  
  },

  getFirstChild: function IUI_getFirstChild(node)
  {
    this.treeWalker = node.ownerDocument.createTreeWalker(node,
      NodeFilter.SHOW_ALL, null, false);
    return this.treeWalker.firstChild();
  },

  getNextSibling: function IUI_getNextSibling(node)
  {
    let next = this.treeWalker.nextSibling();

    if (!next)
      delete this.treeWalker;

    return next;
  },

  



  openInspectorUI: function IUI_openInspectorUI()
  {
    
    this.browser = gBrowser.selectedBrowser;
    this.win = this.browser.contentWindow;
    this.winID = this.getWindowID(this.win);
    this.toolbar = document.getElementById("inspector-toolbar");

    if (!this.domplate) {
      Cu.import("resource:///modules/domplate.jsm", this);
      this.domplateUtils.setDOM(window);
    }

    this.openTreePanel();

    this.toolbar.hidden = null;
    this.inspectCmd.setAttribute("checked", true);
  },

  


  initializeHighlighter: function IUI_initializeHighlighter()
  {
    this.highlighter = new Highlighter(this.browser);
    this.highlighterReady();
  },

  


  initializeStore: function IUI_initializeStore()
  {
    
    if (InspectorStore.isEmpty())
      gBrowser.tabContainer.addEventListener("TabSelect", this, false);

    
    if (InspectorStore.hasID(this.winID)) {
      let selectedNode = InspectorStore.getValue(this.winID, "selectedNode");
      if (selectedNode) {
        this.inspectNode(selectedNode);
      }
    } else {
      
      InspectorStore.addStore(this.winID);
      InspectorStore.setValue(this.winID, "selectedNode", null);
      InspectorStore.setValue(this.winID, "inspecting", true);
      this.win.addEventListener("pagehide", this, true);
    }
  },

  









  closeInspectorUI: function IUI_closeInspectorUI(aKeepStore)
  {
    if (this.closing || !this.win || !this.browser) {
      return;
    }

    this.closing = true;
    this.toolbar.hidden = true;

    if (!aKeepStore) {
      InspectorStore.deleteStore(this.winID);
      this.win.removeEventListener("pagehide", this, true);
    } else {
      
      if (this.selection) {
        InspectorStore.setValue(this.winID, "selectedNode",
          this.selection);
      }
      InspectorStore.setValue(this.winID, "inspecting", this.inspecting);
    }

    if (InspectorStore.isEmpty()) {
      gBrowser.tabContainer.removeEventListener("TabSelect", this, false);
    }

    this.stopInspecting();
    if (this.highlighter) {
      this.highlighter.destroy();
      this.highlighter = null;
    }

    if (this.treePanelDiv) {
      this.treePanelDiv.ownerPanel = null;
      let parent = this.treePanelDiv.parentNode;
      parent.removeChild(this.treePanelDiv);
      delete this.treePanelDiv;
      delete this.treeBrowserDocument;
    }

    if (this.treeIFrame)
      delete this.treeIFrame;
    delete this.ioBox;

    if (this.domplate) {
      this.domplateUtils.setDOM(null);
      delete this.domplate;
      delete this.HTMLTemplates;
      delete this.domplateUtils;
    }

    this.saveToolState(this.winID);
    this.toolsDo(function IUI_toolsHide(aTool) {
      if (aTool.panel) {
        aTool.panel.hidePopup();
      }
    });

    this.inspectCmd.setAttribute("checked", false);
    this.browser = this.win = null; 
    this.winID = null;
    this.selection = null;
    this.treeLoaded = false;

    this.treePanel.addEventListener("popuphidden", function treePanelHidden() {
      InspectorUI.closing = false;
      Services.obs.notifyObservers(null, INSPECTOR_NOTIFICATIONS.CLOSED, null);
    }, false);

    this.treePanel.hidePopup();
    delete this.treePanel;
  },

  



  startInspecting: function IUI_startInspecting()
  {
    document.getElementById("inspector-inspect-toolbutton").checked = true;
    this.attachPageListeners();
    this.inspecting = true;
    this.highlighter.veilTransparentBox.removeAttribute("locked");
  },

  





  stopInspecting: function IUI_stopInspecting(aPreventScroll)
  {
    if (!this.inspecting) {
      return;
    }

    document.getElementById("inspector-inspect-toolbutton").checked = false;
    this.detachPageListeners();
    this.inspecting = false;
    if (this.highlighter.node) {
      this.select(this.highlighter.node, true, true, !aPreventScroll);
    } else {
      this.select(null, true, true);
    }
    this.highlighter.veilTransparentBox.setAttribute("locked", true);
  },

  








  select: function IUI_select(aNode, forceUpdate, aScroll)
  {
    if (!aNode)
      aNode = this.defaultSelection;

    if (forceUpdate || aNode != this.selection) {
      this.selection = aNode;
      if (!this.inspecting) {
        this.highlighter.highlightNode(this.selection);
      }
      this.ioBox.select(this.selection, true, true, aScroll);
    }
    this.toolsDo(function IUI_toolsOnSelect(aTool) {
      if (aTool.panel.state == "open") {
        aTool.onSelect.apply(aTool.context, [aNode]);
      }
    });
  },

  
  

  highlighterReady: function IUI_highlighterReady()
  {
    
    this.initializeStore();

    if (InspectorStore.getValue(this.winID, "inspecting")) {
      this.startInspecting();
    }

    this.win.focus();
    Services.obs.notifyObservers(null, INSPECTOR_NOTIFICATIONS.OPENED, null);
  },

  





  handleEvent: function IUI_handleEvent(event)
  {
    let winID = null;
    let win = null;
    let inspectorClosed = false;

    switch (event.type) {
      case "TabSelect":
        winID = this.getWindowID(gBrowser.selectedBrowser.contentWindow);
        if (this.isTreePanelOpen && winID != this.winID) {
          this.closeInspectorUI(true);
          inspectorClosed = true;
        }

        if (winID && InspectorStore.hasID(winID)) {
          if (inspectorClosed && this.closing) {
            Services.obs.addObserver(function reopenInspectorForTab() {
              Services.obs.removeObserver(reopenInspectorForTab,
                INSPECTOR_NOTIFICATIONS.CLOSED, false);

              InspectorUI.openInspectorUI();
            }, INSPECTOR_NOTIFICATIONS.CLOSED, false);
          } else {
            this.openInspectorUI();
            this.restoreToolState(winID);
          }
        }

        if (InspectorStore.isEmpty()) {
          gBrowser.tabContainer.removeEventListener("TabSelect", this, false);
        }
        break;
      case "pagehide":
        win = event.originalTarget.defaultView;
        
        if (!win || win.frameElement || win.top != win) {
          break;
        }

        win.removeEventListener(event.type, this, true);

        winID = this.getWindowID(win);
        if (winID && winID != this.winID) {
          InspectorStore.deleteStore(winID);
        }

        if (InspectorStore.isEmpty()) {
          gBrowser.tabContainer.removeEventListener("TabSelect", this, false);
        }
        break;
      case "keypress":
        switch (event.keyCode) {
          case KeyEvent.DOM_VK_RETURN:
          case KeyEvent.DOM_VK_ESCAPE:
            if (this.inspecting) {
              this.stopInspecting();
              event.preventDefault();
              event.stopPropagation();
            }
            break;
        }
        break;
    }
  },

  




  onTreeClick: function IUI_onTreeClick(aEvent)
  {
    let node;
    let target = aEvent.target;
    let hitTwisty = false;
    if (this.hasClass(target, "twisty")) {
      node = this.getRepObject(aEvent.target.nextSibling);
      hitTwisty = true;
    } else {
      node = this.getRepObject(aEvent.target);
    }

    if (node) {
      if (hitTwisty) {
        this.ioBox.toggleObject(node);
      } else {
        if (this.inspecting) {
          this.stopInspecting(true);
        } else {
          this.select(node, true, false);
          this.highlighter.highlightNode(node);
        }
      }
    }
  },

  



  attachPageListeners: function IUI_attachPageListeners()
  {
    this.browser.addEventListener("keypress", this, true);
    this.highlighter.attachInspectListeners();
  },

  



  detachPageListeners: function IUI_detachPageListeners()
  {
    this.browser.removeEventListener("keypress", this, true);
    this.highlighter.detachInspectListeners();
  },

  
  

  






  inspectNode: function IUI_inspectNode(aNode)
  {
    this.select(aNode, true, true);
    this.highlighter.highlightNode(aNode);
  },

  








  elementFromPoint: function IUI_elementFromPoint(aDocument, aX, aY)
  {
    let node = aDocument.elementFromPoint(aX, aY);
    if (node && node.contentDocument) {
      switch (node.nodeName.toLowerCase()) {
        case "iframe":
          let rect = node.getBoundingClientRect();
          aX -= rect.left;
          aY -= rect.top;

        case "frame":
          let subnode = this.elementFromPoint(node.contentDocument, aX, aY);
          if (subnode) {
            node = subnode;
          }
      }
    }
    return node;
  },

  
  

  







  hasClass: function IUI_hasClass(aNode, aClass)
  {
    if (!(aNode instanceof Element))
      return false;
    return aNode.classList.contains(aClass);
  },

  






  addClass: function IUI_addClass(aNode, aClass)
  {
    if (aNode instanceof Element)
      aNode.classList.add(aClass);
  },

  






  removeClass: function IUI_removeClass(aNode, aClass)
  {
    if (aNode instanceof Element)
      aNode.classList.remove(aClass);
  },

  





  getWindowID: function IUI_getWindowID(aWindow)
  {
    if (!aWindow) {
      return null;
    }

    let util = {};

    try {
      util = aWindow.QueryInterface(Ci.nsIInterfaceRequestor).
        getInterface(Ci.nsIDOMWindowUtils);
    } catch (ex) { }

    return util.currentInnerWindowID;
  },

  








  getRepObject: function IUI_getRepObject(element)
  {
    let target = null;
    for (let child = element; child; child = child.parentNode) {
      if (this.hasClass(child, "repTarget"))
        target = child;

      if (child.repObject) {
        if (!target && this.hasClass(child.repObject, "repIgnore"))
          break;
        else
          return child.repObject;
      }
    }
    return null;
  },

  



  _log: function LOG(msg)
  {
    Services.console.logStringMessage(msg);
  },

  




  _trace: function TRACE(msg)
  {
    this._log("TRACE: " + msg);
    let frame = Components.stack.caller;
    while (frame = frame.caller) {
      if (frame.language == Ci.nsIProgrammingLanguage.JAVASCRIPT ||
          frame.language == Ci.nsIProgrammingLanguage.JAVASCRIPT2) {
        this._log("filename: " + frame.filename + " lineNumber: " + frame.lineNumber +
          " functionName: " + frame.name);
      }
    }
    this._log("END TRACE");
  },

  

















  registerTool: function IUI_RegisterTool(aRegObj) {
    if (this.tools[aRegObj.id]) {
      return;
    } else {
      let id = aRegObj.id;
      let buttonId = "inspector-" + id + "-toolbutton";
      aRegObj.buttonId = buttonId;

      aRegObj.panel.addEventListener("popuphiding",
        function IUI_toolPanelHiding() {
          btn.setAttribute("checked", "false");
        }, false);
      aRegObj.panel.addEventListener("popupshowing",
        function IUI_toolPanelShowing() {
          btn.setAttribute("checked", "true");
        }, false);

      this.tools[id] = aRegObj;
    }

    let toolbox = document.getElementById("inspector-tools");
    let btn = document.createElement("toolbarbutton");
    btn.setAttribute("id", aRegObj.buttonId);
    btn.setAttribute("label", aRegObj.label);
    btn.setAttribute("tooltiptext", aRegObj.tooltiptext);
    btn.setAttribute("accesskey", aRegObj.accesskey);
    btn.setAttribute("class", "toolbarbutton-text");
    btn.setAttribute("image", aRegObj.icon || "");
    toolbox.appendChild(btn);

    btn.addEventListener("click",
      function IUI_ToolButtonClick(aEvent) {
        if (btn.getAttribute("checked") == "true") {
          aRegObj.onHide.apply(aRegObj.context);
        } else {
          aRegObj.onShow.apply(aRegObj.context, [InspectorUI.selection]);
          aRegObj.onSelect.apply(aRegObj.context, [InspectorUI.selection]);
        }
      }, false);
  },






  saveToolState: function IUI_saveToolState(aWinID)
  {
    let openTools = {};
    this.toolsDo(function IUI_toolsSetId(aTool) {
      if (aTool.panel.state == "open") {
        openTools[aTool.id] = true;
      }
    });
    InspectorStore.setValue(aWinID, "openTools", openTools);
  },







  restoreToolState: function IUI_restoreToolState(aWinID)
  {
    let openTools = InspectorStore.getValue(aWinID, "openTools");
    InspectorUI.selection = InspectorUI.selection;
    if (openTools) {
      this.toolsDo(function IUI_toolsOnShow(aTool) {
        if (aTool.id in openTools) {
          aTool.onShow.apply(aTool.context, [InspectorUI.selection]);
        }
      });
    }
  },
  
  




  toolsDo: function IUI_toolsDo(aFunction)
  {
    for each (let tool in this.tools) {
      aFunction(tool);
    }
  },
};




var InspectorStore = {
  store: {},
  length: 0,

  





  isEmpty: function IS_isEmpty()
  {
    return this.length == 0 ? true : false;
  },

  






  addStore: function IS_addStore(aID)
  {
    let result = false;

    if (!(aID in this.store)) {
      this.store[aID] = {};
      this.length++;
      result = true;
    }

    return result;
  },

  






  deleteStore: function IS_deleteStore(aID)
  {
    let result = false;

    if (aID in this.store) {
      delete this.store[aID];
      this.length--;
      result = true;
    }

    return result;
  },

  





  hasID: function IS_hasID(aID)
  {
    return (aID in this.store);
  },

  






  getValue: function IS_getValue(aID, aKey)
  {
    if (!this.hasID(aID))
      return null;
    if (aKey in this.store[aID])
      return this.store[aID][aKey];
    return null;
  },

  








  setValue: function IS_setValue(aID, aKey, aValue)
  {
    let result = false;

    if (aID in this.store) {
      this.store[aID][aKey] = aValue;
      result = true;
    }

    return result;
  },

  







  deleteValue: function IS_deleteValue(aID, aKey)
  {
    let result = false;

    if (aID in this.store && aKey in this.store[aID]) {
      delete this.store[aID][aKey];
      result = true;
    }

    return result;
  }
};




XPCOMUtils.defineLazyGetter(InspectorUI, "inspectCmd", function () {
  return document.getElementById("Tools:Inspect");
});
