














































const Cc = Components.classes;
const Cu = Components.utils;
const Ci = Components.interfaces;
const Cr = Components.results;

var EXPORTED_SYMBOLS = ["InspectorUI"];

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/TreePanel.jsm");
Cu.import("resource:///modules/highlighter.jsm");
Cu.import("resource:///modules/devtools/LayoutHelpers.jsm");


const INSPECTOR_NOTIFICATIONS = {
  
  
  OPENED: "inspector-opened",

  
  CLOSED: "inspector-closed",

  
  DESTROYED: "inspector-destroyed",

  
  STATE_RESTORED: "inspector-state-restored",

  
  TREEPANELREADY: "inspector-treepanel-ready",

  
  EDITOR_OPENED: "inspector-editor-opened",
  EDITOR_CLOSED: "inspector-editor-closed",
  EDITOR_SAVED: "inspector-editor-saved",
};

const PSEUDO_CLASSES = [":hover", ":active", ":focus"];














function Inspector(aIUI)
{
  this._IUI = aIUI;
  this._winID = aIUI.winID;
  this._listeners = {};
}

Inspector.prototype = {
  


  get locked() {
    return !this._IUI.inspecting;
  },

  


  get selection() {
    return this._IUI.selection;
  },

  




  markDirty: function Inspector_markDirty()
  {
    this._IUI.isDirty = true;
  },

  


  get chromeWindow() {
    return this._IUI.chromeWin;
  },

  







  change: function Inspector_change(aContext)
  {
    this._IUI.nodeChanged(aContext);
  },

  


  _destroy: function Inspector__destroy()
  {
    delete this._IUI;
    delete this._listeners;
  },

  
  

  







  on: function Inspector_on(aEvent, aListener)
  {
    if (!(aEvent in this._listeners)) {
      this._listeners[aEvent] = [];
    }
    this._listeners[aEvent].push(aListener);
  },

  







  once: function Inspector_once(aEvent, aListener)
  {
    let handler = function() {
      this.removeListener(aEvent, handler);
      aListener();
    }.bind(this);
    this.on(aEvent, handler);
  },

  








  removeListener: function Inspector_removeListener(aEvent, aListener)
  {
    this._listeners[aEvent] = this._listeners[aEvent].filter(function(l) aListener != l);
  },

  



  _emit: function Inspector__emit(aEvent)
  {
    if (!(aEvent in this._listeners))
      return;
    for each (let listener in this._listeners[aEvent]) {
      listener.apply(null, arguments);
    }
  }
}











function InspectorUI(aWindow)
{
  
  let tmp = {};
  Cu.import("resource:///modules/devtools/StyleInspector.jsm", tmp);

  this.chromeWin = aWindow;
  this.chromeDoc = aWindow.document;
  this.tabbrowser = aWindow.gBrowser;
  this.tools = {};
  this.toolEvents = {};
  this.store = new InspectorStore();
  this.INSPECTOR_NOTIFICATIONS = INSPECTOR_NOTIFICATIONS;
  this.buildButtonsTooltip();
}

InspectorUI.prototype = {
  browser: null,
  tools: null,
  toolEvents: null,
  inspecting: false,
  ruleViewEnabled: true,
  isDirty: false,
  store: null,

  _currentInspector: null,
  _sidebar: null,

  


  get currentInspector() this._currentInspector,

  


  get sidebar() this._sidebar,

  





  toggleInspectorUI: function IUI_toggleInspectorUI(aEvent)
  {
    if (this.isInspectorOpen) {
      this.closeInspectorUI();
    } else {
      this.openInspectorUI();
    }
  },

  



  buildButtonsTooltip: function IUI_buildButtonsTooltip()
  {
    let keysbundle = Services.strings.createBundle("chrome://global-platform/locale/platformKeys.properties");

    

    let key = this.chromeDoc.getElementById("key_inspect");

    let modifiersAttr = key.getAttribute("modifiers");

    let combo = [];

    if (modifiersAttr.match("accel"))
#ifdef XP_MACOSX
      combo.push(keysbundle.GetStringFromName("VK_META"));
#else
      combo.push(keysbundle.GetStringFromName("VK_CONTROL"));
#endif
    if (modifiersAttr.match("shift"))
      combo.push(keysbundle.GetStringFromName("VK_SHIFT"));
    if (modifiersAttr.match("alt"))
      combo.push(keysbundle.GetStringFromName("VK_ALT"));
    if (modifiersAttr.match("ctrl"))
      combo.push(keysbundle.GetStringFromName("VK_CONTROL"));
    if (modifiersAttr.match("meta"))
      combo.push(keysbundle.GetStringFromName("VK_META"));

    combo.push(key.getAttribute("key"));

    let separator = keysbundle.GetStringFromName("MODIFIER_SEPARATOR");

    let tooltip = this.strings.formatStringFromName("inspectButton.tooltiptext",
      [combo.join(separator)], 1);
    let button = this.chromeDoc.getElementById("inspector-inspect-toolbutton");
    button.setAttribute("tooltiptext", tooltip);

    

    button = this.chromeDoc.getElementById("inspector-treepanel-toolbutton");
#ifdef XP_MACOSX
    
    tooltip = this.strings.GetStringFromName("markupButton.tooltip");
#else
    let altString = keysbundle.GetStringFromName("VK_ALT");
    let accesskey = button.getAttribute("accesskey");
    let separator = keysbundle.GetStringFromName("MODIFIER_SEPARATOR");
    let shortcut = altString + separator + accesskey;
    tooltip = this.strings.formatStringFromName("markupButton.tooltipWithAccesskey",
      [shortcut], 1);
#endif
    button.setAttribute("tooltiptext", tooltip);

  },

  



  toggleInspection: function IUI_toggleInspection()
  {
    if (!this.isInspectorOpen) {
      this.openInspectorUI();
      return;
    }

    if (this.inspecting) {
      this.stopInspecting();
    } else {
      this.startInspecting();
    }
  },

  



  toggleSidebar: function IUI_toggleSidebar()
  {
    if (!this.isSidebarOpen) {
      this.showSidebar();
    } else {
      this.hideSidebar();
    }
  },

  


  toggleHTMLPanel: function TP_toggle()
  {
    if (this.treePanel.isOpen()) {
      this.treePanel.close();
      Services.prefs.setBoolPref("devtools.inspector.htmlPanelOpen", false);
      this.currentInspector._htmlPanelOpen = false;
    } else {
      this.treePanel.open();
      Services.prefs.setBoolPref("devtools.inspector.htmlPanelOpen", true);
      this.currentInspector._htmlPanelOpen = true;
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
    
    if (this.isInspectorOpen) {
      if (aNode) {
        this.inspectNode(aNode);
        this.stopInspecting();
      }
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

    this.chromeWin.Tilt.setup();

    this.treePanel = new TreePanel(this.chromeWin, this);
    this.toolbar.hidden = false;
    this.inspectMenuitem.setAttribute("checked", true);

    
    this.breadcrumbs = new HTMLBreadcrumbs(this);

    this.isDirty = false;

    this.progressListener = new InspectorProgressListener(this);

    this.chromeWin.addEventListener("keypress", this, false);

    
    this.highlighter = new Highlighter(this.chromeWin);

    this.initializeStore();

    this._sidebar = new InspectorStyleSidebar({
      document: this.chromeDoc,
      inspector: this._currentInspector,
    });

    
    
    for each (let tool in InspectorUI._registeredSidebars) {
      this._sidebar.addTool(tool);
    }

    this.setupNavigationKeys();
    this.highlighterReady();

    
    this.chromeDoc.commandDispatcher.advanceFocusIntoSubtree(this.toolbar);

    
    
    
    
    if (!this.toolbar.querySelector("-moz-focusring")) {
      this.win.focus();
    }

  },

  


  initializeStore: function IUI_initializeStore()
  {
    
    if (this.store.isEmpty()) {
      this.tabbrowser.tabContainer.addEventListener("TabSelect", this, false);
    }

    
    if (this.store.hasID(this.winID)) {
      this._currentInspector = this.store.getInspector(this.winID);
      let selectedNode = this.currentInspector._selectedNode;
      if (selectedNode) {
        this.inspectNode(selectedNode);
      }
      this.isDirty = this.currentInspector._isDirty;
    } else {
      
      let inspector = new Inspector(this);
      this.store.addInspector(this.winID, inspector);
      inspector._selectedNode = null;
      inspector._inspecting = true;
      inspector._isDirty = this.isDirty;

      inspector._htmlPanelOpen =
        Services.prefs.getBoolPref("devtools.inspector.htmlPanelOpen");

      inspector._sidebarOpen =
        Services.prefs.getBoolPref("devtools.inspector.sidebarOpen");

      inspector._activeSidebar =
        Services.prefs.getCharPref("devtools.inspector.activeSidebar");

      this.win.addEventListener("pagehide", this, true);

      this._currentInspector = inspector;
    }
  },

  



   setupNavigationKeys: function IUI_setupNavigationKeys()
   {
     
     
     
     

     this.onKeypress = this.onKeypress.bind(this);

     this.highlighter.highlighterContainer.addEventListener("keypress",
       this.onKeypress, true);
     this.win.addEventListener("keypress", this.onKeypress, true);
     this.toolbar.addEventListener("keypress", this.onKeypress, true);
   },

  


   removeNavigationKeys: function IUI_removeNavigationKeys()
   {
      this.highlighter.highlighterContainer.removeEventListener("keypress",
        this.onKeypress, true);
      this.win.removeEventListener("keypress", this.onKeypress, true);
      this.toolbar.removeEventListener("keypress", this.onKeypress, true);
   },

  









  closeInspectorUI: function IUI_closeInspectorUI(aKeepInspector)
  {
    
    
    if (this.treePanel && this.treePanel.editingContext)
      this.treePanel.closeEditor();

    this.treePanel.destroy();

    if (this.closing || !this.win || !this.browser) {
      return;
    }

    let winId = new String(this.winID); 

    this.closing = true;
    this.toolbar.hidden = true;

    this.removeNavigationKeys();

    this.progressListener.destroy();
    delete this.progressListener;

    if (!aKeepInspector) {
      this.win.removeEventListener("pagehide", this, true);
      this.clearPseudoClassLocks();
    } else {
      
      if (this.selection) {
        this.currentInspector._selectedNode = this.selection;
      }
      this.currentInspector._inspecting = this.inspecting;
      this.currentInspector._isDirty = this.isDirty;
    }

    if (this.store.isEmpty()) {
      this.tabbrowser.tabContainer.removeEventListener("TabSelect", this, false);
    }

    this.chromeWin.removeEventListener("keypress", this, false);

    this.stopInspecting();

    
    if (this._sidebar) {
      this._sidebar.destroy();
      this._sidebar = null;
    }

    if (this.highlighter) {
      this.highlighter.destroy();
      this.highlighter = null;
    }

    if (this.breadcrumbs) {
      this.breadcrumbs.destroy();
      this.breadcrumbs = null;
    }

    delete this._currentInspector;
    if (!aKeepInspector)
      this.store.deleteInspector(this.winID);

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

    if (!aKeepInspector)
      Services.obs.notifyObservers(null, INSPECTOR_NOTIFICATIONS.DESTROYED, winId);
  },

  



  startInspecting: function IUI_startInspecting()
  {
    
    
    if (this.treePanel && this.treePanel.editingContext)
      this.treePanel.closeEditor();

    this.inspectToolbutton.checked = true;

    this.inspecting = true;
    this.highlighter.unlock();
    this._notifySelected();
    this._currentInspector._emit("unlocked");
  },

  _notifySelected: function IUI__notifySelected(aFrom)
  {
    this._currentInspector._emit("select", aFrom);
  },

  





  stopInspecting: function IUI_stopInspecting(aPreventScroll)
  {
    if (!this.inspecting) {
      return;
    }

    this.inspectToolbutton.checked = false;

    this.inspecting = false;
    if (this.highlighter.getNode()) {
      this.select(this.highlighter.getNode(), true, !aPreventScroll);
    } else {
      this.select(null, true, true);
    }

    this.highlighter.lock();
    this._notifySelected();
    this._currentInspector._emit("locked");
  },

  










  select: function IUI_select(aNode, forceUpdate, aScroll, aFrom)
  {
    
    
    if (this.treePanel && this.treePanel.editingContext)
      this.treePanel.closeEditor();

    if (!aNode)
      aNode = this.defaultSelection;

    if (forceUpdate || aNode != this.selection) {
      if (aFrom != "breadcrumbs") {
        this.clearPseudoClassLocks();
      }

      this.selection = aNode;
      if (!this.inspecting) {
        this.highlighter.highlight(this.selection);
      }
    }

    this.breadcrumbs.update();
    this.chromeWin.Tilt.update(aNode);
    this.treePanel.select(aNode, aScroll);

    this._notifySelected(aFrom);
  },

  







  togglePseudoClassLock: function IUI_togglePseudoClassLock(aPseudo)
  {
    if (DOMUtils.hasPseudoClassLock(this.selection, aPseudo)) {
      this.breadcrumbs.nodeHierarchy.forEach(function(crumb) {
        DOMUtils.removePseudoClassLock(crumb.node, aPseudo);
      });
    } else {
      let hierarchical = aPseudo == ":hover" || aPseudo == ":active";
      let node = this.selection;
      do {
        DOMUtils.addPseudoClassLock(node, aPseudo);
        node = node.parentNode;
      } while (hierarchical && node.parentNode)
    }
    this.nodeChanged("pseudoclass");
  },

  


  clearPseudoClassLocks: function IUI_clearPseudoClassLocks()
  {
    this.breadcrumbs.nodeHierarchy.forEach(function(crumb) {
      DOMUtils.clearPseudoClassLocks(crumb.node);
    });
  },

  






  nodeChanged: function IUI_nodeChanged(aUpdater)
  {
    this.highlighter.invalidateSize();
    this.breadcrumbs.updateSelectors();
    this._currentInspector._emit("change", aUpdater);
  },

  
  

  highlighterReady: function IUI_highlighterReady()
  {
    let self = this;

    this.highlighter.addListener("locked", function() {
      self.stopInspecting();
    });

    this.highlighter.addListener("unlocked", function() {
      self.startInspecting();
    });

    this.highlighter.addListener("nodeselected", function() {
      self.select(self.highlighter.getNode(), false, false);
    });

    this.highlighter.addListener("pseudoclasstoggled", function(aPseudo) {
      self.togglePseudoClassLock(aPseudo);
    });

    if (this.currentInspector._inspecting) {
      this.startInspecting();
      this.highlighter.unlock();
    } else {
      this.highlighter.lock();
    }

    Services.obs.notifyObservers(null, INSPECTOR_NOTIFICATIONS.STATE_RESTORED, null);

    this.highlighter.highlight();

    if (this.currentInspector._htmlPanelOpen) {
      this.treePanel.open();
    }

    if (this.currentInspector._sidebarOpen) {
      this._sidebar.show();
    }

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
      case "keypress":
        switch (event.keyCode) {
          case this.chromeWin.KeyEvent.DOM_VK_ESCAPE:
            this.closeInspectorUI(false);
            event.preventDefault();
            event.stopPropagation();
            break;
      }
      case "pagehide":
        win = event.originalTarget.defaultView;
        
        if (!win || win.frameElement || win.top != win) {
          break;
        }

        win.removeEventListener(event.type, this, true);

        winID = this.getWindowID(win);
        if (winID && winID != this.winID) {
          this.store.deleteInspector(winID);
        }

        if (this.store.isEmpty()) {
          this.tabbrowser.tabContainer.removeEventListener("TabSelect", this,
                                                         false);
        }
        break;
    }
  },

  


  onKeypress: function IUI_onKeypress(event)
  {
    let node = null;
    let bc = this.breadcrumbs;
    switch (event.keyCode) {
      case this.chromeWin.KeyEvent.DOM_VK_LEFT:
        if (bc.currentIndex != 0)
          node = bc.nodeHierarchy[bc.currentIndex - 1].node;
        if (node && this.highlighter.isNodeHighlightable(node))
          this.highlighter.highlight(node);
        event.preventDefault();
        event.stopPropagation();
        break;
      case this.chromeWin.KeyEvent.DOM_VK_RIGHT:
        if (bc.currentIndex < bc.nodeHierarchy.length - 1)
          node = bc.nodeHierarchy[bc.currentIndex + 1].node;
        if (node && this.highlighter.isNodeHighlightable(node)) {
          this.highlighter.highlight(node);
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
        }
        if (node && this.highlighter.isNodeHighlightable(node)) {
          this.highlighter.highlight(node, true);
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
        }
        if (node && this.highlighter.isNodeHighlightable(node)) {
          this.highlighter.highlight(node, true);
        }
        event.preventDefault();
        event.stopPropagation();
        break;
    }
  },

  



  copyInnerHTML: function IUI_copyInnerHTML()
  {
    clipboardHelper.copyString(this.selection.innerHTML);
  },

  



  copyOuterHTML: function IUI_copyOuterHTML()
  {
    clipboardHelper.copyString(this.selection.outerHTML);
  },

  


  deleteNode: function IUI_deleteNode()
  {
    let selection = this.selection;
    let parent = this.selection.parentNode;

    
    if (this.treePanel.isOpen())
      this.treePanel.deleteChildBox(selection);

    
    parent.removeChild(selection);
    this.breadcrumbs.invalidateHierarchy();

    
    this.inspectNode(parent);
  },

  
  

  








  inspectNode: function IUI_inspectNode(aNode, aScroll)
  {
    this.select(aNode, true, true);
    this.highlighter.highlight(aNode, aScroll);
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

  







  addInspector: function IS_addInspector(aID, aInspector)
  {
    let result = false;

    if (!(aID in this.store)) {
      this.store[aID] = aInspector;
      this.length++;
      result = true;
    }

    return result;
  },

  




  getInspector: function IS_getInspector(aID)
  {
    return this.store[aID] || null;
  },

  






  deleteInspector: function IS_deleteInspector(aID)
  {
    let result = false;

    if (aID in this.store) {
      this.store[aID]._destroy();
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

InspectorUI._registeredSidebars = [];



















InspectorUI.registerSidebar = function IUI_registerSidebar(aRegistration)
{
  
  if (InspectorUI._registeredSidebars.some(function(elt) elt.id == aRegistration.id))
    return false;

  InspectorUI._registeredSidebars.push(aRegistration);

  return true;
}







InspectorUI.unregisterSidebar = function IUI_unregisterSidebar(aID)
{
  InspectorUI._registeredSidebars = InspectorUI._registeredSidebars.filter(function(aReg) aReg.id != aID);
}












function InspectorStyleSidebar(aOptions)
{
  this._tools = {};
  this._chromeDoc = aOptions.document;
  this._inspector = aOptions.inspector;
}

InspectorStyleSidebar.prototype = {

  get visible() !this._box.hasAttribute("hidden"),
  get activePanel() this._deck.selectedPanel._toolID,

  destroy: function ISS_destroy()
  {
    for each (let toolID in Object.getOwnPropertyNames(this._tools)) {
      this.removeTool(toolID);
    }
    delete this._tools;
    this._teardown();
  },

  





  addTool: function ISS_addTool(aRegObj)
  {
    if (aRegObj.id in this._tools) {
      return;
    }

    let btn = this._chromeDoc.createElement("toolbarbutton");
    btn.setAttribute("label", aRegObj.label);
    btn.setAttribute("class", "devtools-toolbarbutton");
    btn.setAttribute("tooltiptext", aRegObj.tooltiptext);
    btn.setAttribute("accesskey", aRegObj.accesskey);
    btn.setAttribute("image", aRegObj.icon || "");
    btn.setAttribute("type", "radio");
    btn.setAttribute("group", "sidebar-tools");
    this._toolbar.appendChild(btn);

    
    let frame = this._chromeDoc.createElement("iframe");
    frame.setAttribute("flex", "1");
    frame._toolID = aRegObj.id;
    this._deck.appendChild(frame);

    
    let onClick = function() {
      this.activatePanel(aRegObj.id);
    }.bind(this);
    btn.addEventListener("click", onClick, true);

    this._tools[aRegObj.id] = {
      id: aRegObj.id,
      registration: aRegObj,
      button: btn,
      frame: frame,
      loaded: false,
      context: null,
      onClick: onClick
    };
  },

  





  removeTool: function ISS_removeTool(aID)
  {
    if (!aID in this._tools) {
      return;
    }
    let tool = this._tools[aID];
    delete this._tools[aID];

    if (tool.loaded && tool.registration.destroy) {
      tool.registration.destroy(tool.context);
    }

    if (tool.onLoad) {
      tool.frame.removeEventListener("load", tool.onLoad, true);
      delete tool.onLoad;
    }

    if (tool.onClick) {
      tool.button.removeEventListener("click", tool.onClick, true);
      delete tool.onClick;
    }

    tool.button.parentNode.removeChild(tool.button);
    tool.frame.parentNode.removeChild(tool.frame);
  },

  


  toggle: function ISS_toggle()
  {
    if (!this.visible) {
      this.show();
    } else {
      this.hide();
    }
  },

  


  show: function ISS_show()
  {
    this._box.removeAttribute("hidden");
    this._splitter.removeAttribute("hidden");
    this._toggleButton.checked = true;

    this._showDefault();

    this._inspector._sidebarOpen = true;
    Services.prefs.setBoolPref("devtools.inspector.sidebarOpen", true);
  },

  


  hide: function ISS_hide()
  {
    this._teardown();
    this._inspector._sidebarOpen = false;
    Services.prefs.setBoolPref("devtools.inspector.sidebarOpen", false);
  },

  


  _teardown: function ISS__teardown()
  {
    this._toggleButton.checked = false;
    this._box.setAttribute("hidden", true);
    this._splitter.setAttribute("hidden", true);
  },

  





  activatePanel: function ISS_activatePanel(aID) {
    let tool = this._tools[aID];
    Services.prefs.setCharPref("devtools.inspector.activeSidebar", aID);
    this._inspector._activeSidebar = aID;
    this._deck.selectedPanel = tool.frame;
    this._showContent(tool);
    tool.button.setAttribute("checked", "true");
    let hasSelected = Array.forEach(this._toolbar.children, function(btn) {
      if (btn != tool.button) {
        btn.removeAttribute("checked");
      }
    });
  },

  







  _showContent: function ISS__showContent(aTool)
  {
    
    
    if (aTool.loaded) {
      this._inspector._emit("sidebaractivated", aTool.id);
      this._inspector._emit("sidebaractivated-" + aTool.id);
      return;
    }

    
    if (aTool.onLoad) {
      return;
    }

    
    aTool.onLoad = function(evt) {
      if (evt.target.location != aTool.registration.contentURL) {
        return;
      }
      aTool.frame.removeEventListener("load", aTool.onLoad, true);
      delete aTool.onLoad;
      aTool.loaded = true;
      aTool.context = aTool.registration.load(this._inspector, aTool.frame);

      this._inspector._emit("sidebaractivated", aTool.id);
      this._inspector._emit("sidebaractivated-" + aTool.id);
    }.bind(this);
    aTool.frame.addEventListener("load", aTool.onLoad, true);
    aTool.frame.setAttribute("src", aTool.registration.contentURL);
  },

  




  _toolContext: function ISS__toolContext(aID) {
    return aID in this._tools ? this._tools[aID].context : null;
  },

  



  _toolObjects: function ISS__toolObjects() {
    return [this._tools[i] for each (i in Object.getOwnPropertyNames(this._tools))];
  },

  



  _showDefault: function ISS__showDefault()
  {
    let hasSelected = Array.some(this._toolbar.children,
      function(btn) btn.hasAttribute("checked"));

    
    this._showContent(this._tools[this.activePanel]);

    if (hasSelected) {
      return;
    }

    let activeID = this._inspector._activeSidebar;
    if (!activeID || !(activeID in this._tools)) {
      activeID = Object.getOwnPropertyNames(this._tools)[0];
    }
    this.activatePanel(activeID);
  },

  
  get _toggleButton() this._chromeDoc.getElementById("inspector-style-button"),
  get _box() this._chromeDoc.getElementById("devtools-sidebar-box"),
  get _splitter() this._chromeDoc.getElementById("devtools-side-splitter"),
  get _toolbar() this._chromeDoc.getElementById("devtools-sidebar-toolbar"),
  get _deck() this._chromeDoc.getElementById("devtools-sidebar-deck"),
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

    
    
    this.container.removeAttribute("overflows");
    this.container._scrollButtonUp.collapsed = true;
    this.container._scrollButtonDown.collapsed = true;

    this.onscrollboxreflow = function() {
      if (this.container._scrollButtonDown.collapsed)
        this.container.removeAttribute("overflows");
      else
        this.container.setAttribute("overflows", true);
    }.bind(this);

    this.container.addEventListener("underflow", this.onscrollboxreflow, false);
    this.container.addEventListener("overflow", this.onscrollboxreflow, false);

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
    for (let i = 0; i < PSEUDO_CLASSES.length; i++) {
      let pseudo = PSEUDO_CLASSES[i];
      if (DOMUtils.hasPseudoClassLock(aNode, pseudo)) {
        text += pseudo;  
      }      
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
    
    let pseudosLabel = this.IUI.chromeDoc.createElement("label");
    pseudosLabel.className = "inspector-breadcrumbs-pseudo-classes plain";

    tagLabel.textContent = aNode.tagName.toLowerCase();
    idLabel.textContent = aNode.id ? ("#" + aNode.id) : "";

    let classesText = "";
    for (let i = 0; i < aNode.classList.length; i++) {
      classesText += "." + aNode.classList[i];
    }
    classesLabel.textContent = classesText;

    let pseudos = PSEUDO_CLASSES.filter(function(pseudo) {
      return DOMUtils.hasPseudoClassLock(aNode, pseudo);
    }, this);
    pseudosLabel.textContent = pseudos.join("");

    fragment.appendChild(tagLabel);
    fragment.appendChild(idLabel);
    fragment.appendChild(classesLabel);
    fragment.appendChild(pseudosLabel);

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
            inspector.select(aNode, true, true, "breadcrumbs");
          }
        })(nodes[i]);

        fragment.appendChild(item);
      }
    }
    this.menu.appendChild(fragment);
    this.menu.openPopup(aButton, "before_start", 0, 0, true, false);
    aButton.setAttribute("siblings-menu-open", "true");
  },

  





  handleEvent: function BC_handleEvent(aEvent)
  {
    if (aEvent.type == "mousedown" && aEvent.button == 0) {
      

      let timer;
      let container = this.container;
      let window = this.IUI.win;

      function openMenu(aEvent) {
        cancelHold();
        let target = aEvent.originalTarget;
        if (target.tagName == "button") {
          target.onBreadcrumbsHold();
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
    this.container.removeEventListener("underflow", this.onscrollboxreflow, false);
    this.container.removeEventListener("overflow", this.onscrollboxreflow, false);
    this.onscrollboxreflow = null;

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
      if (this.hadFocus)
        this.nodeHierarchy[aIdx].button.focus();
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

    button.onkeypress = function onBreadcrumbsKeypress(e) {
      if (e.charCode == Ci.nsIDOMKeyEvent.DOM_VK_SPACE ||
          e.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_RETURN)
        button.click();
    }

    button.onBreadcrumbsClick = function onBreadcrumbsClick() {
      inspector.stopInspecting();
      inspector.select(aNode, true, true, "breadcrumbs");
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
  
  updateSelectors: function BC_updateSelectors()
  {
    for (let i = this.nodeHierarchy.length - 1; i >= 0; i--) {
      let crumb = this.nodeHierarchy[i];
      let button = crumb.button;

      while(button.hasChildNodes()) {
        button.removeChild(button.firstChild);
      }
      button.appendChild(this.prettyPrintNodeAsXUL(crumb.node));
      button.setAttribute("tooltiptext", this.prettyPrintNodeAsText(crumb.node));
    }
  },

  


  update: function BC_update()
  {
    this.menu.hidePopup();

    let cmdDispatcher = this.IUI.chromeDoc.commandDispatcher;
    this.hadFocus = (cmdDispatcher.focusedElement &&
                     cmdDispatcher.focusedElement.parentNode == this.container);

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

    this.updateSelectors();
  },

}




XPCOMUtils.defineLazyGetter(InspectorUI.prototype, "strings",
  function () {
    return Services.strings.createBundle(
            "chrome://browser/locale/devtools/inspector.properties");
  });

XPCOMUtils.defineLazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});

XPCOMUtils.defineLazyGetter(this, "clipboardHelper", function() {
  return Cc["@mozilla.org/widget/clipboardhelper;1"].
    getService(Ci.nsIClipboardHelper);
});
