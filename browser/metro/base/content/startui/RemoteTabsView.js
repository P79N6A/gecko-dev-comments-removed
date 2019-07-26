




'use strict';
Components.utils.import("resource://services-sync/main.js");














function RemoteTabsView(aSet, aSetUIAccessList) {
  View.call(this, aSet);

  this._uiAccessElements = aSetUIAccessList;

  
  
  Weave.Svc.Obs.add("weave:service:sync:finish", this);
  Weave.Svc.Obs.add("weave:service:start-over", this);

  if (this.isSyncEnabled() ) {
    this.populateGrid();
  }
  else {
    this.setUIAccessVisible(false);
  }
}

RemoteTabsView.prototype = Util.extend(Object.create(View.prototype), {
  _set: null,
  _uiAccessElements: [],

  handleItemClick: function tabview_handleItemClick(aItem) {
    let url = aItem.getAttribute("value");
    StartUI.goToURI(url);
  },

  observe: function(subject, topic, data) {
    switch (topic) {
      case "weave:service:sync:finish":
        this.populateGrid();
        break;
      case "weave:service:start-over":
        this.setUIAccessVisible(false);
        break;
    }
  },

  setUIAccessVisible: function setUIAccessVisible(aVisible) {
    for (let elem of this._uiAccessElements) {
      elem.hidden = !aVisible;
    }
  },

  populateGrid: function populateGrid() {

    let tabsEngine = Weave.Service.engineManager.get("tabs");
    let list = this._set;
    let seenURLs = new Set();

    
    
    this._set.clearAll();
    let show = false;
    for (let [guid, client] in Iterator(tabsEngine.getAllClients())) {
      client.tabs.forEach(function({title, urlHistory, icon}) {
        let url = urlHistory[0];
        if (tabsEngine.locallyOpenTabMatchesURL(url) || seenURLs.has(url)) {
          return;
        }
        seenURLs.add(url);
        show = true;

        
        
        

        let item = this._set.appendItem((title || url), url);
        item.setAttribute("iconURI", Weave.Utils.getIcon(icon));

      }, this);
    }
    this.setUIAccessVisible(show);
    this._set.arrangeItems();
    this._set.removeAttribute("fade");
  },

  destruct: function destruct() {
    Weave.Svc.Obs.remove("weave:engine:sync:finish", this);
    Weave.Svc.Obs.remove("weave:service:logout:start-over", this);
    View.prototype.destruct.call(this);
  },

  isSyncEnabled: function isSyncEnabled() {
    return (Weave.Status.checkSetup() != Weave.CLIENT_NOT_CONFIGURED);
  }

});

let RemoteTabsStartView = {
  _view: null,
  get _grid() { return document.getElementById("start-remotetabs-grid"); },

  init: function init() {
    let vbox = document.getElementById("start-remotetabs");
    let uiList = [vbox];
    this._view = new RemoteTabsView(this._grid, uiList);
  },

  uninit: function uninit() {
    if (this._view) {
      this._view.destruct();
    }
  },
};
