





const Cc = Components.classes;
const Cu = Components.utils;
const Ci = Components.interfaces;
const Cr = Components.results;

this.EXPORTED_SYMBOLS = ["InspectorPanel"];

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/MarkupView.jsm");
Cu.import("resource:///modules/devtools/EventEmitter.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");
Cu.import("resource:///modules/devtools/Selection.jsm");
Cu.import("resource:///modules/devtools/Breadcrumbs.jsm");
Cu.import("resource:///modules/devtools/Highlighter.jsm");
Cu.import("resource:///modules/devtools/Sidebar.jsm");

const LAYOUT_CHANGE_TIMER = 250;







this.InspectorPanel = function InspectorPanel(iframeWindow, toolbox) {
  this._toolbox = toolbox;
  this._target = toolbox._target;

  if (this.target.isRemote) {
    throw "Unsupported target";
  }

  this.tabTarget = (this.target.tab != null);
  this.winTarget = (this.target.window != null);

  new EventEmitter(this);

  this.preventNavigateAway = this.preventNavigateAway.bind(this);
  this.onNavigatedAway = this.onNavigatedAway.bind(this);
  this.target.on("will-navigate", this.preventNavigateAway);
  this.target.on("navigate", this.onNavigatedAway);

  this.panelDoc = iframeWindow.document;
  this.panelWin = iframeWindow;
  this.panelWin.inspector = this;

  this.nodemenu = this.panelDoc.getElementById("inspector-node-popup");
  this.lastNodemenuItem = this.nodemenu.lastChild;
  this._setupNodeMenu = this._setupNodeMenu.bind(this);
  this._resetNodeMenu = this._resetNodeMenu.bind(this);
  this.nodemenu.addEventListener("popupshowing", this._setupNodeMenu, true);
  this.nodemenu.addEventListener("popuphiding", this._resetNodeMenu, true);

  
  this._selection = new Selection();
  this.onNewSelection = this.onNewSelection.bind(this);
  this.selection.on("new-node", this.onNewSelection);

  this.breadcrumbs = new HTMLBreadcrumbs(this);

  if (this.tabTarget) {
    this.browser = this.target.tab.linkedBrowser;
    this.scheduleLayoutChange = this.scheduleLayoutChange.bind(this);
    this.browser.addEventListener("resize", this.scheduleLayoutChange, true);

    this.highlighter = new Highlighter(this.target, this, this._toolbox);
    let button = this.panelDoc.getElementById("inspector-inspect-toolbutton");
    button.hidden = false;
    this.updateInspectorButton = function() {
      if (this.highlighter.locked) {
        button.removeAttribute("checked");
      } else {
        button.setAttribute("checked", "true");
      }
    }.bind(this);
    this.highlighter.on("locked", this.updateInspectorButton);
    this.highlighter.on("unlocked", this.updateInspectorButton);
  }

  this._initMarkup();
  this.isReady = false;

  this.once("markuploaded", function() {
    this.isReady = true;

    
    if (this.tabTarget) {
      let root = this.browser.contentDocument.documentElement;
      this._selection.setNode(root);
    }
    if (this.winTarget) {
      let root = this.target.window.document.documentElement;
      this._selection.setNode(root);
    }

    if (this.highlighter) {
      this.highlighter.unlock();
    }

    this.emit("ready");
  }.bind(this));

  this.setupSidebar();
}

InspectorPanel.prototype = {
  


  get selection() {
    return this._selection;
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

  


  setupSidebar: function InspectorPanel_setupSidebar() {
    let tabbox = this.panelDoc.querySelector("#inspector-sidebar");
    this.sidebar = new ToolSidebar(tabbox, this);

    let defaultTab = Services.prefs.getCharPref("devtools.inspector.activeSidebar");

    this._setDefaultSidebar = function(event, toolId) {
      Services.prefs.setCharPref("devtools.inspector.activeSidebar", toolId);
    }.bind(this);

    this.sidebar.on("select", this._setDefaultSidebar);

    this.sidebar.addTab("ruleview",
                        "chrome://browser/content/devtools/cssruleview.xul",
                        "ruleview" == defaultTab);

    this.sidebar.addTab("computedview",
                        "chrome://browser/content/devtools/csshtmltree.xul",
                        "computedview" == defaultTab);

    this.sidebar.addTab("layoutview",
                        "chrome://browser/content/devtools/layoutview/view.xhtml",
                        "layoutview" == defaultTab);

    this.sidebar.show();
  },

  


  onNavigatedAway: function InspectorPanel_onNavigatedAway(event, newWindow) {
    this.selection.setNode(null);
    this._destroyMarkup();
    this.isDirty = false;
    let self = this;
    newWindow.addEventListener("DOMContentLoaded", function onDOMReady() {
      newWindow.removeEventListener("DOMContentLoaded", onDOMReady, true);;
      if (!self.selection.node) {
        self.selection.setNode(newWindow.document.documentElement);
      }
      self._initMarkup();
    }, true);
  },

  


  preventNavigateAway: function InspectorPanel_preventNavigateAway(event, request) {
    if (!this.isDirty) {
      return;
    }

    request.suspend();

    let notificationBox = this._toolbox.getNotificationBox();
    let notification = notificationBox.
      getNotificationWithValue("inspector-page-navigation");

    if (notification) {
      notificationBox.removeNotification(notification, true);
    }

    let cancelRequest = function onCancelRequest() {
      if (request) {
        request.cancel(Cr.NS_BINDING_ABORTED);
        request.resume(); 
        request = null;
      }
    };

    let eventCallback = function onNotificationCallback(event) {
      if (event == "removed") {
        cancelRequest();
      }
    };

    let buttons = [
      {
        id: "inspector.confirmNavigationAway.buttonLeave",
        label: this.strings.GetStringFromName("confirmNavigationAway.buttonLeave"),
        accessKey: this.strings.GetStringFromName("confirmNavigationAway.buttonLeaveAccesskey"),
        callback: function onButtonLeave() {
          if (request) {
            request.resume();
            request = null;
            return true;
          }
          return false;
        }.bind(this),
      },
      {
        id: "inspector.confirmNavigationAway.buttonStay",
        label: this.strings.GetStringFromName("confirmNavigationAway.buttonStay"),
        accessKey: this.strings.GetStringFromName("confirmNavigationAway.buttonStayAccesskey"),
        callback: cancelRequest
      },
    ];

    let message = this.strings.GetStringFromName("confirmNavigationAway.message2");

    notification = notificationBox.appendNotification(message,
      "inspector-page-navigation", "chrome://browser/skin/Info.png",
      notificationBox.PRIORITY_WARNING_HIGH, buttons, eventCallback);

    
    
    notification.persistence = -1;
  },

  


  onNewSelection: function InspectorPanel_onNewSelection() {
    this.cancelLayoutChange();
  },

  


  destroy: function InspectorPanel__destroy() {
    if (this._destroyed) {
      return;
    }
    this.cancelLayoutChange();
    this._destroyed = true;

    this._toolbox = null;

    if (this.browser) {
      this.browser.removeEventListener("resize", this.scheduleLayoutChange, true);
      this.browser = null;
    }

    this.target.off("will-navigate", this.preventNavigateAway);
    this.target.off("navigate", this.onNavigatedAway);

    if (this.highlighter) {
      this.highlighter.off("locked", this.updateInspectorButton);
      this.highlighter.off("unlocked", this.updateInspectorButton);
      this.highlighter.destroy();
    }

    this.sidebar.off("select", this._setDefaultSidebar);
    this.sidebar.destroy();
    this.sidebar = null;

    this.nodemenu.removeEventListener("popupshowing", this._setupNodeMenu, true);
    this.nodemenu.removeEventListener("popuphiding", this._resetNodeMenu, true);
    this.breadcrumbs.destroy();
    this.selection.off("new-node", this.onNewSelection);
    this._destroyMarkup();
    this._selection.destroy();
    this._selection = null;
    this.panelWin.inspector = null;
    this.target = null;
    this.panelDoc = null;
    this.panelWin = null;
    this.breadcrumbs = null;
    this.lastNodemenuItem = null;
    this.nodemenu = null;
    this.highlighter = null;
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

  


  _setupNodeMenu: function InspectorPanel_setupNodeMenu() {
    
    for (let name of ["hover", "active", "focus"]) {
      let menu = this.panelDoc.getElementById("node-menu-pseudo-" + name);
      let checked = DOMUtils.hasPseudoClassLock(this.selection.node, ":" + name);
      menu.setAttribute("checked", checked);
    }

    
    let deleteNode = this.panelDoc.getElementById("node-menu-delete");
    if (this.selection.isRoot()) {
      deleteNode.setAttribute("disabled", "true");
    } else {
      deleteNode.removeAttribute("disabled");
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

    
    this._boundMarkupFrameLoad = function InspectorPanel_initMarkupPanel_onload() {
      this._markupFrame.contentWindow.focus();
      this._onMarkupFrameLoad();
    }.bind(this);
    this._markupFrame.addEventListener("load", this._boundMarkupFrameLoad, true);

    this._markupBox.setAttribute("hidden", true);
    this._markupBox.appendChild(this._markupFrame);
    this._markupFrame.setAttribute("src", "chrome://browser/content/devtools/markup-view.xhtml");
  },

  _onMarkupFrameLoad: function InspectorPanel__onMarkupFrameLoad() {
    this._markupFrame.removeEventListener("load", this._boundMarkupFrameLoad, true);
    delete this._boundMarkupFrameLoad;

    this._markupBox.removeAttribute("hidden");

    let controllerWindow;
    if (this.tabTarget) {
      controllerWindow = this.target.tab.ownerDocument.defaultView;
    } else if (this.winTarget) {
      controllerWindow = this.target.window;
    }
    this.markup = new MarkupView(this, this._markupFrame, controllerWindow);

    this.emit("markuploaded");
  },

  _destroyMarkup: function InspectorPanel__destroyMarkup() {
    if (this._boundMarkupFrameLoad) {
      this._markupFrame.removeEventListener("load", this._boundMarkupFrameLoad, true);
      delete this._boundMarkupFrameLoad;
    }

    if (this.markup) {
      this.markup.destroy();
      delete this.markup;
    }

    if (this._markupFrame) {
      this._markupFrame.parentNode.removeChild(this._markupFrame);
      delete this._markupFrame;
    }
  },

  


  togglePseudoClass: function InspectorPanel_togglePseudoClass(aPseudo) {
    if (this.selection.isElementNode()) {
      if (DOMUtils.hasPseudoClassLock(this.selection.node, aPseudo)) {
        this.breadcrumbs.nodeHierarchy.forEach(function(crumb) {
          DOMUtils.removePseudoClassLock(crumb.node, aPseudo);
        });
      } else {
        let hierarchical = aPseudo == ":hover" || aPseudo == ":active";
        let node = this.selection.node;
        do {
          DOMUtils.addPseudoClassLock(node, aPseudo);
          node = node.parentNode;
        } while (hierarchical && node.parentNode)
      }
    }
    this.selection.emit("pseudoclass");
  },

  


  copyInnerHTML: function InspectorPanel_copyInnerHTML()
  {
    if (!this.selection.isNode()) {
      return;
    }
    let toCopy = this.selection.node.innerHTML;
    if (toCopy) {
      clipboardHelper.copyString(toCopy);
    }
  },

  


  copyOuterHTML: function InspectorPanel_copyOuterHTML()
  {
    if (!this.selection.isNode()) {
      return;
    }
    let toCopy = this.selection.node.outerHTML;
    if (toCopy) {
      clipboardHelper.copyString(toCopy);
    }
  },

  


  deleteNode: function IUI_deleteNode() {
    if (!this.selection.isNode() ||
         this.selection.isRoot()) {
      return;
    }

    let toDelete = this.selection.node;

    let parent = this.selection.node.parentNode;

    
    
    if (this.markup) {
      this.markup.deleteNode(toDelete);
    } else {
      
      parent.removeChild(toDelete);
    }
  },

  



  scheduleLayoutChange: function Inspector_scheduleLayoutChange()
  {
    if (this._timer) {
      return null;
    }
    this._timer = this.panelWin.setTimeout(function() {
      this.emit("layout-change");
      this._timer = null;
    }.bind(this), LAYOUT_CHANGE_TIMER);
  },

  



  cancelLayoutChange: function Inspector_cancelLayoutChange()
  {
    if (this._timer) {
      this.panelWin.clearTimeout(this._timer);
      delete this._timer;
    }
  },

}




XPCOMUtils.defineLazyGetter(InspectorPanel.prototype, "strings",
  function () {
    return Services.strings.createBundle(
            "chrome://browser/locale/devtools/inspector.properties");
  });

XPCOMUtils.defineLazyGetter(this, "clipboardHelper", function() {
  return Cc["@mozilla.org/widget/clipboardhelper;1"].
    getService(Ci.nsIClipboardHelper);
});


XPCOMUtils.defineLazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});
