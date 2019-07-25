










































const Cu = Components.utils;
const Ci = Components.interfaces;
const Cr = Components.results;

var EXPORTED_SYMBOLS = ["InspectorUI"];

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

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

  
  TREEPANELREADY: "inspector-treepanel-ready",

  
  EDITOR_OPENED: "inspector-editor-opened",
  EDITOR_CLOSED: "inspector-editor-closed",
  EDITOR_SAVED: "inspector-editor-saved",
};
















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

    
    
    this.buildControls(controlsBox);

    this.browser.addEventListener("resize", this, true);
    this.browser.addEventListener("scroll", this, true);

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

  







  buildControls: function Highlighter_buildControls(aParent)
  {
    this.buildCloseButton(aParent);
    this.buildInfobar(aParent);
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

  





  buildCloseButton: function Highlighter_buildCloseButton(aParent)
  {
    let closeButton = this.chromeDoc.createElement("box");
    closeButton.id = "highlighter-close-button";
    closeButton.appendChild(this.chromeDoc.createElement("image"));

    let boundCloseEventHandler = this.IUI.closeInspectorUI.bind(this.IUI, false);

    closeButton.addEventListener("click", boundCloseEventHandler, false);

    aParent.appendChild(closeButton);

    this.boundCloseEventHandler = boundCloseEventHandler;
    this.closeButton = closeButton;
  },

  


  destroy: function Highlighter_destroy()
  {
    this.browser.removeEventListener("scroll", this, true);
    this.browser.removeEventListener("resize", this, true);
    this.closeButton.removeEventListener("click", this.boundCloseEventHandler, false);
    this.boundCloseEventHandler = null;
    this.closeButton.parentNode.removeChild(this.closeButton);
    this.closeButton = null;
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
    
    if (!this.node || !this.isNodeHighlightable(this.node)) {
      return;
    }

    if (aScroll) {
      this.node.scrollIntoView();
    }

    let clientRect = this.node.getBoundingClientRect();

    
    
    let rect = {top: clientRect.top,
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
    let oldRect = this._contentRect;

    if (oldRect && aRect.top == oldRect.top && aRect.left == oldRect.left &&
        aRect.width == oldRect.width && aRect.height == oldRect.height) {
      return this._highlighting; 
    }

    
    let zoom =
      this.win.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
      .getInterface(Components.interfaces.nsIDOMWindowUtils)
      .screenPixelsPerCSSPixel;

    
    let aRectScaled = {};
    for (let prop in aRect) {
      aRectScaled[prop] = aRect[prop] * zoom;
    }

    if (aRectScaled.left >= 0 && aRectScaled.top >= 0 &&
        aRectScaled.width > 0 && aRectScaled.height > 0) {
      
      
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
    Services.obs.notifyObservers(null,
      INSPECTOR_NOTIFICATIONS.UNHIGHLIGHTING, null);
  },

  


  updateInfobar: function Highlighter_updateInfobar()
  {
    
    this.nodeInfo.tagNameLabel.textContent = this.node.tagName;

    
    this.nodeInfo.idLabel.textContent = this.node.id;

    
    let classes = this.nodeInfo.classesBox;
    while (classes.hasChildNodes()) {
      classes.removeChild(classes.firstChild);
    }

    if (this.node.className) {
      let fragment = this.chromeDoc.createDocumentFragment();
      for (let i = 0; i < this.node.classList.length; i++) {
        let classLabel = this.chromeDoc.createElement("label");
        classLabel.className = "highlighter-nodeinfobar-class plain";
        classLabel.textContent = this.node.classList[i];
        fragment.appendChild(classLabel);
      }
      classes.appendChild(fragment);
    }
  },

  


  moveInfobar: function Highlighter_moveInfobar()
  {
    let rect = this._highlightRect;
    if (rect && this._highlighting) {
      this.nodeInfo.container.removeAttribute("disabled");
      
      if (rect.top < this.nodeInfo.barHeight) {
        
        if (rect.top + rect.height +
            this.nodeInfo.barHeight > this.win.innerHeight) {
          
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
        if (left + barWidth > this.win.innerWidth) {
          left = this.win.innerWidth - barWidth;
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











function InspectorUI(aWindow)
{
  this.chromeWin = aWindow;
  this.chromeDoc = aWindow.document;
  this.tabbrowser = aWindow.gBrowser;
  this.tools = {};
  this.toolEvents = {};
  this.store = new InspectorStore();
  this.INSPECTOR_NOTIFICATIONS = INSPECTOR_NOTIFICATIONS;
}

InspectorUI.prototype = {
  browser: null,
  tools: null,
  toolEvents: null,
  inspecting: false,
  treePanelEnabled: true,
  isDirty: false,
  store: null,

  





  toggleInspectorUI: function IUI_toggleInspectorUI(aEvent)
  {
    if (this.isInspectorOpen) {
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

  




  get isInspectorOpen()
  {
    return this.toolbar && !this.toolbar.hidden && this.highlighter;
  },

  


  get defaultSelection()
  {
    let doc = this.win.document;
    return doc.documentElement ? doc.documentElement.lastElementChild : null;
  },

  







  openInspectorUI: function IUI_openInspectorUI(aNode)
  {
    
    if (this.isInspectorOpen && aNode) {
      this.inspectNode(aNode);
      this.stopInspecting();
      return;
    }

    
    
    function inspectObserver(aElement) {
      Services.obs.removeObserver(boundInspectObserver,
                                  INSPECTOR_NOTIFICATIONS.OPENED,
                                  false);
      this.inspectNode(aElement);
      this.stopInspecting();
    };

    var boundInspectObserver = inspectObserver.bind(this, aNode);

    if (aNode) {
      
      Services.obs.addObserver(boundInspectObserver,
                               INSPECTOR_NOTIFICATIONS.OPENED,
                               false);
    }
    
    this.browser = this.tabbrowser.selectedBrowser;
    this.win = this.browser.contentWindow;
    this.winID = this.getWindowID(this.win);
    this.toolbar = this.chromeDoc.getElementById("inspector-toolbar");
    this.inspectMenuitem = this.chromeDoc.getElementById("Tools:Inspect");
    this.inspectToolbutton =
      this.chromeDoc.getElementById("inspector-inspect-toolbutton");

    this.initTools();

    if (!this.TreePanel && this.treePanelEnabled) {
      Cu.import("resource:///modules/TreePanel.jsm", this);
      this.treePanel = new this.TreePanel(this.chromeWin, this);
    }

    this.toolbar.hidden = false;
    this.inspectMenuitem.setAttribute("checked", true);

    this.isDirty = false;

    this.progressListener = new InspectorProgressListener(this);

    
    this.initializeHighlighter();
  },

  


  initTools: function IUI_initTools()
  {
    
    if (Services.prefs.getBoolPref("devtools.styleinspector.enabled") &&
        !this.toolRegistered("styleinspector")) {
      let stylePanel = StyleInspector.createPanel(true);
      this.registerTool({
        id: "styleinspector",
        label: StyleInspector.l10n("style.highlighter.button.label"),
        tooltiptext: StyleInspector.l10n("style.highlighter.button.tooltip"),
        accesskey: StyleInspector.l10n("style.highlighter.accesskey"),
        context: stylePanel,
        get isOpen() stylePanel.isOpen(),
        onSelect: stylePanel.selectNode,
        show: stylePanel.showTool,
        hide: stylePanel.hideTool,
        dim: stylePanel.dimTool,
        panel: stylePanel,
        unregister: stylePanel.destroy,
      });
      this.stylePanel = stylePanel;
    }
  },

  


  initializeHighlighter: function IUI_initializeHighlighter()
  {
    this.highlighter = new Highlighter(this);
    this.highlighterReady();
  },

  


  initializeStore: function IUI_initializeStore()
  {
    
    if (this.store.isEmpty()) {
      this.tabbrowser.tabContainer.addEventListener("TabSelect", this, false);
    }

    
    if (this.store.hasID(this.winID)) {
      let selectedNode = this.store.getValue(this.winID, "selectedNode");
      if (selectedNode) {
        this.inspectNode(selectedNode);
      }
      this.isDirty = this.store.getValue(this.winID, "isDirty");
    } else {
      
      this.store.addStore(this.winID);
      this.store.setValue(this.winID, "selectedNode", null);
      this.store.setValue(this.winID, "inspecting", true);
      this.store.setValue(this.winID, "isDirty", this.isDirty);
      this.win.addEventListener("pagehide", this, true);
    }
  },

  









  closeInspectorUI: function IUI_closeInspectorUI(aKeepStore)
  {
    
    
    if (this.treePanel && this.treePanel.editingContext)
      this.treePanel.closeEditor();

    if (this.closing || !this.win || !this.browser) {
      return;
    }

    this.closing = true;
    this.toolbar.hidden = true;

    this.progressListener.destroy();
    delete this.progressListener;

    if (!aKeepStore) {
      this.store.deleteStore(this.winID);
      this.win.removeEventListener("pagehide", this, true);
    } else {
      
      if (this.selection) {
        this.store.setValue(this.winID, "selectedNode",
          this.selection);
      }
      this.store.setValue(this.winID, "inspecting", this.inspecting);
      this.store.setValue(this.winID, "isDirty", this.isDirty);
    }

    if (this.store.isEmpty()) {
      this.tabbrowser.tabContainer.removeEventListener("TabSelect", this, false);
    }

    this.stopInspecting();

    this.saveToolState(this.winID);
    this.toolsDo(function IUI_toolsHide(aTool) {
      this.unregisterTool(aTool);
    }.bind(this));

    if (this.highlighter) {
      this.highlighter.destroy();
      this.highlighter = null;
    }

    this.inspectMenuitem.setAttribute("checked", false);
    this.browser = this.win = null; 
    this.winID = null;
    this.selection = null;
    this.closing = false;
    this.isDirty = false;

    delete this.treePanel;
    delete this.stylePanel;
    delete this.toolbar;
    delete this.TreePanel;
    Services.obs.notifyObservers(null, INSPECTOR_NOTIFICATIONS.CLOSED, null);
  },

  



  startInspecting: function IUI_startInspecting()
  {
    
    
    if (this.treePanel && this.treePanel.editingContext)
      this.treePanel.closeEditor();

    this.inspectToolbutton.checked = true;
    this.attachPageListeners();
    this.inspecting = true;
    this.toolsDim(true);
    this.highlighter.veilContainer.removeAttribute("locked");
  },

  





  stopInspecting: function IUI_stopInspecting(aPreventScroll)
  {
    if (!this.inspecting) {
      return;
    }

    this.inspectToolbutton.checked = false;
    this.detachPageListeners();
    this.inspecting = false;
    this.toolsDim(false);
    if (this.highlighter.node) {
      this.select(this.highlighter.node, true, true, !aPreventScroll);
    } else {
      this.select(null, true, true);
    }
    this.highlighter.veilContainer.setAttribute("locked", true);
  },

  








  select: function IUI_select(aNode, forceUpdate, aScroll)
  {
    
    
    if (this.treePanel && this.treePanel.editingContext)
      this.treePanel.closeEditor();

    if (!aNode)
      aNode = this.defaultSelection;

    if (forceUpdate || aNode != this.selection) {
      this.selection = aNode;
      if (!this.inspecting) {
        this.highlighter.highlightNode(this.selection);
      }
    }

    this.toolsSelect(aScroll);
  },

  
  

  highlighterReady: function IUI_highlighterReady()
  {
    
    this.initializeStore();

    if (this.store.getValue(this.winID, "inspecting")) {
      this.startInspecting();
    }

    this.restoreToolState(this.winID);

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
        winID = this.getWindowID(this.tabbrowser.selectedBrowser.contentWindow);
        if (this.isInspectorOpen && winID != this.winID) {
          this.closeInspectorUI(true);
          inspectorClosed = true;
        }

        if (winID && this.store.hasID(winID)) {
          if (inspectorClosed && this.closing) {
            Services.obs.addObserver(function reopenInspectorForTab() {
              Services.obs.removeObserver(reopenInspectorForTab,
                INSPECTOR_NOTIFICATIONS.CLOSED, false);

              this.openInspectorUI();
            }.bind(this), INSPECTOR_NOTIFICATIONS.CLOSED, false);
          } else {
            this.openInspectorUI();
          }
        }

        if (this.store.isEmpty()) {
          this.tabbrowser.tabContainer.removeEventListener("TabSelect", this,
                                                         false);
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
          this.store.deleteStore(winID);
        }

        if (this.store.isEmpty()) {
          this.tabbrowser.tabContainer.removeEventListener("TabSelect", this,
                                                         false);
        }
        break;
      case "keypress":
        switch (event.keyCode) {
          case this.chromeWin.KeyEvent.DOM_VK_RETURN:
          case this.chromeWin.KeyEvent.DOM_VK_ESCAPE:
            if (this.inspecting) {
              this.stopInspecting();
              event.preventDefault();
              event.stopPropagation();
            }
            break;
          case this.chromeWin.KeyEvent.DOM_VK_LEFT:
            let node;
            if (this.selection) {
              node = this.selection.parentNode;
            } else {
              node = this.defaultSelection;
            }
            if (node && this.highlighter.isNodeHighlightable(node)) {
              this.inspectNode(node, true);
            }
            event.preventDefault();
            event.stopPropagation();
            break;
          case this.chromeWin.KeyEvent.DOM_VK_RIGHT:
            if (this.selection) {
              
              for (let i = 0; i < this.selection.childNodes.length; i++) {
                node = this.selection.childNodes[i];
                if (node && this.highlighter.isNodeHighlightable(node)) {
                  break;
                }
              }
            } else {
              node = this.defaultSelection;
            }
            if (node && this.highlighter.isNodeHighlightable(node)) {
              this.inspectNode(node, true);
            }
            event.preventDefault();
            event.stopPropagation();
            break;
          case this.chromeWin.KeyEvent.DOM_VK_UP:
            if (this.selection) {
              
              node = this.selection.previousSibling;
              while (node && !this.highlighter.isNodeHighlightable(node)) {
                node = node.previousSibling;
              }
            } else {
              node = this.defaultSelection;
            }
            if (node && this.highlighter.isNodeHighlightable(node)) {
              this.inspectNode(node, true);
            }
            event.preventDefault();
            event.stopPropagation();
            break;
          case this.chromeWin.KeyEvent.DOM_VK_DOWN:
            if (this.selection) {
              
              node = this.selection.nextSibling;
              while (node && !this.highlighter.isNodeHighlightable(node)) {
                node = node.nextSibling;
              }
            } else {
              node = this.defaultSelection;
            }
            if (node && this.highlighter.isNodeHighlightable(node)) {
              this.inspectNode(node, true);
            }
            event.preventDefault();
            event.stopPropagation();
            break;
        }
        break;
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

  
  

  








  inspectNode: function IUI_inspectNode(aNode, aScroll)
  {
    this.select(aNode, true, true);
    this.highlighter.highlightNode(aNode, { scroll: aScroll });
  },

  








  elementFromPoint: function IUI_elementFromPoint(aDocument, aX, aY)
  {
    let node = aDocument.elementFromPoint(aX, aY);
    if (node && node.contentDocument) {
      if (node instanceof Ci.nsIDOMHTMLIFrameElement) {
        let rect = node.getBoundingClientRect();

        
        let [offsetTop, offsetLeft] = this.getIframeContentOffset(node);

        aX -= rect.left + offsetLeft;
        aY -= rect.top + offsetTop;

        if (aX < 0 || aY < 0) {
          
          return node;
        }
      }
      if (node instanceof Ci.nsIDOMHTMLIFrameElement ||
          node instanceof Ci.nsIDOMHTMLFrameElement) {
        let subnode = this.elementFromPoint(node.contentDocument, aX, aY);
        if (subnode) {
          node = subnode;
        }
      }
    }
    return node;
  },

  
  

  













  getIframeContentOffset: function IUI_getIframeContentOffset(aIframe)
  {
    let style = aIframe.contentWindow.getComputedStyle(aIframe, null);

    let paddingTop = parseInt(style.getPropertyValue("padding-top"));
    let paddingLeft = parseInt(style.getPropertyValue("padding-left"));

    let borderTop = parseInt(style.getPropertyValue("border-top-width"));
    let borderLeft = parseInt(style.getPropertyValue("border-left-width"));

    return [borderTop + paddingTop, borderLeft + paddingLeft];
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

  







  getToolbarButtonId: function IUI_createButtonId(anId)
  {
    return "inspector-" + anId + "-toolbutton";
  },

  






















  registerTool: function IUI_registerTool(aRegObj)
  {
    if (this.toolRegistered(aRegObj.id)) {
      return;
    }

    this.tools[aRegObj.id] = aRegObj;

    let buttonContainer = this.chromeDoc.getElementById("inspector-tools");
    let btn = this.chromeDoc.createElement("toolbarbutton");
    let buttonId = this.getToolbarButtonId(aRegObj.id);
    btn.setAttribute("id", buttonId);
    btn.setAttribute("label", aRegObj.label);
    btn.setAttribute("tooltiptext", aRegObj.tooltiptext);
    btn.setAttribute("accesskey", aRegObj.accesskey);
    btn.setAttribute("image", aRegObj.icon || "");
    buttonContainer.appendChild(btn);

    





    let toolEvents = this.toolEvents;
    function bindToolEvent(aWidget, aEvent, aCallback) {
      toolEvents[aWidget.id + "_" + aEvent] = aCallback;
      aWidget.addEventListener(aEvent, aCallback, false);
    }

    bindToolEvent(btn, "click",
      function IUI_toolButtonClick(aEvent) {
        if (btn.checked) {
          this.toolHide(aRegObj);
        } else {
          this.toolShow(aRegObj);
        }
      }.bind(this));

    if (aRegObj.panel) {
      bindToolEvent(aRegObj.panel, "popuphiding",
        function IUI_toolPanelHiding() {
          btn.checked = false;
        });
    }
  },

  



  toolShow: function IUI_toolShow(aTool)
  {
    aTool.show.call(aTool.context, this.selection);
    this.chromeDoc.getElementById(this.getToolbarButtonId(aTool.id)).checked = true;
  },

  



  toolHide: function IUI_toolHide(aTool)
  {
    aTool.hide.call(aTool.context);
    this.chromeDoc.getElementById(this.getToolbarButtonId(aTool.id)).checked = false;
  },

  





  unregisterTool: function IUI_unregisterTool(aRegObj)
  {
    let button = this.chromeDoc.getElementById(this.getToolbarButtonId(aRegObj.id));

    




    let toolEvents = this.toolEvents;
    function unbindToolEvent(aWidget, aEvent) {
      let toolEvent = aWidget.id + "_" + aEvent;
      aWidget.removeEventListener(aEvent, toolEvents[toolEvent], false);
      delete toolEvents[toolEvent]
    };

    let buttonContainer = this.chromeDoc.getElementById("inspector-tools");
    unbindToolEvent(button, "click");

    if (aRegObj.panel)
      unbindToolEvent(aRegObj.panel, "popuphiding");

    buttonContainer.removeChild(button);

    if (aRegObj.unregister)
      aRegObj.unregister.call(aRegObj.context);

    delete this.tools[aRegObj.id];
  },

  




  saveToolState: function IUI_saveToolState(aWinID)
  {
    let openTools = {};
    this.toolsDo(function IUI_toolsSetId(aTool) {
      if (aTool.isOpen) {
        openTools[aTool.id] = true;
      }
    });
    this.store.setValue(aWinID, "openTools", openTools);
  },

  





  restoreToolState: function IUI_restoreToolState(aWinID)
  {
    let openTools = this.store.getValue(aWinID, "openTools");
    if (openTools) {
      this.toolsDo(function IUI_toolsOnShow(aTool) {
        if (aTool.id in openTools) {
          this.toolShow(aTool);
        }
      }.bind(this));
    }
  },

  





  toolsSelect: function IUI_toolsSelect(aScroll)
  {
    let selection = this.selection;
    this.toolsDo(function IUI_toolsOnSelect(aTool) {
      if (aTool.isOpen) {
        aTool.onSelect.call(aTool.context, selection, aScroll);
      }
    });
  },

  



  toolsDim: function IUI_toolsDim(aState)
  {
    this.toolsDo(function IUI_toolsOnSelect(aTool) {
      if (aTool.isOpen && "dim" in aTool) {
        aTool.dim.call(aTool.context, aState);
      }
    });
  },

  



  toolsDo: function IUI_toolsDo(aFunction)
  {
    for each (let tool in this.tools) {
      aFunction(tool);
    }
  },

  



  toolRegistered: function IUI_toolRegistered(aId)
  {
    return aId in this.tools;
  },

  



  destroy: function IUI_destroy()
  {
    if (this.isInspectorOpen) {
      this.closeInspectorUI();
    }

    delete this.store;
    delete this.chromeDoc;
    delete this.chromeWin;
    delete this.tabbrowser;
  },
};





function InspectorStore()
{
  this.store = {};
}
InspectorStore.prototype = {
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












function InspectorProgressListener(aInspector)
{
  this.IUI = aInspector;
  this.IUI.tabbrowser.addProgressListener(this);
}

InspectorProgressListener.prototype = {
  onStateChange:
  function IPL_onStateChange(aProgress, aRequest, aFlag, aStatus)
  {
    
    if (!this.IUI.isInspectorOpen) {
      this.destroy();
      return;
    }

    
    if (!(aFlag & Ci.nsIWebProgressListener.STATE_START)) {
      return;
    }

    
    
    if (aProgress.DOMWindow != this.IUI.win) {
      return;
    }

    if (this.IUI.isDirty) {
      this.showNotification(aRequest);
    } else {
      this.IUI.closeInspectorUI();
    }
  },

  







  showNotification: function IPL_showNotification(aRequest)
  {
    aRequest.suspend();

    let notificationBox = this.IUI.tabbrowser.getNotificationBox(this.IUI.browser);
    let notification = notificationBox.
      getNotificationWithValue("inspector-page-navigation");

    if (notification) {
      notificationBox.removeNotification(notification, true);
    }

    let cancelRequest = function onCancelRequest() {
      if (aRequest) {
        aRequest.cancel(Cr.NS_BINDING_ABORTED);
        aRequest.resume(); 
        aRequest = null;
      }
    };

    let eventCallback = function onNotificationCallback(aEvent) {
      if (aEvent == "removed") {
        cancelRequest();
      }
    };

    let buttons = [
      {
        id: "inspector.confirmNavigationAway.buttonLeave",
        label: this.IUI.strings.
          GetStringFromName("confirmNavigationAway.buttonLeave"),
        accessKey: this.IUI.strings.
          GetStringFromName("confirmNavigationAway.buttonLeaveAccesskey"),
        callback: function onButtonLeave() {
          if (aRequest) {
            aRequest.resume();
            aRequest = null;
            this.IUI.closeInspectorUI();
          }
        }.bind(this),
      },
      {
        id: "inspector.confirmNavigationAway.buttonStay",
        label: this.IUI.strings.
          GetStringFromName("confirmNavigationAway.buttonStay"),
        accessKey: this.IUI.strings.
          GetStringFromName("confirmNavigationAway.buttonStayAccesskey"),
        callback: cancelRequest
      },
    ];

    let message = this.IUI.strings.
      GetStringFromName("confirmNavigationAway.message");

    notification = notificationBox.appendNotification(message,
      "inspector-page-navigation", "chrome://browser/skin/Info.png",
      notificationBox.PRIORITY_WARNING_HIGH, buttons, eventCallback);

    
    
    notification.persistence = -1;
  },

  


  destroy: function IPL_destroy()
  {
    this.IUI.tabbrowser.removeProgressListener(this);

    let notificationBox = this.IUI.tabbrowser.getNotificationBox(this.IUI.browser);
    let notification = notificationBox.
      getNotificationWithValue("inspector-page-navigation");

    if (notification) {
      notificationBox.removeNotification(notification, true);
    }

    delete this.IUI;
  },
};




XPCOMUtils.defineLazyGetter(InspectorUI.prototype, "strings",
  function () {
    return Services.strings.
           createBundle("chrome://browser/locale/inspector.properties");
  });

XPCOMUtils.defineLazyGetter(this, "StyleInspector", function () {
  var obj = {};
  Cu.import("resource:///modules/devtools/StyleInspector.jsm", obj);
  return obj.StyleInspector;
});

