











































let Storage = {
  GROUP_DATA_IDENTIFIER: "tabview-group",
  GROUPS_DATA_IDENTIFIER: "tabview-groups",
  TAB_DATA_IDENTIFIER: "tabview-tab",
  UI_DATA_IDENTIFIER: "tabview-ui",

  
  
  
  toString: function Storage_toString() {
    return "[Storage]";
  },

  
  
  
  init: function Storage_init() {
    this._sessionStore =
      Cc["@mozilla.org/browser/sessionstore;1"].
        getService(Ci.nsISessionStore);
  },

  
  
  uninit: function Storage_uninit () {
    this._sessionStore = null;
  },

  
  
  
  wipe: function Storage_wipe() {
    try {
      var self = this;

      
      AllTabs.tabs.forEach(function(tab) {
        self.saveTab(tab, null);
      });

      
      this.saveGroupItemsData(gWindow, {});
      this.saveUIData(gWindow, {});

      this._sessionStore.setWindowValue(gWindow, this.GROUP_DATA_IDENTIFIER,
        JSON.stringify({}));
    } catch (e) {
      Utils.log("Error in wipe: "+e);
    }
  },

  
  
  
  saveTab: function Storage_saveTab(tab, data) {
    Utils.assert(tab, "tab");

    this._sessionStore.setTabValue(tab, this.TAB_DATA_IDENTIFIER,
      JSON.stringify(data));
  },

  
  
  
  getTabData: function Storage_getTabData(tab) {
    Utils.assert(tab, "tab");

    let existingData = null;

    try {
      let tabData = this._sessionStore.getTabValue(tab, this.TAB_DATA_IDENTIFIER);
      if (tabData != "")
        existingData = JSON.parse(tabData);
    } catch (e) {
      
      Utils.log(e);
    }

    return existingData;
  },

  
  
  
  getTabState: function Storage_getTabState(tab) {
    Utils.assert(tab, "tab");
    let tabState;

    try {
      tabState = JSON.parse(this._sessionStore.getTabState(tab));
    } catch (e) {}

    return tabState;
  },

  
  
  
  saveGroupItem: function Storage_saveGroupItem(win, data) {
    var id = data.id;
    var existingData = this.readGroupItemData(win);
    existingData[id] = data;
    this._sessionStore.setWindowValue(win, this.GROUP_DATA_IDENTIFIER,
      JSON.stringify(existingData));
  },

  
  
  
  deleteGroupItem: function Storage_deleteGroupItem(win, id) {
    var existingData = this.readGroupItemData(win);
    delete existingData[id];
    this._sessionStore.setWindowValue(win, this.GROUP_DATA_IDENTIFIER,
      JSON.stringify(existingData));
  },

  
  
  
  readGroupItemData: function Storage_readGroupItemData(win) {
    var existingData = {};
    let data;
    try {
      data = this._sessionStore.getWindowValue(win, this.GROUP_DATA_IDENTIFIER);
      if (data)
        existingData = JSON.parse(data);
    } catch (e) {
      
      Utils.log("Error in readGroupItemData: "+e, data);
    }
    return existingData;
  },

  
  
  
  readWindowBusyState: function Storage_readWindowBusyState(win) {
    let state;

    try {
      let data = this._sessionStore.getWindowState(win);
      if (data)
        state = JSON.parse(data);
    } catch (e) {
      Utils.log("Error while parsing window state");
    }

    return (state && state.windows[0].busy);
  },

  
  
  
  saveGroupItemsData: function Storage_saveGroupItemsData(win, data) {
    this.saveData(win, this.GROUPS_DATA_IDENTIFIER, data);
  },

  
  
  
  readGroupItemsData: function Storage_readGroupItemsData(win) {
    return this.readData(win, this.GROUPS_DATA_IDENTIFIER);
  },

  
  
  
  saveUIData: function Storage_saveUIData(win, data) {
    this.saveData(win, this.UI_DATA_IDENTIFIER, data);
  },

  
  
  
  readUIData: function Storage_readUIData(win) {
    return this.readData(win, this.UI_DATA_IDENTIFIER);
  },

  
  
  
  saveVisibilityData: function Storage_saveVisibilityData(win, data) {
    this._sessionStore.setWindowValue(
      win, win.TabView.VISIBILITY_IDENTIFIER, data);
  },

  
  
  
  saveData: function Storage_saveData(win, id, data) {
    try {
      this._sessionStore.setWindowValue(win, id, JSON.stringify(data));
    } catch (e) {
      Utils.log("Error in saveData: "+e);
    }
  },

  
  
  
  readData: function Storage_readData(win, id) {
    var existingData = {};
    try {
      var data = this._sessionStore.getWindowValue(win, id);
      if (data)
        existingData = JSON.parse(data);
    } catch (e) {
      Utils.log("Error in readData: "+e);
    }

    return existingData;
  }
};

