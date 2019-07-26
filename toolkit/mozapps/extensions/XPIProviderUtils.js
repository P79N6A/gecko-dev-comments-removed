



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/AddonManager.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AddonRepository",
                                  "resource://gre/modules/AddonRepository.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DeferredSave",
                                  "resource://gre/modules/DeferredSave.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");

["LOG", "WARN", "ERROR"].forEach(function(aName) {
  Object.defineProperty(this, aName, {
    get: function logFuncGetter () {
      Cu.import("resource://gre/modules/AddonLogging.jsm");

      LogManager.getLogger("addons.xpi-utils", this);
      return this[aName];
    },
    configurable: true
  });
}, this);


const KEY_PROFILEDIR                  = "ProfD";
const FILE_DATABASE                   = "extensions.sqlite";
const FILE_JSON_DB                    = "extensions.json";
const FILE_OLD_DATABASE               = "extensions.rdf";
const FILE_XPI_ADDONS_LIST            = "extensions.ini";


#expand const DB_SCHEMA                       = __MOZ_EXTENSIONS_DB_SCHEMA__;


const LAST_SQLITE_DB_SCHEMA           = 14;
const PREF_DB_SCHEMA                  = "extensions.databaseSchema";
const PREF_PENDING_OPERATIONS         = "extensions.pendingOperations";
const PREF_EM_ENABLED_ADDONS          = "extensions.enabledAddons";
const PREF_EM_DSS_ENABLED             = "extensions.dss.enabled";



const DB_METADATA        = ["syncGUID",
                            "installDate",
                            "updateDate",
                            "size",
                            "sourceURI",
                            "releaseNotesURI",
                            "applyBackgroundUpdates"];
const DB_BOOL_METADATA   = ["visible", "active", "userDisabled", "appDisabled",
                            "pendingUninstall", "bootstrap", "skinnable",
                            "softDisabled", "isForeignInstall",
                            "hasBinaryComponents", "strictCompatibility"];


const PROP_JSON_FIELDS = ["id", "syncGUID", "location", "version", "type",
                          "internalName", "updateURL", "updateKey", "optionsURL",
                          "optionsType", "aboutURL", "iconURL", "icon64URL",
                          "defaultLocale", "visible", "active", "userDisabled",
                          "appDisabled", "pendingUninstall", "descriptor", "installDate",
                          "updateDate", "applyBackgroundUpdates", "bootstrap",
                          "skinnable", "size", "sourceURI", "releaseNotesURI",
                          "softDisabled", "foreignInstall", "hasBinaryComponents",
                          "strictCompatibility", "locales", "targetApplications",
                          "targetPlatforms"];


const ASYNC_SAVE_DELAY_MS = 20;

const PREFIX_ITEM_URI                 = "urn:mozilla:item:";
const RDFURI_ITEM_ROOT                = "urn:mozilla:item:root"
const PREFIX_NS_EM                    = "http://www.mozilla.org/2004/em-rdf#";

XPCOMUtils.defineLazyServiceGetter(this, "gRDF", "@mozilla.org/rdf/rdf-service;1",
                                   Ci.nsIRDFService);

function EM_R(aProperty) {
  return gRDF.GetResource(PREFIX_NS_EM + aProperty);
}








function getRDFValue(aLiteral) {
  if (aLiteral instanceof Ci.nsIRDFLiteral)
    return aLiteral.Value;
  if (aLiteral instanceof Ci.nsIRDFResource)
    return aLiteral.Value;
  if (aLiteral instanceof Ci.nsIRDFInt)
    return aLiteral.Value;
  return null;
}












function getRDFProperty(aDs, aResource, aProperty) {
  return getRDFValue(aDs.GetTarget(aResource, EM_R(aProperty), true));
}




function getRepositoryAddon(aAddon, aCallback) {
  if (!aAddon) {
    aCallback(aAddon);
    return;
  }
  function completeAddon(aRepositoryAddon) {
    aAddon._repositoryAddon = aRepositoryAddon;
    aAddon.compatibilityOverrides = aRepositoryAddon ?
                                      aRepositoryAddon.compatibilityOverrides :
                                      null;
    aCallback(aAddon);
  }
  AddonRepository.getCachedAddonByID(aAddon.id, completeAddon);
}




function makeSafe(aCallback) {
  return function(...aArgs) {
    try {
      aCallback(...aArgs);
    }
    catch(ex) {
      WARN("XPI Database callback failed", ex);
    }
  }
}

















function asyncMap(aObjects, aMethod, aCallback) {
  var resultsPending = aObjects.length;
  var results = []
  if (resultsPending == 0) {
    aCallback(results);
    return;
  }

  function asyncMap_gotValue(aIndex, aValue) {
    results[aIndex] = aValue;
    if (--resultsPending == 0) {
      aCallback(results);
    }
  }

  aObjects.map(function asyncMap_each(aObject, aIndex, aArray) {
    try {
      aMethod(aObject, function asyncMap_callback(aResult) {
        asyncMap_gotValue(aIndex, aResult);
      });
    }
    catch (e) {
      WARN("Async map function failed", e);
      asyncMap_gotValue(aIndex, undefined);
    }
  });
}







function resultRows(aStatement) {
  try {
    while (stepStatement(aStatement))
      yield aStatement.row;
  }
  finally {
    aStatement.reset();
  }
}










function logSQLError(aError, aErrorString) {
  ERROR("SQL error " + aError + ": " + aErrorString);
}







function asyncErrorLogger(aError) {
  logSQLError(aError.result, aError.message);
}








function stepStatement(aStatement) {
  try {
    return aStatement.executeStep();
  }
  catch (e) {
    logSQLError(XPIDatabase.connection.lastError,
                XPIDatabase.connection.lastErrorString);
    throw e;
  }
}














function copyProperties(aObject, aProperties, aTarget) {
  if (!aTarget)
    aTarget = {};
  aProperties.forEach(function(aProp) {
    aTarget[aProp] = aObject[aProp];
  });
  return aTarget;
}













function copyRowProperties(aRow, aProperties, aTarget) {
  if (!aTarget)
    aTarget = {};
  aProperties.forEach(function(aProp) {
    aTarget[aProp] = aRow.getResultByName(aProp);
  });
  return aTarget;
}










function DBAddonInternal(aLoaded) {
  copyProperties(aLoaded, PROP_JSON_FIELDS, this);

  if (aLoaded._installLocation) {
    this._installLocation = aLoaded._installLocation;
    this.location = aLoaded._installLocation._name;
  }
  else if (aLoaded.location) {
    this._installLocation = XPIProvider.installLocationsByName[this.location];
  }

  this._key = this.location + ":" + this.id;

  try {
    this._sourceBundle = this._installLocation.getLocationForID(this.id);
  }
  catch (e) {
    
    
    
  }

  XPCOMUtils.defineLazyGetter(this, "pendingUpgrade",
    function DBA_pendingUpgradeGetter() {
      for (let install of XPIProvider.installs) {
        if (install.state == AddonManager.STATE_INSTALLED &&
            !(install.addon.inDatabase) &&
            install.addon.id == this.id &&
            install.installLocation == this._installLocation) {
          delete this.pendingUpgrade;
          return this.pendingUpgrade = install.addon;
        }
      };
      return null;
    });
}

DBAddonInternal.prototype = {
  applyCompatibilityUpdate: function DBA_applyCompatibilityUpdate(aUpdate, aSyncCompatibility) {
    this.targetApplications.forEach(function(aTargetApp) {
      aUpdate.targetApplications.forEach(function(aUpdateTarget) {
        if (aTargetApp.id == aUpdateTarget.id && (aSyncCompatibility ||
            Services.vc.compare(aTargetApp.maxVersion, aUpdateTarget.maxVersion) < 0)) {
          aTargetApp.minVersion = aUpdateTarget.minVersion;
          aTargetApp.maxVersion = aUpdateTarget.maxVersion;
          XPIDatabase.saveChanges();
        }
      });
    });
    XPIProvider.updateAddonDisabledState(this);
  },

  get inDatabase() {
    return true;
  },

  toJSON: function() {
    return copyProperties(this, PROP_JSON_FIELDS);
  }
}

DBAddonInternal.prototype.__proto__ = AddonInternal.prototype;




function _findAddon(addonDB, aFilter) {
  for (let [, addon] of addonDB) {
    if (aFilter(addon)) {
      return addon;
    }
  }
  return null;
}




function _filterDB(addonDB, aFilter) {
  let addonList = [];
  for (let [, addon] of addonDB) {
    if (aFilter(addon)) {
      addonList.push(addon);
    }
  }

  return addonList;
}

this.XPIDatabase = {
  
  initialized: false,
  
  jsonFile: FileUtils.getFile(KEY_PROFILEDIR, [FILE_JSON_DB], true),
  
  migrateData: null,
  
  activeBundles: null,

  
  _loadError: null,

  
  get lastError() {
    if (this._loadError)
      return this._loadError;
    if (this._deferredSave)
      return this._deferredSave.lastError;
    return null;
  },

  


  saveChanges: function() {
    if (!this.initialized) {
      throw new Error("Attempt to use XPI database when it is not initialized");
    }

    if (!this._deferredSave) {
      this._deferredSave = new DeferredSave(this.jsonFile.path,
                                            () => JSON.stringify(this),
                                            ASYNC_SAVE_DELAY_MS);
    }

    let promise = this._deferredSave.saveChanges();
    if (!this._schemaVersionSet) {
      this._schemaVersionSet = true;
      promise.then(
        count => {
          
          
          LOG("XPI Database saved, setting schema version preference to " + DB_SCHEMA);
          Services.prefs.setIntPref(PREF_DB_SCHEMA, DB_SCHEMA);
          
          this._loadError = null;
        },
        error => {
          
          this._schemaVersionSet = false;
          WARN("Failed to save XPI database", error);
          
          
          this._loadError = null;
        });
    }
  },

  flush: function() {
    
    if (!this._deferredSave) {
      return Promise.resolve(0);
    }

    return this._deferredSave.flush();
  },

  



  toJSON: function() {
    if (!this.addonDB) {
      
      throw new Error("Attempt to save database without loading it first");
    }

    let toSave = {
      schemaVersion: DB_SCHEMA,
      addons: [...this.addonDB.values()]
    };
    return toSave;
  },

  









  getMigrateDataFromSQLITE: function XPIDB_getMigrateDataFromSQLITE() {
    let connection = null;
    let dbfile = FileUtils.getFile(KEY_PROFILEDIR, [FILE_DATABASE], true);
    
    try {
      connection = Services.storage.openUnsharedDatabase(dbfile);
    }
    catch (e) {
      WARN("Failed to open sqlite database " + dbfile.path + " for upgrade", e);
      return null;
    }
    LOG("Migrating data from sqlite");
    let migrateData = this.getMigrateDataFromDatabase(connection);
    connection.close();
    return migrateData;
  },

  

















  syncLoadDB: function XPIDB_syncLoadDB(aRebuildOnError) {
    this.migrateData = null;
    let fstream = null;
    let data = "";
    try {
      let readTimer = AddonManagerPrivate.simpleTimer("XPIDB_syncRead_MS");
      LOG("Opening XPI database " + this.jsonFile.path);
      fstream = Components.classes["@mozilla.org/network/file-input-stream;1"].
              createInstance(Components.interfaces.nsIFileInputStream);
      fstream.init(this.jsonFile, -1, 0, 0);
      let cstream = null;
      try {
        cstream = Components.classes["@mozilla.org/intl/converter-input-stream;1"].
                createInstance(Components.interfaces.nsIConverterInputStream);
        cstream.init(fstream, "UTF-8", 0, 0);
        let (str = {}) {
          let read = 0;
          do {
            read = cstream.readString(0xffffffff, str); 
            data += str.value;
          } while (read != 0);
        }
        readTimer.done();
        this.parseDB(data, aRebuildOnError);
      }
      catch(e) {
        ERROR("Failed to load XPI JSON data from profile", e);
        let rebuildTimer = AddonManagerPrivate.simpleTimer("XPIDB_rebuildReadFailed_MS");
        this.rebuildDatabase(aRebuildOnError);
        rebuildTimer.done();
      }
      finally {
        if (cstream)
          cstream.close();
      }
    }
    catch (e) {
      if (e.result === Cr.NS_ERROR_FILE_NOT_FOUND) {
        this.upgradeDB(aRebuildOnError);
      }
      else {
        this.rebuildUnreadableDB(e, aRebuildOnError);
      }
    }
    finally {
      if (fstream)
        fstream.close();
    }
    
    
    if (this._dbPromise) {
      AddonManagerPrivate.recordSimpleMeasure("XPIDB_overlapped_load", 1);
      this._dbPromise.resolve(this.addonDB);
    }
    else
      this._dbPromise = Promise.resolve(this.addonDB);
  },

  




  parseDB: function(aData, aRebuildOnError) {
    let parseTimer = AddonManagerPrivate.simpleTimer("XPIDB_parseDB_MS");
    try {
      
      let inputAddons = JSON.parse(aData);
      
      if (!("schemaVersion" in inputAddons) || !("addons" in inputAddons)) {
        parseTimer.done();
        
        ERROR("bad JSON file contents");
        AddonManagerPrivate.recordSimpleMeasure("XPIDB_startupError", "badJSON");
        let rebuildTimer = AddonManagerPrivate.simpleTimer("XPIDB_rebuildBadJSON_MS");
        this.rebuildDatabase(aRebuildOnError);
        rebuildTimer.done();
        return;
      }
      if (inputAddons.schemaVersion != DB_SCHEMA) {
        
        
        
        AddonManagerPrivate.recordSimpleMeasure("XPIDB_startupError",
                                                "schemaMismatch-" + inputAddons.schemaVersion);
        LOG("JSON schema mismatch: expected " + DB_SCHEMA +
            ", actual " + inputAddons.schemaVersion);
        
        
        
      }
      
      
      let addonDB = new Map();
      for (let loadedAddon of inputAddons.addons) {
        let newAddon = new DBAddonInternal(loadedAddon);
        addonDB.set(newAddon._key, newAddon);
      };
      parseTimer.done();
      this.addonDB = addonDB;
      LOG("Successfully read XPI database");
      this.initialized = true;
    }
    catch(e) {
      
      
      parseTimer.done();
      if (e.name == "SyntaxError") {
        ERROR("Syntax error parsing saved XPI JSON data");
        AddonManagerPrivate.recordSimpleMeasure("XPIDB_startupError", "syntax");
      }
      else {
        ERROR("Failed to load XPI JSON data from profile", e);
        AddonManagerPrivate.recordSimpleMeasure("XPIDB_startupError", "other");
      }
      let rebuildTimer = AddonManagerPrivate.simpleTimer("XPIDB_rebuildReadFailed_MS");
      this.rebuildDatabase(aRebuildOnError);
      rebuildTimer.done();
    }
  },

  


  upgradeDB: function(aRebuildOnError) {
    let upgradeTimer = AddonManagerPrivate.simpleTimer("XPIDB_upgradeDB_MS");
    try {
      let schemaVersion = Services.prefs.getIntPref(PREF_DB_SCHEMA);
      if (schemaVersion <= LAST_SQLITE_DB_SCHEMA) {
        
        LOG("Attempting to upgrade from SQLITE database");
        this.migrateData = this.getMigrateDataFromSQLITE();
      }
      else {
        
        
        AddonManagerPrivate.recordSimpleMeasure("XPIDB_startupError", "dbMissing");
      }
    }
    catch(e) {
      
      
      this.migrateData = this.getMigrateDataFromRDF();
    }

    this.rebuildDatabase(aRebuildOnError);
    upgradeTimer.done();
  },

  



  rebuildUnreadableDB: function(aError, aRebuildOnError) {
    let rebuildTimer = AddonManagerPrivate.simpleTimer("XPIDB_rebuildUnreadableDB_MS");
    WARN("Extensions database " + this.jsonFile.path +
        " exists but is not readable; rebuilding", aError);
    
    
    this._loadError = aError;
    AddonManagerPrivate.recordSimpleMeasure("XPIDB_startupError", "unreadable");
    this.rebuildDatabase(aRebuildOnError);
    rebuildTimer.done();
  },

  







  asyncLoadDB: function XPIDB_asyncLoadDB() {
    
    if (this._dbPromise) {
      return this._dbPromise;
    }

    LOG("Starting async load of XPI database " + this.jsonFile.path);
    AddonManagerPrivate.recordSimpleMeasure("XPIDB_async_load", XPIProvider.runPhase);
    let readOptions = {
      outExecutionDuration: 0
    };
    return this._dbPromise = OS.File.read(this.jsonFile.path, null, readOptions).then(
      byteArray => {
        LOG("Async JSON file read took " + readOptions.outExecutionDuration + " MS");
        AddonManagerPrivate.recordSimpleMeasure("XPIDB_asyncRead_MS",
          readOptions.outExecutionDuration);
        if (this._addonDB) {
          LOG("Synchronous load completed while waiting for async load");
          return this.addonDB;
        }
        LOG("Finished async read of XPI database, parsing...");
        let decodeTimer = AddonManagerPrivate.simpleTimer("XPIDB_decode_MS");
        let decoder = new TextDecoder();
        let data = decoder.decode(byteArray);
        decodeTimer.done();
        this.parseDB(data, true);
        return this.addonDB;
      })
    .then(null,
      error => {
        if (this._addonDB) {
          LOG("Synchronous load completed while waiting for async load");
          return this.addonDB;
        }
        if (error.becauseNoSuchFile) {
          this.upgradeDB(true);
        }
        else {
          
          this.rebuildUnreadableDB(error, true);
        }
        return this.addonDB;
      });
  },

  








  rebuildDatabase: function XIPDB_rebuildDatabase(aRebuildOnError) {
    this.addonDB = new Map();
    this.initialized = true;

    if (XPIProvider.installStates && XPIProvider.installStates.length == 0) {
      
      LOG("Rebuilding XPI database with no extensions");
      return;
    }

    
    
    if (!this.migrateData)
      this.activeBundles = this.getActiveBundles();

    if (aRebuildOnError) {
      WARN("Rebuilding add-ons database from installed extensions.");
      try {
        XPIProvider.processFileChanges(XPIProvider.installStates, {}, false);
      }
      catch (e) {
        ERROR("Failed to rebuild XPI database from installed extensions", e);
      }
      
      Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, true);
    }
  },

  







  getActiveBundles: function XPIDB_getActiveBundles() {
    let bundles = [];

    
    let addonsList = FileUtils.getFile(KEY_PROFILEDIR, [FILE_XPI_ADDONS_LIST],
                                       true);

    if (!addonsList.exists())
      
      
      return null;

    try {
      let iniFactory = Cc["@mozilla.org/xpcom/ini-parser-factory;1"]
                         .getService(Ci.nsIINIParserFactory);
      let parser = iniFactory.createINIParser(addonsList);
      let keys = parser.getKeys("ExtensionDirs");

      while (keys.hasMore())
        bundles.push(parser.getString("ExtensionDirs", keys.getNext()));
    }
    catch (e) {
      WARN("Failed to parse extensions.ini", e);
      return null;
    }

    
    for (let id in XPIProvider.bootstrappedAddons)
      bundles.push(XPIProvider.bootstrappedAddons[id].descriptor);

    return bundles;
  },

  





  getMigrateDataFromRDF: function XPIDB_getMigrateDataFromRDF(aDbWasMissing) {

    
    let rdffile = FileUtils.getFile(KEY_PROFILEDIR, [FILE_OLD_DATABASE], true);
    if (!rdffile.exists())
      return null;

    LOG("Migrating data from " + FILE_OLD_DATABASE);
    let migrateData = {};

    try {
      let ds = gRDF.GetDataSourceBlocking(Services.io.newFileURI(rdffile).spec);
      let root = Cc["@mozilla.org/rdf/container;1"].
                 createInstance(Ci.nsIRDFContainer);
      root.Init(ds, gRDF.GetResource(RDFURI_ITEM_ROOT));
      let elements = root.GetElements();

      while (elements.hasMoreElements()) {
        let source = elements.getNext().QueryInterface(Ci.nsIRDFResource);

        let location = getRDFProperty(ds, source, "installLocation");
        if (location) {
          if (!(location in migrateData))
            migrateData[location] = {};
          let id = source.ValueUTF8.substring(PREFIX_ITEM_URI.length);
          migrateData[location][id] = {
            version: getRDFProperty(ds, source, "version"),
            userDisabled: false,
            targetApplications: []
          }

          let disabled = getRDFProperty(ds, source, "userDisabled");
          if (disabled == "true" || disabled == "needs-disable")
            migrateData[location][id].userDisabled = true;

          let targetApps = ds.GetTargets(source, EM_R("targetApplication"),
                                         true);
          while (targetApps.hasMoreElements()) {
            let targetApp = targetApps.getNext()
                                      .QueryInterface(Ci.nsIRDFResource);
            let appInfo = {
              id: getRDFProperty(ds, targetApp, "id")
            };

            let minVersion = getRDFProperty(ds, targetApp, "updatedMinVersion");
            if (minVersion) {
              appInfo.minVersion = minVersion;
              appInfo.maxVersion = getRDFProperty(ds, targetApp, "updatedMaxVersion");
            }
            else {
              appInfo.minVersion = getRDFProperty(ds, targetApp, "minVersion");
              appInfo.maxVersion = getRDFProperty(ds, targetApp, "maxVersion");
            }
            migrateData[location][id].targetApplications.push(appInfo);
          }
        }
      }
    }
    catch (e) {
      WARN("Error reading " + FILE_OLD_DATABASE, e);
      migrateData = null;
    }

    return migrateData;
  },

  





  getMigrateDataFromDatabase: function XPIDB_getMigrateDataFromDatabase(aConnection) {
    let migrateData = {};

    
    
    try {
      var stmt = aConnection.createStatement("PRAGMA table_info(addon)");

      const REQUIRED = ["internal_id", "id", "location", "userDisabled",
                        "installDate", "version"];

      let reqCount = 0;
      let props = [];
      for (let row in resultRows(stmt)) {
        if (REQUIRED.indexOf(row.name) != -1) {
          reqCount++;
          props.push(row.name);
        }
        else if (DB_METADATA.indexOf(row.name) != -1) {
          props.push(row.name);
        }
        else if (DB_BOOL_METADATA.indexOf(row.name) != -1) {
          props.push(row.name);
        }
      }

      if (reqCount < REQUIRED.length) {
        ERROR("Unable to read anything useful from the database");
        return null;
      }
      stmt.finalize();

      stmt = aConnection.createStatement("SELECT " + props.join(",") + " FROM addon");
      for (let row in resultRows(stmt)) {
        if (!(row.location in migrateData))
          migrateData[row.location] = {};
        let addonData = {
          targetApplications: []
        }
        migrateData[row.location][row.id] = addonData;

        props.forEach(function(aProp) {
          if (aProp == "isForeignInstall")
            addonData.foreignInstall = (row[aProp] == 1);
          if (DB_BOOL_METADATA.indexOf(aProp) != -1)
            addonData[aProp] = row[aProp] == 1;
          else
            addonData[aProp] = row[aProp];
        })
      }

      var taStmt = aConnection.createStatement("SELECT id, minVersion, " +
                                                   "maxVersion FROM " +
                                                   "targetApplication WHERE " +
                                                   "addon_internal_id=:internal_id");

      for (let location in migrateData) {
        for (let id in migrateData[location]) {
          taStmt.params.internal_id = migrateData[location][id].internal_id;
          delete migrateData[location][id].internal_id;
          for (let row in resultRows(taStmt)) {
            migrateData[location][id].targetApplications.push({
              id: row.id,
              minVersion: row.minVersion,
              maxVersion: row.maxVersion
            });
          }
        }
      }
    }
    catch (e) {
      
      ERROR("Error migrating data", e);
      return null;
    }
    finally {
      if (taStmt)
        taStmt.finalize();
      if (stmt)
        stmt.finalize();
    }

    return migrateData;
  },

  





  shutdown: function XPIDB_shutdown() {
    LOG("shutdown");
    if (this.initialized) {
      
      if (this.lastError)
        this.saveChanges();

      this.initialized = false;

      if (this._deferredSave) {
        AddonManagerPrivate.recordSimpleMeasure(
            "XPIDB_saves_total", this._deferredSave.totalSaves);
        AddonManagerPrivate.recordSimpleMeasure(
            "XPIDB_saves_overlapped", this._deferredSave.overlappedSaves);
        AddonManagerPrivate.recordSimpleMeasure(
            "XPIDB_saves_late", this._deferredSave.dirty ? 1 : 0);
      }

      
      
      let flushPromise = this.flush();
      flushPromise.then(null, error => {
          ERROR("Flush of XPI database failed", error);
          AddonManagerPrivate.recordSimpleMeasure("XPIDB_shutdownFlush_failed", 1);
          
          
          Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, true);
        })
        .then(count => {
          
          delete this.addonDB;
          delete this._dbPromise;
          
          delete this._deferredSave;
          
          delete this._schemaVersionSet;
        });
      return flushPromise;
    }
    return Promise.resolve(0);
  },

  








  getInstallLocations: function XPIDB_getInstallLocations() {
    let locations = new Set();
    if (!this.addonDB)
      return locations;

    for (let [, addon] of this.addonDB) {
      locations.add(addon.location);
    }
    return locations;
  },

  








  getAddonList: function(aFilter, aCallback) {
    this.asyncLoadDB().then(
      addonDB => {
        let addonList = _filterDB(addonDB, aFilter);
        asyncMap(addonList, getRepositoryAddon, makeSafe(aCallback));
      })
    .then(null,
        error => {
          ERROR("getAddonList failed", e);
          makeSafe(aCallback)([]);
        });
  },

  







  getAddon: function(aFilter, aCallback) {
    return this.asyncLoadDB().then(
      addonDB => {
        getRepositoryAddon(_findAddon(addonDB, aFilter), makeSafe(aCallback));
      })
    .then(null,
        error => {
          ERROR("getAddon failed", e);
          makeSafe(aCallback)(null);
        });
  },

  







  getAddonsInLocation: function XPIDB_getAddonsInLocation(aLocation) {
    return _filterDB(this.addonDB, aAddon => (aAddon.location == aLocation));
  },

  










  getAddonInLocation: function XPIDB_getAddonInLocation(aId, aLocation, aCallback) {
    this.asyncLoadDB().then(
        addonDB => getRepositoryAddon(addonDB.get(aLocation + ":" + aId),
                                      makeSafe(aCallback)));
  },

  







  getVisibleAddonForID: function XPIDB_getVisibleAddonForID(aId, aCallback) {
    this.getAddon(aAddon => ((aAddon.id == aId) && aAddon.visible),
                  aCallback);
  },

  







  getVisibleAddons: function XPIDB_getVisibleAddons(aTypes, aCallback) {
    this.getAddonList(aAddon => (aAddon.visible &&
                                 (!aTypes || (aTypes.length == 0) ||
                                  (aTypes.indexOf(aAddon.type) > -1))),
                      aCallback);
  },

  






  getAddonsByType: function XPIDB_getAddonsByType(aType) {
    if (!this.addonDB) {
      
      
      
      WARN("Synchronous load of XPI database due to getAddonsByType(" + aType + ")");
      AddonManagerPrivate.recordSimpleMeasure("XPIDB_lateOpen_byType", XPIProvider.runPhase);
      this.syncLoadDB(true);
    }
    return _filterDB(this.addonDB, aAddon => (aAddon.type == aType));
  },

  






  getVisibleAddonForInternalName: function XPIDB_getVisibleAddonForInternalName(aInternalName) {
    if (!this.addonDB) {
      
      WARN("Synchronous load of XPI database due to getVisibleAddonForInternalName");
      AddonManagerPrivate.recordSimpleMeasure("XPIDB_lateOpen_forInternalName",
          XPIProvider.runPhase);
      this.syncLoadDB(true);
    }
    
    return _findAddon(this.addonDB,
                      aAddon => aAddon.visible &&
                                (aAddon.internalName == aInternalName));
  },

  







  getVisibleAddonsWithPendingOperations:
    function XPIDB_getVisibleAddonsWithPendingOperations(aTypes, aCallback) {

    this.getAddonList(
        aAddon => (aAddon.visible &&
                   (aAddon.pendingUninstall ||
                    
                    
                    
                    (aAddon.active == (aAddon.userDisabled || aAddon.appDisabled))) &&
                   (!aTypes || (aTypes.length == 0) || (aTypes.indexOf(aAddon.type) > -1))),
        aCallback);
  },

  









  getAddonBySyncGUID: function XPIDB_getAddonBySyncGUID(aGUID, aCallback) {
    this.getAddon(aAddon => aAddon.syncGUID == aGUID,
                  aCallback);
  },

  







  getAddons: function XPIDB_getAddons() {
    if (!this.addonDB) {
      return [];
    }
    return _filterDB(this.addonDB, aAddon => true);
  },

  








  addAddonMetadata: function XPIDB_addAddonMetadata(aAddon, aDescriptor) {
    if (!this.addonDB) {
      AddonManagerPrivate.recordSimpleMeasure("XPIDB_lateOpen_addMetadata",
          XPIProvider.runPhase);
      this.syncLoadDB(false);
    }

    let newAddon = new DBAddonInternal(aAddon);
    newAddon.descriptor = aDescriptor;
    this.addonDB.set(newAddon._key, newAddon);
    if (newAddon.visible) {
      this.makeAddonVisible(newAddon);
    }

    this.saveChanges();
    return newAddon;
  },

  











  updateAddonMetadata: function XPIDB_updateAddonMetadata(aOldAddon, aNewAddon,
                                                          aDescriptor) {
    this.removeAddonMetadata(aOldAddon);
    aNewAddon.syncGUID = aOldAddon.syncGUID;
    aNewAddon.installDate = aOldAddon.installDate;
    aNewAddon.applyBackgroundUpdates = aOldAddon.applyBackgroundUpdates;
    aNewAddon.foreignInstall = aOldAddon.foreignInstall;
    aNewAddon.active = (aNewAddon.visible && !aNewAddon.userDisabled &&
                        !aNewAddon.appDisabled && !aNewAddon.pendingUninstall);

    
    return this.addAddonMetadata(aNewAddon, aDescriptor);
  },

  





  removeAddonMetadata: function XPIDB_removeAddonMetadata(aAddon) {
    this.addonDB.delete(aAddon._key);
    this.saveChanges();
  },

  








  makeAddonVisible: function XPIDB_makeAddonVisible(aAddon) {
    LOG("Make addon " + aAddon._key + " visible");
    for (let [, otherAddon] of this.addonDB) {
      if ((otherAddon.id == aAddon.id) && (otherAddon._key != aAddon._key)) {
        LOG("Hide addon " + otherAddon._key);
        otherAddon.visible = false;
      }
    }
    aAddon.visible = true;
    this.saveChanges();
  },

  







  setAddonProperties: function XPIDB_setAddonProperties(aAddon, aProperties) {
    for (let key in aProperties) {
      aAddon[key] = aProperties[key];
    }
    this.saveChanges();
  },

  









  setAddonSyncGUID: function XPIDB_setAddonSyncGUID(aAddon, aGUID) {
    
    function excludeSyncGUID(otherAddon) {
      return (otherAddon._key != aAddon._key) && (otherAddon.syncGUID == aGUID);
    }
    let otherAddon = _findAddon(this.addonDB, excludeSyncGUID);
    if (otherAddon) {
      throw new Error("Addon sync GUID conflict for addon " + aAddon._key +
          ": " + otherAddon._key + " already has GUID " + aGUID);
    }
    aAddon.syncGUID = aGUID;
    this.saveChanges();
  },

  





  updateAddonActive: function XPIDB_updateAddonActive(aAddon, aActive) {
    LOG("Updating active state for add-on " + aAddon.id + " to " + aActive);

    aAddon.active = aActive;
    this.saveChanges();
  },

  


  updateActiveAddons: function XPIDB_updateActiveAddons() {
    if (!this.addonDB) {
      WARN("updateActiveAddons called when DB isn't loaded");
      
      AddonManagerPrivate.recordSimpleMeasure("XPIDB_lateOpen_updateActive",
          XPIProvider.runPhase);
      this.syncLoadDB(true);
    }
    LOG("Updating add-on states");
    for (let [, addon] of this.addonDB) {
      let newActive = (addon.visible && !addon.userDisabled &&
                      !addon.softDisabled && !addon.appDisabled &&
                      !addon.pendingUninstall);
      if (newActive != addon.active) {
        addon.active = newActive;
        this.saveChanges();
      }
    }
  },

  



  writeAddonsList: function XPIDB_writeAddonsList() {
    if (!this.addonDB) {
      
      AddonManagerPrivate.recordSimpleMeasure("XPIDB_lateOpen_writeList",
          XPIProvider.runPhase);
      this.syncLoadDB(true);
    }
    Services.appinfo.invalidateCachesOnRestart();

    let addonsList = FileUtils.getFile(KEY_PROFILEDIR, [FILE_XPI_ADDONS_LIST],
                                       true);
    let enabledAddons = [];
    let text = "[ExtensionDirs]\r\n";
    let count = 0;
    let fullCount = 0;

    let activeAddons = _filterDB(
      this.addonDB,
      aAddon => aAddon.active && !aAddon.bootstrap && (aAddon.type != "theme"));

    for (let row of activeAddons) {
      text += "Extension" + (count++) + "=" + row.descriptor + "\r\n";
      enabledAddons.push(encodeURIComponent(row.id) + ":" +
                         encodeURIComponent(row.version));
    }
    fullCount += count;

    
    
    text += "\r\n[ThemeDirs]\r\n";

    let dssEnabled = false;
    try {
      dssEnabled = Services.prefs.getBoolPref(PREF_EM_DSS_ENABLED);
    } catch (e) {}

    let themes = [];
    if (dssEnabled) {
      themes = _filterDB(this.addonDB, aAddon => aAddon.type == "theme");
    }
    else {
      let activeTheme = _findAddon(
        this.addonDB,
        aAddon => (aAddon.type == "theme") &&
                  (aAddon.internalName == XPIProvider.selectedSkin));
      if (activeTheme) {
        themes.push(activeTheme);
      }
    }

    if (themes.length > 0) {
      count = 0;
      for (let row of themes) {
        text += "Extension" + (count++) + "=" + row.descriptor + "\r\n";
        enabledAddons.push(encodeURIComponent(row.id) + ":" +
                           encodeURIComponent(row.version));
      }
      fullCount += count;
    }

    if (fullCount > 0) {
      LOG("Writing add-ons list");

      try {
        let addonsListTmp = FileUtils.getFile(KEY_PROFILEDIR, [FILE_XPI_ADDONS_LIST + ".tmp"],
                                              true);
        var fos = FileUtils.openFileOutputStream(addonsListTmp);
        fos.write(text, text.length);
        fos.close();
        addonsListTmp.moveTo(addonsListTmp.parent, FILE_XPI_ADDONS_LIST);

        Services.prefs.setCharPref(PREF_EM_ENABLED_ADDONS, enabledAddons.join(","));
      }
      catch (e) {
        ERROR("Failed to write add-ons list to " + addonsListTmp.parent + "/" +
              FILE_XPI_ADDONS_LIST, e);
        return false;
      }
    }
    else {
      if (addonsList.exists()) {
        LOG("Deleting add-ons list");
        try {
          addonsList.remove(false);
        }
        catch (e) {
          ERROR("Failed to remove " + addonsList.path, e);
          return false;
        }
      }

      Services.prefs.clearUserPref(PREF_EM_ENABLED_ADDONS);
    }
    return true;
  }
};
