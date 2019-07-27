





const {Cu} = require("chrome");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");

var {Promise: promise} = require("resource://gre/modules/Promise.jsm");
var EventEmitter = require("devtools/toolkit/event-emitter");
var Telemetry = require("devtools/shared/telemetry");

const XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";












































function ToolSidebar(tabbox, panel, uid, options={}) {
  EventEmitter.decorate(this);

  this._tabbox = tabbox;
  this._uid = uid;
  this._panelDoc = this._tabbox.ownerDocument;
  this._toolPanel = panel;
  this._options = options;

  this._onTabBoxOverflow = this._onTabBoxOverflow.bind(this);
  this._onTabBoxUnderflow = this._onTabBoxUnderflow.bind(this);

  try {
    this._width = Services.prefs.getIntPref("devtools.toolsidebar-width." + this._uid);
  } catch(e) {}

  if (!options.disableTelemetry) {
    this._telemetry = new Telemetry();
  }

  this._tabbox.tabpanels.addEventListener("select", this, true);

  this._tabs = new Map();

  
  this.addExistingTabs();

  if (this._options.hideTabstripe) {
    this._tabbox.setAttribute("hidetabs", "true");
  }

  if (this._options.showAllTabsMenu) {
    this.addAllTabsMenu();
  }

  this._toolPanel.emit("sidebar-created", this);
}

exports.ToolSidebar = ToolSidebar;

ToolSidebar.prototype = {
  TAB_ID_PREFIX: "sidebar-tab-",

  TABPANEL_ID_PREFIX: "sidebar-panel-",

  







  addAllTabsMenu: function() {
    if (this._allTabsBtn) {
      return;
    }

    let tabs = this._tabbox.tabs;

    
    let allTabsContainer = this._panelDoc.createElementNS(XULNS, "box");
    this._tabbox.insertBefore(allTabsContainer, tabs);

    
    allTabsContainer.appendChild(tabs);
    tabs.setAttribute("flex", "1");

    
    this._allTabsBtn = this._panelDoc.createElementNS(XULNS, "toolbarbutton");
    this._allTabsBtn.setAttribute("class", "devtools-sidebar-alltabs");
    this._allTabsBtn.setAttribute("type", "menu");
    this._allTabsBtn.setAttribute("label", l10n("sidebar.showAllTabs.label"));
    this._allTabsBtn.setAttribute("tooltiptext", l10n("sidebar.showAllTabs.tooltip"));
    this._allTabsBtn.setAttribute("hidden", "true");
    allTabsContainer.appendChild(this._allTabsBtn);

    let menuPopup = this._panelDoc.createElementNS(XULNS, "menupopup");
    this._allTabsBtn.appendChild(menuPopup);

    
    tabs.addEventListener("overflow", this._onTabBoxOverflow, false);
    tabs.addEventListener("underflow", this._onTabBoxUnderflow, false);

    
    
    for (let [id, tab] of this._tabs) {
      this._addItemToAllTabsMenu(id, tab, tab.hasAttribute("selected"));
    }
  },

  removeAllTabsMenu: function() {
    if (!this._allTabsBtn) {
      return;
    }

    let tabs = this._tabbox.tabs;

    tabs.removeEventListener("overflow", this._onTabBoxOverflow, false);
    tabs.removeEventListener("underflow", this._onTabBoxUnderflow, false);

    
    this._tabbox.insertBefore(tabs, this._tabbox.tabpanels);
    this._tabbox.querySelector("box").remove();

    this._allTabsBtn = null;
  },

  _onTabBoxOverflow: function() {
    this._allTabsBtn.removeAttribute("hidden");
  },

  _onTabBoxUnderflow: function() {
    this._allTabsBtn.setAttribute("hidden", "true");
  },

  


  _addItemToAllTabsMenu: function(id, tab, selected=false) {
    if (!this._allTabsBtn) {
      return;
    }

    let item = this._panelDoc.createElementNS(XULNS, "menuitem");
    item.setAttribute("id", "sidebar-alltabs-item-" + id);
    item.setAttribute("label", tab.getAttribute("label"));
    item.setAttribute("type", "checkbox");
    if (selected) {
      item.setAttribute("checked", true);
    }
    
    
    item.setAttribute("autocheck", false);

    this._allTabsBtn.querySelector("menupopup").appendChild(item);

    item.addEventListener("click", () => {
      this._tabbox.selectedTab = tab;
    }, false);

    tab.allTabsMenuItem = item;

    return item;
  },

  






  addTab: function(id, url, selected=false) {
    let iframe = this._panelDoc.createElementNS(XULNS, "iframe");
    iframe.className = "iframe-" + id;
    iframe.setAttribute("flex", "1");
    iframe.setAttribute("src", url);
    iframe.tooltip = "aHTMLTooltip";

    
    let tab = this._panelDoc.createElementNS(XULNS, "tab");
    this._tabbox.tabs.appendChild(tab);
    tab.setAttribute("label", ""); 
    tab.setAttribute("id", this.TAB_ID_PREFIX + id);

    
    let allTabsItem = this._addItemToAllTabsMenu(id, tab, selected);

    let onIFrameLoaded = (event) => {
      let doc = event.target;
      let win = doc.defaultView;
      tab.setAttribute("label", doc.title);

      if (allTabsItem) {
        allTabsItem.setAttribute("label", doc.title);
      }

      iframe.removeEventListener("load", onIFrameLoaded, true);
      if ("setPanel" in win) {
        win.setPanel(this._toolPanel, iframe);
      }
      this.emit(id + "-ready");
    };

    iframe.addEventListener("load", onIFrameLoaded, true);

    let tabpanel = this._panelDoc.createElementNS(XULNS, "tabpanel");
    tabpanel.setAttribute("id", this.TABPANEL_ID_PREFIX + id);
    tabpanel.appendChild(iframe);
    this._tabbox.tabpanels.appendChild(tabpanel);

    this._tooltip = this._panelDoc.createElementNS(XULNS, "tooltip");
    this._tooltip.id = "aHTMLTooltip";
    tabpanel.appendChild(this._tooltip);
    this._tooltip.page = true;

    tab.linkedPanel = this.TABPANEL_ID_PREFIX + id;

    
    this._tabs.set(id, tab);

    if (selected) {
      
      
      
      this._panelDoc.defaultView.setTimeout(() => {
        this.select(id);
      }, 10);
    }

    this.emit("new-tab-registered", id);
  },

  untitledTabsIndex: 0,

  


  addExistingTabs: function() {
    let knownTabs = [...this._tabs.values()];

    for (let tab of this._tabbox.tabs.querySelectorAll("tab")) {
      if (knownTabs.indexOf(tab) !== -1) {
        continue;
      }

      
      let id = tab.getAttribute("id") || "untitled-tab-" + (this.untitledTabsIndex++);

      
      this._tabs.set(id, tab);
      this.emit("new-tab-registered", id);
    }
  },

  






  removeTab: Task.async(function*(tabId, tabPanelId) {
    
    let tab = this.getTab(tabId);
    if (!tab) {
      return;
    }

    let win = this.getWindowForTab(tabId);
    if (win && ("destroy" in win)) {
      yield win.destroy();
    }

    tab.remove();

    
    let panel = this.getTabPanel(tabPanelId || tabId);
    if (panel) {
      panel.remove();
    }

    this._tabs.delete(tabId);
    this.emit("tab-unregistered", tabId);
  }),

  







  toggleTab: function(isVisible, id, tabPanelId) {
    
    let tab = this.getTab(id);
    if (!tab) {
      return;
    }
    tab.hidden = !isVisible;

    
    if (this._allTabsBtn) {
      this._allTabsBtn.querySelector("#sidebar-alltabs-item-" + id).hidden = !isVisible;
    }

    
    
    let tabPanel = this.getTabPanel(id);
    if (!tabPanel && tabPanelId) {
      tabPanel = this.getTabPanel(tabPanelId);
    }
    if (tabPanel) {
      tabPanel.hidden = !isVisible;
    }
  },

  


  select: function(id) {
    let tab = this.getTab(id);
    if (tab) {
      this._tabbox.selectedTab = tab;
    }
  },

  


  getCurrentTabID: function() {
    let currentID = null;
    for (let [id, tab] of this._tabs) {
      if (this._tabbox.tabs.selectedItem == tab) {
        currentID = id;
        break;
      }
    }
    return currentID;
  },

  




  getTabPanel: function(id) {
    
    
    return this._tabbox.tabpanels.querySelector("#" + this.TABPANEL_ID_PREFIX + id + ", #" + id);
  },

  




  getTab: function(id) {
    return this._tabs.get(id);
  },

  


  handleEvent: function(event) {
    if (event.type !== "select" || this._destroyed) {
      return;
    }

    if (this._currentTool == this.getCurrentTabID()) {
      
      return;
    }

    let previousTool = this._currentTool;
    this._currentTool = this.getCurrentTabID();
    if (previousTool) {
      if (this._telemetry) {
        this._telemetry.toolClosed(previousTool);
      }
      this.emit(previousTool + "-unselected");
    }

    if (this._telemetry) {
      this._telemetry.toolOpened(this._currentTool);
    }

    this.emit(this._currentTool + "-selected");
    this.emit("select", this._currentTool);

    
    
    if (this._destroyed) {
      return;
    }

    
    
    let tab = this._tabbox.selectedTab;
    if (tab.allTabsMenuItem) {
      for (let otherItem of this._allTabsBtn.querySelectorAll("menuitem")) {
        otherItem.removeAttribute("checked");
      }
      tab.allTabsMenuItem.setAttribute("checked", true);
    }
  },

  


  toggle: function() {
    if (this._tabbox.hasAttribute("hidden")) {
      this.show();
    } else {
      this.hide();
    }
  },

  


  show: function() {
    if (this._width) {
      this._tabbox.width = this._width;
    }
    this._tabbox.removeAttribute("hidden");

    this.emit("show");
  },

  


  hide: function() {
    Services.prefs.setIntPref("devtools.toolsidebar-width." + this._uid, this._tabbox.width);
    this._tabbox.setAttribute("hidden", "true");

    this.emit("hide");
  },

  


  getWindowForTab: function(id) {
    if (!this._tabs.has(id)) {
      return null;
    }

    
    let panel = this.getTabPanel(id);
    if (!panel || !panel.firstChild || !panel.firstChild.contentWindow) {
      return;
    }
    return panel.firstChild.contentWindow;
  },

  


  destroy: Task.async(function*() {
    if (this._destroyed) {
      return;
    }
    this._destroyed = true;

    Services.prefs.setIntPref("devtools.toolsidebar-width." + this._uid, this._tabbox.width);

    if (this._allTabsBtn) {
      this.removeAllTabsMenu();
    }

    this._tabbox.tabpanels.removeEventListener("select", this, true);

    
    
    
    while (this._tabbox.tabpanels && this._tabbox.tabpanels.hasChildNodes()) {
      let panel = this._tabbox.tabpanels.firstChild;
      let win = panel.firstChild.contentWindow;
      if (win && ("destroy" in win)) {
        yield win.destroy();
      }
      panel.remove();
    }

    while (this._tabbox.tabs && this._tabbox.tabs.hasChildNodes()) {
      this._tabbox.tabs.removeChild(this._tabbox.tabs.firstChild);
    }

    if (this._currentTool && this._telemetry) {
      this._telemetry.toolClosed(this._currentTool);
    }

    this._toolPanel.emit("sidebar-destroyed", this);

    this._tabs = null;
    this._tabbox = null;
    this._panelDoc = null;
    this._toolPanel = null;
  })
}

XPCOMUtils.defineLazyGetter(this, "l10n", function() {
  let bundle = Services.strings.createBundle("chrome://browser/locale/devtools/toolbox.properties");
  let l10n = function(aName, ...aArgs) {
    try {
      if (aArgs.length == 0) {
        return bundle.GetStringFromName(aName);
      } else {
        return bundle.formatStringFromName(aName, aArgs, aArgs.length);
      }
    } catch (ex) {
      Services.console.logStringMessage("Error reading '" + aName + "'");
    }
  };
  return l10n;
});
