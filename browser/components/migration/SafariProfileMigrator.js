



"use strict";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource:///modules/MigrationUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Downloads",
                                  "resource://gre/modules/Downloads.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PropertyListUtils",
                                  "resource://gre/modules/PropertyListUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FormHistory",
                                  "resource://gre/modules/FormHistory.jsm");

function Bookmarks(aBookmarksFile) {
  this._file = aBookmarksFile;
}
Bookmarks.prototype = {
  type: MigrationUtils.resourceTypes.BOOKMARKS,

  migrate: function B_migrate(aCallback) {
    return Task.spawn(function* () {
      let dict = yield new Promise(resolve =>
        PropertyListUtils.read(this._file, resolve)
      );
      if (!dict)
        throw new Error("Could not read Bookmarks.plist");
      let children = dict.get("Children");
      if (!children)
        throw new Error("Invalid Bookmarks.plist format");

      let collection = dict.get("Title") == "com.apple.ReadingList" ?
        this.READING_LIST_COLLECTION : this.ROOT_COLLECTION;
      yield this._migrateCollection(children, collection);
    }.bind(this)).then(() => aCallback(true),
                        e => { Cu.reportError(e); aCallback(false) });
  },

  
  ROOT_COLLECTION:         0,
  MENU_COLLECTION:         1,
  TOOLBAR_COLLECTION:      2,
  READING_LIST_COLLECTION: 3,

  







  _migrateCollection: Task.async(function* (aEntries, aCollection) {
    
    
    
    

    let entriesFiltered = [];
    if (aCollection == this.ROOT_COLLECTION) {
      for (let entry of aEntries) {
        let type = entry.get("WebBookmarkType");
        if (type == "WebBookmarkTypeList" && entry.has("Children")) {
          let title = entry.get("Title");
          let children = entry.get("Children");
          if (title == "BookmarksBar")
            yield this._migrateCollection(children, this.TOOLBAR_COLLECTION);
          else if (title == "BookmarksMenu")
            yield this._migrateCollection(children, this.MENU_COLLECTION);
          else if (title == "com.apple.ReadingList")
            yield this._migrateCollection(children, this.READING_LIST_COLLECTION);
          else if (entry.get("ShouldOmitFromUI") !== true)
            entriesFiltered.push(entry);
        }
        else if (type == "WebBookmarkTypeLeaf") {
          entriesFiltered.push(entry);
        }
      }
    }
    else {
      entriesFiltered = aEntries;
    }

    if (entriesFiltered.length == 0)
      return;

    let folderGuid = -1;
    switch (aCollection) {
      case this.ROOT_COLLECTION: {
        
        
        
        
        
        
        folderGuid = PlacesUtils.bookmarks.unfiledGuid;
        break;
      }
      case this.MENU_COLLECTION: {
        folderGuid = PlacesUtils.bookmarks.menuGuid;
        if (!MigrationUtils.isStartupMigration) {
          folderGuid =
            yield MigrationUtils.createImportedBookmarksFolder("Safari", folderGuid);
        }
        break;
      }
      case this.TOOLBAR_COLLECTION: {
        folderGuid = PlacesUtils.bookmarks.toolbarGuid;
        if (!MigrationUtils.isStartupMigration) {
          folderGuid =
            yield MigrationUtils.createImportedBookmarksFolder("Safari", folderGuid);
        }
        break;
      }
      case this.READING_LIST_COLLECTION: {
        
        
        
        folderGuid = (yield PlacesUtils.bookmarks.insert({
            parentGuid: PlacesUtils.bookmarks.menuGuid,
            type: PlacesUtils.bookmarks.TYPE_FOLDER,
            title: MigrationUtils.getLocalizedString("importedSafariReadingList"),
          })).guid;
        break;
      }
      default:
        throw new Error("Unexpected value for aCollection!");
    }
    if (folderGuid == -1)
      throw new Error("Invalid folder GUID");

    yield this._migrateEntries(entriesFiltered, folderGuid);
  }),

  
  
  _migrateEntries: Task.async(function* (entries, parentGuid) {
    for (let entry of entries) {
      let type = entry.get("WebBookmarkType");
      if (type == "WebBookmarkTypeList" && entry.has("Children")) {
        let title = entry.get("Title");
        let newFolderGuid = (yield PlacesUtils.bookmarks.insert({
          parentGuid, type: PlacesUtils.bookmarks.TYPE_FOLDER, title
        })).guid;

        
        if (entry.has("Children"))
          yield this._migrateEntries(entry.get("Children"), newFolderGuid, false);
      }
      else if (type == "WebBookmarkTypeLeaf" && entry.has("URLString")) {
        let title;
        if (entry.has("URIDictionary"))
          title = entry.get("URIDictionary").get("title");

        try {
          yield PlacesUtils.bookmarks.insert({
            parentGuid, url: entry.get("URLString"), title
          });
        } catch(ex) {
          Cu.reportError("Invalid Safari bookmark: " + ex);
        }
      }
    }
  })
};

function History(aHistoryFile) {
  this._file = aHistoryFile;
}
History.prototype = {
  type: MigrationUtils.resourceTypes.HISTORY,

  
  
  
  _parseCocoaDate: function H___parseCocoaDate(aCocoaDateStr) {
    let asDouble = parseFloat(aCocoaDateStr);
    if (!isNaN(asDouble)) {
      
      let date = new Date("1 January 2001, GMT");
      date.setMilliseconds(asDouble * 1000);
      return date * 1000;
    }
    return 0;
  },

  migrate: function H_migrate(aCallback) {
    PropertyListUtils.read(this._file, function migrateHistory(aDict) {
      try {
        if (!aDict)
          throw new Error("Could not read history property list");
        if (!aDict.has("WebHistoryDates"))
          throw new Error("Unexpected history-property list format");

        
        
        let transType = PlacesUtils.history.TRANSITION_LINK;

        let places = [];
        let entries = aDict.get("WebHistoryDates");
        for (let entry of entries) {
          if (entry.has("lastVisitedDate")) {
            let visitDate = this._parseCocoaDate(entry.get("lastVisitedDate"));
            try {
              places.push({ uri: NetUtil.newURI(entry.get("")),
                            title: entry.get("title"),
                            visits: [{ transitionType: transType,
                                       visitDate: visitDate }] });
            }
            catch(ex) {
              
              
              Cu.reportError(ex)
            }
          }
        }
        if (places.length > 0) {
          PlacesUtils.asyncHistory.updatePlaces(places, {
            _success: false,
            handleResult: function() {
              
              this._success = true;
            },
            handleError: function() {},
            handleCompletion: function() {
              aCallback(this._success);
            }
          });
        }
        else {
          aCallback(false);
        }
      }
      catch(ex) {
        Cu.reportError(ex);
        aCallback(false);
      }
    }.bind(this));
  }
};









function MainPreferencesPropertyList(aPreferencesFile) {
  this._file = aPreferencesFile;
  this._callbacks = [];
}
MainPreferencesPropertyList.prototype = {
  


  read: function MPPL_read(aCallback) {
    if ("_dict" in this) {
      aCallback(this._dict);
      return;
    }

    let alreadyReading = this._callbacks.length > 0;
    this._callbacks.push(aCallback);
    if (!alreadyReading) {
      PropertyListUtils.read(this._file, function readPrefs(aDict) {
        this._dict = aDict;
        for (let callback of this._callbacks) {
          try {
            callback(aDict);
          }
          catch(ex) {
            Cu.reportError(ex);
          }
        }
        this._callbacks.splice(0);
      }.bind(this));
    }
  },

  
  
  _readSync: function MPPL__readSync() {
    if ("_dict" in this)
      return this._dict;
  
    let inputStream = Cc["@mozilla.org/network/file-input-stream;1"].
                      createInstance(Ci.nsIFileInputStream);
    inputStream.init(this._file, -1, -1, 0);
    let binaryStream = Cc["@mozilla.org/binaryinputstream;1"].
                       createInstance(Ci.nsIBinaryInputStream);
    binaryStream.setInputStream(inputStream);
    let bytes = binaryStream.readByteArray(inputStream.available());
    this._dict = PropertyListUtils._readFromArrayBufferSync(
      new Uint8Array(bytes).buffer);
    return this._dict;
  }
};

function Preferences(aMainPreferencesPropertyListInstance) {
  this._mainPreferencesPropertyList = aMainPreferencesPropertyListInstance;
}
Preferences.prototype = {
  type: MigrationUtils.resourceTypes.SETTINGS,

  migrate: function MPR_migrate(aCallback) {
    this._mainPreferencesPropertyList.read(aDict => {
      Task.spawn(function* () {
        if (!aDict)
          throw new Error("Could not read preferences file");

        this._dict = aDict;

        let invert = function(webkitVal) !webkitVal;
        this._set("AutoFillPasswords", "signon.rememberSignons");
        this._set("OpenNewTabsInFront", "browser.tabs.loadInBackground", invert);
        this._set("WebKitJavaScriptCanOpenWindowsAutomatically",
                   "dom.disable_open_during_load", invert);

        
        this._set("WebContinuousSpellCheckingEnabled",
                  "layout.spellcheckDefault", Number);

        
        
        
        
        
        
        this._set("WebKitDisplayImagesKey", "permissions.default.image",
                  function(webkitVal) webkitVal ? 1 : 2);

#ifdef XP_WIN
        
        
        
        
        
        
        this._set("WebKitCookieStorageAcceptPolicy",
          "network.cookie.cookieBehavior",
          function(webkitVal) webkitVal == 0 ? 0 : webkitVal == 1 ? 2 : 1);
#endif

        this._migrateFontSettings();
        yield this._migrateDownloadsFolder();

      }.bind(this)).then(() => aCallback(true), ex => {
        Cu.reportError(ex);
        aCallback(false);
      }).catch(Cu.reportError);
    });
  },

  














  _set: function MPR_set(aSafariKey, aMozPref, aConvertFunction) {
    if (this._dict.has(aSafariKey)) {
      let safariVal = this._dict.get(aSafariKey);
      let mozVal = aConvertFunction !== undefined ?
                   aConvertFunction(safariVal) : safariVal;
      switch (typeof(mozVal)) {
        case "string":
          Services.prefs.setCharPref(aMozPref, mozVal);
          break;
        case "number":
          Services.prefs.setIntPref(aMozPref, mozVal);
          break;
        case "boolean":
          Services.prefs.setBoolPref(aMozPref, mozVal);
          break;
        case "undefined":
          return false;
        default:
          throw new Error("Unexpected value type: " + typeof(mozVal));
      }
    }
    return true;
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  _migrateFontSettings: function MPR__migrateFontSettings() {
    
    
    if (this._dict.has("WebKitMinimumFontSize")) {
      let minimumSize = this._dict.get("WebKitMinimumFontSize");
      if (typeof(minimumSize) == "number") {
        let prefs = Services.prefs.getChildList("font.minimum-size");
        for (let pref of prefs) {
          Services.prefs.setIntPref(pref, minimumSize);
        }
      }
      else {
        Cu.reportError("WebKitMinimumFontSize was set to an invalid value: " +
                       minimumSize);
      }
    }

    
    
    let lang = this._getLocaleLangGroup();

    let anySet = false;
    let fontType = Services.prefs.getCharPref("font.default." + lang);
    anySet |= this._set("WebKitFixedFont", "font.name.monospace." + lang);
    anySet |= this._set("WebKitDefaultFixedFontSize", "font.size.fixed." + lang);
    anySet |= this._set("WebKitStandardFont",
                        "font.name." + fontType + "." + lang);
    anySet |= this._set("WebKitDefaultFontSize", "font.size.variable." + lang);

    
    
    if (anySet)
      Services.prefs.setCharPref("font.language.group", lang);
  },

  
  _getLocaleLangGroup: function MPR__getLocaleLangGroup() {
    let locale = Services.locale.getLocaleComponentForUserAgent();

    
    let localeLangGroup = "x-unicode";
    let bundle = Services.strings.createBundle(
      "resource://gre/res/langGroups.properties");
    try {
      localeLangGroup = bundle.GetStringFromName(locale);
    }
    catch(ex) {
      let hyphenAt = locale.indexOf("-");
      if (hyphenAt != -1) {
        try {
          localeLangGroup = bundle.GetStringFromName(locale.substr(0, hyphenAt));
        }
        catch(ex2) { }
      }
    }
    return localeLangGroup;
  },

  _migrateDownloadsFolder: Task.async(function* () {
    
    
    let key;
    if (this._dict.has("DownloadsPath"))
      key = "DownloadsPath";
    else if (this._dict.has("DownloadPath"))
      key = "DownloadPath";
    else
      return;

    let downloadsFolder = FileUtils.File(this._dict.get(key));

    
    
    
    let folderListVal = 2;
    if (downloadsFolder.equals(FileUtils.getDir("Desk", []))) {
      folderListVal = 0;
    }
    else {
      let systemDownloadsPath = yield Downloads.getSystemDownloadsDirectory();
      let systemDownloadsFolder = FileUtils.File(systemDownloadsPath);
      if (downloadsFolder.equals(systemDownloadsFolder))
        folderListVal = 1;
    }
    Services.prefs.setIntPref("browser.download.folderList", folderListVal);
    Services.prefs.setComplexValue("browser.download.dir", Ci.nsILocalFile,
                                   downloadsFolder);
  }),
};

function SearchStrings(aMainPreferencesPropertyListInstance) {
  this._mainPreferencesPropertyList = aMainPreferencesPropertyListInstance;
}
SearchStrings.prototype = {
  type: MigrationUtils.resourceTypes.OTHERDATA,

  migrate: function SS_migrate(aCallback) {
    this._mainPreferencesPropertyList.read(MigrationUtils.wrapMigrateFunction(
      function migrateSearchStrings(aDict) {
        if (!aDict)
          throw new Error("Could not get preferences dictionary");

        if (aDict.has("RecentSearchStrings")) {
          let recentSearchStrings = aDict.get("RecentSearchStrings");
          if (recentSearchStrings && recentSearchStrings.length > 0) {
            let changes = [{op: "add",
                            fieldname: "searchbar-history",
                            value: searchString}
                           for (searchString of recentSearchStrings)];
            FormHistory.update(changes);
          }
        }
      }.bind(this), aCallback));
  }
};

#ifdef XP_MACOSX



function WebFoundationCookieBehavior(aWebFoundationFile) {
  this._file = aWebFoundationFile;
}
WebFoundationCookieBehavior.prototype = {
  type: MigrationUtils.resourceTypes.SETTINGS,

  migrate: function WFPL_migrate(aCallback) {
    PropertyListUtils.read(this._file, MigrationUtils.wrapMigrateFunction(
      function migrateCookieBehavior(aDict) {
        if (!aDict)
          throw new Error("Could not read com.apple.WebFoundation.plist");

        if (aDict.has("NSHTTPAcceptCookies")) {
          
          
          
          
          let acceptCookies = aDict.get("NSHTTPAcceptCookies");
          let cookieValue = acceptCookies == "never" ? 2 :
                            acceptCookies == "current page" ? 1 : 0;
          Services.prefs.setIntPref("network.cookie.cookieBehavior",
                                    cookieValue);
        }
      }.bind(this), aCallback));
  }
};
#endif

function SafariProfileMigrator() {
}

SafariProfileMigrator.prototype = Object.create(MigratorPrototype);

SafariProfileMigrator.prototype.getResources = function SM_getResources() {
  let profileDir =
#ifdef XP_MACOSX
    FileUtils.getDir("ULibDir", ["Safari"], false);
#else
    FileUtils.getDir("AppData", ["Apple Computer", "Safari"], false);
#endif
  if (!profileDir.exists())
    return null;

  let resources = [];
  let pushProfileFileResource = function(aFileName, aConstructor) {
    let file = profileDir.clone();
    file.append(aFileName);
    if (file.exists())
      resources.push(new aConstructor(file));
  };

  pushProfileFileResource("History.plist", History);
  pushProfileFileResource("Bookmarks.plist", Bookmarks);
  
  
  
  
  
  
  pushProfileFileResource("ReadingList.plist", Bookmarks);

  let prefsDir = 
#ifdef XP_MACOSX
    FileUtils.getDir("UsrPrfs", [], false);
#else
    FileUtils.getDir("AppData", ["Apple Computer", "Preferences"], false);
#endif

  let prefs = this.mainPreferencesPropertyList;
  if (prefs) {
    resources.push(new Preferences(prefs));
    resources.push(new SearchStrings(prefs));
  }

#ifdef XP_MACOSX
  
  
  let wfFile = FileUtils.getFile("UsrPrfs", ["com.apple.WebFoundation.plist"]);
  if (wfFile.exists())
    resources.push(new WebFoundationCookieBehavior(wfFile));
#endif

  return resources;
};

Object.defineProperty(SafariProfileMigrator.prototype, "mainPreferencesPropertyList", {
  get: function get_mainPreferencesPropertyList() {
    if (this._mainPreferencesPropertyList === undefined) {
      let file = 
#ifdef XP_MACOSX
        FileUtils.getDir("UsrPrfs", [], false);
#else
        FileUtils.getDir("AppData", ["Apple Computer", "Preferences"], false);
#endif
      if (file.exists()) {
        file.append("com.apple.Safari.plist");
        if (file.exists()) {
          return this._mainPreferencesPropertyList =
            new MainPreferencesPropertyList(file);
        }
      }
      return this._mainPreferencesPropertyList = null;
    }
    return this._mainPreferencesPropertyList;
  }
});

Object.defineProperty(SafariProfileMigrator.prototype, "sourceHomePageURL", {
  get: function get_sourceHomePageURL() {
    if (this.mainPreferencesPropertyList) {
      let dict = this.mainPreferencesPropertyList._readSync();
      if (dict.has("HomePage"))
        return dict.get("HomePage");
    }
    return "";
  }
});

SafariProfileMigrator.prototype.classDescription = "Safari Profile Migrator";
SafariProfileMigrator.prototype.contractID = "@mozilla.org/profile/migrator;1?app=browser&type=safari";
SafariProfileMigrator.prototype.classID = Components.ID("{4b609ecf-60b2-4655-9df4-dc149e474da1}");

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SafariProfileMigrator]);
