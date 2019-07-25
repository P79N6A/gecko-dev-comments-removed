



"use strict";

let EXPORTED_SYMBOLS = ["MigrationUtils", "MigratorPrototype"];

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Dict",
                                  "resource://gre/modules/Dict.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");

let gMigrators = null;
let gProfileStartup = null;

















let MigratorPrototype = {
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
    
    if (MigrationUtils.isStartupMigration && !this.startupOnlyMigrator)
      MigrationUtils.profileStartup.doStartup();

    let resources = this._getMaybeCachedResources(aProfile);
    if (resources.length == 0)
      throw new Error("migrate called for a non-existent source");

    if (aItems != Ci.nsIBrowserProfileMigrator.ALL)
      resources = [r for each (r in resources) if (aItems & r.type)];

    
    
    let resourcesGroupedByItems = new Dict();
    resources.forEach(function(resource) {
      if (resourcesGroupedByItems.has(resource.type))
        resourcesGroupedByItems.get(resource.type).push(resource);
      else
        resourcesGroupedByItems.set(resource.type, [resource]);
    });

    if (resourcesGroupedByItems.count == 0)
      throw new Error("No items to import");

    let notify = function(aMsg, aItemType) {
      Services.obs.notifyObservers(null, aMsg, aItemType);
    }

    notify("Migration:Started");
    resourcesGroupedByItems.listkeys().forEach(function(migrationType) {
      let migrationTypeA = migrationType;
      let itemResources = resourcesGroupedByItems.get(migrationType);
      notify("Migration:ItemBeforeMigrate", migrationType);

      let itemSuccess = false;
      itemResources.forEach(function(resource) {
        let resourceDone = function(aSuccess) {
          let resourceIndex = itemResources.indexOf(resource);
          if (resourceIndex != -1) {
            itemResources.splice(resourceIndex, 1);
            itemSuccess |= aSuccess;
            if (itemResources.length == 0) {
              resourcesGroupedByItems.del(migrationType);
              notify(itemSuccess ?
                     "Migration:ItemAfterMigrate" : "Migration:ItemError",
                     migrationType);
              if (resourcesGroupedByItems.count == 0)
                notify("Migration:Ended");
            }
          }
        };

        Services.tm.mainThread.dispatch(function() {
          
          
          try {
            resource.migrate(resourceDone);
          }
          catch(ex) {
            Cu.reportError(ex);
            resourceDone(false);
          }
        }, Ci.nsIThread.DISPATCH_NORMAL);
      });
    });
  },

  





  get sourceExists() {
    if (this.startupOnlyMigrator && !MigrationUtils.isStartupMigration)
      return false;

    
    
    
    let profiles = this.sourceProfiles;
    return (!profiles && this.getResources("")) ||
           (profiles && profiles.length > 0);
  },

  
  _getMaybeCachedResources: function PMB__getMaybeCachedResources(aProfile) {
    if (this._resourcesByProfile) {
      if (aProfile in this._resourcesByProfile)
        return this._resourcesByProfile[aProfile];
    }
    else {
      this._resourcesByProfile = { };
    }
    return this._resourcesByProfile[aProfile] = this.getResources(aProfile);
  }
};

let MigrationUtils = Object.freeze({
  


































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

  













  createImportedBookmarksFolder:
  function MU_createImportedBookmarksFolder(aSourceNameStr, aParentId) {
    let bundle = Services.strings.createBundle(
     "chrome://browser/locale/migration/migration.properties");
    let sourceName = bundle.GetStringFromName("sourceName" + aSourceNameStr);
    let folderName = bundle.formatStringFromName("importedBookmarksFolder",
                                                 [sourceName], 1);
    return PlacesUtils.bookmarks.createFolder(
      aParentId, folderName, PlacesUtils.bookmarks.DEFAULT_INDEX);
  },

  get _migrators() gMigrators ? gMigrators : gMigrators = new Dict(),

  
















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
      catch(ex) {
        Cu.reportError("Could not get migrator '" + aKey + "' (" + ex + ")");
      }
      this._migrators.set(aKey, migrator);
    }

    return migrator && migrator.sourceExists ? migrator : null;
  },

  
  get isStartupMigration() gProfileStartup != null,

  





  get profileStartup() gProfileStartup,

  
























  showMigrationWizard:
  function MU_showMigrationWizard(aOpener, aProfileStartup, aMigratorKey,
                                  aSkipImportSourcePage) {
    let features = "chrome,dialog,modal,centerscreen,titlebar";
    let params = null;
    if (!aProfileStartup) {
#ifdef XP_MACOSX
      let win = Services.wm.getMostRecentWindow("Browser:MigrationWizard");
      if (win) {
        win.focus();
        return;
      }
      features = "centerscreen,chrome,resizable=no";
#endif
    }
    else {
      if (!aMigratorKey)
        throw new Error("aMigratorKey must be set for startup migration");

      let migrator = this.getMigrator(aMigratorKey);
      if (!migrator) {
        throw new Error("startMigration was asked to open auto-migrate from a non-existent source: " +
                        aMigratorKey);
      }
      else {
        gProfileStartup = aProfileStartup;
      }
      
      
      
      params = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
      let keyCSTR = Cc["@mozilla.org/supports-cstring;1"].
                    createInstance(Ci.nsISupportsCString);
      keyCSTR.data = aMigratorKey;
      let skipImportSourcePageBool = Cc["@mozilla.org/supports-PRBool;1"].
                                     createInstance(Ci.nsISupportsPRBool);
      params.appendElement(keyCSTR, false);
      params.appendElement(migrator, false);
      params.appendElement(aProfileStartup, false);

      if (aSkipImportSourcePage === true) {
        let wrappedBool = Cc["@mozilla.org/supports-PRBool;1"].
                          createInstance(Ci.nsISupportsPRBool);
        wrappedBool.data = true;
        params.appendElement(wrappedBool);
      }
    }

    Services.ww.openWindow(null,
                           "chrome://browser/content/migration/migration.xul",
                           "_blank",
                           features,
                           params);
  },

  


  finishMigration: function MU_finishMigration() {
    gMigrators = null;
    gProfileStartup = null;
  }
});
