












































const Cc = Components.classes;
const Cu = Components.utils;
const Ci = Components.interfaces;
const Cr = Components.results;

var EXPORTED_SYMBOLS = ["InspectorUI"];

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/TreePanel.jsm");
Cu.import("resource:///modules/devtools/CssRuleView.jsm");
Cu.import("resource:///modules/highlighter.jsm");
Cu.import("resource:///modules/devtools/LayoutHelpers.jsm");


const INSPECTOR_NOTIFICATIONS = {
  
  
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

const PSEUDO_CLASSES = [":hover", ":active", ":focus"];











function InspectorUI(aWindow)
{
  this.chromeWin = aWindow;
  this.chromeDoc = aWindow.document;
  this.tabbrowser = aWindow.gBrowser;
  this.tools = {};
  this.toolEvents = {};
  this.store = new InspectorStore();
  this.INSPECTOR_NOTIFICATIONS = INSPECTOR_NOTIFICATIONS;

  
  let keysbundle = Services.strings.createBundle(
    "chrome://global/locale/keys.properties");
  let returnString = keysbundle.GetStringFromName("VK_RETURN");
  let tooltip = this.strings.formatStringFromName("inspectButton.tooltiptext",
    [returnString], 1);
  let button = this.chromeDoc.getElementById("inspector-inspect-toolbutton");
  button.setAttribute("tooltiptext", tooltip);
}

InspectorUI.prototype = {
  browser: null,
  tools: null,
  toolEvents: null,
  inspecting: false,
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

      let activePanel = this.sidebarTools[0];
      let activeId = this.store.getValue(this.winID, "activeSidebar");
      if (activeId && this.tools[activeId]) {
        activePanel = this.tools[activeId];
      }
      this.activateSidebarPanel(activePanel.id);
    }

    this.store.setValue(this.winID, "sidebarOpen", true);
    Services.prefs.setBoolPref("devtools.inspector.sidebarOpen", true);
  },

  


  _destroySidebar: function IUI_destroySidebar()
  {
    this.sidebarBox.setAttribute("hidden", "true");
    this.sidebarSplitter.setAttribute("hidden", "true");
    this.stylingButton.checked = false;
  },

  


  hideSidebar: function IUI_hideSidebar()
  {
    this._destroySidebar();
    this.store.setValue(this.winID, "sidebarOpen", false);
    Services.prefs.setBoolPref("devtools.inspector.sidebarOpen", false);
  },

  



  toggleSidebar: function IUI_toggleSidebar()
  {
    if (!this.isSidebarOpen) {
      this.showSidebar();
    } else {
      this.hideSidebar();
    }
  },

  


  activateSidebarPanel: function IUI_activateSidebarPanel(aID)
  {
    let buttonId = this.getToolbarButtonId(aID);
    this.chromeDoc.getElementById(buttonId).click();
  },

  get activeSidebarPanel()
  {
    for each (let tool in this.sidebarTools) {
      if (this.sidebarDeck.selectedPanel == this.getToolIframe(tool)) {
        return tool.id;
      }
    }
    return null;
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

  


  toggleHTMLPanel: function TP_toggle()
  {
    if (this.treePanel.isOpen()) {
      this.treePanel.close();
      Services.prefs.setBoolPref("devtools.inspector.htmlPanelOpen", false);
      this.store.setValue(this.winID, "htmlPanelOpen", false);
    } else {
      this.treePanel.open();
      Services.prefs.setBoolPref("devtools.inspector.htmlPanelOpen", true);
      this.store.setValue(this.winID, "htmlPanelOpen", true);
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

    this.initTools();
    this.chromeWin.Tilt.setup();

    this.treePanel = new TreePanel(this.chromeWin, this);

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

    this.chromeWin.addEventListener("keypress", this, false);

    
    this.highlighter = new Highlighter(this.chromeWin);

    this.setupNavigationKeys();
    this.highlighterReady();
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
      onChanged: this.changeInRuleView,
      panel: null,
      unregister: this.destroyRuleView,
      sidebar: true,
    };

    this.registerTool(this.ruleViewObject);
  },

  


  initTools: function IUI_initTools()
  {
    
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

      this.store.setValue(this.winID, "htmlPanelOpen",
        Services.prefs.getBoolPref("devtools.inspector.htmlPanelOpen"));

      this.store.setValue(this.winID, "sidebarOpen",
        Services.prefs.getBoolPref("devtools.inspector.sidebarOpen"));

      this.store.setValue(this.winID, "activeSidebar",
        Services.prefs.getCharPref("devtools.inspector.activeSidebar"));

      this.win.addEventListener("pagehide", this, true);
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

  









  closeInspectorUI: function IUI_closeInspectorUI(aKeepStore)
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

    if (!aKeepStore) {
      this.store.deleteStore(this.winID);
      this.win.removeEventListener("pagehide", this, true);
      this.clearPseudoClassLocks();
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

    this.chromeWin.removeEventListener("keypress", this, false);

    this.stopInspecting();

    this.toolsDo(function IUI_toolsHide(aTool) {
      this.unregisterTool(aTool);
    }.bind(this));

    
    this._destroySidebar();

    if (this.highlighter) {
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

    this.inspecting = true;
    this.toolsDim(true);
    this.highlighter.unlock();
  },

  





  stopInspecting: function IUI_stopInspecting(aPreventScroll)
  {
    if (!this.inspecting) {
      return;
    }

    this.inspectToolbutton.checked = false;

    this.inspecting = false;
    this.toolsDim(false);
    if (this.highlighter.getNode()) {
      this.select(this.highlighter.getNode(), true, !aPreventScroll);
    } else {
      this.select(null, true, true);
    }
    this.highlighter.lock();
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

    this.toolsSelect(aScroll);
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
    this.nodeChanged();
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
    this.toolsOnChanged(aUpdater);
  },

  
  

  highlighterReady: function IUI_highlighterReady()
  {
    
    this.initializeStore();

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

    if (this.store.getValue(this.winID, "inspecting")) {
      this.startInspecting();
      this.highlighter.unlock();
    } else {
      this.highlighter.lock();
    }

    Services.obs.notifyObservers(null, INSPECTOR_NOTIFICATIONS.STATE_RESTORED, null);

    this.win.focus();
    this.highlighter.highlight();

    if (this.store.getValue(this.winID, "htmlPanelOpen")) {
      this.treePanel.open();
    }

    if (this.store.getValue(this.winID, "sidebarOpen")) {
      this.showSidebar();
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
          this.store.deleteStore(winID);
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
      this.cssRuleViewBoundCSSLinkClicked = this.ruleViewCSSLinkClicked.bind(this);
      this.ruleView.element.addEventListener("CssRuleViewCSSLinkClicked",
                                             this.cssRuleViewBoundCSSLinkClicked);
      this.cssRuleViewBoundMouseDown = this.ruleViewMouseDown.bind(this);
      this.ruleView.element.addEventListener("mousedown",
                                             this.cssRuleViewBoundMouseDown);
      this.cssRuleViewBoundMouseUp = this.ruleViewMouseUp.bind(this);
      this.ruleView.element.addEventListener("mouseup",
                                             this.cssRuleViewBoundMouseUp);
      this.cssRuleViewBoundMouseMove = this.ruleViewMouseMove.bind(this);
      this.cssRuleViewBoundMenuUpdate = this.ruleViewMenuUpdate.bind(this);

      this.cssRuleViewBoundCopy = this.ruleViewCopy.bind(this);
      iframe.addEventListener("copy", this.cssRuleViewBoundCopy);

      this.cssRuleViewBoundCopyRule = this.ruleViewCopyRule.bind(this);
      this.cssRuleViewBoundCopyDeclaration =
        this.ruleViewCopyDeclaration.bind(this);
      this.cssRuleViewBoundCopyProperty = this.ruleViewCopyProperty.bind(this);
      this.cssRuleViewBoundCopyPropertyValue =
        this.ruleViewCopyPropertyValue.bind(this);

      
      this.ruleViewAddContextMenu();

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
  
  


  changeInRuleView: function IUI_selectInRuleView()
  {
    if (this.ruleView)
      this.ruleView.nodeChanged();
  },

  ruleViewChanged: function IUI_ruleViewChanged()
  {
    this.isDirty = true;
    this.nodeChanged(this.ruleViewObject);
  },

  






  ruleViewCSSLinkClicked: function(aEvent)
  {
    if (!this.chromeWin) {
      return;
    }

    let rule = aEvent.detail.rule;
    let styleSheet = rule.sheet;

    if (styleSheet) {
      this.chromeWin.StyleEditor.openChrome(styleSheet, rule.ruleLine);
    } else {
      let href = rule.elementStyle.element.ownerDocument.location.href;
      this.chromeWin.openUILinkIn("view-source:" + href, "window");
    }
  },

  





  ruleViewMouseDown: function IUI_ruleViewMouseDown(aEvent)
  {
    this.ruleView.element.addEventListener("mousemove",
      this.cssRuleViewBoundMouseMove);
  },

  





  ruleViewMouseUp: function IUI_ruleViewMouseUp(aEvent)
  {
    this.ruleView.element.removeEventListener("mousemove",
      this.cssRuleViewBoundMouseMove);
    this.ruleView._selectionMode = false;
  },

  





  ruleViewMouseMove: function IUI_ruleViewMouseMove(aEvent)
  {
    this.ruleView._selectionMode = true;
  },

  


  ruleViewAddContextMenu: function IUI_ruleViewAddContextMenu()
  {
    let iframe = this.getToolIframe(this.ruleViewObject);
    let popupSet = this.chromeDoc.getElementById("mainPopupSet");
    let menu = this.chromeDoc.createElement("menupopup");
    menu.addEventListener("popupshowing", this.cssRuleViewBoundMenuUpdate);
    menu.id = "rule-view-context-menu";

    
    let label = styleInspectorStrings
      .GetStringFromName("rule.contextmenu.copyselection");
    let accessKey = styleInspectorStrings
      .GetStringFromName("rule.contextmenu.copyselection.accesskey");
    let item = this.chromeDoc.createElement("menuitem");
    item.id = "rule-view-copy";
    item.setAttribute("label", label);
    item.setAttribute("accesskey", accessKey);
    item.addEventListener("command", this.cssRuleViewBoundCopy);
    menu.appendChild(item);

    
    label = styleInspectorStrings.
      GetStringFromName("rule.contextmenu.copyrule");
    accessKey = styleInspectorStrings.
      GetStringFromName("rule.contextmenu.copyrule.accesskey");
    item = this.chromeDoc.createElement("menuitem");
    item.id = "rule-view-copy-rule";
    item.setAttribute("label", label);
    item.setAttribute("accesskey", accessKey);
    item.addEventListener("command", this.cssRuleViewBoundCopyRule);
    menu.appendChild(item);

    
    label = styleInspectorStrings.
      GetStringFromName("rule.contextmenu.copydeclaration");
    accessKey = styleInspectorStrings.
      GetStringFromName("rule.contextmenu.copydeclaration.accesskey");
    item = this.chromeDoc.createElement("menuitem");
    item.id = "rule-view-copy-declaration";
    item.setAttribute("label", label);
    item.setAttribute("accesskey", accessKey);
    item.addEventListener("command", this.cssRuleViewBoundCopyDeclaration);
    menu.appendChild(item);

    
    label = styleInspectorStrings.
      GetStringFromName("rule.contextmenu.copyproperty");
    accessKey = styleInspectorStrings.
      GetStringFromName("rule.contextmenu.copyproperty.accesskey");
    item = this.chromeDoc.createElement("menuitem");
    item.id = "rule-view-copy-property";
    item.setAttribute("label", label);
    item.setAttribute("accesskey", accessKey);
    item.addEventListener("command", this.cssRuleViewBoundCopyProperty);
    menu.appendChild(item);

    
    label = styleInspectorStrings.
      GetStringFromName("rule.contextmenu.copypropertyvalue");
    accessKey = styleInspectorStrings.
      GetStringFromName("rule.contextmenu.copypropertyvalue.accesskey");
    item = this.chromeDoc.createElement("menuitem");
    item.id = "rule-view-copy-property-value";
    item.setAttribute("label", label);
    item.setAttribute("accesskey", accessKey);
    item.addEventListener("command", this.cssRuleViewBoundCopyPropertyValue);
    menu.appendChild(item);

    popupSet.appendChild(menu);

    iframe.setAttribute("context", menu.id);
  },

  





  ruleViewMenuUpdate: function IUI_ruleViewMenuUpdate(aEvent)
  {
    let iframe = this.getToolIframe(this.ruleViewObject);
    let win = iframe.contentWindow;

    
    let disable = win.getSelection().isCollapsed;
    let menuitem = this.chromeDoc.getElementById("rule-view-copy");
    menuitem.disabled = disable;

    
    let node = this.chromeDoc.popupNode;
    if (!node.classList.contains("ruleview-property") &&
        !node.classList.contains("ruleview-computed")) {
      while (node = node.parentElement) {
        if (node.classList.contains("ruleview-property") ||
          node.classList.contains("ruleview-computed")) {
          break;
        }
      }
    }
    let disablePropertyItems = !node || (node &&
      !node.classList.contains("ruleview-property") &&
      !node.classList.contains("ruleview-computed"));

    menuitem = this.chromeDoc.querySelector("#rule-view-copy-declaration");
    menuitem.disabled = disablePropertyItems;
    menuitem = this.chromeDoc.querySelector("#rule-view-copy-property");
    menuitem.disabled = disablePropertyItems;
    menuitem = this.chromeDoc.querySelector("#rule-view-copy-property-value");
    menuitem.disabled = disablePropertyItems;
  },

  




  ruleViewCopy: function IUI_ruleViewCopy(aEvent)
  {
    let iframe = this.getToolIframe(this.ruleViewObject);
    let win = iframe.contentWindow;
    let text = win.getSelection().toString();

    
    text = text.replace(/(\r?\n)\r?\n/g, "$1");

    
    let inline = styleInspectorStrings.GetStringFromName("rule.sourceInline");
    let rx = new RegExp("^" + inline + "\\r?\\n?", "g");
    text = text.replace(rx, "");

    
    text = text.replace(/[\w\.]+:\d+(\r?\n)/g, "$1");

    
    let inheritedFrom = styleInspectorStrings
      .GetStringFromName("rule.inheritedSource");
    inheritedFrom = inheritedFrom.replace(/\s%S\s\(%S\)/g, "");
    rx = new RegExp("(\r?\n)" + inheritedFrom + ".*", "g");
    text = text.replace(rx, "$1");

    clipboardHelper.copyString(text);

    if (aEvent) {
      aEvent.preventDefault();
    }
  },

  




  ruleViewCopyRule: function IUI_ruleViewCopyRule(aEvent)
  {
    let node = this.chromeDoc.popupNode;
    if (node.className != "ruleview-code") {
      if (node.className == "ruleview-rule-source") {
        node = node.nextElementSibling;
      } else {
        while (node = node.parentElement) {
          if (node.className == "ruleview-code") {
            break;
          }
        }
      }
    }

    if (node.className == "ruleview-code") {
      
      
      
      
      node = node.cloneNode();
      let computed = node.querySelector(".ruleview-computedlist");
      if (computed) {
        computed.parentNode.removeChild(computed);
      }
      let autosizer = node.querySelector(".autosizer");
      if (autosizer) {
        autosizer.parentNode.removeChild(autosizer);
      }
    }

    let text = node.textContent;

    
    if (osString == "WINNT") {
      text = text.replace(/{/g, "{\r\n    ");
      text = text.replace(/;/g, ";\r\n    ");
      text = text.replace(/\s*}/g, "\r\n}");
    } else {
      text = text.replace(/{/g, "{\n    ");
      text = text.replace(/;/g, ";\n    ");
      text = text.replace(/\s*}/g, "\n}");
    }

    clipboardHelper.copyString(text);
  },

  




  ruleViewCopyDeclaration: function IUI_ruleViewCopyDeclaration(aEvent)
  {
    let node = this.chromeDoc.popupNode;
    if (!node.classList.contains("ruleview-property") &&
        !node.classList.contains("ruleview-computed")) {
      while (node = node.parentElement) {
        if (node.classList.contains("ruleview-property") ||
            node.classList.contains("ruleview-computed")) {
          break;
        }
      }
    }

    
    
    
    
    node = node.cloneNode();
    let computed = node.querySelector(".ruleview-computedlist");
    if (computed) {
      computed.parentNode.removeChild(computed);
    }
    clipboardHelper.copyString(node.textContent);
  },

  




  ruleViewCopyProperty: function IUI_ruleViewCopyProperty(aEvent)
  {
    let node = this.chromeDoc.popupNode;

    if (!node.classList.contains("ruleview-propertyname")) {
      node = node.querySelector(".ruleview-propertyname");
    }

    if (node) {
      clipboardHelper.copyString(node.textContent);
    }
  },

  




  ruleViewCopyPropertyValue: function IUI_ruleViewCopyPropertyValue(aEvent)
  {
    let node = this.chromeDoc.popupNode;

    if (!node.classList.contains("ruleview-propertyvalue")) {
      node = node.querySelector(".ruleview-propertyvalue");
    }

    if (node) {
      clipboardHelper.copyString(node.textContent);
    }
  },

  


  destroyRuleView: function IUI_destroyRuleView()
  {
    let iframe = this.getToolIframe(this.ruleViewObject);
    iframe.removeEventListener("copy", this.cssRuleViewBoundCopy);
    iframe.parentNode.removeChild(iframe);

    if (this.ruleView) {
      let menu = this.chromeDoc.querySelector("#rule-view-context-menu");
      if (menu) {
        
        let menuitem = this.chromeDoc.querySelector("#rule-view-copy");
        menuitem.removeEventListener("command", this.cssRuleViewBoundCopy);

        
        menuitem = this.chromeDoc.querySelector("#rule-view-copy-rule");
        menuitem.removeEventListener("command", this.cssRuleViewBoundCopyRule);

        
        menuitem = this.chromeDoc.querySelector("#rule-view-copy-declaration");
        menuitem.removeEventListener("command",
                                     this.cssRuleViewBoundCopyDeclaration);

        
        menuitem = this.chromeDoc.querySelector("#rule-view-copy-property");
        menuitem.removeEventListener("command",
                                     this.cssRuleViewBoundCopyProperty);

        
        menuitem = this.chromeDoc.querySelector("#rule-view-copy-property-value");
        menuitem.removeEventListener("command",
                                     this.cssRuleViewBoundCopyPropertyValue);

        menu.removeEventListener("popupshowing", this.cssRuleViewBoundMenuUpdate);
        menu.parentNode.removeChild(menu);
      }

      this.ruleView.element.removeEventListener("CssRuleViewChanged",
                                                this.boundRuleViewChanged);
      this.ruleView.element.removeEventListener("CssRuleViewCSSLinkClicked",
                                                this.cssRuleViewBoundCSSLinkClicked);
      this.ruleView.element.removeEventListener("mousedown",
                                                this.cssRuleViewBoundMouseDown);
      this.ruleView.element.removeEventListener("mouseup",
                                                this.cssRuleViewBoundMouseUp);
      this.ruleView.element.removeEventListener("mousemove",
                                                this.cssRuleViewBoundMouseMove);
      delete boundRuleViewChanged;
      this.ruleView.clear();
      delete this.ruleView;
    }
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
    iframe.setAttribute("tooltip", "aHTMLTooltip");
    iframe.addEventListener("mousedown", iframe.focus);
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
      Services.prefs.setCharPref("devtools.inspector.activeSidebar", aTool.id);
      this.store.setValue(this.winID, "activeSidebar", aTool.id);
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

    
    let iframe = this.getToolIframe(aRegObj);
    iframe.removeEventListener("mousedown", iframe.focus);

    
    this.sidebarToolbar.removeChild(btn);

    
    
    if (aRegObj.unregister)
      aRegObj.unregister.call(aRegObj.context);

    delete this.tools[aRegObj.id];
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
      if ("dim" in aTool) {
        aTool.dim.call(aTool.context, aState);
      }
    });
  },

  






  toolsOnChanged: function IUI_toolsChanged(aUpdater)
  {
    this.toolsDo(function IUI_toolsOnChanged(aTool) {
      if (("onChanged" in aTool) && aTool != aUpdater) {
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

XPCOMUtils.defineLazyGetter(this, "StyleInspector", function () {
  var obj = {};
  Cu.import("resource:///modules/devtools/StyleInspector.jsm", obj);
  return obj.StyleInspector;
});

XPCOMUtils.defineLazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});

XPCOMUtils.defineLazyGetter(this, "clipboardHelper", function() {
  return Cc["@mozilla.org/widget/clipboardhelper;1"].
    getService(Ci.nsIClipboardHelper);
});

XPCOMUtils.defineLazyGetter(this, "styleInspectorStrings", function() {
  return Services.strings.createBundle(
    "chrome://browser/locale/devtools/styleinspector.properties");
});

XPCOMUtils.defineLazyGetter(this, "osString", function() {
  return Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).OS;
});
