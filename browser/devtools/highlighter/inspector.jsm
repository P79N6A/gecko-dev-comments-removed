











































const Cu = Components.utils;
const Ci = Components.interfaces;
const Cr = Components.results;

var EXPORTED_SYMBOLS = ["InspectorUI"];

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/TreePanel.jsm");
Cu.import("resource:///modules/devtools/CssRuleView.jsm");

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

  
  DESTROYED: "inspector-destroyed",

  
  STATE_RESTORED: "inspector-state-restored",

  
  TREEPANELREADY: "inspector-treepanel-ready",

  
  RULEVIEWREADY: "inspector-ruleview-ready",

  
  EDITOR_OPENED: "inspector-editor-opened",
  EDITOR_CLOSED: "inspector-editor-closed",
  EDITOR_SAVED: "inspector-editor-saved",
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
  ruleViewEnabled: true,
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

  


  showSidebar: function IUI_showSidebar()
  {
    this.sidebarBox.removeAttribute("hidden");
    this.sidebarSplitter.removeAttribute("hidden");
    this.stylingButton.checked = true;

    
    
    if (!Array.some(this.sidebarToolbar.children,
      function(btn) btn.hasAttribute("checked"))) {
        let firstButtonId = this.getToolbarButtonId(this.sidebarTools[0].id);
        this.chromeDoc.getElementById(firstButtonId).click();
    }
  },

  


  hideSidebar: function IUI_hideSidebar()
  {
    this.sidebarBox.setAttribute("hidden", "true");
    this.sidebarSplitter.setAttribute("hidden", "true");
    this.stylingButton.checked = false;
  },

  



  toggleSidebar: function IUI_toggleSidebar()
  {
    if (!this.isSidebarOpen) {
      this.showSidebar();
    } else {
      this.hideSidebar();
    }
  },

  


  get isSidebarOpen()
  {
    return this.stylingButton.checked &&
          !this.sidebarBox.hidden &&
          !this.sidebarSplitter.hidden;
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
    this.chromeWin.Tilt.setup();

    if (this.treePanelEnabled) {
      this.treePanel = new TreePanel(this.chromeWin, this);
    }

    if (Services.prefs.getBoolPref("devtools.ruleview.enabled") &&
        !this.toolRegistered("ruleview")) {
      this.registerRuleView();
    }

    if (Services.prefs.getBoolPref("devtools.styleinspector.enabled") &&
        !this.toolRegistered("styleinspector")) {
      this.stylePanel = new StyleInspector(this.chromeWin, this);
    }

    this.toolbar.hidden = false;
    this.inspectMenuitem.setAttribute("checked", true);

    
    this.breadcrumbs = new HTMLBreadcrumbs(this);

    this.isDirty = false;

    this.progressListener = new InspectorProgressListener(this);

    
    this.initializeHighlighter();
  },

  


  registerRuleView: function IUI_registerRuleView()
  {
    let isOpen = this.isRuleViewOpen.bind(this);

    this.ruleViewObject = {
      id: "ruleview",
      label: this.strings.GetStringFromName("ruleView.label"),
      tooltiptext: this.strings.GetStringFromName("ruleView.tooltiptext"),
      accesskey: this.strings.GetStringFromName("ruleView.accesskey"),
      context: this,
      get isOpen() isOpen(),
      show: this.openRuleView,
      hide: this.closeRuleView,
      onSelect: this.selectInRuleView,
      panel: null,
      unregister: this.destroyRuleView,
      sidebar: true,
    };

    this.registerTool(this.ruleViewObject);
  },

  


  initTools: function IUI_initTools()
  {
    
  },

  


  initializeHighlighter: function IUI_initializeHighlighter()
  {
    this.highlighter = new Highlighter(this);
    this.browser.addEventListener("keypress", this, true);
    this.highlighter.highlighterContainer.addEventListener("keypress", this, true);
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

    let winId = new String(this.winID); 

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
    this.browser.removeEventListener("keypress", this, true);

    this.saveToolState(this.winID);
    this.toolsDo(function IUI_toolsHide(aTool) {
      this.unregisterTool(aTool);
    }.bind(this));

    
    this.hideSidebar();

    if (this.highlighter) {
      this.highlighter.highlighterContainer.removeEventListener("keypress",
                                                                this,
                                                                true);
      this.highlighter.destroy();
      this.highlighter = null;
    }

    if (this.breadcrumbs) {
      this.breadcrumbs.destroy();
      this.breadcrumbs = null;
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
    Services.obs.notifyObservers(null, INSPECTOR_NOTIFICATIONS.CLOSED, null);
    if (!aKeepStore)
      Services.obs.notifyObservers(null, INSPECTOR_NOTIFICATIONS.DESTROYED, winId);
  },

  



  startInspecting: function IUI_startInspecting()
  {
    
    
    if (this.treePanel && this.treePanel.editingContext)
      this.treePanel.closeEditor();

    this.inspectToolbutton.checked = true;
    this.highlighter.attachInspectListeners();

    this.inspecting = true;
    this.toolsDim(true);
    this.highlighter.veilContainer.removeAttribute("locked");
    this.highlighter.nodeInfo.container.removeAttribute("locked");
  },

  





  stopInspecting: function IUI_stopInspecting(aPreventScroll)
  {
    if (!this.inspecting) {
      return;
    }

    this.inspectToolbutton.checked = false;
    
    
    
    
    this.highlighter.detachInspectListeners();

    this.inspecting = false;
    this.toolsDim(false);
    if (this.highlighter.node) {
      this.select(this.highlighter.node, true, true, !aPreventScroll);
    } else {
      this.select(null, true, true);
    }
    this.highlighter.veilContainer.setAttribute("locked", true);
    this.highlighter.nodeInfo.container.setAttribute("locked", true);
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

    this.breadcrumbs.update();
    this.chromeWin.Tilt.update(aNode);

    this.toolsSelect(aScroll);
  },

  






  nodeChanged: function IUI_nodeChanged(aUpdater)
  {
    this.highlighter.highlight();
    this.toolsOnChanged(aUpdater);
  },

  
  

  highlighterReady: function IUI_highlighterReady()
  {
    
    this.initializeStore();

    if (this.store.getValue(this.winID, "inspecting")) {
      this.startInspecting();
    }

    this.restoreToolState(this.winID);

    this.win.focus();
    Services.obs.notifyObservers({wrappedJSObject: this},
                                 INSPECTOR_NOTIFICATIONS.OPENED, null);
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
          case this.chromeWin.KeyEvent.DOM_VK_ESCAPE:
            this.closeInspectorUI(false);
            event.preventDefault();
            event.stopPropagation();
            break;
          case this.chromeWin.KeyEvent.DOM_VK_RETURN:
            this.toggleInspection();
            event.preventDefault();
            event.stopPropagation();
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

  
  

  


  isRuleViewOpen: function IUI_isRuleViewOpen()
  {
    return this.isSidebarOpen && this.ruleButton.hasAttribute("checked") &&
      (this.sidebarDeck.selectedPanel == this.getToolIframe(this.ruleViewObject));
  },

  


  get ruleButton()
  {
    return this.chromeDoc.getElementById(
      this.getToolbarButtonId(this.ruleViewObject.id));
  },

  


  openRuleView: function IUI_openRuleView()
  {
    let iframe = this.getToolIframe(this.ruleViewObject);
    if (iframe.getAttribute("src")) {
      
      return;
    }

    let boundLoadListener = function() {
      iframe.removeEventListener("load", boundLoadListener, true);
      let doc = iframe.contentDocument;

      let winID = this.winID;
      let ruleViewStore = this.store.getValue(winID, "ruleView");
      if (!ruleViewStore) {
        ruleViewStore = {};
        this.store.setValue(winID, "ruleView", ruleViewStore);
      }

      this.ruleView = new CssRuleView(doc, ruleViewStore);

      this.boundRuleViewChanged = this.ruleViewChanged.bind(this);
      this.ruleView.element.addEventListener("CssRuleViewChanged",
                                             this.boundRuleViewChanged);

      doc.documentElement.appendChild(this.ruleView.element);
      this.ruleView.highlight(this.selection);
      Services.obs.notifyObservers(null,
        INSPECTOR_NOTIFICATIONS.RULEVIEWREADY, null);
    }.bind(this);

    iframe.addEventListener("load", boundLoadListener, true);

    iframe.setAttribute("src", "chrome://browser/content/devtools/cssruleview.xul");
  },

  



  closeRuleView: function IUI_closeRuleView()
  {
    
  },

  



  selectInRuleView: function IUI_selectInRuleView(aNode)
  {
    if (this.ruleView)
      this.ruleView.highlight(aNode);
  },

  ruleViewChanged: function IUI_ruleViewChanged()
  {
    this.isDirty = true;
    this.nodeChanged(this.ruleViewObject);
  },

  


  destroyRuleView: function IUI_destroyRuleView()
  {
    let iframe = this.getToolIframe(this.ruleViewObject);
    iframe.parentNode.removeChild(iframe);

    if (this.ruleView) {
      this.ruleView.element.removeEventListener("CssRuleViewChanged",
                                                this.boundRuleViewChanged);
      delete boundRuleViewChanged;
      this.ruleView.clear();
      delete this.ruleView;
    }
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

  





  bindToolEvent: function IUI_bindToolEvent(aWidget, aEvent, aCallback)
  {
    this.toolEvents[aWidget.id + "_" + aEvent] = aCallback;
    aWidget.addEventListener(aEvent, aCallback, false);
  },

  























  registerTool: function IUI_registerTool(aRegObj)
  {
    if (this.toolRegistered(aRegObj.id)) {
      return;
    }

    this.tools[aRegObj.id] = aRegObj;

    let buttonContainer = this.chromeDoc.getElementById("inspector-tools");
    let btn;

    
    if (aRegObj.sidebar) {
      this.createSidebarTool(aRegObj);
      return;
    }

    btn = this.chromeDoc.createElement("toolbarbutton");
    let buttonId = this.getToolbarButtonId(aRegObj.id);
    btn.setAttribute("id", buttonId);
    btn.setAttribute("class", "devtools-toolbarbutton");
    btn.setAttribute("label", aRegObj.label);
    btn.setAttribute("tooltiptext", aRegObj.tooltiptext);
    btn.setAttribute("accesskey", aRegObj.accesskey);
    btn.setAttribute("image", aRegObj.icon || "");
    buttonContainer.insertBefore(btn, this.stylingButton);

    this.bindToolEvent(btn, "click",
      function IUI_toolButtonClick(aEvent) {
        if (btn.checked) {
          this.toolHide(aRegObj);
        } else {
          this.toolShow(aRegObj);
        }
      }.bind(this));

    
    if (aRegObj.panel) {
      this.bindToolEvent(aRegObj.panel, "popuphiding",
        function IUI_toolPanelHiding() {
          btn.checked = false;
        });
    }
  },

  get sidebarBox()
  {
    return this.chromeDoc.getElementById("devtools-sidebar-box");
  },

  get sidebarToolbar()
  {
    return this.chromeDoc.getElementById("devtools-sidebar-toolbar");
  },

  get sidebarDeck()
  {
    return this.chromeDoc.getElementById("devtools-sidebar-deck");
  },

  get sidebarSplitter()
  {
    return this.chromeDoc.getElementById("devtools-side-splitter");
  },

  get stylingButton()
  {
    return this.chromeDoc.getElementById("inspector-style-button");
  },

  



  createSidebarTool: function IUI_createSidebarTab(aRegObj)
  {
    
    let btn = this.chromeDoc.createElement("toolbarbutton");
    let buttonId = this.getToolbarButtonId(aRegObj.id);

    btn.id = buttonId;
    btn.setAttribute("label", aRegObj.label);
    btn.setAttribute("class", "devtools-toolbarbutton");
    btn.setAttribute("tooltiptext", aRegObj.tooltiptext);
    btn.setAttribute("accesskey", aRegObj.accesskey);
    btn.setAttribute("image", aRegObj.icon || "");
    btn.setAttribute("type", "radio");
    btn.setAttribute("group", "sidebar-tools");
    this.sidebarToolbar.appendChild(btn);

    
    let iframe = this.chromeDoc.createElement("iframe");
    iframe.id = "devtools-sidebar-iframe-" + aRegObj.id;
    iframe.setAttribute("flex", "1");
    this.sidebarDeck.appendChild(iframe);

    
    this.bindToolEvent(btn, "click", function showIframe() {
      this.toolShow(aRegObj);
    }.bind(this));
  },

  




  getToolIframe: function IUI_getToolIFrame(aRegObj)
  {
    return this.chromeDoc.getElementById("devtools-sidebar-iframe-" + aRegObj.id);
  },

  



  toolShow: function IUI_toolShow(aTool)
  {
    let btn = this.chromeDoc.getElementById(this.getToolbarButtonId(aTool.id));
    btn.setAttribute("checked", "true");
    if (aTool.sidebar) {
      this.sidebarDeck.selectedPanel = this.getToolIframe(aTool);
      this.sidebarTools.forEach(function(other) {
        if (other != aTool)
          this.chromeDoc.getElementById(
            this.getToolbarButtonId(other.id)).removeAttribute("checked");
      }.bind(this));
    }

    aTool.show.call(aTool.context, this.selection);
  },

  



  toolHide: function IUI_toolHide(aTool)
  {
    aTool.hide.call(aTool.context);

    let btn = this.chromeDoc.getElementById(this.getToolbarButtonId(aTool.id));
    btn.removeAttribute("checked");
  },

  




  unbindToolEvent: function IUI_unbindToolEvent(aWidget, aEvent)
  {
    let toolEvent = aWidget.id + "_" + aEvent;
    aWidget.removeEventListener(aEvent, this.toolEvents[toolEvent], false);
    delete this.toolEvents[toolEvent]
  },

  





  unregisterTool: function IUI_unregisterTool(aRegObj)
  {
    
    if (aRegObj.sidebar) {
      this.unregisterSidebarTool(aRegObj);
      return;
    }

    let button = this.chromeDoc.getElementById(this.getToolbarButtonId(aRegObj.id));
    let buttonContainer = this.chromeDoc.getElementById("inspector-tools");

    
    this.unbindToolEvent(button, "click");

    
    if (aRegObj.panel)
      this.unbindToolEvent(aRegObj.panel, "popuphiding");

    
    buttonContainer.removeChild(button);

    
    if (aRegObj.unregister)
      aRegObj.unregister.call(aRegObj.context);

    delete this.tools[aRegObj.id];
  },

  





  unregisterSidebarTool: function IUI_unregisterSidebarTool(aRegObj)
  {
    
    let buttonId = this.getToolbarButtonId(aRegObj.id);
    let btn = this.chromeDoc.getElementById(buttonId);
    this.unbindToolEvent(btn, "click");

    
    this.sidebarToolbar.removeChild(btn);

    
    
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
    let activeSidebarTool;
    if (openTools) {
      this.toolsDo(function IUI_toolsOnShow(aTool) {
        if (aTool.id in openTools) {
          if (aTool.sidebar && !this.isSidebarOpen) {
            this.showSidebar();
            activeSidebarTool = aTool;
          }
          this.toolShow(aTool);
        }
      }.bind(this));
      this.sidebarTools.forEach(function(tool) {
        if (tool != activeSidebarTool)
          this.chromeDoc.getElementById(
            this.getToolbarButtonId(tool.id)).removeAttribute("checked");
      }.bind(this));
    }
    Services.obs.notifyObservers(null, INSPECTOR_NOTIFICATIONS.STATE_RESTORED, null);
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
    this.toolsDo(function IUI_toolsDim(aTool) {
      if (aTool.isOpen && "dim" in aTool) {
        aTool.dim.call(aTool.context, aState);
      }
    });
  },

  






  toolsOnChanged: function IUI_toolsChanged(aUpdater)
  {
    this.toolsDo(function IUI_toolsOnChanged(aTool) {
      if (aTool.isOpen && ("onChanged" in aTool) && aTool != aUpdater) {
        aTool.onChanged.call(aTool.context);
      }
    });
  },

  



  toolsDo: function IUI_toolsDo(aFunction)
  {
    for each (let tool in this.tools) {
      aFunction(tool);
    }
  },

  


  get sidebarTools()
  {
    let sidebarTools = [];
    for each (let tool in this.tools)
      if (tool.sidebar)
        sidebarTools.push(tool);
    return sidebarTools;
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

    let isStart = aFlag & Ci.nsIWebProgressListener.STATE_START;
    let isDocument = aFlag & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT;
    let isNetwork = aFlag & Ci.nsIWebProgressListener.STATE_IS_NETWORK;
    let isRequest = aFlag & Ci.nsIWebProgressListener.STATE_IS_REQUEST;

    
    if (!isStart || !isDocument || !isRequest || !isNetwork) {
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
            return true;
          }
          return false;
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

















function HTMLBreadcrumbs(aInspector)
{
  this.IUI = aInspector;
  this.DOMHelpers = new DOMHelpers(this.IUI.win);
  this._init();
}

HTMLBreadcrumbs.prototype = {
  _init: function BC__init()
  {
    this.container = this.IUI.chromeDoc.getElementById("inspector-breadcrumbs");
    this.container.addEventListener("mousedown", this, true);

    
    this.nodeHierarchy = [];

    
    this.currentIndex = -1;

    
    this.menu = this.IUI.chromeDoc.createElement("menupopup");
    this.menu.id = "inspector-breadcrumbs-menu";

    let popupSet = this.IUI.chromeDoc.getElementById("mainPopupSet");
    popupSet.appendChild(this.menu);

    this.menu.addEventListener("popuphiding", (function() {
      while (this.menu.hasChildNodes()) {
        this.menu.removeChild(this.menu.firstChild);
      }
      let button = this.container.querySelector("button[siblings-menu-open]");
      button.removeAttribute("siblings-menu-open");
    }).bind(this), false);
  },

  





  prettyPrintNodeAsText: function BC_prettyPrintNodeText(aNode)
  {
    let text = aNode.tagName.toLowerCase();
    if (aNode.id) {
      text += "#" + aNode.id;
    }
    for (let i = 0; i < aNode.classList.length; i++) {
      text += "." + aNode.classList[i];
    }
    return text;
  },


  








  prettyPrintNodeAsXUL: function BC_prettyPrintNodeXUL(aNode)
  {
    let fragment = this.IUI.chromeDoc.createDocumentFragment();

    let tagLabel = this.IUI.chromeDoc.createElement("label");
    tagLabel.className = "inspector-breadcrumbs-tag plain";

    let idLabel = this.IUI.chromeDoc.createElement("label");
    idLabel.className = "inspector-breadcrumbs-id plain";

    let classesLabel = this.IUI.chromeDoc.createElement("label");
    classesLabel.className = "inspector-breadcrumbs-classes plain";

    tagLabel.textContent = aNode.tagName.toLowerCase();
    idLabel.textContent = aNode.id ? ("#" + aNode.id) : "";

    let classesText = "";
    for (let i = 0; i < aNode.classList.length; i++) {
      classesText += "." + aNode.classList[i];
    }
    classesLabel.textContent = classesText;

    fragment.appendChild(tagLabel);
    fragment.appendChild(idLabel);
    fragment.appendChild(classesLabel);

    return fragment;
  },

  





  openSiblingMenu: function BC_openSiblingMenu(aButton, aNode)
  {
    let title = this.IUI.chromeDoc.createElement("menuitem");
    title.setAttribute("label",
      this.IUI.strings.GetStringFromName("breadcrumbs.siblings"));
    title.setAttribute("disabled", "true");

    let separator = this.IUI.chromeDoc.createElement("menuseparator");

    this.menu.appendChild(title);
    this.menu.appendChild(separator);

    let fragment = this.IUI.chromeDoc.createDocumentFragment();

    let nodes = aNode.parentNode.childNodes;
    for (let i = 0; i < nodes.length; i++) {
      if (nodes[i].nodeType == aNode.ELEMENT_NODE) {
        let item = this.IUI.chromeDoc.createElement("menuitem");
        let inspector = this.IUI;
        if (nodes[i] === aNode) {
          item.setAttribute("disabled", "true");
          item.setAttribute("checked", "true");
        }

        item.setAttribute("type", "radio");
        item.setAttribute("label", this.prettyPrintNodeAsText(nodes[i]));

        item.onmouseup = (function(aNode) {
          return function() {
            inspector.select(aNode, true, true);
          }
        })(nodes[i]);

        fragment.appendChild(item);
      }
    }
    this.menu.appendChild(fragment);
    this.menu.openPopup(aButton, "before_start", 0, 0, true, false);
  },

  





  handleEvent: function BC_handleEvent(aEvent)
  {
    if (aEvent.type == "mousedown") {
      

      let timer;
      let container = this.container;
      let window = this.IUI.win;

      function openMenu(aEvent) {
        cancelHold();
        let target = aEvent.originalTarget;
        if (target.tagName == "button") {
          target.onBreadcrumbsHold();
          target.setAttribute("siblings-menu-open", "true");
        }
      }

      function handleClick(aEvent) {
        cancelHold();
        let target = aEvent.originalTarget;
        if (target.tagName == "button") {
          target.onBreadcrumbsClick();
        }
      }

      function cancelHold(aEvent) {
        window.clearTimeout(timer);
        container.removeEventListener("mouseout", cancelHold, false);
        container.removeEventListener("mouseup", handleClick, false);
      }

      container.addEventListener("mouseout", cancelHold, false);
      container.addEventListener("mouseup", handleClick, false);
      timer = window.setTimeout(openMenu, 500, aEvent);
    }
  },

  


  destroy: function BC_destroy()
  {
    this.empty();
    this.container.removeEventListener("mousedown", this, true);
    this.menu.parentNode.removeChild(this.menu);
    this.container = null;
    this.nodeHierarchy = null;
  },

  


  empty: function BC_empty()
  {
    while (this.container.hasChildNodes()) {
      this.container.removeChild(this.container.firstChild);
    }
  },

  


  invalidateHierarchy: function BC_invalidateHierarchy()
  {
    this.menu.hidePopup();
    this.nodeHierarchy = [];
    this.empty();
  },

  




  setCursor: function BC_setCursor(aIdx)
  {
    
    if (this.currentIndex > -1 && this.currentIndex < this.nodeHierarchy.length) {
      this.nodeHierarchy[this.currentIndex].button.removeAttribute("checked");
    }
    if (aIdx > -1) {
      this.nodeHierarchy[aIdx].button.setAttribute("checked", "true");
    }
    this.currentIndex = aIdx;
  },

  





  indexOf: function BC_indexOf(aNode)
  {
    let i = this.nodeHierarchy.length - 1;
    for (let i = this.nodeHierarchy.length - 1; i >= 0; i--) {
      if (this.nodeHierarchy[i].node === aNode) {
        return i;
      }
    }
    return -1;
  },

  





  cutAfter: function BC_cutAfter(aIdx)
  {
    while (this.nodeHierarchy.length > (aIdx + 1)) {
      let toRemove = this.nodeHierarchy.pop();
      this.container.removeChild(toRemove.button);
    }
  },

  





  buildButton: function BC_buildButton(aNode)
  {
    let button = this.IUI.chromeDoc.createElement("button");
    let inspector = this.IUI;
    button.appendChild(this.prettyPrintNodeAsXUL(aNode));
    button.className = "inspector-breadcrumbs-button";

    button.setAttribute("tooltiptext", this.prettyPrintNodeAsText(aNode));

    button.onBreadcrumbsClick = function onBreadcrumbsClick() {
      inspector.stopInspecting();
      inspector.select(aNode, true, true);
    };

    button.onclick = (function _onBreadcrumbsRightClick(aEvent) {
      if (aEvent.button == 2) {
        this.openSiblingMenu(button, aNode);
      }
    }).bind(this);

    button.onBreadcrumbsHold = (function _onBreadcrumbsHold() {
      this.openSiblingMenu(button, aNode);
    }).bind(this);
    return button;
  },

  




  expand: function BC_expand(aNode)
  {
      let fragment = this.IUI.chromeDoc.createDocumentFragment();
      let toAppend = aNode;
      let lastButtonInserted = null;
      let originalLength = this.nodeHierarchy.length;
      let stopNode = null;
      if (originalLength > 0) {
        stopNode = this.nodeHierarchy[originalLength - 1].node;
      }
      while (toAppend && toAppend.tagName && toAppend != stopNode) {
        let button = this.buildButton(toAppend);
        fragment.insertBefore(button, lastButtonInserted);
        lastButtonInserted = button;
        this.nodeHierarchy.splice(originalLength, 0, {node: toAppend, button: button});
        toAppend = this.DOMHelpers.getParentObject(toAppend);
      }
      this.container.appendChild(fragment, this.container.firstChild);
  },

  








  getFirstHighlightableChild: function BC_getFirstHighlightableChild(aNode)
  {
    let nextChild = this.DOMHelpers.getChildObject(aNode, 0);
    let fallback = null;

    while (nextChild) {
      if (this.IUI.highlighter.isNodeHighlightable(nextChild)) {
        return nextChild;
      }
      if (!fallback && nextChild.nodeType == aNode.ELEMENT_NODE) {
        fallback = nextChild;
      }
      nextChild = this.DOMHelpers.getNextSibling(nextChild);
    }
    return fallback;
  },

  





  getCommonAncestor: function BC_getCommonAncestor(aNode)
  {
    let node = aNode;
    while (node) {
      let idx = this.indexOf(node);
      if (idx > -1) {
        return idx;
      } else {
        node = this.DOMHelpers.getParentObject(node);
      }
    }
    return -1;
  },

  



  ensureFirstChild: function BC_ensureFirstChild()
  {
    
    if (this.currentIndex == this.nodeHierarchy.length - 1) {
      let node = this.nodeHierarchy[this.currentIndex].node;
      let child = this.getFirstHighlightableChild(node);
      
      if (child) {
        
        this.expand(child);
      }
    }
  },

  


  scroll: function BC_scroll()
  {
    

    let scrollbox = this.container;
    let element = this.nodeHierarchy[this.currentIndex].button;
    scrollbox.ensureElementIsVisible(element);
  },

  


  update: function BC_update()
  {
    this.menu.hidePopup();

    let selection = this.IUI.selection;
    let idx = this.indexOf(selection);

    
    if (idx > -1) {
      
      this.setCursor(idx);
    } else {
      
      if (this.nodeHierarchy.length > 0) {
        
        
        let parent = this.DOMHelpers.getParentObject(selection);
        let idx = this.getCommonAncestor(parent);
        this.cutAfter(idx);
      }
      
      
      this.expand(selection);

      
      idx = this.indexOf(selection);
      this.setCursor(idx);
    }
    
    this.ensureFirstChild();

    
    this.scroll();
  }
}




XPCOMUtils.defineLazyGetter(InspectorUI.prototype, "strings",
  function () {
    return Services.strings.createBundle(
            "chrome://browser/locale/devtools/inspector.properties");
  });

XPCOMUtils.defineLazyGetter(this, "StyleInspector", function () {
  var obj = {};
  Cu.import("resource:///modules/devtools/StyleInspector.jsm", obj);
  return obj.StyleInspector;
});

