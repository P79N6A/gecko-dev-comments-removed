# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:





let DevEdition = {
  _prefName: "browser.devedition.theme.enabled",
  _themePrefName: "general.skins.selectedSkin",
  _lwThemePrefName: "lightweightThemes.isThemeSelected",
  _devtoolsThemePrefName: "devtools.theme",

  styleSheetLocation: "chrome://browser/skin/devedition.css",
  styleSheet: null,
  defaultThemeID: "{972ce4c6-7e08-4474-a285-3208198ce6fd}",

  init: function () {
    this._updateDevtoolsThemeAttribute();
    this._updateStyleSheetFromPrefs();

    
    
    
    Services.prefs.addObserver(this._lwThemePrefName, this, false);
    Services.prefs.addObserver(this._prefName, this, false);
    Services.prefs.addObserver(this._devtoolsThemePrefName, this, false);
    Services.obs.addObserver(this, "lightweight-theme-styling-update", false);
  },

  observe: function (subject, topic, data) {
    if (topic == "lightweight-theme-styling-update") {
      let newTheme = JSON.parse(data);
      if (!newTheme || newTheme.id === this.defaultThemeID) {
        
        this._updateStyleSheetFromPrefs();
      } else {
        
        
        this._toggleStyleSheet(false);
      }
    }

    if (topic == "nsPref:changed") {
      if (data == this._devtoolsThemePrefName) {
        this._updateDevtoolsThemeAttribute();
      } else {
        this._updateStyleSheetFromPrefs();
      }
    }
  },

  _updateDevtoolsThemeAttribute: function() {
    
    
    document.documentElement.setAttribute("devtoolstheme",
      Services.prefs.getCharPref(this._devtoolsThemePrefName));
    ToolbarIconColor.inferFromText();
    this._updateStyleSheetFromPrefs();
  },

  _updateStyleSheetFromPrefs: function() {
    let lightweightThemeSelected = false;
    try {
      lightweightThemeSelected = Services.prefs.getBoolPref(this._lwThemePrefName);
    } catch(e) {}

    let defaultThemeSelected = false;
    try {
       defaultThemeSelected = Services.prefs.getCharPref(this._themePrefName) == "classic/1.0";
    } catch(e) {}

    let deveditionThemeEnabled = Services.prefs.getBoolPref(this._prefName) &&
      !lightweightThemeSelected && defaultThemeSelected;

    this._toggleStyleSheet(deveditionThemeEnabled);
  },

  handleEvent: function(e) {
    if (e.type === "load") {
      this.styleSheet.removeEventListener("load", this);
      gBrowser.tabContainer._positionPinnedTabs();
      ToolbarIconColor.inferFromText();
      Services.obs.notifyObservers(window, "devedition-theme-state-changed", true);
    }
  },

  _toggleStyleSheet: function(deveditionThemeEnabled) {
    if (deveditionThemeEnabled && !this.styleSheet) {
      let styleSheetAttr = `href="${this.styleSheetLocation}" type="text/css"`;
      this.styleSheet = document.createProcessingInstruction(
        'xml-stylesheet', styleSheetAttr);
      this.styleSheet.addEventListener("load", this);
      document.insertBefore(this.styleSheet, document.documentElement);
      
      
    } else if (!deveditionThemeEnabled && this.styleSheet) {
      this.styleSheet.removeEventListener("load", this);
      this.styleSheet.remove();
      this.styleSheet = null;
      gBrowser.tabContainer._positionPinnedTabs();
      ToolbarIconColor.inferFromText();
      Services.obs.notifyObservers(window, "devedition-theme-state-changed", false);
    }
  },

  uninit: function () {
    Services.prefs.removeObserver(this._lwThemePrefName, this);
    Services.prefs.removeObserver(this._prefName, this);
    Services.prefs.removeObserver(this._devtoolsThemePrefName, this);
    Services.obs.removeObserver(this, "lightweight-theme-styling-update", false);
    if (this.styleSheet) {
      this.styleSheet.removeEventListener("load", this);
    }
    this.styleSheet = null;
  }
};
