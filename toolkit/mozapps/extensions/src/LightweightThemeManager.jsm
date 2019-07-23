



































var EXPORTED_SYMBOLS = ["LightweightThemeManager"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const MAX_USED_THEMES_COUNT = 8;
const MAX_PREVIEW_SECONDS = 30;

var LightweightThemeManager = {
  get usedThemes () {
    try {
      return JSON.parse(_prefs.getCharPref("usedThemes"));
    } catch (e) {
      return [];
    }
  },

  get currentTheme () {
    try {
      if (_prefs.getBoolPref("isThemeSelected"))
        return this.usedThemes[0];
    } catch (e) {}
    return null;
  },

  set currentTheme (aData) {
    let cancel = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
    cancel.data = false;
    _observerService.notifyObservers(cancel, "lightweight-theme-change-requested",
                                     JSON.stringify(aData));

    if (aData) {
      let usedThemes = _usedThemesExceptId(aData.id);
      if (cancel.data && _prefs.getBoolPref("isThemeSelected"))
        usedThemes.splice(1, 0, aData);
      else
        usedThemes.unshift(aData);
      _updateUsedThemes(usedThemes);
    }

    if (cancel.data)
      return null;

    if (_previewTimer) {
      _previewTimer.cancel();
      _previewTimer = null;
    }

    _prefs.setBoolPref("isThemeSelected", aData != null);
    _notifyWindows(aData);

    return aData;
  },

  getUsedTheme: function (aId) {
    var usedThemes = this.usedThemes;
    for (let i = 0; i < usedThemes.length; i++) {
      if (usedThemes[i].id == aId)
        return usedThemes[i];
    }
    return null;
  },

  forgetUsedTheme: function (aId) {
    var currentTheme = this.currentTheme;
    if (currentTheme && currentTheme.id == aId)
      this.currentTheme = null;

    _updateUsedThemes(_usedThemesExceptId(aId));
  },

  previewTheme: function (aData) {
    if (!aData)
      return;

    let cancel = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
    cancel.data = false;
    _observerService.notifyObservers(cancel, "lightweight-theme-preview-requested",
                                     JSON.stringify(aData));
    if (cancel.data)
      return;

    if (_previewTimer)
      _previewTimer.cancel();
    else
      _previewTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    _previewTimer.initWithCallback(_previewTimerCallback,
                                   MAX_PREVIEW_SECONDS * 1000,
                                   _previewTimer.TYPE_ONE_SHOT);

    _notifyWindows(aData);
  },

  resetPreview: function () {
    if (_previewTimer) {
      _previewTimer.cancel();
      _previewTimer = null;
      _notifyWindows(this.currentTheme);
    }
  }
};

function _usedThemesExceptId(aId)
  LightweightThemeManager.usedThemes.filter(function (t) t.id != aId);

function _updateUsedThemes(aList) {
  if (aList.length > MAX_USED_THEMES_COUNT)
    aList.length = MAX_USED_THEMES_COUNT;

  _prefs.setCharPref("usedThemes", JSON.stringify(aList));

  _observerService.notifyObservers(null, "lightweight-theme-list-changed", null);
}

function _notifyWindows(aThemeData) {
  _observerService.notifyObservers(null, "lightweight-theme-changed",
                                   JSON.stringify(aThemeData));
}

var _previewTimer;
var _previewTimerCallback = {
  notify: function () {
    LightweightThemeManager.resetPreview();
  }
}

__defineGetter__("_prefs", function () {
  delete this._prefs;
  return this._prefs =
         Cc["@mozilla.org/preferences-service;1"]
           .getService(Ci.nsIPrefService).getBranch("lightweightThemes.");
});

__defineGetter__("_observerService", function () {
  delete this._observerService;
  return this._observerService =
         Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
});
