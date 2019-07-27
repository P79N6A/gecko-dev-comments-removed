



"use strict";

this.EXPORTED_SYMBOLS = ["MigrationUtils", "MigratorPrototype"];

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;

const TOPIC_WILL_IMPORT_BOOKMARKS = "initial-migration-will-import-default-bookmarks";
const TOPIC_DID_IMPORT_BOOKMARKS = "initial-migration-did-import-default-bookmarks";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "BookmarkHTMLUtils",
                                  "resource://gre/modules/BookmarkHTMLUtils.jsm");

let gMigrators = null;
let gProfileStartup = null;
let gMigrationBundle = null;

function getMigrationBundle() {
  if (!gMigrationBundle) {
    gMigrationBundle = Services.strings.createBundle(
     "chrome://browser/locale/migration/migration.properties");
  }
  return gMigrationBundle;
}








function getMigratorKeyForDefaultBrowser() {
  const APP_DESC_TO_KEY = {
    "Internet Explorer": "ie",
    "Safari":            "safari",
    "Firefox":           "firefox",
    "Google Chrome":     "chrome",  
    "Chrome":            "chrome",  
  };

  let browserDesc = "";
  try {
    let browserDesc =
      Cc["@mozilla.org/uriloader/external-protocol-service;1"].
      getService(Ci.nsIExternalProtocolService).
      getApplicationDescription("http");
    return APP_DESC_TO_KEY[browserDesc] || "";
  }
  catch(ex) {
    Cu.reportError("Could not detect default browser: " + ex);
  }
  return "";
}

















this.MigratorPrototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIBrowserProfileMigrator]),

  













  get sourceProfiles() null,

  











































  getResources: function MP_getResources(aProfile) {
    throw new Error("getResources must be overridden");
  },

  










  get startupOnlyMigrator() false,

  



  get sourceHomePageURL() "",

  





  getMigrateData: function MP_getMigrateData(aProfile) {
    let types = [r.type for each (r in this._getMaybeCachedResources(aProfile))];
    return types.reduce(function(a, b) a |= b, 0);
  },

  





  migrate: function MP_migrate(aItems, aStartup, aProfile) {
    let resources = this._getMaybeCachedResources(aProfile);
    if (resources.length == 0)
      throw new Error("migrate called for a non-existent source");

    if (aItems != Ci.nsIBrowserProfileMigrator.ALL)
      resources = [r for each (r in resources) if (aItems & r.type)];

    
    function doMigrate() {
      
      
      let resourcesGroupedByItems = new Map();
      resources.forEach(function(resource) {
        if (resourcesGroupedByItems.has(resource.type))
          resourcesGroupedByItems.get(resource.type).push(resource);
        else
          resourcesGroupedByItems.set(resource.type, [resource]);
      });

      if (resourcesGroupedByItems.size == 0)
        throw new Error("No items to import");

      let notify = function(aMsg, aItemType) {
        Services.obs.notifyObservers(null, aMsg, aItemType);
      }

      notify("Migration:Started");
      for (let [key, value] of resourcesGroupedByItems) {
      	
      	let migrationType = key, itemResources = value;

        notify("Migration:ItemBeforeMigrate", migrationType);

        let itemSuccess = false;
        for (let res of itemResources) {
          let resource = res;
          let resourceDone = function(aSuccess) {
            let resourceIndex = itemResources.indexOf(resource);
            if (resourceIndex != -1) {
              itemResources.splice(resourceIndex, 1);
              itemSuccess |= aSuccess;
              if (itemResources.length == 0) {
                resourcesGroupedByItems.delete(migrationType);
                notify(itemSuccess ?
                       "Migration:ItemAfterMigrate" : "Migration:ItemError",
                       migrationType);
                if (resourcesGroupedByItems.size == 0)
                  notify("Migration:Ended");
              }
            }
          }

          Services.tm.mainThread.dispatch(function() {
            
            
            try {
              resource.migrate(resourceDone);
            }
            catch(ex) {
              Cu.reportError(ex);
              resourceDone(false);
            }
          }, Ci.nsIThread.DISPATCH_NORMAL);
        }
      }
    }

    if (MigrationUtils.isStartupMigration && !this.startupOnlyMigrator) {
      MigrationUtils.profileStartup.doStartup();

      
      
      
      
      const BOOKMARKS = MigrationUtils.resourceTypes.BOOKMARKS;
      let migratingBookmarks = resources.some(function(r) r.type == BOOKMARKS);
      if (migratingBookmarks) {
        let browserGlue = Cc["@mozilla.org/browser/browserglue;1"].
                          getService(Ci.nsIObserver);
        browserGlue.observe(null, TOPIC_WILL_IMPORT_BOOKMARKS, "");

        
        let onImportComplete = function() {
          browserGlue.observe(null, TOPIC_DID_IMPORT_BOOKMARKS, "");
          doMigrate();
        };
        BookmarkHTMLUtils.importFromURL(
          "resource:///defaults/profile/bookmarks.html", true).then(
          onImportComplete, onImportComplete);
        return;
      }
    }
    doMigrate();
  },

  





  get sourceExists() {
    if (this.startupOnlyMigrator && !MigrationUtils.isStartupMigration)
      return false;

    
    
    
    let exists = false;
    try {
      let profiles = this.sourceProfiles;
      if (!profiles) {
        let resources = this._getMaybeCachedResources("");
        if (resources && resources.length > 0)
          exists = true;
      }
      else {
        exists = profiles.length > 0;
      }
    }
    catch(ex) {
      Cu.reportError(ex);
    }
    return exists;
  },

  
  _getMaybeCachedResources: function PMB__getMaybeCachedResources(aProfile) {
    let profileKey = aProfile ? aProfile.id : "";
    if (this._resourcesByProfile) {
      if (profileKey in this._resourcesByProfile)
        return this._resourcesByProfile[profileKey];
    }
    else {
      this._resourcesByProfile = { };
    }
    return this._resourcesByProfile[profileKey] = this.getResources(aProfile);
  }
};

this.MigrationUtils = Object.freeze({
  resourceTypes: {
    SETTINGS:   Ci.nsIBrowserProfileMigrator.SETTINGS,
    COOKIES:    Ci.nsIBrowserProfileMigrator.COOKIES,
    HISTORY:    Ci.nsIBrowserProfileMigrator.HISTORY,
    FORMDATA:   Ci.nsIBrowserProfileMigrator.FORMDATA,
    PASSWORDS:  Ci.nsIBrowserProfileMigrator.PASSWORDS,
    BOOKMARKS:  Ci.nsIBrowserProfileMigrator.BOOKMARKS,
    OTHERDATA:  Ci.nsIBrowserProfileMigrator.OTHERDATA,
    SESSION:    Ci.nsIBrowserProfileMigrator.SESSION,
  },

  


































  wrapMigrateFunction: function MU_wrapMigrateFunction(aFunction, aCallback) {
    return function() {
      let success = false;
      try {
        aFunction.apply(null, arguments);
        success = true;
      }
      catch(ex) {
        Cu.reportError(ex);
      }
      
      
      
      aCallback(success);
    }
  },

  















  getLocalizedString: function MU_getLocalizedString(aKey, aReplacements) {
    const OVERRIDES = {
      "4_firefox": "4_firefox_history_and_bookmarks",
      "64_firefox": "64_firefox_other"
    };
    aKey = OVERRIDES[aKey] || aKey;

    if (aReplacements === undefined)
      return getMigrationBundle().GetStringFromName(aKey);
    return getMigrationBundle().formatStringFromName(
      aKey, aReplacements, aReplacements.length);
  },

  













  createImportedBookmarksFolder:
  function MU_createImportedBookmarksFolder(aSourceNameStr, aParentId) {
    let source = this.getLocalizedString("sourceName" + aSourceNameStr);
    let label = this.getLocalizedString("importedBookmarksFolder", [source]);
    return PlacesUtils.bookmarks.createFolder(
      aParentId, label, PlacesUtils.bookmarks.DEFAULT_INDEX);
  },

  get _migrators() {
    return gMigrators ? gMigrators : gMigrators = new Map();
  },

  

















  getMigrator: function MU_getMigrator(aKey) {
    let migrator = null;
    if (this._migrators.has(aKey)) {
      migrator = this._migrators.get(aKey);
    }
    else {
      try {
        migrator = Cc["@mozilla.org/profile/migrator;1?app=browser&type=" +
                      aKey].createInstance(Ci.nsIBrowserProfileMigrator);
      }
      catch(ex) { }
      this._migrators.set(aKey, migrator);
    }

    return migrator && migrator.sourceExists ? migrator : null;
  },

  
  
  get migrators() {
    let migratorKeysOrdered = [
#ifdef XP_WIN
      "firefox", "ie", "chrome", "safari"
#elifdef XP_MACOSX
      "firefox", "safari", "chrome"
#elifdef XP_UNIX
      "firefox", "chrome"
#endif
    ];

    
    
    let defaultBrowserKey = getMigratorKeyForDefaultBrowser();
    if (defaultBrowserKey)
      migratorKeysOrdered.sort(function (a, b) b == defaultBrowserKey ? 1 : 0);

    for (let migratorKey of migratorKeysOrdered) {
      let migrator = this.getMigrator(migratorKey);
      if (migrator)
        yield migrator;
    }
  },

  
  get isStartupMigration() gProfileStartup != null,

  





  get profileStartup() gProfileStartup,

  










  showMigrationWizard:
  function MU_showMigrationWizard(aOpener, aParams) {
    let features = "chrome,dialog,modal,centerscreen,titlebar,resizable=no";
#ifdef XP_MACOSX
    if (!this.isStartupMigration) {
      let win = Services.wm.getMostRecentWindow("Browser:MigrationWizard");
      if (win) {
        win.focus();
        return;
      }
      
      
      features = "centerscreen,chrome,resizable=no";
    }
#endif

    Services.ww.openWindow(aOpener,
                           "chrome://browser/content/migration/migration.xul",
                           "_blank",
                           features,
                           aParams);
  },

  

















  startupMigration:
  function MU_startupMigrator(aProfileStartup, aMigratorKey) {
    if (!aProfileStartup) {
      throw new Error("an profile-startup instance is required for startup-migration");
    }
    gProfileStartup = aProfileStartup;

    let skipSourcePage = false, migrator = null, migratorKey = "";
    if (aMigratorKey) {
      migrator = this.getMigrator(aMigratorKey);
      if (!migrator) {
        
        
        this.finishMigration();
        throw new Error("startMigration was asked to open auto-migrate from " +
                        "a non-existent source: " + aMigratorKey);
      }
      migratorKey = aMigratorKey;
      skipSourcePage = true;
    }
    else {
      let defaultBrowserKey = getMigratorKeyForDefaultBrowser();
      if (defaultBrowserKey) {
        migrator = this.getMigrator(defaultBrowserKey);
        if (migrator)
          migratorKey = defaultBrowserKey;
      }
    }

    if (!migrator) {
      
      
      try {
        this.migrators.next();
      }
      catch(ex) {
        this.finishMigration();
        if (!(ex instanceof StopIteration))
          throw ex;
        return;
      }
    }

    let params = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
    let keyCSTR = Cc["@mozilla.org/supports-cstring;1"].
                  createInstance(Ci.nsISupportsCString);
    keyCSTR.data = migratorKey;
    let skipImportSourcePageBool = Cc["@mozilla.org/supports-PRBool;1"].
                                   createInstance(Ci.nsISupportsPRBool);
    skipImportSourcePageBool.data = skipSourcePage;
    params.appendElement(keyCSTR, false);
    params.appendElement(migrator, false);
    params.appendElement(aProfileStartup, false);
    params.appendElement(skipImportSourcePageBool, false);

    this.showMigrationWizard(null, params);
  },

  


  finishMigration: function MU_finishMigration() {
    gMigrators = null;
    gProfileStartup = null;
    gMigrationBundle = null;
  }
});
