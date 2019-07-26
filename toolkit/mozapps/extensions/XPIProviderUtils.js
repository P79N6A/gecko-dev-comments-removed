



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AddonRepository",
                                  "resource://gre/modules/AddonRepository.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");


["LOG", "WARN", "ERROR"].forEach(function(aName) {
  Object.defineProperty(this, aName, {
    get: function logFuncGetter () {
      Components.utils.import("resource://gre/modules/AddonLogging.jsm");

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

const FIELDS_ADDON = "internal_id, id, syncGUID, location, version, type, " +
                     "internalName, updateURL, updateKey, optionsURL, " +
                     "optionsType, aboutURL, iconURL, icon64URL, " +
                     "defaultLocale, visible, active, userDisabled, " +
                     "appDisabled, pendingUninstall, descriptor, " +
                     "installDate, updateDate, applyBackgroundUpdates, bootstrap, " +
                     "skinnable, size, sourceURI, releaseNotesURI, softDisabled, " +
                     "isForeignInstall, hasBinaryComponents, strictCompatibility";



const PROP_METADATA      = ["id", "version", "type", "internalName", "updateURL",
                            "updateKey", "optionsURL", "optionsType", "aboutURL",
                            "iconURL", "icon64URL"];
const PROP_LOCALE_SINGLE = ["name", "description", "creator", "homepageURL"];
const PROP_LOCALE_MULTI  = ["developers", "translators", "contributors"];
const PROP_TARGETAPP     = ["id", "minVersion", "maxVersion"];


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


const PREFIX_ITEM_URI                 = "urn:mozilla:item:";
const RDFURI_ITEM_ROOT                = "urn:mozilla:item:root"
const PREFIX_NS_EM                    = "http://www.mozilla.org/2004/em-rdf#";

Object.defineProperty(this, "gRDF", {
  get: function gRDFGetter() {
    delete this.gRDF;
    return this.gRDF = Cc["@mozilla.org/rdf/rdf-service;1"].
                       getService(Ci.nsIRDFService);
  },
  configurable: true
});

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











function AsyncAddonListCallback(aCallback) {
  this.callback = aCallback;
  this.addons = [];
}

AsyncAddonListCallback.prototype = {
  callback: null,
  complete: false,
  count: 0,
  addons: null,

  handleResult: function AsyncAddonListCallback_handleResult(aResults) {
    let row = null;
    while ((row = aResults.getNextRow())) {
      this.count++;
      let self = this;
      XPIDatabase.makeAddonFromRowAsync(row, function handleResult_makeAddonFromRowAsync(aAddon) {
        function completeAddon(aRepositoryAddon) {
          aAddon._repositoryAddon = aRepositoryAddon;
          aAddon.compatibilityOverrides = aRepositoryAddon ?
                                            aRepositoryAddon.compatibilityOverrides :
                                            null;
          self.addons.push(aAddon);
          if (self.complete && self.addons.length == self.count)
           self.callback(self.addons);
        }

        if ("getCachedAddonByID" in AddonRepository)
          AddonRepository.getCachedAddonByID(aAddon.id, completeAddon);
        else
          completeAddon(null);
      });
    }
  },

  handleError: asyncErrorLogger,

  handleCompletion: function AsyncAddonListCallback_handleCompletion(aReason) {
    this.complete = true;
    if (this.addons.length == this.count)
      this.callback(this.addons);
  }
};




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








function executeStatement(aStatement) {
  try {
    aStatement.execute();
  }
  catch (e) {
    logSQLError(XPIDatabase.connection.lastError,
                XPIDatabase.connection.lastErrorString);
    throw e;
  }
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

  Object.defineProperty(this, "pendingUpgrade", {
    get: function DBA_pendingUpgradeGetter() {
      delete this.pendingUpgrade;
      for (let install of XPIProvider.installs) {
        if (install.state == AddonManager.STATE_INSTALLED &&
            !(install.addon.inDatabase) &&
            install.addon.id == this.id &&
            install.installLocation == this._installLocation) {
          return this.pendingUpgrade = install.addon;
        }
      };
    },
    configurable: true
  });
}

DBAddonInternal.prototype = {
  applyCompatibilityUpdate: function DBA_applyCompatibilityUpdate(aUpdate, aSyncCompatibility) {
    XPIDatabase.beginTransaction();
    this.targetApplications.forEach(function(aTargetApp) {
      aUpdate.targetApplications.forEach(function(aUpdateTarget) {
        if (aTargetApp.id == aUpdateTarget.id && (aSyncCompatibility ||
            Services.vc.compare(aTargetApp.maxVersion, aUpdateTarget.maxVersion) < 0)) {
          aTargetApp.minVersion = aUpdateTarget.minVersion;
          aTargetApp.maxVersion = aUpdateTarget.maxVersion;
        }
      });
    });
    XPIProvider.updateAddonDisabledState(this);
    XPIDatabase.commitTransaction();
  },
  get inDatabase() {
    return true;
  }
}

DBAddonInternal.prototype.__proto__ = AddonInternal.prototype;

this.XPIDatabase = {
  
  initialized: false,
  
  statementCache: {},
  
  
  addonCache: [],
  
  transactionCount: 0,
  
  dbfile: FileUtils.getFile(KEY_PROFILEDIR, [FILE_DATABASE], true),
  jsonFile: FileUtils.getFile(KEY_PROFILEDIR, [FILE_JSON_DB], true),
  
  migrateData: null,
  
  activeBundles: null,

  
  statements: {
    _getDefaultLocale: "SELECT id, name, description, creator, homepageURL " +
                       "FROM locale WHERE id=:id",
    _getLocales: "SELECT addon_locale.locale, locale.id, locale.name, " +
                 "locale.description, locale.creator, locale.homepageURL " +
                 "FROM addon_locale JOIN locale ON " +
                 "addon_locale.locale_id=locale.id WHERE " +
                 "addon_internal_id=:internal_id",
    _getTargetApplications: "SELECT addon_internal_id, id, minVersion, " +
                            "maxVersion FROM targetApplication WHERE " +
                            "addon_internal_id=:internal_id",
    _getTargetPlatforms: "SELECT os, abi FROM targetPlatform WHERE " +
                         "addon_internal_id=:internal_id",
    _readLocaleStrings: "SELECT locale_id, type, value FROM locale_strings " +
                        "WHERE locale_id=:id",

    clearVisibleAddons: "UPDATE addon SET visible=0 WHERE id=:id",
    updateAddonActive: "UPDATE addon SET active=:active WHERE " +
                       "internal_id=:internal_id",

    getActiveAddons: "SELECT " + FIELDS_ADDON + " FROM addon WHERE active=1 AND " +
                     "type<>'theme' AND bootstrap=0",
    getActiveTheme: "SELECT " + FIELDS_ADDON + " FROM addon WHERE " +
                    "internalName=:internalName AND type='theme'",
    getThemes: "SELECT " + FIELDS_ADDON + " FROM addon WHERE type='theme'",

    getAddonInLocation: "SELECT " + FIELDS_ADDON + " FROM addon WHERE id=:id " +
                        "AND location=:location",
    getAddons: "SELECT " + FIELDS_ADDON + " FROM addon",
    getAddonsByType: "SELECT " + FIELDS_ADDON + " FROM addon WHERE type=:type",
    getAddonsInLocation: "SELECT " + FIELDS_ADDON + " FROM addon WHERE " +
                         "location=:location",
    getInstallLocations: "SELECT DISTINCT location FROM addon",
    getVisibleAddonForID: "SELECT " + FIELDS_ADDON + " FROM addon WHERE " +
                          "visible=1 AND id=:id",
    getVisibleAddonForInternalName: "SELECT " + FIELDS_ADDON + " FROM addon " +
                                    "WHERE visible=1 AND internalName=:internalName",
    getVisibleAddons: "SELECT " + FIELDS_ADDON + " FROM addon WHERE visible=1",
    getVisibleAddonsWithPendingOperations: "SELECT " + FIELDS_ADDON + " FROM " +
                                           "addon WHERE visible=1 " +
                                           "AND (pendingUninstall=1 OR " +
                                           "MAX(userDisabled,appDisabled)=active)",
    getAddonBySyncGUID: "SELECT " + FIELDS_ADDON + " FROM addon " +
                        "WHERE syncGUID=:syncGUID",
    makeAddonVisible: "UPDATE addon SET visible=1 WHERE internal_id=:internal_id",
    removeAddonMetadata: "DELETE FROM addon WHERE internal_id=:internal_id",
    
    
    setActiveAddons: "UPDATE addon SET active=MIN(visible, 1 - userDisabled, " +
                     "1 - softDisabled, 1 - appDisabled, 1 - pendingUninstall)",
    setAddonProperties: "UPDATE addon SET userDisabled=:userDisabled, " +
                        "appDisabled=:appDisabled, " +
                        "softDisabled=:softDisabled, " +
                        "pendingUninstall=:pendingUninstall, " +
                        "applyBackgroundUpdates=:applyBackgroundUpdates WHERE " +
                        "internal_id=:internal_id",
    setAddonDescriptor: "UPDATE addon SET descriptor=:descriptor WHERE " +
                        "internal_id=:internal_id",
    setAddonSyncGUID: "UPDATE addon SET syncGUID=:syncGUID WHERE " +
                      "internal_id=:internal_id",
    updateTargetApplications: "UPDATE targetApplication SET " +
                              "minVersion=:minVersion, maxVersion=:maxVersion " +
                              "WHERE addon_internal_id=:internal_id AND id=:id",

    createSavepoint: "SAVEPOINT 'default'",
    releaseSavepoint: "RELEASE SAVEPOINT 'default'",
    rollbackSavepoint: "ROLLBACK TO SAVEPOINT 'default'"
  },

  get dbfileExists() {
    delete this.dbfileExists;
    return this.dbfileExists = this.dbfile.exists();
  },
  set dbfileExists(aValue) {
    delete this.dbfileExists;
    return this.dbfileExists = aValue;
  },

  




  writeJSON: function XPIDB_writeJSON() {
    
    let addons = [];
    for (let aKey in this.addonDB) {
      addons.push(copyProperties(this.addonDB[aKey], PROP_JSON_FIELDS));
    }
    let toSave = {
      schemaVersion: DB_SCHEMA,
      addons: addons
    };

    let stream = FileUtils.openSafeFileOutputStream(this.jsonFile);
    let converter = Cc["@mozilla.org/intl/converter-output-stream;1"].
      createInstance(Ci.nsIConverterOutputStream);
    try {
      converter.init(stream, "UTF-8", 0, 0x0000);
      
      converter.writeString(JSON.stringify(toSave, null, 2));
      converter.flush();
      
      FileUtils.closeSafeFileOutputStream(stream);
      converter.close();
    }
    catch(e) {
      ERROR("Failed to save database to JSON", e);
      stream.close();
    }
  },

  





  openJSONDatabase: function XPIDB_openJSONDatabase() {
    dump("XPIDB_openJSONDatabase\n");
    try {
      let data = "";
      let fstream = Components.classes["@mozilla.org/network/file-input-stream;1"].
              createInstance(Components.interfaces.nsIFileInputStream);
      let cstream = Components.classes["@mozilla.org/intl/converter-input-stream;1"].
              createInstance(Components.interfaces.nsIConverterInputStream);
      fstream.init(this.jsonFile, -1, 0, 0);
      cstream.init(fstream, "UTF-8", 0, 0);
      let (str = {}) {
        let read = 0;
        do {
          read = cstream.readString(0xffffffff, str); 
          data += str.value;
        } while (read != 0);
      }
      cstream.close();
      let inputAddons = JSON.parse(data);
      
      if (!("schemaVersion" in inputAddons) || !("addons" in inputAddons)) {
        
        ERROR("bad JSON file contents");
        delete this.addonDB;
        this.addonDB = {};
        return false;
      }
      if (inputAddons.schemaVersion != DB_SCHEMA) {
        
        ERROR("JSON schema upgrade needed");
        return false;
      }
      
      
      delete this.addonDB;
      let addonDB = {}
      inputAddons.addons.forEach(function(loadedAddon) {
        let newAddon = new DBAddonInternal(loadedAddon);
        addonDB[newAddon._key] = newAddon;
      });
      this.addonDB = addonDB;
      
      return true;
    }
    catch(e) {
      
      ERROR("Failed to load XPI JSON data from profile", e);
      
      delete this.addonDB;
      this.addonDB = {};
      return false;
    }
  },

  








  beginTransaction: function XPIDB_beginTransaction() {
    this.transactionCount++;
  },

  



  commitTransaction: function XPIDB_commitTransaction() {
    if (this.transactionCount == 0) {
      ERROR("Attempt to commit one transaction too many.");
      return;
    }

    this.transactionCount--;

    if (this.transactionCount == 0) {
      
      this.writeJSON();
    }
  },

  



  rollbackTransaction: function XPIDB_rollbackTransaction() {
    if (this.transactionCount == 0) {
      ERROR("Attempt to rollback one transaction too many.");
      return;
    }

    this.transactionCount--;
    
  },

  








  openDatabaseFile: function XPIDB_openDatabaseFile(aDBFile) {
    LOG("Opening database");
    let connection = null;

    
    try {
      connection = Services.storage.openUnsharedDatabase(aDBFile);
      this.dbfileExists = true;
    }
    catch (e) {
      ERROR("Failed to open database (1st attempt)", e);
      
      
      if (e.result != Cr.NS_ERROR_STORAGE_BUSY) {
        try {
          aDBFile.remove(true);
        }
        catch (e) {
          ERROR("Failed to remove database that could not be opened", e);
        }
        try {
          connection = Services.storage.openUnsharedDatabase(aDBFile);
        }
        catch (e) {
          ERROR("Failed to open database (2nd attempt)", e);

          
          
          
          return Services.storage.openSpecialDatabase("memory");
        }
      }
      else {
        return Services.storage.openSpecialDatabase("memory");
      }
    }

    connection.executeSimpleSQL("PRAGMA synchronous = FULL");
    connection.executeSimpleSQL("PRAGMA locking_mode = EXCLUSIVE");

    return connection;
  },

  






  openConnection: function XPIDB_openConnection(aRebuildOnError, aForceOpen) {
    this.openJSONDatabase();
    this.initialized = true;
    return;
    

    delete this.connection;

    if (!aForceOpen && !this.dbfileExists) {
      this.connection = null;
      return;
    }

    this.migrateData = null;

    this.connection = this.openDatabaseFile(this.dbfile);

    
    
    let schemaVersion = this.connection.schemaVersion;
    if (schemaVersion != DB_SCHEMA) {
      
      
      
      if (schemaVersion != 0) {
        LOG("Migrating data from schema " + schemaVersion);
        this.migrateData = this.getMigrateDataFromDatabase();

        
        this.connection.close();
        try {
          if (this.dbfileExists)
            this.dbfile.remove(true);

          
          this.connection = this.openDatabaseFile(this.dbfile);
        }
        catch (e) {
          ERROR("Failed to remove old database", e);
          
          
          this.connection = Services.storage.openSpecialDatabase("memory");
        }
      }
      else {
        let dbSchema = 0;
        try {
          dbSchema = Services.prefs.getIntPref(PREF_DB_SCHEMA);
        } catch (e) {}

        if (dbSchema == 0) {
          
          this.migrateData = this.getMigrateDataFromRDF();
        }
      }

      
      try {
        this.createSchema();
      }
      catch (e) {
        
        
        this.connection = Services.storage.openSpecialDatabase("memory");
      }

      
      
      if (!this.migrateData)
        this.activeBundles = this.getActiveBundles();

      if (aRebuildOnError) {
        WARN("Rebuilding add-ons database from installed extensions.");
        this.beginTransaction();
        try {
          let state = XPIProvider.getInstallLocationStates();
          XPIProvider.processFileChanges(state, {}, false);
          
          Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, true);
          this.commitTransaction();
        }
        catch (e) {
          ERROR("Error processing file changes", e);
          this.rollbackTransaction();
        }
      }
    }

    
    
    if (this.connection.databaseFile) {
      Services.prefs.setIntPref(PREF_DB_SCHEMA, DB_SCHEMA);
      Services.prefs.savePrefFile(null);
    }

    
    for (let i = 0; i < this.transactionCount; i++)
      this.connection.executeSimpleSQL("SAVEPOINT 'default'");
  },

  


  get addonDB() {
    delete this.addonDB;
    this.openJSONDatabase();
    return this.addonDB;
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

  





  getMigrateDataFromDatabase: function XPIDB_getMigrateDataFromDatabase() {
    let migrateData = {};

    
    
    try {
      var stmt = this.connection.createStatement("PRAGMA table_info(addon)");

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

      stmt = this.connection.createStatement("SELECT " + props.join(",") + " FROM addon");
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

      var taStmt = this.connection.createStatement("SELECT id, minVersion, " +
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

  


  shutdown: function XPIDB_shutdown(aCallback) {
    LOG("shutdown");
    if (this.initialized) {
      if (this.transactionCount > 0) {
        ERROR(this.transactionCount + " outstanding transactions, rolling back.");
        while (this.transactionCount > 0)
          this.rollbackTransaction();
      }

      
      
      
      
      
      

      this.initialized = false;

      
      
      delete this.addonDB;
      Object.defineProperty(this, "addonDB", {
        get: function addonsGetter() {
          this.openJSONDatabase();
          return this.addonDB;
        },
        configurable: true
      });
      
      
      
      if (aCallback)
        aCallback();
    }
    else {
      if (aCallback)
        aCallback();
    }
  },

  







  getInstallLocations: function XPIDB_getInstallLocations() {
    if (!this.addonDB)
      return [];

    let locations = {};
    for each (let addon in this.addonDB) {
      locations[addon.location] = 1;
    }
    return Object.keys(locations);
  },

  






  _listAddons: function XPIDB_listAddons(aFilter) {
    if (!this.addonDB)
      return [];

    let addonList = [];
    for (let key in this.addonDB) {
      let addon = this.addonDB[key];
      if (aFilter(addon)) {
        addonList.push(addon);
      }
    }

    return addonList;
  },

  






  _findAddon: function XPIDB_findAddon(aFilter) {
    if (!this.addonDB)
      return null;

    for (let key in this.addonDB) {
      let addon = this.addonDB[key];
      if (aFilter(addon)) {
        return addon;
      }
    }

    return null;
  },

  






  getAddonsInLocation: function XPIDB_getAddonsInLocation(aLocation) {
    return this._listAddons(function inLocation(aAddon) {return (aAddon.location == aLocation);});
  },

  











  getAddonInLocation: function XPIDB_getAddonInLocation(aId, aLocation, aCallback) {
    getRepositoryAddon(this.addonDB[aLocation + ":" + aId], aCallback);
  },

  








  getVisibleAddonForID: function XPIDB_getVisibleAddonForID(aId, aCallback) {
    let addon = this._findAddon(function visibleID(aAddon) {return ((aAddon.id == aId) && aAddon.visible)});
    getRepositoryAddon(addon, aCallback);
  },

  








  getVisibleAddons: function XPIDB_getVisibleAddons(aTypes, aCallback) {
    let addons = this._listAddons(function visibleType(aAddon) {
      return (aAddon.visible && (!aTypes || (aTypes.length == 0) || (aTypes.indexOf(aAddon.type) > -1)))
    });
    asyncMap(addons, getRepositoryAddon, aCallback);
  },

  






  getAddonsByType: function XPIDB_getAddonsByType(aType) {
    return this._listAddons(function byType(aAddon) { return aAddon.type == aType; });
  },

  






  getVisibleAddonForInternalName: function XPIDB_getVisibleAddonForInternalName(aInternalName) {
    return this._findAddon(function visibleInternalName(aAddon) {
      return (aAddon.visible && (aAddon.internalName == aInternalName));
    });
  },

  








  getVisibleAddonsWithPendingOperations:
    function XPIDB_getVisibleAddonsWithPendingOperations(aTypes, aCallback) {

    let addons = this._listAddons(function visibleType(aAddon) {
      return (aAddon.visible &&
        (aAddon.pendingUninstall ||
         
         
         
         (aAddon.active == (aAddon.userDisabled || aAddon.appDisabled))) &&
        (!aTypes || (aTypes.length == 0) || (aTypes.indexOf(aAddon.type) > -1)))
    });
    asyncMap(addons, getRepositoryAddon, aCallback);
  },

  










  getAddonBySyncGUID: function XPIDB_getAddonBySyncGUID(aGUID, aCallback) {
    let addon = this._findAddon(function bySyncGUID(aAddon) { return aAddon.syncGUID == aGUID; });
    getRepositoryAddon(addon, aCallback);
  },

  




  getAddons: function XPIDB_getAddons() {
    return this._listAddons(function(aAddon) {return true;});
  },

  








  addAddonMetadata: function XPIDB_addAddonMetadata(aAddon, aDescriptor) {
    
    
    
    
    if (!this.addonDB)
      this.openConnection(false, true);

    this.beginTransaction();

    let newAddon = new DBAddonInternal(aAddon);
    newAddon.descriptor = aDescriptor;
    this.addonDB[newAddon._key] = newAddon;
    if (newAddon.visible) {
      this.makeAddonVisible(newAddon);
    }

    this.commitTransaction();
    return newAddon;
  },

  











  updateAddonMetadata: function XPIDB_updateAddonMetadata(aOldAddon, aNewAddon,
                                                          aDescriptor) {
    this.beginTransaction();

    
    try {
      this.removeAddonMetadata(aOldAddon);
      aNewAddon.syncGUID = aOldAddon.syncGUID;
      aNewAddon.installDate = aOldAddon.installDate;
      aNewAddon.applyBackgroundUpdates = aOldAddon.applyBackgroundUpdates;
      aNewAddon.foreignInstall = aOldAddon.foreignInstall;
      aNewAddon.active = (aNewAddon.visible && !aNewAddon.userDisabled &&
                          !aNewAddon.appDisabled && !aNewAddon.pendingUninstall)

      let newDBAddon = this.addAddonMetadata(aNewAddon, aDescriptor);
      this.commitTransaction();
      return newDBAddon;
    }
    catch (e) {
      this.rollbackTransaction();
      throw e;
    }
  },

  





  removeAddonMetadata: function XPIDB_removeAddonMetadata(aAddon) {
    this.beginTransaction();
    delete this.addonDB[aAddon._key];
    this.commitTransaction();
  },

  








  makeAddonVisible: function XPIDB_makeAddonVisible(aAddon) {
    this.beginTransaction();
    LOG("Make addon " + aAddon._key + " visible");
    for (let key in this.addonDB) {
      let otherAddon = this.addonDB[key];
      if ((otherAddon.id == aAddon.id) && (otherAddon._key != aAddon._key)) {
        LOG("Hide addon " + otherAddon._key);
        otherAddon.visible = false;
      }
    }
    aAddon.visible = true;
    this.commitTransaction();
  },

  







  setAddonProperties: function XPIDB_setAddonProperties(aAddon, aProperties) {
    this.beginTransaction();
    for (let key in aProperties) {
      aAddon[key] = aProperties[key];
    }
    this.commitTransaction();
  },

  








  setAddonSyncGUID: function XPIDB_setAddonSyncGUID(aAddon, aGUID) {
    
    function excludeSyncGUID(otherAddon) {
      return (otherAddon._key != aAddon._key) && (otherAddon.syncGUID == aGUID);
    }
    let otherAddon = this._findAddon(excludeSyncGUID);
    if (otherAddon) {
      throw new Error("Addon sync GUID conflict for addon " + aAddon._key +
          ": " + otherAddon._key + " already has GUID " + aGUID);
    }
    this.beginTransaction();
    aAddon.syncGUID = aGUID;
    this.commitTransaction();
  },

  








  setAddonDescriptor: function XPIDB_setAddonDescriptor(aAddon, aDescriptor) {
    this.beginTransaction();
    aAddon.descriptor = aDescriptor;
    this.commitTransaction();
  },

  





  updateAddonActive: function XPIDB_updateAddonActive(aAddon, aActive) {
    LOG("Updating active state for add-on " + aAddon.id + " to " + aActive);

    this.beginTransaction();
    aAddon.active = aActive;
    this.commitTransaction();
  },

  


  updateActiveAddons: function XPIDB_updateActiveAddons() {
    
    
    LOG("Updating add-on states");
    this.beginTransaction();
    for (let key in this.addonDB) {
      let addon = this.addonDB[key];
      addon.active = (addon.visible && !addon.userDisabled &&
                      !addon.softDisabled && !addon.appDisabled &&
                      !addon.pendingUninstall);
    }
    this.commitTransaction();
  },

  


  writeAddonsList: function XPIDB_writeAddonsList() {
    Services.appinfo.invalidateCachesOnRestart();

    let addonsList = FileUtils.getFile(KEY_PROFILEDIR, [FILE_XPI_ADDONS_LIST],
                                       true);
    let enabledAddons = [];
    let text = "[ExtensionDirs]\r\n";
    let count = 0;
    let fullCount = 0;

    let activeAddons = this._listAddons(function active(aAddon) {
      return aAddon.active && !aAddon.bootstrap && (aAddon.type != "theme");
    });

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
      themes = this._listAddons(function isTheme(aAddon){ return aAddon.type == "theme"; });
    }
    else {
      let activeTheme = this._findAddon(function isSelected(aAddon) {
        return ((aAddon.type == "theme") && (aAddon.internalName == XPIProvider.selectedSkin));
      });
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

      let addonsListTmp = FileUtils.getFile(KEY_PROFILEDIR, [FILE_XPI_ADDONS_LIST + ".tmp"],
                                            true);
      var fos = FileUtils.openFileOutputStream(addonsListTmp);
      fos.write(text, text.length);
      fos.close();
      addonsListTmp.moveTo(addonsListTmp.parent, FILE_XPI_ADDONS_LIST);

      Services.prefs.setCharPref(PREF_EM_ENABLED_ADDONS, enabledAddons.join(","));
    }
    else {
      if (addonsList.exists()) {
        LOG("Deleting add-ons list");
        addonsList.remove(false);
      }

      Services.prefs.clearUserPref(PREF_EM_ENABLED_ADDONS);
    }
  }
};
