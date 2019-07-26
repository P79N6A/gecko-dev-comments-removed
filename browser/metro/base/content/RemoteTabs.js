




'use strict';
Components.utils.import("resource://services-sync/main.js");










function RemoteTabsView(aSet, aSetUIAccess) {
  this._set = aSet;
  this._set.controller = this;
  this._uiAccessElement = aSetUIAccess;

  
  
  Weave.Svc.Obs.add("weave:service:setup-complete", this);
  Weave.Svc.Obs.add("weave:service:sync:finish", this);
  Weave.Svc.Obs.add("weave:service:start-over", this);
  if (this.isSyncEnabled() ) {
    this.populateTabs();
    this.populateGrid();
    this.setUIAccessVisible(true);
  }
  else {
    this.setUIAccessVisible(false);
  }
}

RemoteTabsView.prototype = {
  _set: null,
  _uiAccessElement: null,

  handleItemClick: function tabview_handleItemClick(aItem) {
    let url = aItem.getAttribute("value");
    BrowserUI.goToURI(url);
  },

  observe: function(subject, topic, data) {
    switch (topic) {
      case "weave:service:setup-complete":
        this.populateTabs();
        this.setUIAccessVisible(true);
        break;
      case "weave:service:sync:finish":
        this.populateGrid();
        break;
      case "weave:service:start-over":
        this.setUIAccessVisible(false);
        break;
    }
  },

  setUIAccessVisible: function setUIAccessVisible(aVisible) {
    this._uiAccessElement.hidden = !aVisible;
  },

  populateGrid: function populateGrid() {

    let tabsEngine = Weave.Service.engineManager.get("tabs");
    let list = this._set;
    let seenURLs = new Set();

    for (let [guid, client] in Iterator(tabsEngine.getAllClients())) {
      client.tabs.forEach(function({title, urlHistory, icon}) {
        let url = urlHistory[0];
        if (tabsEngine.locallyOpenTabMatchesURL(url) || seenURLs.has(url)) {
          return;
        }
        seenURLs.add(url);

        
        
        

        let item = this._set.appendItem((title || url), url);
        item.setAttribute("iconURI", Weave.Utils.getIcon(icon));

      }, this);
    }
  },

  populateTabs: function populateTabs() {
    Weave.Service.scheduler.scheduleNextSync(0);
  },

  destruct: function destruct() {
    Weave.Svc.Obs.remove("weave:service:setup-complete", this);
    Weave.Svc.Obs.remove("weave:engine:sync:finish", this);
    Weave.Svc.Obs.remove("weave:service:logout:start-over", this);
  },

  isSyncEnabled: function isSyncEnabled() {
    return (Weave.Status.checkSetup() != Weave.CLIENT_NOT_CONFIGURED);
  }

};

let RemoteTabsStartView = {
  _view: null,
  get _grid() { return document.getElementById("start-remotetabs-grid"); },

  init: function init() {
    let vbox = document.getElementById("start-remotetabs");
    this._view = new RemoteTabsView(this._grid, vbox);
  },

  uninit: function uninit() {
    this._view.destruct();
  },

  show: function show() {
    this._grid.arrangeItems();
  }
};

let RemoteTabsPanelView = {
  _view: null,

  get _grid() { return document.getElementById("remotetabs-list"); },
  get visible() { return PanelUI.isPaneVisible("remotetabs-container"); },

  init: function init() {
    
    let menuEntry = document.getElementById("menuitem-remotetabs");
    this._view = new RemoteTabsView(this._grid, menuEntry);
  },

  show: function show() {
    this._grid.arrangeItems();
  },

  uninit: function uninit() {
    this._view.destruct();
  }
};
