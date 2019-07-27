





const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource://gre/modules/Services.jsm");

let promise = require("devtools/toolkit/deprecated-sync-thenables");
let EventEmitter = require("devtools/toolkit/event-emitter");
let {CssLogic} = require("devtools/styleinspector/css-logic");
let clipboard = require("sdk/clipboard");

loader.lazyGetter(this, "MarkupView", () => require("devtools/markupview/markup-view").MarkupView);
loader.lazyGetter(this, "HTMLBreadcrumbs", () => require("devtools/inspector/breadcrumbs").HTMLBreadcrumbs);
loader.lazyGetter(this, "ToolSidebar", () => require("devtools/framework/sidebar").ToolSidebar);
loader.lazyGetter(this, "SelectorSearch", () => require("devtools/inspector/selector-search").SelectorSearch);

const LAYOUT_CHANGE_TIMER = 250;

































function InspectorPanel(iframeWindow, toolbox) {
  this._toolbox = toolbox;
  this._target = toolbox._target;
  this.panelDoc = iframeWindow.document;
  this.panelWin = iframeWindow;
  this.panelWin.inspector = this;
  this._inspector = null;

  this._onBeforeNavigate = this._onBeforeNavigate.bind(this);
  this._target.on("will-navigate", this._onBeforeNavigate);

  EventEmitter.decorate(this);
}

exports.InspectorPanel = InspectorPanel;

InspectorPanel.prototype = {
  


  open: function InspectorPanel_open() {
    return this.target.makeRemote().then(() => {
      return this._getPageStyle();
    }).then(() => {
      return this._getDefaultNodeForSelection();
    }).then(defaultSelection => {
      return this._deferredOpen(defaultSelection);
    }).then(null, console.error);
  },

  get toolbox() {
    return this._toolbox;
  },

  get inspector() {
    return this._toolbox.inspector;
  },

  get walker() {
    return this._toolbox.walker;
  },

  get selection() {
    return this._toolbox.selection;
  },

  get isOuterHTMLEditable() {
    return this._target.client.traits.editOuterHTML;
  },

  get hasUrlToImageDataResolver() {
    return this._target.client.traits.urlToImageDataResolver;
  },

  _deferredOpen: function(defaultSelection) {
    let deferred = promise.defer();

    this.onNewRoot = this.onNewRoot.bind(this);
    this.walker.on("new-root", this.onNewRoot);

    this.nodemenu = this.panelDoc.getElementById("inspector-node-popup");
    this.lastNodemenuItem = this.nodemenu.lastChild;
    this._setupNodeMenu = this._setupNodeMenu.bind(this);
    this._resetNodeMenu = this._resetNodeMenu.bind(this);
    this.nodemenu.addEventListener("popupshowing", this._setupNodeMenu, true);
    this.nodemenu.addEventListener("popuphiding", this._resetNodeMenu, true);

    this.onNewSelection = this.onNewSelection.bind(this);
    this.selection.on("new-node-front", this.onNewSelection);
    this.onBeforeNewSelection = this.onBeforeNewSelection.bind(this);
    this.selection.on("before-new-node-front", this.onBeforeNewSelection);
    this.onDetached = this.onDetached.bind(this);
    this.selection.on("detached-front", this.onDetached);

    this.breadcrumbs = new HTMLBreadcrumbs(this);

    if (this.target.isLocalTab) {
      this.browser = this.target.tab.linkedBrowser;
      this.scheduleLayoutChange = this.scheduleLayoutChange.bind(this);
      this.browser.addEventListener("resize", this.scheduleLayoutChange, true);

      
      
      
      this.updateDebuggerPausedWarning = () => {
        let notificationBox = this._toolbox.getNotificationBox();
        let notification = notificationBox.getNotificationWithValue("inspector-script-paused");
        if (!notification && this._toolbox.currentToolId == "inspector" &&
            this.target.isThreadPaused) {
          let message = this.strings.GetStringFromName("debuggerPausedWarning.message");
          notificationBox.appendNotification(message,
            "inspector-script-paused", "", notificationBox.PRIORITY_WARNING_HIGH);
        }

        if (notification && this._toolbox.currentToolId != "inspector") {
          notificationBox.removeNotification(notification);
        }

        if (notification && !this.target.isThreadPaused) {
          notificationBox.removeNotification(notification);
        }

      };
      this.target.on("thread-paused", this.updateDebuggerPausedWarning);
      this.target.on("thread-resumed", this.updateDebuggerPausedWarning);
      this._toolbox.on("select", this.updateDebuggerPausedWarning);
      this.updateDebuggerPausedWarning();
    }

    this._initMarkup();
    this.isReady = false;

    this.once("markuploaded", () => {
      this.isReady = true;

      
      this.selection.setNodeFront(defaultSelection, "inspector-open");

      this.markup.expandNode(this.selection.nodeFront);

      this.emit("ready");
      deferred.resolve(this);
    });

    this.setupSearchBox();
    this.setupSidebar();

    return deferred.promise;
  },

  _onBeforeNavigate: function() {
    this._defaultNode = null;
    this.selection.setNodeFront(null);
    this._destroyMarkup();
    this.isDirty = false;
    this._pendingSelection = null;
  },

  _getPageStyle: function() {
    return this._toolbox.inspector.getPageStyle().then(pageStyle => {
      this.pageStyle = pageStyle;
    });
  },

  


  _getDefaultNodeForSelection: function() {
    if (this._defaultNode) {
      return this._defaultNode;
    }
    let walker = this.walker;
    let rootNode = null;
    let pendingSelection = this._pendingSelection;

    
    
    let hasNavigated = () => pendingSelection !== this._pendingSelection;

    
    
    return walker.getRootNode().then(aRootNode => {
      if (hasNavigated()) {
        return promise.reject("navigated; resolution of _defaultNode aborted");
      }

      rootNode = aRootNode;
      if (this.selectionCssSelector) {
        return walker.querySelector(rootNode, this.selectionCssSelector);
      }
    }).then(front => {
      if (hasNavigated()) {
        return promise.reject("navigated; resolution of _defaultNode aborted");
      }

      if (front) {
        return front;
      }
      return walker.querySelector(rootNode, "body");
    }).then(front => {
      if (hasNavigated()) {
        return promise.reject("navigated; resolution of _defaultNode aborted");
      }

      if (front) {
        return front;
      }
      return this.walker.documentElement(this.walker.rootNode);
    }).then(node => {
      if (walker !== this.walker) {
        promise.reject(null);
      }
      this._defaultNode = node;
      return node;
    });
  },

  


  get target() {
    return this._target;
  },

  


  set target(value) {
    this._target = value;
  },

  


  get viewSourceUtils() {
    return this.panelWin.gViewSourceUtils;
  },

  




  markDirty: function InspectorPanel_markDirty() {
    this.isDirty = true;
  },

  


  setupSearchBox: function InspectorPanel_setupSearchBox() {
    
    if (this.searchSuggestions) {
      this.searchSuggestions.destroy();
      this.searchSuggestions = null;
    }
    this.searchBox = this.panelDoc.getElementById("inspector-searchbox");
    this.searchSuggestions = new SelectorSearch(this, this.searchBox);
  },

  


  setupSidebar: function InspectorPanel_setupSidebar() {
    let tabbox = this.panelDoc.querySelector("#inspector-sidebar");
    this.sidebar = new ToolSidebar(tabbox, this, "inspector");

    let defaultTab = Services.prefs.getCharPref("devtools.inspector.activeSidebar");

    this._setDefaultSidebar = (event, toolId) => {
      Services.prefs.setCharPref("devtools.inspector.activeSidebar", toolId);
    };

    this.sidebar.on("select", this._setDefaultSidebar);

    this.sidebar.addTab("ruleview",
                        "chrome://browser/content/devtools/cssruleview.xhtml",
                        "ruleview" == defaultTab);

    this.sidebar.addTab("computedview",
                        "chrome://browser/content/devtools/computedview.xhtml",
                        "computedview" == defaultTab);

    if (Services.prefs.getBoolPref("devtools.fontinspector.enabled") && !this.target.isRemote) {
      this.sidebar.addTab("fontinspector",
                          "chrome://browser/content/devtools/fontinspector/font-inspector.xhtml",
                          "fontinspector" == defaultTab);
    }

    this.sidebar.addTab("layoutview",
                        "chrome://browser/content/devtools/layoutview/view.xhtml",
                        "layoutview" == defaultTab);

    let ruleViewTab = this.sidebar.getTab("ruleview");

    this.sidebar.show();
  },

  


  onNewRoot: function InspectorPanel_onNewRoot() {
    this._defaultNode = null;
    this.selection.setNodeFront(null);
    this._destroyMarkup();
    this.isDirty = false;

    let onNodeSelected = defaultNode => {
      
      
      if (this._pendingSelection != onNodeSelected) {
        return;
      }
      this._pendingSelection = null;
      this.selection.setNodeFront(defaultNode, "navigateaway");

      this._initMarkup();
      this.once("markuploaded", () => {
        if (!this.markup) {
          return;
        }
        this.markup.expandNode(this.selection.nodeFront);
        this.setupSearchBox();
        this.emit("new-root");
      });
    };
    this._pendingSelection = onNodeSelected;
    this._getDefaultNodeForSelection().then(onNodeSelected, console.error);
  },

  _selectionCssSelector: null,

  




  set selectionCssSelector(cssSelector = null) {
    this._selectionCssSelector = {
      selector: cssSelector,
      url: this._target.url
    };
  },

  



  get selectionCssSelector() {
    if (this._selectionCssSelector &&
        this._selectionCssSelector.url === this._target.url) {
      return this._selectionCssSelector.selector;
    } else {
      return null;
    }
  },

  


  onNewSelection: function InspectorPanel_onNewSelection(event, value, reason) {
    if (reason === "selection-destroy") {
      return;
    }

    this.cancelLayoutChange();

    
    
    let selection = this.selection.nodeFront;

    
    
    if (reason !== "navigateaway" &&
        this.selection.node &&
        this.selection.isElementNode()) {
      this.selectionCssSelector = CssLogic.findCssSelector(this.selection.node);
    }

    let selfUpdate = this.updating("inspector-panel");
    Services.tm.mainThread.dispatch(() => {
      try {
        selfUpdate(selection);
      } catch(ex) {
        console.error(ex);
      }
    }, Ci.nsIThread.DISPATCH_NORMAL);
  },

  





  updating: function(name) {
    if (this._updateProgress && this._updateProgress.node != this.selection.nodeFront) {
      this.cancelUpdate();
    }

    if (!this._updateProgress) {
      
      var self = this;
      this._updateProgress = {
        node: this.selection.nodeFront,
        outstanding: new Set(),
        checkDone: function() {
          if (this !== self._updateProgress) {
            return;
          }
          if (this.node !== self.selection.nodeFront) {
            self.cancelUpdate();
            return;
          }
          if (this.outstanding.size !== 0) {
            return;
          }

          self._updateProgress = null;
          self.emit("inspector-updated", name);
        },
      };
    }

    let progress = this._updateProgress;
    let done = function() {
      progress.outstanding.delete(done);
      progress.checkDone();
    };
    progress.outstanding.add(done);
    return done;
  },

  


  cancelUpdate: function() {
    this._updateProgress = null;
  },

  


  onBeforeNewSelection: function InspectorPanel_onBeforeNewSelection(event,
                                                                     node) {
    if (this.breadcrumbs.indexOf(node) == -1) {
      
      this.clearPseudoClasses();
    }
  },

  




  onDetached: function InspectorPanel_onDetached(event, parentNode) {
    this.cancelLayoutChange();
    this.breadcrumbs.cutAfter(this.breadcrumbs.indexOf(parentNode));
    this.selection.setNodeFront(parentNode ? parentNode : this._defaultNode, "detached");
  },

  


  destroy: function InspectorPanel__destroy() {
    if (this._panelDestroyer) {
      return this._panelDestroyer;
    }

    if (this.walker) {
      this.walker.off("new-root", this.onNewRoot);
      this.pageStyle = null;
    }

    this.cancelUpdate();
    this.cancelLayoutChange();

    if (this.browser) {
      this.browser.removeEventListener("resize", this.scheduleLayoutChange, true);
      this.browser = null;
    }

    this.target.off("will-navigate", this._onBeforeNavigate);

    this.target.off("thread-paused", this.updateDebuggerPausedWarning);
    this.target.off("thread-resumed", this.updateDebuggerPausedWarning);
    this._toolbox.off("select", this.updateDebuggerPausedWarning);

    this.sidebar.off("select", this._setDefaultSidebar);
    this.sidebar.destroy();
    this.sidebar = null;

    this.nodemenu.removeEventListener("popupshowing", this._setupNodeMenu, true);
    this.nodemenu.removeEventListener("popuphiding", this._resetNodeMenu, true);
    this.breadcrumbs.destroy();
    this.searchSuggestions.destroy();
    this.searchBox = null;
    this.selection.off("new-node-front", this.onNewSelection);
    this.selection.off("before-new-node", this.onBeforeNewSelection);
    this.selection.off("before-new-node-front", this.onBeforeNewSelection);
    this.selection.off("detached-front", this.onDetached);
    this._panelDestroyer = this._destroyMarkup();
    this.panelWin.inspector = null;
    this.target = null;
    this.panelDoc = null;
    this.panelWin = null;
    this.breadcrumbs = null;
    this.searchSuggestions = null;
    this.lastNodemenuItem = null;
    this.nodemenu = null;
    this._toolbox = null;

    return this._panelDestroyer;
  },

  


  showNodeMenu: function InspectorPanel_showNodeMenu(aButton, aPosition, aExtraItems) {
    if (aExtraItems) {
      for (let item of aExtraItems) {
        this.nodemenu.appendChild(item);
      }
    }
    this.nodemenu.openPopup(aButton, aPosition, 0, 0, true, false);
  },

  hideNodeMenu: function InspectorPanel_hideNodeMenu() {
    this.nodemenu.hidePopup();
  },

  



  _getClipboardContentForOuterHTML: function Inspector_getClipboardContentForOuterHTML() {
    let flavors = clipboard.currentFlavors;
    if (flavors.indexOf("text") != -1 ||
        (flavors.indexOf("html") != -1 && flavors.indexOf("image") == -1)) {
      let content = clipboard.get();
      if (content && content.trim().length > 0) {
        return content;
      }
    }
    return null;
  },

  


  _setupNodeMenu: function InspectorPanel_setupNodeMenu() {
    let isSelectionElement = this.selection.isElementNode();

    
    for (let name of ["hover", "active", "focus"]) {
      let menu = this.panelDoc.getElementById("node-menu-pseudo-" + name);

      if (isSelectionElement) {
        let checked = this.selection.nodeFront.hasPseudoClassLock(":" + name);
        menu.setAttribute("checked", checked);
        menu.removeAttribute("disabled");
      } else {
        menu.setAttribute("disabled", "true");
      }
    }

    
    let deleteNode = this.panelDoc.getElementById("node-menu-delete");
    if (this.selection.isRoot() || this.selection.isDocumentTypeNode()) {
      deleteNode.setAttribute("disabled", "true");
    } else {
      deleteNode.removeAttribute("disabled");
    }

    
    
    let unique = this.panelDoc.getElementById("node-menu-copyuniqueselector");
    let copyInnerHTML = this.panelDoc.getElementById("node-menu-copyinner");
    let copyOuterHTML = this.panelDoc.getElementById("node-menu-copyouter");
    if (isSelectionElement) {
      unique.removeAttribute("disabled");
      copyInnerHTML.removeAttribute("disabled");
      copyOuterHTML.removeAttribute("disabled");
    } else {
      unique.setAttribute("disabled", "true");
      copyInnerHTML.setAttribute("disabled", "true");
      copyOuterHTML.setAttribute("disabled", "true");
    }

    
    
    let editHTML = this.panelDoc.getElementById("node-menu-edithtml");
    if (this.isOuterHTMLEditable && isSelectionElement) {
      editHTML.removeAttribute("disabled");
    } else {
      editHTML.setAttribute("disabled", "true");
    }

    
    
    
    let pasteOuterHTML = this.panelDoc.getElementById("node-menu-pasteouterhtml");
    if (this.isOuterHTMLEditable && isSelectionElement &&
        this._getClipboardContentForOuterHTML()) {
      pasteOuterHTML.removeAttribute("disabled");
    } else {
      pasteOuterHTML.setAttribute("disabled", "true");
    }

    
    
    let copyImageData = this.panelDoc.getElementById("node-menu-copyimagedatauri");
    let markupContainer = this.markup.getContainer(this.selection.nodeFront);
    if (markupContainer && markupContainer.isPreviewable()) {
      copyImageData.removeAttribute("disabled");
    } else {
      copyImageData.setAttribute("disabled", "true");
    }
  },

  _resetNodeMenu: function InspectorPanel_resetNodeMenu() {
    
    while (this.lastNodemenuItem.nextSibling) {
      let toDelete = this.lastNodemenuItem.nextSibling;
      toDelete.parentNode.removeChild(toDelete);
    }
  },

  _initMarkup: function InspectorPanel_initMarkup() {
    let doc = this.panelDoc;

    this._markupBox = doc.getElementById("markup-box");

    
    this._markupFrame = doc.createElement("iframe");
    this._markupFrame.setAttribute("flex", "1");
    this._markupFrame.setAttribute("tooltip", "aHTMLTooltip");
    this._markupFrame.setAttribute("context", "inspector-node-popup");

    
    this._boundMarkupFrameLoad = this._onMarkupFrameLoad.bind(this);
    this._markupFrame.addEventListener("load", this._boundMarkupFrameLoad, true);

    this._markupBox.setAttribute("collapsed", true);
    this._markupBox.appendChild(this._markupFrame);
    this._markupFrame.setAttribute("src", "chrome://browser/content/devtools/markup-view.xhtml");
    this._markupFrame.setAttribute("aria-label", this.strings.GetStringFromName("inspector.panelLabel.markupView"))
  },

  _onMarkupFrameLoad: function InspectorPanel__onMarkupFrameLoad() {
    this._markupFrame.removeEventListener("load", this._boundMarkupFrameLoad, true);
    delete this._boundMarkupFrameLoad;

    this._markupFrame.contentWindow.focus();

    this._markupBox.removeAttribute("collapsed");

    let controllerWindow = this._toolbox.doc.defaultView;
    this.markup = new MarkupView(this, this._markupFrame, controllerWindow);

    this.emit("markuploaded");
  },

  _destroyMarkup: function InspectorPanel__destroyMarkup() {
    let destroyPromise;

    if (this._boundMarkupFrameLoad) {
      this._markupFrame.removeEventListener("load", this._boundMarkupFrameLoad, true);
      this._boundMarkupFrameLoad = null;
    }

    if (this.markup) {
      destroyPromise = this.markup.destroy();
      this.markup = null;
    } else {
      destroyPromise = promise.resolve();
    }

    if (this._markupFrame) {
      this._markupFrame.parentNode.removeChild(this._markupFrame);
      this._markupFrame = null;
    }

    this._markupBox = null;

    return destroyPromise;
  },

  


  togglePseudoClass: function InspectorPanel_togglePseudoClass(aPseudo) {
    if (this.selection.isElementNode()) {
      let node = this.selection.nodeFront;
      if (node.hasPseudoClassLock(aPseudo)) {
        return this.walker.removePseudoClassLock(node, aPseudo, {parents: true});
      }

      let hierarchical = aPseudo == ":hover" || aPseudo == ":active";
      return this.walker.addPseudoClassLock(node, aPseudo, {parents: hierarchical});
    }
  },

  


  showDOMProperties: function InspectorPanel_showDOMProperties() {
    this._toolbox.openSplitConsole().then(() => {
      let panel = this._toolbox.getPanel("webconsole");
      let jsterm = panel.hud.jsterm;

      jsterm.execute("inspect($0)");
      jsterm.focusInput();
    });
  },

  


  clearPseudoClasses: function InspectorPanel_clearPseudoClasses() {
    if (!this.walker) {
      return;
    }
    return this.walker.clearPseudoClassLocks().then(null, console.error);
  },

  


  editHTML: function InspectorPanel_editHTML()
  {
    if (!this.selection.isNode()) {
      return;
    }
    if (this.markup) {
      this.markup.beginEditingOuterHTML(this.selection.nodeFront);
    }
  },

  


  pasteOuterHTML: function InspectorPanel_pasteOuterHTML()
  {
    let content = this._getClipboardContentForOuterHTML();
    if (content) {
      let node = this.selection.nodeFront;
      this.markup.getNodeOuterHTML(node).then((oldContent) => {
        this.markup.updateNodeOuterHTML(node, content, oldContent);
      });
    }
  },

  


  copyInnerHTML: function InspectorPanel_copyInnerHTML()
  {
    if (!this.selection.isNode()) {
      return;
    }
    this._copyLongStr(this.walker.innerHTML(this.selection.nodeFront));
  },

  


  copyOuterHTML: function InspectorPanel_copyOuterHTML()
  {
    if (!this.selection.isNode()) {
      return;
    }

    this._copyLongStr(this.walker.outerHTML(this.selection.nodeFront));
  },

  


  copyImageDataUri: function InspectorPanel_copyImageDataUri()
  {
    let container = this.markup.getContainer(this.selection.nodeFront);
    if (container && container.isPreviewable()) {
      container.copyImageDataUri();
    }
  },

  _copyLongStr: function InspectorPanel_copyLongStr(promise)
  {
    return promise.then(longstr => {
      return longstr.string().then(toCopy => {
        longstr.release().then(null, console.error);
        clipboardHelper.copyString(toCopy);
      });
    }).then(null, console.error);
  },

  


  copyUniqueSelector: function InspectorPanel_copyUniqueSelector()
  {
    if (!this.selection.isNode()) {
      return;
    }

    let toCopy = CssLogic.findCssSelector(this.selection.node);
    if (toCopy) {
      clipboardHelper.copyString(toCopy);
    }
  },

  


  deleteNode: function IUI_deleteNode() {
    if (!this.selection.isNode() ||
         this.selection.isRoot()) {
      return;
    }

    
    
    if (this.markup) {
      this.markup.deleteNode(this.selection.nodeFront);
    } else {
      
      this.walker.removeNode(this.selection.nodeFront);
    }
  },

  



  immediateLayoutChange: function Inspector_immediateLayoutChange()
  {
    this.emit("layout-change");
  },

  



  scheduleLayoutChange: function Inspector_scheduleLayoutChange(event)
  {
    
    if (this.browser.contentWindow === event.target) {
      if (this._timer) {
        return null;
      }
      this._timer = this.panelWin.setTimeout(() => {
        this.emit("layout-change");
        this._timer = null;
      }, LAYOUT_CHANGE_TIMER);
    }
  },

  



  cancelLayoutChange: function Inspector_cancelLayoutChange()
  {
    if (this._timer) {
      this.panelWin.clearTimeout(this._timer);
      delete this._timer;
    }
  }
};




loader.lazyGetter(InspectorPanel.prototype, "strings",
  function () {
    return Services.strings.createBundle(
            "chrome://browser/locale/devtools/inspector.properties");
  });

loader.lazyGetter(this, "clipboardHelper", function() {
  return Cc["@mozilla.org/widget/clipboardhelper;1"].
    getService(Ci.nsIClipboardHelper);
});


loader.lazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});
