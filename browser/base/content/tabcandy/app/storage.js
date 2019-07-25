








































Storage = {
  GROUP_DATA_IDENTIFIER:  "tabcandy-group",
  GROUPS_DATA_IDENTIFIER: "tabcandy-groups",
  TAB_DATA_IDENTIFIER:    "tabcandy-tab",
  UI_DATA_IDENTIFIER:    "tabcandy-ui",

  
  init: function() {
    this._sessionStore = Components.classes["@mozilla.org/browser/sessionstore;1"]
                                   .getService(Components.interfaces.nsISessionStore);
  },

  
  wipe: function() {
    try {
      var win = Utils.getCurrentWindow();
      
      
      if (!win)
      	win = Utils.getCurrentWindow();
      
      var self = this;
      
      
      Tabs.forEach(function(tab) {
        self.saveTab(tab.raw, null);
      });
      
      
      this.saveGroupsData(win, {});
      this.saveUIData(win, {});
      
      this._sessionStore.setWindowValue(win, this.GROUP_DATA_IDENTIFIER,
        JSON.stringify({}));
    } catch (e) {
      Utils.log("Error in wipe: "+e);
    }
  },
  
  
  saveTab: function(tab, data) {
    Utils.assert('tab', tab);

    this._sessionStore.setTabValue(tab, this.TAB_DATA_IDENTIFIER,
      JSON.stringify(data));
  },

  
  getTabData: function(tab) {
    Utils.assert('tab', tab);

    var existingData = null;
    try {

      var tabData = this._sessionStore.getTabValue(tab, this.TAB_DATA_IDENTIFIER);
      if (tabData != "") {
        existingData = JSON.parse(tabData);
      }
    } catch (e) {
      
      Utils.log(e);
    }
    

    return existingData;
  },

  
  saveGroup: function(win, data) {
    var id = data.id;
    var existingData = this.readGroupData(win);
    existingData[id] = data;
    this._sessionStore.setWindowValue(win, this.GROUP_DATA_IDENTIFIER,
      JSON.stringify(existingData));
  },

  
  deleteGroup: function(win, id) {
    var existingData = this.readGroupData(win);
    delete existingData[id];
    this._sessionStore.setWindowValue(win, this.GROUP_DATA_IDENTIFIER,
      JSON.stringify(existingData));
  },

  
  readGroupData: function(win) {
    var existingData = {};
    try {

      existingData = JSON.parse(
        this._sessionStore.getWindowValue(win, this.GROUP_DATA_IDENTIFIER)
      );
    } catch (e) {
      
      Utils.log("Error in readGroupData: "+e);
    }
    return existingData;
  },

  
  saveGroupsData: function(win, data) {
    this.saveData(win, this.GROUPS_DATA_IDENTIFIER, data);
  },

  
  readGroupsData: function(win) {
    return this.readData(win, this.GROUPS_DATA_IDENTIFIER);
  },
  
  
  saveUIData: function(win, data) {
    this.saveData(win, this.UI_DATA_IDENTIFIER, data);
  },

  
  readUIData: function(win) {
    return this.readData(win, this.UI_DATA_IDENTIFIER);
  },
  
  
  saveData: function(win, id, data) {
    try {
      this._sessionStore.setWindowValue(win, id, JSON.stringify(data));
    } catch (e) {
      Utils.log("Error in saveData: "+e);
    }
    

  },

  
  readData: function(win, id) {
    var existingData = {};
    try {
      var data = this._sessionStore.getWindowValue(win, id);
      if(data)
        existingData = JSON.parse(data);
    } catch (e) {
      Utils.log("Error in readData: "+e);
    }
    

    return existingData;
  }
};

Storage.init();

