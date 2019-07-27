



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const kMainKey = "Software\\Microsoft\\Internet Explorer\\Main";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource:///modules/MigrationUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ctypes",
                                  "resource://gre/modules/ctypes.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "WindowsRegistry",
                                  "resource://gre/modules/WindowsRegistry.jsm");




let CtypesHelpers = {
  _structs: {},
  _functions: {},
  _libs: {},

  


  initialize: function CH_initialize() {
    const WORD = ctypes.uint16_t;
    const DWORD = ctypes.uint32_t;
    const BOOL = ctypes.int;

    this._structs.SYSTEMTIME = new ctypes.StructType('SYSTEMTIME', [
      {wYear: WORD},
      {wMonth: WORD},
      {wDayOfWeek: WORD},
      {wDay: WORD},
      {wHour: WORD},
      {wMinute: WORD},
      {wSecond: WORD},
      {wMilliseconds: WORD}
    ]);

    this._structs.FILETIME = new ctypes.StructType('FILETIME', [
      {dwLowDateTime: DWORD},
      {dwHighDateTime: DWORD}
    ]);

    try {
      this._libs.kernel32 = ctypes.open("Kernel32");
      this._functions.FileTimeToSystemTime =
        this._libs.kernel32.declare("FileTimeToSystemTime",
                                    ctypes.default_abi,
                                    BOOL,
                                    this._structs.FILETIME.ptr,
                                    this._structs.SYSTEMTIME.ptr);
    } catch (ex) {
      this.finalize();
    }
  },

  


  finalize: function CH_finalize() {
    this._structs = {};
    this._functions = {};
    for each (let lib in this._libs) {
      try {
        lib.close();
      } catch (ex) {}
    }
    this._libs = {};
  },

  








  fileTimeToDate: function CH_fileTimeToDate(aTimeHi, aTimeLo) {
    let fileTime = this._structs.FILETIME();
    fileTime.dwLowDateTime = aTimeLo;
    fileTime.dwHighDateTime = aTimeHi;
    let systemTime = this._structs.SYSTEMTIME();
    let result = this._functions.FileTimeToSystemTime(fileTime.address(),
                                                      systemTime.address());
    if (result == 0)
      throw new Error(ctypes.winLastError);

    return new Date(systemTime.wYear,
                    systemTime.wMonth - 1,
                    systemTime.wDay,
                    systemTime.wHour,
                    systemTime.wMinute,
                    systemTime.wSecond,
                    systemTime.wMilliseconds);
  }
};








function hostIsIPAddress(aHost) {
  try {
    Services.eTLD.getBaseDomainFromHost(aHost);
  } catch (e if e.result == Cr.NS_ERROR_HOST_IS_IP_ADDRESS) {
    return true;
  } catch (e) {}
  return false;
}




function Bookmarks() {
}

Bookmarks.prototype = {
  type: MigrationUtils.resourceTypes.BOOKMARKS,

  get exists() !!this._favoritesFolder,

  __favoritesFolder: null,
  get _favoritesFolder() {
    if (!this.__favoritesFolder) {
      let favoritesFolder = Services.dirsvc.get("Favs", Ci.nsIFile);
      if (favoritesFolder.exists() && favoritesFolder.isReadable())
        this.__favoritesFolder = favoritesFolder;
    }
    return this.__favoritesFolder;
  },

  __toolbarFolderName: null,
  get _toolbarFolderName() {
    if (!this.__toolbarFolderName) {
      
      
      
      let folderName = WindowsRegistry.readRegKey(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                                                  "Software\\Microsoft\\Internet Explorer\\Toolbar",
                                                  "LinksFolderName");
      this.__toolbarFolderName = folderName || "Links";
    }
    return this.__toolbarFolderName;
  },

  migrate: function B_migrate(aCallback) {
    PlacesUtils.bookmarks.runInBatchMode({
      runBatched: (function migrateBatched() {
        
        let destFolderId = PlacesUtils.bookmarksMenuFolderId;
        if (!MigrationUtils.isStartupMigration) {
          destFolderId =
            MigrationUtils.createImportedBookmarksFolder("IE", destFolderId);
        }

        this._migrateFolder(this._favoritesFolder, destFolderId);

        aCallback(true);
      }).bind(this)
    }, null);
  },

  _migrateFolder: function B__migrateFolder(aSourceFolder, aDestFolderId) {
    
    
    
    let entries = aSourceFolder.directoryEntries;
    while (entries.hasMoreElements()) {
      let entry = entries.getNext().QueryInterface(Ci.nsIFile);
      try {
        
        
        
        
        if (entry.path == entry.target && entry.isDirectory()) {
          let destFolderId;
          if (entry.leafName == this._toolbarFolderName &&
              entry.parent.equals(this._favoritesFolder)) {
            
            destFolderId = PlacesUtils.toolbarFolderId;
            if (!MigrationUtils.isStartupMigration) {
              destFolderId =
                MigrationUtils.createImportedBookmarksFolder("IE", destFolderId);
            }
          }
          else {
            
            destFolderId =
              PlacesUtils.bookmarks.createFolder(aDestFolderId, entry.leafName,
                                                 PlacesUtils.bookmarks.DEFAULT_INDEX);
          }

          if (entry.isReadable()) {
            
            this._migrateFolder(entry, destFolderId);
          }
        }
        else {
          
          
          let matches = entry.leafName.match(/(.+)\.url$/i);
          if (matches) {
            let fileHandler = Cc["@mozilla.org/network/protocol;1?name=file"].
                              getService(Ci.nsIFileProtocolHandler);
            let uri = fileHandler.readURLFile(entry);
            let title = matches[1];

            PlacesUtils.bookmarks.insertBookmark(aDestFolderId,
                                                 uri,
                                                 PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                 title);
          }
        }
      } catch (ex) {
        Components.utils.reportError("Unable to import IE favorite (" + entry.leafName + "): " + ex);
      }
    }
  }
};

function History() {
}

History.prototype = {
  type: MigrationUtils.resourceTypes.HISTORY,

  get exists() true,

  __typedURLs: null,
  get _typedURLs() {
    if (!this.__typedURLs) {
      
      
      
      
      this.__typedURLs = {};
      let registry = Cc["@mozilla.org/windows-registry-key;1"].
                     createInstance(Ci.nsIWindowsRegKey);
      try {
        registry.open(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                      "Software\\Microsoft\\Internet Explorer\\TypedURLs",
                      Ci.nsIWindowsRegKey.ACCESS_READ);
        for (let entry = 1; registry.hasValue("url" + entry); entry++) {
          let url = registry.readStringValue("url" + entry);
          this.__typedURLs[url] = true;
        }
      } catch (ex) {
      } finally {
        registry.close();
      }
    }
    return this.__typedURLs;
  },

  migrate: function H_migrate(aCallback) {
    let places = [];
    let historyEnumerator = Cc["@mozilla.org/profile/migrator/iehistoryenumerator;1"].
                            createInstance(Ci.nsISimpleEnumerator);
    while (historyEnumerator.hasMoreElements()) {
      let entry = historyEnumerator.getNext().QueryInterface(Ci.nsIPropertyBag2);
      let uri = entry.get("uri").QueryInterface(Ci.nsIURI);
      
      
      
      if (["http", "https", "ftp", "file"].indexOf(uri.scheme) == -1) {
        continue;
      }

      let title = entry.get("title");
      
      if (title.length == 0) {
        continue;
      }

      
      let transitionType = this._typedURLs[uri.spec] ?
                             Ci.nsINavHistoryService.TRANSITION_TYPED :
                             Ci.nsINavHistoryService.TRANSITION_LINK;
      let lastVisitTime = entry.get("time");

      places.push(
        { uri: uri,
          title: title,
          visits: [{ transitionType: transitionType,
                     visitDate: lastVisitTime }]
        }
      );
    }

    
    if (places.length == 0) {
      aCallback(true);
      return;
    }

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
};

function Cookies() {
}

Cookies.prototype = {
  type: MigrationUtils.resourceTypes.COOKIES,

  get exists() !!this._cookiesFolder,

  __cookiesFolder: null,
  get _cookiesFolder() {
    
    
    
    
    
    
    
    
    if (!this.__cookiesFolder) {
      let cookiesFolder = Services.dirsvc.get("CookD", Ci.nsIFile);
      if (cookiesFolder.exists() && cookiesFolder.isReadable()) {
        
        if (Services.appinfo.QueryInterface(Ci.nsIWinAppHelper).userCanElevate) {
          cookiesFolder.append("Low");
        }
        this.__cookiesFolder = cookiesFolder;
      }
    }
    return this.__cookiesFolder;
  },

  migrate: function C_migrate(aCallback) {
    CtypesHelpers.initialize();

    let cookiesGenerator = (function genCookie() {
      let success = false;
      let entries = this._cookiesFolder.directoryEntries;
      while (entries.hasMoreElements()) {
        let entry = entries.getNext().QueryInterface(Ci.nsIFile);
        
        if (!entry.isFile() || !/\.txt$/.test(entry.leafName))
          continue;

        this._readCookieFile(entry, function(aSuccess) {
          
          if (aSuccess)
            success = true;
          try {
            cookiesGenerator.next();
          } catch (ex) {}
        });

        yield undefined;
      }

      CtypesHelpers.finalize();

      aCallback(success);
    }).apply(this);
    cookiesGenerator.next();
  },

  _readCookieFile: function C__readCookieFile(aFile, aCallback) {
    let fileReader = Cc["@mozilla.org/files/filereader;1"].
                     createInstance(Ci.nsIDOMFileReader);
    fileReader.addEventListener("loadend", (function onLoadEnd() {
      fileReader.removeEventListener("loadend", onLoadEnd, false);

      if (fileReader.readyState != fileReader.DONE) {
        Cu.reportError("Could not read cookie contents: " + fileReader.error);
        aCallback(false);
        return;
      }

      let success = true;
      try {
        this._parseCookieBuffer(fileReader.result);
      } catch (ex) {
        Components.utils.reportError("Unable to migrate cookie: " + ex);
        success = false;
      } finally {
        aCallback(success);
      }
    }).bind(this), false);
    fileReader.readAsText(File(aFile));
  },

  

















  _parseCookieBuffer: function C__parseCookieBuffer(aTextBuffer) {
    
    let records = [r for each (r in aTextBuffer.split("*\n")) if (r)];
    for (let record of records) {
      let [name, value, hostpath, flags,
           expireTimeLo, expireTimeHi] = record.split("\n");

      
      if (value.length == 0)
        continue;

      let hostLen = hostpath.indexOf("/");
      let host = hostpath.substr(0, hostLen);
      let path = hostpath.substr(hostLen);

      
      
      if (host.length > 0) {
        
        Services.cookies.remove(host, name, path, false);
        
        if (host[0] != "." && !hostIsIPAddress(host))
          host = "." + host;
      }

      let expireTime = CtypesHelpers.fileTimeToDate(Number(expireTimeHi),
                                                    Number(expireTimeLo));
      Services.cookies.add(host,
                           path,
                           name,
                           value,
                           Number(flags) & 0x1, 
                           false, 
                           false, 
                           expireTime);
    }
  }
};

function Settings() {
}

Settings.prototype = {
  type: MigrationUtils.resourceTypes.SETTINGS,

  get exists() true,

  migrate: function S_migrate(aCallback) {
    
    function yesNoToBoolean(v) v == "yes";

    
    
    
    function parseAcceptLanguageList(v) {
      return v.match(/([a-z]{1,8}(-[a-z]{1,8})?)\s*(;\s*q\s*=\s*(1|0\.[0-9]+))?/gi)
              .sort(function (a , b) {
                let qA = parseFloat(a.split(";q=")[1]) || 1.0;
                let qB = parseFloat(b.split(";q=")[1]) || 1.0;
                return qA < qB ? 1 : qA == qB ? 0 : -1;
              })
              .map(function(a) a.split(";")[0]);
    }

    
    
    

    

    this._set("Software\\Microsoft\\Internet Explorer\\International",
              "AcceptLanguage",
              "intl.accept_languages",
              parseAcceptLanguageList);
    
    this._set("Software\\Microsoft\\Internet Explorer\\International\\Scripts\\3",
              "IEFixedFontName",
              "font.name.monospace.x-western");
    this._set(kMainKey,
              "Use FormSuggest",
              "browser.formfill.enable",
              yesNoToBoolean);
    this._set(kMainKey,
              "FormSuggest Passwords",
              "signon.rememberSignons",
              yesNoToBoolean);
    this._set(kMainKey,
              "Anchor Underline",
              "browser.underline_anchors",
              yesNoToBoolean);
    this._set(kMainKey,
              "Display Inline Images",
              "permissions.default.image",
              function (v) yesNoToBoolean(v) ? 1 : 2);
    this._set(kMainKey,
              "Move System Caret",
              "accessibility.browsewithcaret",
              yesNoToBoolean);
    this._set("Software\\Microsoft\\Internet Explorer\\Settings",
              "Always Use My Colors",
              "browser.display.document_color_use",
              function (v) !Boolean(v) ? 0 : 2);
    this._set("Software\\Microsoft\\Internet Explorer\\Settings",
              "Always Use My Font Face",
              "browser.display.use_document_fonts",
              function (v) !Boolean(v));
    this._set(kMainKey,
              "SmoothScroll",
              "general.smoothScroll",
              Boolean);
    this._set("Software\\Microsoft\\Internet Explorer\\TabbedBrowsing\\",
              "WarnOnClose",
              "browser.tabs.warnOnClose",
              Boolean);
    this._set("Software\\Microsoft\\Internet Explorer\\TabbedBrowsing\\",
              "OpenInForeground",
              "browser.tabs.loadInBackground",
              function (v) !Boolean(v));

    aCallback(true);
  },

  












  _set: function S__set(aPath, aKey, aPref, aTransformFn) {
    let value = WindowsRegistry.readRegKey(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                                           aPath, aKey);
    
    if (value === undefined)
      return;

    if (aTransformFn)
      value = aTransformFn(value);

    switch (typeof(value)) {
      case "string":
        Services.prefs.setCharPref(aPref, value);
        break;
      case "number":
        Services.prefs.setIntPref(aPref, value);
        break;
      case "boolean":
        Services.prefs.setBoolPref(aPref, value);
        break;
      default:
        throw new Error("Unexpected value type: " + typeof(value));
    }
  }
};




function IEProfileMigrator()
{
}

IEProfileMigrator.prototype = Object.create(MigratorPrototype);

IEProfileMigrator.prototype.getResources = function IE_getResources() {
  let resources = [
    new Bookmarks()
  , new History()
  , new Cookies()
  , new Settings()
  ];
  return [r for each (r in resources) if (r.exists)];
};

Object.defineProperty(IEProfileMigrator.prototype, "sourceHomePageURL", {
  get: function IE_get_sourceHomePageURL() {
    let defaultStartPage = WindowsRegistry.readRegKey(Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE,
                                                      kMainKey, "Default_Page_URL");
    let startPage = WindowsRegistry.readRegKey(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                                               kMainKey, "Start Page");
    
    
    
    let homepage = startPage != defaultStartPage ? startPage : "";

    
    
    
    
    let secondaryPages = WindowsRegistry.readRegKey(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                                                    kMainKey, "Secondary Start Pages");
    if (secondaryPages) {
      if (homepage)
        secondaryPages.unshift(homepage);
      homepage = secondaryPages.join("|");
    }

    return homepage;
  }
});

IEProfileMigrator.prototype.classDescription = "IE Profile Migrator";
IEProfileMigrator.prototype.contractID = "@mozilla.org/profile/migrator;1?app=browser&type=ie";
IEProfileMigrator.prototype.classID = Components.ID("{3d2532e3-4932-4774-b7ba-968f5899d3a4}");

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([IEProfileMigrator]);
