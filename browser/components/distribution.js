



































EXPORTED_SYMBOLS = [ "DistributionCustomizer" ];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

function DistributionCustomizer() {
  this._distroDir = this._dirSvc.get("XCurProcD", Ci.nsIFile);
  this._distroDir.append("distribution");

  let iniFile = this._distroDir.clone();
  iniFile.append("distribution.ini");
  this._iniExists = iniFile.exists();

  if (!this._iniExists)
    return;

  this._ini = Cc["@mozilla.org/xpcom/ini-parser-factory;1"].
    getService(Ci.nsIINIParserFactory).createINIParser(iniFile);

  this._prefs = this._prefSvc.getBranch(null);
  this._locale = this._prefs.getCharPref("general.useragent.locale");

}
DistributionCustomizer.prototype = {
  __bmSvc: null,
  get _bmSvc() {
    if (!this.__bmSvc)
      this.__bmSvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                   getService(Ci.nsINavBookmarksService);
    return this.__bmSvc;
  },

  __annoSvc: null,
  get _annoSvc() {
    if (!this.__annoSvc)
      this.__annoSvc = Cc["@mozilla.org/browser/annotation-service;1"].
                   getService(Ci.nsIAnnotationService);
    return this.__annoSvc;
  },

  __livemarkSvc: null,
  get _livemarkSvc() {
    if (!this.__livemarkSvc)
      this.__livemarkSvc = Cc["@mozilla.org/browser/livemark-service;2"].
                   getService(Ci.nsILivemarkService);
    return this.__livemarkSvc;
  },

  __dirSvc: null,
  get _dirSvc() {
    if (!this.__dirSvc)
      this.__dirSvc = Cc["@mozilla.org/file/directory_service;1"].
        getService(Ci.nsIProperties);
    return this.__dirSvc;
  },

  __prefSvc: null,
  get _prefSvc() {
    if (!this.__prefSvc)
      this.__prefSvc = Cc["@mozilla.org/preferences-service;1"].
        getService(Ci.nsIPrefService);
    return this.__prefSvc;
  },

  __iosvc: null,
  get _iosvc() {
    if (!this.__iosvc)
      this.__iosvc = Cc["@mozilla.org/network/io-service;1"].
                   getService(Ci.nsIIOService);
    return this.__iosvc;
  },

  _locale: "en-US",
  _distroDir: null,
  _iniExists: false,
  _ini: null,


  _makeURI: function DIST__makeURI(spec) {
    return this._iosvc.newURI(spec, null, null);
  },
  _parseBookmarksSection: function DIST_parseBookmarksSection(parentId, section) {
    let keys = [];
    for (let i in enumerate(this._ini.getKeys(section)))
      keys.push(i);
    keys.sort();
    let items = {};
    let defaultItemId = -1;
    let maxItemId = -1;

    for (let i = 0; i < keys.length; i++) {
      let m = /^item\.(\d+)\.(\w+)\.?(\w*)/.exec(keys[i]);
      if (m) {
        let [foo, iid, iprop, ilocale] = m;

        if (ilocale)
          continue;

        if (!items[iid])
          items[iid] = {};
        if (keys.indexOf(keys[i] + "." + this._locale) >= 0) {
          items[iid][iprop] = this._ini.getString(section, keys[i] + "." +
                                                  this._locale);
        } else {
          items[iid][iprop] = this._ini.getString(section, keys[i]);
        }

        if (iprop == "type" && items[iid]["type"] == "default")
          defaultItemId = iid;

        if (maxItemId < iid)
          maxItemId = iid;
      } else {
        dump("Key did not match: " + keys[i] + "\n");
      }
    }

    let prependIndex = 0;
    for (let iid = 0; iid <= maxItemId; iid++) {
      if (!items[iid])
        continue;

      let index = -1;
      let newId;

      switch (items[iid]["type"]) {
      case "default":
        break;

      case "folder":
        if (iid < defaultItemId)
          index = prependIndex++;

        newId = this._bmSvc.createFolder(parentId, items[iid]["title"], index);

        this._parseBookmarksSection(newId, "BookmarksFolder-" +
                                    items[iid]["folderId"]);

        if (items[iid]["description"])
          this._annoSvc.setItemAnnotation(newId, "bookmarkProperties/description",
                                          items[iid]["description"], 0,
                                          this._annoSvc.EXPIRE_NEVER);

        break;

      case "separator":
        if (iid < defaultItemId)
          index = prependIndex++;
        this._bmSvc.insertSeparator(parentId, index);
        break;

      case "livemark":
        if (iid < defaultItemId)
          index = prependIndex++;

        newId = this._livemarkSvc.
          createLivemark(parentId,
                         items[iid]["title"],
                         this._makeURI(items[iid]["siteLink"]),
                         this._makeURI(items[iid]["feedLink"]),
                         index);
        break;

      case "bookmark":
      default:
        if (iid < defaultItemId)
          index = prependIndex++;

        newId = this._bmSvc.insertBookmark(parentId,
                                           this._makeURI(items[iid]["link"]),
                                           index, items[iid]["title"]);

        if (items[iid]["description"])
          this._annoSvc.setItemAnnotation(newId, "bookmarkProperties/description",
                                          items[iid]["description"], 0,
                                          this._annoSvc.EXPIRE_NEVER);

        break;
      }
    }
  },
  applyCustomizations: function DIST_applyCustomizations() {
    if (!this._iniExists)
      return;

    
    
    
    this._prefSvc.QueryInterface(Ci.nsIObserver);
    this._prefSvc.observe(null, "reload-default-prefs", null);

    let sections = enumToObject(this._ini.getSections());

    
    
    if (!sections["Global"])
      return;
    let globalPrefs = enumToObject(this._ini.getKeys("Global"));
    if (!(globalPrefs["id"] && globalPrefs["version"] && globalPrefs["about"]))
      return;

    let bmProcessed = false;
    let bmProcessedPref;

    try {
        bmProcessedPref = this._ini.getString("Global",
                                              "bookmarks.initialized.pref");
    } catch (e) {
      bmProcessedPref = "distribution." +
        this._ini.getString("Global", "id") + ".bookmarksProcessed";
    }

    try {
      bmProcessed = this._prefs.getBoolPref(bmProcessedPref);
    } catch (e) {}

    if (!bmProcessed) {
      if (sections["BookmarksMenu"])
        this._parseBookmarksSection(this._bmSvc.bookmarksMenuFolder,
                                    "BookmarksMenu");
      if (sections["BookmarksToolbar"])
        this._parseBookmarksSection(this._bmSvc.toolbarFolder,
                                    "BookmarksToolbar");
      this._prefs.setBoolPref(bmProcessedPref, true);
    }
  },
  applyPrefDefaults: function DIST_applyPrefDefaults() {
    if (!this._iniExists)
      return;

    let sections = enumToObject(this._ini.getSections());

    
    if (!sections["Global"])
      return;
    let globalPrefs = enumToObject(this._ini.getKeys("Global"));
    if (!(globalPrefs["id"] && globalPrefs["version"] && globalPrefs["about"]))
      return;

    let defaults = this._prefSvc.getDefaultBranch(null);

    
    

    defaults.setCharPref("distribution.id", this._ini.getString("Global", "id"));
    defaults.setCharPref("distribution.version",
                         this._ini.getString("Global", "version"));

    let partnerAbout = Cc["@mozilla.org/supports-string;1"].
      createInstance(Ci.nsISupportsString);
    if (globalPrefs["about." + this._locale]) {
      partnerAbout.data = this._ini.getString("Global", "about." + this._locale);
    } else {
      partnerAbout.data = this._ini.getString("Global", "about");
    }
    defaults.setComplexValue("distribution.about",
                             Ci.nsISupportsString, partnerAbout);

    if (sections["Preferences"]) {
      for (let key in enumerate(this._ini.getKeys("Preferences"))) {
        try {
          let value = eval(this._ini.getString("Preferences", key));
          switch (typeof value) {
          case "boolean":
            defaults.setBoolPref(key, value);
            break;
          case "number":
            defaults.setIntPref(key, value);
            break;
          case "string":
            defaults.setCharPref(key, value);
            break;
          case "undefined":
            defaults.setCharPref(key, value);
            break;
          }
        } catch (e) {  }
      }
    }

    
    
    

    let localizedStr = Cc["@mozilla.org/pref-localizedstring;1"].
      createInstance(Ci.nsIPrefLocalizedString);

    if (sections["LocalizablePreferences"]) {
      for (let key in enumerate(this._ini.getKeys("LocalizablePreferences"))) {
        try {
          let value = eval(this._ini.getString("LocalizablePreferences", key));
          value = value.replace("%LOCALE%", this._locale, "g");
          localizedStr.data = "data:text/plain," + key + "=" + value;
          defaults.setComplexValue(key, Ci.nsIPrefLocalizedString, localizedStr);
        } catch (e) {  }
      }
    }

    if (sections["LocalizablePreferences-" + this._locale]) {
      for (let key in enumerate(this._ini.getKeys("LocalizablePreferences-" + this._locale))) {
        try {
          let value = eval(this._ini.getString("LocalizablePreferences-" + this._locale, key));
          localizedStr.data = "data:text/plain," + key + "=" + value;
          defaults.setComplexValue(key, Ci.nsIPrefLocalizedString, localizedStr);
        } catch (e) {  }
      }
    }
  }
};

function enumerate(UTF8Enumerator) {
  while (UTF8Enumerator.hasMore())
    yield UTF8Enumerator.getNext();
}

function enumToObject(UTF8Enumerator) {
  let ret = {};
  for (let i in enumerate(UTF8Enumerator))
    ret[i] = 1;
  return ret;
}
