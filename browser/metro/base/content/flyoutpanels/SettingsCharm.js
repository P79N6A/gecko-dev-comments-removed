



var SettingsCharm = {
  _entries: new Map(),
  _nextId: 0,

  




  addEntry: function addEntry(aEntry) {
    try {
      let id = Services.metro.addSettingsPanelEntry(aEntry.label);
      this._entries.set(id, aEntry);
    } catch (e) {
      
      Cu.reportError(e);
    }
  },

  init: function SettingsCharm_init() {
    Services.obs.addObserver(this, "metro-settings-entry-selected", false);

    
    this.addEntry({
        label: Strings.browser.GetStringFromName("optionsCharm"),
        onselected: function() FlyoutPanelsUI.show('PrefsFlyoutPanel')
    });

    
    this.addEntry({
        label: Strings.browser.GetStringFromName("searchCharm"),
        onselected: function() FlyoutPanelsUI.show('SearchFlyoutPanel')
    });











    
    this.addEntry({
        label: Strings.browser.GetStringFromName("aboutCharm1"),
        onselected: function() FlyoutPanelsUI.show('AboutFlyoutPanel')
    });

    
    this.addEntry({
        label: Strings.browser.GetStringFromName("helpOnlineCharm"),
        onselected: function() {
          let url = Services.urlFormatter.formatURLPref("app.support.baseURL");
          BrowserUI.addAndShowTab(url, Browser.selectedTab);
        }
    });
  },

  observe: function SettingsCharm_observe(aSubject, aTopic, aData) {
    if (aTopic == "metro-settings-entry-selected") {
      let entry = this._entries.get(parseInt(aData, 10));
      if (entry)
        entry.onselected();
    }
  },

  uninit: function SettingsCharm_uninit() {
    Services.obs.removeObserver(this, "metro-settings-entry-selected");
  }
};
