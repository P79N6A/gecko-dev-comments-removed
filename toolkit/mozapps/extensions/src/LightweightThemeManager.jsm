



































var EXPORTED_SYMBOLS = ["LightweightThemeManager"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const MAX_USED_THEMES_COUNT = 8;
const MAX_PREVIEW_SECONDS = 30;
const PERSIST_ENABLED = true;
const PERSIST_BYPASS_CACHE = false;
const PERSIST_FILES = {
  headerURL: "lightweighttheme-header",
  footerURL: "lightweighttheme-footer"
};

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

__defineGetter__("_ioService", function () {
  delete this._ioService;
  return this._ioService =
         Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
});

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
        var data = this.usedThemes[0];
    } catch (e) {}

    if (!data)
      return null;

    if (PERSIST_ENABLED) {
      for (let key in PERSIST_FILES) {
        try {
          if (data[key] && _prefs.getBoolPref("persisted." + key))
            data[key] = _getLocalImageURI(PERSIST_FILES[key]).spec;
        } catch (e) {}
      }
    }

    return data;
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

    if (PERSIST_ENABLED && aData)
      _persistImages(aData);

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
};

function _persistImages(aData) {
  function onSuccess(key) function () {
    let current = LightweightThemeManager.currentTheme;
    if (current && current.id == aData.id)
      _prefs.setBoolPref("persisted." + key, true);
  };

  for (let key in PERSIST_FILES) {
    _prefs.setBoolPref("persisted." + key, false);
    if (aData[key])
      _persistImage(aData[key], PERSIST_FILES[key], onSuccess(key));
  }
}

function _getLocalImageURI(localFileName) {
  var localFile = Cc["@mozilla.org/file/directory_service;1"]
                     .getService(Ci.nsIProperties)
                     .get("ProfD", Ci.nsILocalFile);
  localFile.append(localFileName);
  return _ioService.newFileURI(localFile);
}

function _persistImage(sourceURL, localFileName, callback) {
  var targetURI = _getLocalImageURI(localFileName);
  var sourceURI = _ioService.newURI(sourceURL, null, null);

  var persist = Cc["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"]
                  .createInstance(Ci.nsIWebBrowserPersist);

  persist.persistFlags =
    Ci.nsIWebBrowserPersist.PERSIST_FLAGS_REPLACE_EXISTING_FILES |
    Ci.nsIWebBrowserPersist.PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION |
    (PERSIST_BYPASS_CACHE ?
       Ci.nsIWebBrowserPersist.PERSIST_FLAGS_BYPASS_CACHE :
       Ci.nsIWebBrowserPersist.PERSIST_FLAGS_FROM_CACHE);

  persist.progressListener = new _persistProgressListener(callback);

  persist.saveURI(sourceURI, null, null, null, null, targetURI);
}

function _persistProgressListener(callback) {
  this.onLocationChange = function () {};
  this.onProgressChange = function () {};
  this.onStatusChange   = function () {};
  this.onSecurityChange = function () {};
  this.onStateChange    = function (aWebProgress, aRequest, aStateFlags, aStatus) {
    if (aRequest &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
      try {
        if (aRequest.QueryInterface(Ci.nsIHttpChannel).requestSucceeded) {
          
          callback();
          return;
        }
      } catch (e) { }
      
    }
  };
}
