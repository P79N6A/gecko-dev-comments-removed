

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










function PanelHighlighter(aBrowser)
{
  this.panel = document.getElementById("highlighter-panel");
  this.panel.hidden = false;
  this.browser = aBrowser;
  this.win = this.browser.contentWindow;
}

PanelHighlighter.prototype = {

  





  highlight: function PanelHighlighter_highlight(scroll)
  {
    
    if (!this.isNodeHighlightable()) {
      return;
    }

    this.unhighlight();

    let rect = this.node.getBoundingClientRect();

    if (scroll) {
      this.node.scrollIntoView();
    }

    if (this.viewContainsRect(rect)) {
      
      this.panel.openPopup(this.node, "overlap", 0, 0, false, false);
      this.panel.sizeTo(rect.width, rect.height);
    } else {
      this.highlightVisibleRegion(rect);
    }
  },

  







  highlightNode: function PanelHighlighter_highlightNode(aNode, aParams)
  {
    this.node = aNode;
    this.highlight(aParams && aParams.scroll);
  },

  






  highlightVisibleRegion: function PanelHighlighter_highlightVisibleRegion(aRect)
  {
    let offsetX = 0;
    let offsetY = 0;
    let width = 0;
    let height = 0;
    let visibleWidth = this.win.innerWidth;
    let visibleHeight = this.win.innerHeight;

    
    
    if (aRect.top > visibleHeight || aRect.left > visibleWidth ||
        aRect.bottom < 0 || aRect.right < 0) {
      return false;
    }

    
    
    
    offsetX = aRect.left < 0 ? Math.abs(aRect.left) : 0;
    offsetY = aRect.top < 0 ? Math.abs(aRect.top) : 0;

    
    
    width = aRect.right > visibleWidth ? visibleWidth - aRect.left :
      aRect.width;
    width -= offsetX;

    
    height = aRect.bottom > visibleHeight ? visibleHeight - aRect.top :
      aRect.height;
    height -= offsetY;

    
    
    if (width > 0 && height > 0) {
      this.panel.openPopup(this.node, "overlap", offsetX, offsetY, false,
        false);
      this.panel.sizeTo(width, height);
      return true;
    }

    return false;
  },

  


  unhighlight: function PanelHighlighter_unhighlight()
  {
    if (this.isHighlighting) {
      this.panel.hidePopup();
    }
  },

  




  get isHighlighting()
  {
    return this.panel.state == "open";
  },

  









  midPoint: function PanelHighlighter_midPoint(aPointA, aPointB)
  {
    let pointC = { };
    pointC.x = (aPointB.x - aPointA.x) / 2 + aPointA.x;
    pointC.y = (aPointB.y - aPointA.y) / 2 + aPointA.y;
    return pointC;
  },

  






  get highlitNode()
  {
    
    if (!this.isHighlighting) {
      return null;
    }

    let browserRect = this.browser.getBoundingClientRect();
    let clientRect = this.panel.getBoundingClientRect();

    
    let a = {
      x: clientRect.left - browserRect.left,
      y: clientRect.top - browserRect.top
    };

    
    let b = {
      x: clientRect.right - browserRect.left,
      y: clientRect.bottom - browserRect.top
    };

    
    let midpoint = this.midPoint(a, b);

    return InspectorUI.elementFromPoint(this.win.document, midpoint.x,
      midpoint.y);
  },

  




  isNodeHighlightable: function PanelHighlighter_isNodeHighlightable()
  {
    if (!this.node) {
      return false;
    }
    let nodeName = this.node.nodeName.toLowerCase();
    if (nodeName[0] == '#') {
      return false;
    }
    return !INSPECTOR_INVISIBLE_ELEMENTS[nodeName];
  },

  







  viewContainsRect: function PanelHighlighter_viewContainsRect(aRect)
  {
    let visibleWidth = this.win.innerWidth;
    let visibleHeight = this.win.innerHeight;

    return ((0 <= aRect.left) && (aRect.right <= visibleWidth) &&
        (0 <= aRect.top) && (aRect.bottom <= visibleHeight))
  },

  
  

  





  handleMouseMove: function PanelHighlighter_handleMouseMove(aEvent)
  {
    if (!InspectorUI.inspecting) {
      return;
    }
    let browserRect = this.browser.getBoundingClientRect();
    let element = InspectorUI.elementFromPoint(this.win.document,
      aEvent.clientX - browserRect.left, aEvent.clientY - browserRect.top);
    if (element && element != this.node) {
      InspectorUI.inspectNode(element);
    }
  },

  






  handlePixelScroll: function PanelHighlighter_handlePixelScroll(aEvent) {
    if (!InspectorUI.inspecting) {
      return;
    }
    let browserRect = this.browser.getBoundingClientRect();
    let element = InspectorUI.elementFromPoint(this.win.document,
      aEvent.clientX - browserRect.left, aEvent.clientY - browserRect.top);
    let win = element.ownerDocument.defaultView;

    if (aEvent.axis == aEvent.HORIZONTAL_AXIS) {
      win.scrollBy(aEvent.detail, 0);
    } else {
      win.scrollBy(0, aEvent.detail);
    }
  }
};







var InspectorUI = {
  browser: null,
  selectEventsSuppressed: false,
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

    
    this.initializeStore();

    if (InspectorStore.getValue(this.winID, "inspecting"))
      this.startInspecting();

    this.notifyReady();
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
      if (this.embeddedBrowserParents) {
        let skipParent = this.embeddedBrowserParents[node];
        
        if (skipParent)
          return skipParent;
      } else 
        return null;
    } else if (!parentNode.localName) {
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
        if (!this.embeddedBrowserParents)
          this.embeddedBrowserParents = {};
        let skipChild = node.contentDocument.documentElement;
        this.embeddedBrowserParents[skipChild] = node;
        return skipChild;  
      }
      return null;
    }

    if (node instanceof GetSVGDocument) {
      
      if (index == 0) {
        if (!this.embeddedBrowserParents)
          this.embeddedBrowserParents = {};
        let skipChild = node.getSVGDocument().documentElement;
        this.embeddedBrowserParents[skipChild] = node;
        return skipChild;  
      }
      return null;
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
    if (!this.domplate) {
      Cu.import("resource:///modules/domplate.jsm", this);
      this.domplateUtils.setDOM(window);
    }

    
    this.openTreePanel();

    this.win.document.addEventListener("scroll", this, false);
    this.win.addEventListener("resize", this, false);
    this.inspectCmd.setAttribute("checked", true);
  },

  


  initializeHighlighter: function IUI_initializeHighlighter()
  {
    this.highlighter = new PanelHighlighter(this.browser);
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

    this.win.document.removeEventListener("scroll", this, false);
    this.win.removeEventListener("resize", this, false);
    this.stopInspecting();
    if (this.highlighter && this.highlighter.isHighlighting) {
      this.highlighter.unhighlight();
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

    this.inspectCmd.setAttribute("checked", false);
    this.browser = this.win = null; 
    this.winID = null;
    this.selection = null;
    this.treeLoaded = false;

    this.treePanel.addEventListener("popuphidden", function treePanelHidden() {
      InspectorUI.closing = false;
      Services.obs.notifyObservers(null, "inspector-closed", null);
    }, false);

    this.treePanel.hidePopup();
    delete this.treePanel;
  },

  



  startInspecting: function IUI_startInspecting()
  {
    this.attachPageListeners();
    this.inspecting = true;
  },

  



  stopInspecting: function IUI_stopInspecting()
  {
    if (!this.inspecting)
      return;
    this.detachPageListeners();
    this.inspecting = false;
    if (this.highlighter.node) {
      this.select(this.highlighter.node, true, true);
    }
  },

  








  select: function IUI_select(aNode, forceUpdate, aScroll)
  {
    if (!aNode)
      aNode = this.defaultSelection;

    if (forceUpdate || aNode != this.selection) {
      this.selection = aNode;
      let box = this.ioBox.createObjectBox(this.selection);
      if (!this.inspecting) {
        this.highlighter.highlightNode(this.selection);
      }
      this.ioBox.select(aNode, true, true, aScroll);
    }
  },

  
  

  notifyReady: function IUI_notifyReady()
  {
    Services.obs.notifyObservers(null, "inspector-opened", null);
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
                "inspector-closed", false);
              InspectorUI.openInspectorUI();
            }, "inspector-closed", false);
          } else {
            this.openInspectorUI();
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
            this.stopInspecting();
            break;
        }
        break;
      case "mousemove":
        let element = this.elementFromPoint(event.target.ownerDocument,
          event.clientX, event.clientY);
        if (element && element != this.node) {
          this.inspectNode(element);
        }
        break;
      case "click":
        this.stopInspecting();
        break;
      case "scroll":
      case "resize":
        this.highlighter.highlight();
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
      if (hitTwisty)
        this.ioBox.toggleObject(node);
      this.select(node, false, false);
    }
  },

  



  attachPageListeners: function IUI_attachPageListeners()
  {
    this.win.addEventListener("keypress", this, true);
    this.browser.addEventListener("mousemove", this, true);
    this.browser.addEventListener("click", this, true);
  },

  



  detachPageListeners: function IUI_detachPageListeners()
  {
    this.win.removeEventListener("keypress", this, true);
    this.browser.removeEventListener("mousemove", this, true);
    this.browser.removeEventListener("click", this, true);
  },

  
  

  






  inspectNode: function IUI_inspectNode(aNode)
  {
    this.highlighter.highlightNode(aNode);
    this.selectEventsSuppressed = true;
    this.select(aNode, true, true);
    this.selectEventsSuppressed = false;
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
}




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

