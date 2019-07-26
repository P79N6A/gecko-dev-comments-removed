





const {Cu} = require("chrome");

Cu.import("resource://gre/modules/Services.jsm");

var Promise = require("sdk/core/promise");
var EventEmitter = require("devtools/shared/event-emitter");
var Telemetry = require("devtools/shared/telemetry");

const XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";














function ToolSidebar(tabbox, panel, uid, showTabstripe=true)
{
  EventEmitter.decorate(this);

  this._tabbox = tabbox;
  this._uid = uid;
  this._panelDoc = this._tabbox.ownerDocument;
  this._toolPanel = panel;

  try {
    this._width = Services.prefs.getIntPref("devtools.toolsidebar-width." + this._uid);
  } catch(e) {}

  this._telemetry = new Telemetry();

  this._tabbox.tabpanels.addEventListener("select", this, true);

  this._tabs = new Map();

  if (!showTabstripe) {
    this._tabbox.setAttribute("hidetabs", "true");
  }
}

exports.ToolSidebar = ToolSidebar;

ToolSidebar.prototype = {
  






  addTab: function ToolSidebar_addTab(id, url, selected=false) {
    let iframe = this._panelDoc.createElementNS(XULNS, "iframe");
    iframe.className = "iframe-" + id;
    iframe.setAttribute("flex", "1");
    iframe.setAttribute("src", url);
    iframe.tooltip = "aHTMLTooltip";

    let tab = this._tabbox.tabs.appendItem();
    tab.setAttribute("label", ""); 

    let onIFrameLoaded = function() {
      tab.setAttribute("label", iframe.contentDocument.title);
      iframe.removeEventListener("load", onIFrameLoaded, true);
      if ("setPanel" in iframe.contentWindow) {
        iframe.contentWindow.setPanel(this._toolPanel, iframe);
      }
      this.emit(id + "-ready");
    }.bind(this);

    iframe.addEventListener("load", onIFrameLoaded, true);

    let tabpanel = this._panelDoc.createElementNS(XULNS, "tabpanel");
    tabpanel.setAttribute("id", "sidebar-panel-" + id);
    tabpanel.appendChild(iframe);
    this._tabbox.tabpanels.appendChild(tabpanel);

    this._tooltip = this._panelDoc.createElementNS(XULNS, "tooltip");
    this._tooltip.id = "aHTMLTooltip";
    tabpanel.appendChild(this._tooltip);
    this._tooltip.page = true;

    tab.linkedPanel = "sidebar-panel-" + id;

    
    this._tabs.set(id, tab);

    if (selected) {
      
      
      
      this._panelDoc.defaultView.setTimeout(function() {
        this.select(id);
      }.bind(this), 10);
    }

    this.emit("new-tab-registered", id);
  },

  


  select: function ToolSidebar_select(id) {
    let tab = this._tabs.get(id);
    if (tab) {
      this._tabbox.selectedTab = tab;
    }
  },

  


  getCurrentTabID: function ToolSidebar_getCurrentTabID() {
    let currentID = null;
    for (let [id, tab] of this._tabs) {
      if (this._tabbox.tabs.selectedItem == tab) {
        currentID = id;
        break;
      }
    }
    return currentID;
  },

  





  getTab: function ToolSidebar_getTab(id) {
    return this._tabbox.tabpanels.querySelector("#sidebar-panel-" + id);
  },

  


  handleEvent: function ToolSidebar_eventHandler(event) {
    if (event.type == "select") {
      let previousTool = this._currentTool;
      this._currentTool = this.getCurrentTabID();
      if (previousTool) {
        this._telemetry.toolClosed(previousTool);
        this.emit(previousTool + "-unselected");
      }

      this._telemetry.toolOpened(this._currentTool);
      this.emit(this._currentTool + "-selected");
      this.emit("select", this._currentTool);
    }
  },

  


  toggle: function ToolSidebar_toggle() {
    if (this._tabbox.hasAttribute("hidden")) {
      this.show();
    } else {
      this.hide();
    }
  },

  


  show: function ToolSidebar_show() {
    if (this._width) {
      this._tabbox.width = this._width;
    }
    this._tabbox.removeAttribute("hidden");
  },

  


  hide: function ToolSidebar_hide() {
    Services.prefs.setIntPref("devtools.toolsidebar-width." + this._uid, this._tabbox.width);
    this._tabbox.setAttribute("hidden", "true");
  },

  


  getWindowForTab: function ToolSidebar_getWindowForTab(id) {
    if (!this._tabs.has(id)) {
      return null;
    }

    let panel = this._panelDoc.getElementById(this._tabs.get(id).linkedPanel);
    return panel.firstChild.contentWindow;
  },

  


  destroy: function ToolSidebar_destroy() {
    if (this._destroyed) {
      return Promise.resolve(null);
    }
    this._destroyed = true;

    Services.prefs.setIntPref("devtools.toolsidebar-width." + this._uid, this._tabbox.width);

    this._tabbox.tabpanels.removeEventListener("select", this, true);

    while (this._tabbox.tabpanels.hasChildNodes()) {
      this._tabbox.tabpanels.removeChild(this._tabbox.tabpanels.firstChild);
    }

    while (this._tabbox.tabs.hasChildNodes()) {
      this._tabbox.tabs.removeChild(this._tabbox.tabs.firstChild);
    }

    if (this._currentTool) {
      this._telemetry.toolClosed(this._currentTool);
    }

    this._tabs = null;
    this._tabbox = null;
    this._panelDoc = null;
    this._toolPanel = null;

    return Promise.resolve(null);
  },
}
