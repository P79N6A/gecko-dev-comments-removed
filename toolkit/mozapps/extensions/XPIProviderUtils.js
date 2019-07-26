



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
  this.__defineGetter__(aName, function() {
    Components.utils.import("resource://gre/modules/AddonLogging.jsm");

    LogManager.getLogger("addons.xpi-utils", this);
    return this[aName];
  })
}, this);


const KEY_PROFILEDIR                  = "ProfD";
const FILE_DATABASE                   = "extensions.sqlite";
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



const PREFIX_ITEM_URI                 = "urn:mozilla:item:";
const RDFURI_ITEM_ROOT                = "urn:mozilla:item:root"
const PREFIX_NS_EM                    = "http://www.mozilla.org/2004/em-rdf#";


var XPIProvider;


this.__defineGetter__("gRDF", function() {
  delete this.gRDF;
  return this.gRDF = Cc["@mozilla.org/rdf/rdf-service;1"].
                     getService(Ci.nsIRDFService);
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

  handleResult: function(aResults) {
    let row = null;
    while ((row = aResults.getNextRow())) {
      this.count++;
      let self = this;
      XPIDatabase.makeAddonFromRowAsync(row, function(aAddon) {
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

  handleCompletion: function(aReason) {
    this.complete = true;
    if (this.addons.length == this.count)
      this.callback(this.addons);
  }
};








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

var XPIDatabase = {
  
  initialized: false,
  
  statementCache: {},
  
  
  addonCache: [],
  
  transactionCount: 0,
  
  dbfile: FileUtils.getFile(KEY_PROFILEDIR, [FILE_DATABASE], true),
  
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

    addAddonMetadata_addon: "INSERT INTO addon VALUES (NULL, :id, :syncGUID, " +
                            ":location, :version, :type, :internalName, " +
                            ":updateURL, :updateKey, :optionsURL, " +
                            ":optionsType, :aboutURL, " +
                            ":iconURL, :icon64URL, :locale, :visible, :active, " +
                            ":userDisabled, :appDisabled, :pendingUninstall, " +
                            ":descriptor, :installDate, :updateDate, " +
                            ":applyBackgroundUpdates, :bootstrap, :skinnable, " +
                            ":size, :sourceURI, :releaseNotesURI, :softDisabled, " +
                            ":isForeignInstall, :hasBinaryComponents, " +
                            ":strictCompatibility)",
    addAddonMetadata_addon_locale: "INSERT INTO addon_locale VALUES " +
                                   "(:internal_id, :name, :locale)",
    addAddonMetadata_locale: "INSERT INTO locale (name, description, creator, " +
                             "homepageURL) VALUES (:name, :description, " +
                             ":creator, :homepageURL)",
    addAddonMetadata_strings: "INSERT INTO locale_strings VALUES (:locale, " +
                              ":type, :value)",
    addAddonMetadata_targetApplication: "INSERT INTO targetApplication VALUES " +
                                        "(:internal_id, :id, :minVersion, " +
                                        ":maxVersion)",
    addAddonMetadata_targetPlatform: "INSERT INTO targetPlatform VALUES " +
                                     "(:internal_id, :os, :abi)",

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

  








  beginTransaction: function XPIDB_beginTransaction() {
    if (this.initialized)
      this.getStatement("createSavepoint").execute();
    this.transactionCount++;
  },

  



  commitTransaction: function XPIDB_commitTransaction() {
    if (this.transactionCount == 0) {
      ERROR("Attempt to commit one transaction too many.");
      return;
    }

    if (this.initialized)
      this.getStatement("releaseSavepoint").execute();
    this.transactionCount--;
  },

  



  rollbackTransaction: function XPIDB_rollbackTransaction() {
    if (this.transactionCount == 0) {
      ERROR("Attempt to rollback one transaction too many.");
      return;
    }

    if (this.initialized) {
      this.getStatement("rollbackSavepoint").execute();
      this.getStatement("releaseSavepoint").execute();
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
    delete this.connection;

    if (!aForceOpen && !this.dbfileExists) {
      this.connection = null;
      return {};
    }

    this.initialized = true;
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

  


  get connection() {
    this.openConnection(true);
    return this.connection;
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
      for each (let stmt in this.statementCache)
        stmt.finalize();
      this.statementCache = {};
      this.addonCache = [];

      if (this.transactionCount > 0) {
        ERROR(this.transactionCount + " outstanding transactions, rolling back.");
        while (this.transactionCount > 0)
          this.rollbackTransaction();
      }

      
      
      if (!this.connection.databaseFile)
        Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, true);

      this.initialized = false;
      let connection = this.connection;
      delete this.connection;

      
      
      this.__defineGetter__("connection", function() {
        this.openConnection(true);
        return this.connection;
      });

      connection.asyncClose(function() {
        LOG("Database closed");
        aCallback();
      });
    }
    else {
      if (aCallback)
        aCallback();
    }
  },

  










  getStatement: function XPIDB_getStatement(aKey, aSql) {
    if (aKey in this.statementCache)
      return this.statementCache[aKey];
    if (!aSql)
      aSql = this.statements[aKey];

    try {
      return this.statementCache[aKey] = this.connection.createStatement(aSql);
    }
    catch (e) {
      ERROR("Error creating statement " + aKey + " (" + aSql + ")");
      throw e;
    }
  },

  


  createSchema: function XPIDB_createSchema() {
    LOG("Creating database schema");
    this.beginTransaction();

    
    try {
      this.connection.createTable("addon",
                                  "internal_id INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                  "id TEXT, syncGUID TEXT, " +
                                  "location TEXT, version TEXT, " +
                                  "type TEXT, internalName TEXT, updateURL TEXT, " +
                                  "updateKey TEXT, optionsURL TEXT, " +
                                  "optionsType TEXT, aboutURL TEXT, iconURL TEXT, " +
                                  "icon64URL TEXT, defaultLocale INTEGER, " +
                                  "visible INTEGER, active INTEGER, " +
                                  "userDisabled INTEGER, appDisabled INTEGER, " +
                                  "pendingUninstall INTEGER, descriptor TEXT, " +
                                  "installDate INTEGER, updateDate INTEGER, " +
                                  "applyBackgroundUpdates INTEGER, " +
                                  "bootstrap INTEGER, skinnable INTEGER, " +
                                  "size INTEGER, sourceURI TEXT, " +
                                  "releaseNotesURI TEXT, softDisabled INTEGER, " +
                                  "isForeignInstall INTEGER, " +
                                  "hasBinaryComponents INTEGER, " +
                                  "strictCompatibility INTEGER, " +
                                  "UNIQUE (id, location), " +
                                  "UNIQUE (syncGUID)");
      this.connection.createTable("targetApplication",
                                  "addon_internal_id INTEGER, " +
                                  "id TEXT, minVersion TEXT, maxVersion TEXT, " +
                                  "UNIQUE (addon_internal_id, id)");
      this.connection.createTable("targetPlatform",
                                  "addon_internal_id INTEGER, " +
                                  "os, abi TEXT, " +
                                  "UNIQUE (addon_internal_id, os, abi)");
      this.connection.createTable("addon_locale",
                                  "addon_internal_id INTEGER, "+
                                  "locale TEXT, locale_id INTEGER, " +
                                  "UNIQUE (addon_internal_id, locale)");
      this.connection.createTable("locale",
                                  "id INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                  "name TEXT, description TEXT, creator TEXT, " +
                                  "homepageURL TEXT");
      this.connection.createTable("locale_strings",
                                  "locale_id INTEGER, type TEXT, value TEXT");
      this.connection.executeSimpleSQL("CREATE INDEX locale_strings_idx ON " +
        "locale_strings (locale_id)");
      this.connection.executeSimpleSQL("CREATE TRIGGER delete_addon AFTER DELETE " +
        "ON addon BEGIN " +
        "DELETE FROM targetApplication WHERE addon_internal_id=old.internal_id; " +
        "DELETE FROM targetPlatform WHERE addon_internal_id=old.internal_id; " +
        "DELETE FROM addon_locale WHERE addon_internal_id=old.internal_id; " +
        "DELETE FROM locale WHERE id=old.defaultLocale; " +
        "END");
      this.connection.executeSimpleSQL("CREATE TRIGGER delete_addon_locale AFTER " +
        "DELETE ON addon_locale WHEN NOT EXISTS " +
        "(SELECT * FROM addon_locale WHERE locale_id=old.locale_id) BEGIN " +
        "DELETE FROM locale WHERE id=old.locale_id; " +
        "END");
      this.connection.executeSimpleSQL("CREATE TRIGGER delete_locale AFTER " +
        "DELETE ON locale BEGIN " +
        "DELETE FROM locale_strings WHERE locale_id=old.id; " +
        "END");
      this.connection.schemaVersion = DB_SCHEMA;
      this.commitTransaction();
    }
    catch (e) {
      ERROR("Failed to create database schema", e);
      logSQLError(this.connection.lastError, this.connection.lastErrorString);
      this.rollbackTransaction();
      this.connection.close();
      this.connection = null;
      throw e;
    }
  },

  





  _readLocaleStrings: function XPIDB__readLocaleStrings(aLocale) {
    let stmt = this.getStatement("_readLocaleStrings");

    stmt.params.id = aLocale.id;
    for (let row in resultRows(stmt)) {
      if (!(row.type in aLocale))
        aLocale[row.type] = [];
      aLocale[row.type].push(row.value);
    }
  },

  






  _getLocales: function XPIDB__getLocales(aAddon) {
    let stmt = this.getStatement("_getLocales");

    let locales = [];
    stmt.params.internal_id = aAddon._internal_id;
    for (let row in resultRows(stmt)) {
      let locale = {
        id: row.id,
        locales: [row.locale]
      };
      copyProperties(row, PROP_LOCALE_SINGLE, locale);
      locales.push(locale);
    }
    locales.forEach(function(aLocale) {
      this._readLocaleStrings(aLocale);
    }, this);
    return locales;
  },

  







  _getDefaultLocale: function XPIDB__getDefaultLocale(aAddon) {
    let stmt = this.getStatement("_getDefaultLocale");

    stmt.params.id = aAddon._defaultLocale;
    if (!stepStatement(stmt))
      throw new Error("Missing default locale for " + aAddon.id);
    let locale = copyProperties(stmt.row, PROP_LOCALE_SINGLE);
    locale.id = aAddon._defaultLocale;
    stmt.reset();
    this._readLocaleStrings(locale);
    return locale;
  },

  






  _getTargetApplications: function XPIDB__getTargetApplications(aAddon) {
    let stmt = this.getStatement("_getTargetApplications");

    stmt.params.internal_id = aAddon._internal_id;
    return [copyProperties(row, PROP_TARGETAPP) for each (row in resultRows(stmt))];
  },

  






  _getTargetPlatforms: function XPIDB__getTargetPlatforms(aAddon) {
    let stmt = this.getStatement("_getTargetPlatforms");

    stmt.params.internal_id = aAddon._internal_id;
    return [copyProperties(row, ["os", "abi"]) for each (row in resultRows(stmt))];
  },

  







  makeAddonFromRow: function XPIDB_makeAddonFromRow(aRow) {
    if (this.addonCache[aRow.internal_id]) {
      let addon = this.addonCache[aRow.internal_id].get();
      if (addon)
        return addon;
    }

    let addon = new XPIProvider.DBAddonInternal();
    addon._internal_id = aRow.internal_id;
    addon._installLocation = XPIProvider.installLocationsByName[aRow.location];
    addon._descriptor = aRow.descriptor;
    addon._defaultLocale = aRow.defaultLocale;
    copyProperties(aRow, PROP_METADATA, addon);
    copyProperties(aRow, DB_METADATA, addon);
    DB_BOOL_METADATA.forEach(function(aProp) {
      addon[aProp] = aRow[aProp] != 0;
    });
    try {
      addon._sourceBundle = addon._installLocation.getLocationForID(addon.id);
    }
    catch (e) {
      
      
      
    }

    this.addonCache[aRow.internal_id] = Components.utils.getWeakReference(addon);
    return addon;
  },

  







  fetchAddonMetadata: function XPIDB_fetchAddonMetadata(aAddon) {
    function readLocaleStrings(aLocale, aCallback) {
      let stmt = XPIDatabase.getStatement("_readLocaleStrings");

      stmt.params.id = aLocale.id;
      stmt.executeAsync({
        handleResult: function(aResults) {
          let row = null;
          while ((row = aResults.getNextRow())) {
            let type = row.getResultByName("type");
            if (!(type in aLocale))
              aLocale[type] = [];
            aLocale[type].push(row.getResultByName("value"));
          }
        },

        handleError: asyncErrorLogger,

        handleCompletion: function(aReason) {
          aCallback();
        }
      });
    }

    function readDefaultLocale() {
      delete aAddon.defaultLocale;
      let stmt = XPIDatabase.getStatement("_getDefaultLocale");

      stmt.params.id = aAddon._defaultLocale;
      stmt.executeAsync({
        handleResult: function(aResults) {
          aAddon.defaultLocale = copyRowProperties(aResults.getNextRow(),
                                                   PROP_LOCALE_SINGLE);
          aAddon.defaultLocale.id = aAddon._defaultLocale;
        },

        handleError: asyncErrorLogger,

        handleCompletion: function(aReason) {
          if (aAddon.defaultLocale) {
            readLocaleStrings(aAddon.defaultLocale, readLocales);
          }
          else {
            ERROR("Missing default locale for " + aAddon.id);
            readLocales();
          }
        }
      });
    }

    function readLocales() {
      delete aAddon.locales;
      aAddon.locales = [];
      let stmt = XPIDatabase.getStatement("_getLocales");

      stmt.params.internal_id = aAddon._internal_id;
      stmt.executeAsync({
        handleResult: function(aResults) {
          let row = null;
          while ((row = aResults.getNextRow())) {
            let locale = {
              id: row.getResultByName("id"),
              locales: [row.getResultByName("locale")]
            };
            copyRowProperties(row, PROP_LOCALE_SINGLE, locale);
            aAddon.locales.push(locale);
          }
        },

        handleError: asyncErrorLogger,

        handleCompletion: function(aReason) {
          let pos = 0;
          function readNextLocale() {
            if (pos < aAddon.locales.length)
              readLocaleStrings(aAddon.locales[pos++], readNextLocale);
            else
              readTargetApplications();
          }

          readNextLocale();
        }
      });
    }

    function readTargetApplications() {
      delete aAddon.targetApplications;
      aAddon.targetApplications = [];
      let stmt = XPIDatabase.getStatement("_getTargetApplications");

      stmt.params.internal_id = aAddon._internal_id;
      stmt.executeAsync({
        handleResult: function(aResults) {
          let row = null;
          while ((row = aResults.getNextRow()))
            aAddon.targetApplications.push(copyRowProperties(row, PROP_TARGETAPP));
        },

        handleError: asyncErrorLogger,

        handleCompletion: function(aReason) {
          readTargetPlatforms();
        }
      });
    }

    function readTargetPlatforms() {
      delete aAddon.targetPlatforms;
      aAddon.targetPlatforms = [];
      let stmt = XPIDatabase.getStatement("_getTargetPlatforms");

      stmt.params.internal_id = aAddon._internal_id;
      stmt.executeAsync({
        handleResult: function(aResults) {
          let row = null;
          while ((row = aResults.getNextRow()))
            aAddon.targetPlatforms.push(copyRowProperties(row, ["os", "abi"]));
        },

        handleError: asyncErrorLogger,

        handleCompletion: function(aReason) {
          let callbacks = aAddon._pendingCallbacks;
          delete aAddon._pendingCallbacks;
          callbacks.forEach(function(aCallback) {
            aCallback(aAddon);
          });
        }
      });
    }

    readDefaultLocale();
  },

  







  makeAddonFromRowAsync: function XPIDB_makeAddonFromRowAsync(aRow, aCallback) {
    let internal_id = aRow.getResultByName("internal_id");
    if (this.addonCache[internal_id]) {
      let addon = this.addonCache[internal_id].get();
      if (addon) {
        
        
        
        if ("_pendingCallbacks" in addon)
          addon._pendingCallbacks.push(aCallback);
        else
          aCallback(addon);
        return;
      }
    }

    let addon = new XPIProvider.DBAddonInternal();
    addon._internal_id = internal_id;
    let location = aRow.getResultByName("location");
    addon._installLocation = XPIProvider.installLocationsByName[location];
    addon._descriptor = aRow.getResultByName("descriptor");
    copyRowProperties(aRow, PROP_METADATA, addon);
    addon._defaultLocale = aRow.getResultByName("defaultLocale");
    copyRowProperties(aRow, DB_METADATA, addon);
    DB_BOOL_METADATA.forEach(function(aProp) {
      addon[aProp] = aRow.getResultByName(aProp) != 0;
    });
    try {
      addon._sourceBundle = addon._installLocation.getLocationForID(addon.id);
    }
    catch (e) {
      
      
      
    }

    this.addonCache[internal_id] = Components.utils.getWeakReference(addon);
    addon._pendingCallbacks = [aCallback];
    this.fetchAddonMetadata(addon);
  },

  







  getInstallLocations: function XPIDB_getInstallLocations() {
    if (!this.connection)
      return [];

    let stmt = this.getStatement("getInstallLocations");

    return [row.location for each (row in resultRows(stmt))];
  },

  






  getAddonsInLocation: function XPIDB_getAddonsInLocation(aLocation) {
    if (!this.connection)
      return [];

    let stmt = this.getStatement("getAddonsInLocation");

    stmt.params.location = aLocation;
    return [this.makeAddonFromRow(row) for each (row in resultRows(stmt))];
  },

  










  getAddonInLocation: function XPIDB_getAddonInLocation(aId, aLocation, aCallback) {
    if (!this.connection) {
      aCallback(null);
      return;
    }

    let stmt = this.getStatement("getAddonInLocation");

    stmt.params.id = aId;
    stmt.params.location = aLocation;
    stmt.executeAsync(new AsyncAddonListCallback(function(aAddons) {
      if (aAddons.length == 0) {
        aCallback(null);
        return;
      }
      
      
      if (aAddons.length > 1)
        ERROR("Multiple addons with ID " + aId + " found in location " + aLocation);
      aCallback(aAddons[0]);
    }));
  },

  







  getVisibleAddonForID: function XPIDB_getVisibleAddonForID(aId, aCallback) {
    if (!this.connection) {
      aCallback(null);
      return;
    }

    let stmt = this.getStatement("getVisibleAddonForID");

    stmt.params.id = aId;
    stmt.executeAsync(new AsyncAddonListCallback(function(aAddons) {
      if (aAddons.length == 0) {
        aCallback(null);
        return;
      }
      
      
      if (aAddons.length > 1)
        ERROR("Multiple visible addons with ID " + aId + " found");
      aCallback(aAddons[0]);
    }));
  },

  







  getVisibleAddons: function XPIDB_getVisibleAddons(aTypes, aCallback) {
    if (!this.connection) {
      aCallback([]);
      return;
    }

    let stmt = null;
    if (!aTypes || aTypes.length == 0) {
      stmt = this.getStatement("getVisibleAddons");
    }
    else {
      let sql = "SELECT " + FIELDS_ADDON + " FROM addon WHERE visible=1 AND " +
                "type IN (";
      for (let i = 1; i <= aTypes.length; i++) {
        sql += "?" + i;
        if (i < aTypes.length)
          sql += ",";
      }
      sql += ")";

      
      stmt = this.getStatement("getVisibleAddons_" + aTypes.length, sql);
      for (let i = 0; i < aTypes.length; i++)
        stmt.bindByIndex(i, aTypes[i]);
    }

    stmt.executeAsync(new AsyncAddonListCallback(aCallback));
  },

  






  getAddonsByType: function XPIDB_getAddonsByType(aType) {
    if (!this.connection)
      return [];

    let stmt = this.getStatement("getAddonsByType");

    stmt.params.type = aType;
    return [this.makeAddonFromRow(row) for each (row in resultRows(stmt))];
  },

  






  getVisibleAddonForInternalName: function XPIDB_getVisibleAddonForInternalName(aInternalName) {
    if (!this.connection)
      return null;

    let stmt = this.getStatement("getVisibleAddonForInternalName");

    let addon = null;
    stmt.params.internalName = aInternalName;

    if (stepStatement(stmt))
      addon = this.makeAddonFromRow(stmt.row);

    stmt.reset();
    return addon;
  },

  







  getVisibleAddonsWithPendingOperations:
    function XPIDB_getVisibleAddonsWithPendingOperations(aTypes, aCallback) {
    if (!this.connection) {
      aCallback([]);
      return;
    }

    let stmt = null;
    if (!aTypes || aTypes.length == 0) {
      stmt = this.getStatement("getVisibleAddonsWithPendingOperations");
    }
    else {
      let sql = "SELECT * FROM addon WHERE visible=1 AND " +
                "(pendingUninstall=1 OR MAX(userDisabled,appDisabled)=active) " +
                "AND type IN (";
      for (let i = 1; i <= aTypes.length; i++) {
        sql += "?" + i;
        if (i < aTypes.length)
          sql += ",";
      }
      sql += ")";

      
      stmt = this.getStatement("getVisibleAddonsWithPendingOperations_" +
                               aTypes.length, sql);
      for (let i = 0; i < aTypes.length; i++)
        stmt.bindByIndex(i, aTypes[i]);
    }

    stmt.executeAsync(new AsyncAddonListCallback(aCallback));
  },

  









  getAddonBySyncGUID: function XPIDB_getAddonBySyncGUID(aGUID, aCallback) {
    let stmt = this.getStatement("getAddonBySyncGUID");
    stmt.params.syncGUID = aGUID;

    stmt.executeAsync(new AsyncAddonListCallback(function(aAddons) {
      if (aAddons.length == 0) {
        aCallback(null);
        return;
      }
      aCallback(aAddons[0]);
    }));
  },

  




  getAddons: function XPIDB_getAddons() {
    if (!this.connection)
      return [];

    let stmt = this.getStatement("getAddons");

    return [this.makeAddonFromRow(row) for each (row in resultRows(stmt))];
  },

  







  addAddonMetadata: function XPIDB_addAddonMetadata(aAddon, aDescriptor) {
    
    if (!this.connection)
      this.openConnection(false, true);

    this.beginTransaction();

    var self = this;
    function insertLocale(aLocale) {
      let localestmt = self.getStatement("addAddonMetadata_locale");
      let stringstmt = self.getStatement("addAddonMetadata_strings");

      copyProperties(aLocale, PROP_LOCALE_SINGLE, localestmt.params);
      executeStatement(localestmt);
      let row = XPIDatabase.connection.lastInsertRowID;

      PROP_LOCALE_MULTI.forEach(function(aProp) {
        aLocale[aProp].forEach(function(aStr) {
          stringstmt.params.locale = row;
          stringstmt.params.type = aProp;
          stringstmt.params.value = aStr;
          executeStatement(stringstmt);
        });
      });
      return row;
    }

    
    try {

      if (aAddon.visible) {
        let stmt = this.getStatement("clearVisibleAddons");
        stmt.params.id = aAddon.id;
        executeStatement(stmt);
      }

      let stmt = this.getStatement("addAddonMetadata_addon");

      stmt.params.locale = insertLocale(aAddon.defaultLocale);
      stmt.params.location = aAddon._installLocation.name;
      stmt.params.descriptor = aDescriptor;
      copyProperties(aAddon, PROP_METADATA, stmt.params);
      copyProperties(aAddon, DB_METADATA, stmt.params);
      DB_BOOL_METADATA.forEach(function(aProp) {
        stmt.params[aProp] = aAddon[aProp] ? 1 : 0;
      });
      executeStatement(stmt);
      let internal_id = this.connection.lastInsertRowID;

      stmt = this.getStatement("addAddonMetadata_addon_locale");
      aAddon.locales.forEach(function(aLocale) {
        let id = insertLocale(aLocale);
        aLocale.locales.forEach(function(aName) {
          stmt.params.internal_id = internal_id;
          stmt.params.name = aName;
          stmt.params.locale = id;
          executeStatement(stmt);
        });
      });

      stmt = this.getStatement("addAddonMetadata_targetApplication");

      aAddon.targetApplications.forEach(function(aApp) {
        stmt.params.internal_id = internal_id;
        stmt.params.id = aApp.id;
        stmt.params.minVersion = aApp.minVersion;
        stmt.params.maxVersion = aApp.maxVersion;
        executeStatement(stmt);
      });

      stmt = this.getStatement("addAddonMetadata_targetPlatform");

      aAddon.targetPlatforms.forEach(function(aPlatform) {
        stmt.params.internal_id = internal_id;
        stmt.params.os = aPlatform.os;
        stmt.params.abi = aPlatform.abi;
        executeStatement(stmt);
      });

      this.commitTransaction();
    }
    catch (e) {
      this.rollbackTransaction();
      throw e;
    }
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

      this.addAddonMetadata(aNewAddon, aDescriptor);
      this.commitTransaction();
    }
    catch (e) {
      this.rollbackTransaction();
      throw e;
    }
  },

  







  updateTargetApplications: function XPIDB_updateTargetApplications(aAddon,
                                                                    aTargets) {
    this.beginTransaction();

    
    try {
      let stmt = this.getStatement("updateTargetApplications");
      aTargets.forEach(function(aTarget) {
        stmt.params.internal_id = aAddon._internal_id;
        stmt.params.id = aTarget.id;
        stmt.params.minVersion = aTarget.minVersion;
        stmt.params.maxVersion = aTarget.maxVersion;
        executeStatement(stmt);
      });
      this.commitTransaction();
    }
    catch (e) {
      this.rollbackTransaction();
      throw e;
    }
  },

  





  removeAddonMetadata: function XPIDB_removeAddonMetadata(aAddon) {
    let stmt = this.getStatement("removeAddonMetadata");
    stmt.params.internal_id = aAddon._internal_id;
    executeStatement(stmt);
  },

  








  makeAddonVisible: function XPIDB_makeAddonVisible(aAddon) {
    let stmt = this.getStatement("clearVisibleAddons");
    stmt.params.id = aAddon.id;
    executeStatement(stmt);

    stmt = this.getStatement("makeAddonVisible");
    stmt.params.internal_id = aAddon._internal_id;
    executeStatement(stmt);

    aAddon.visible = true;
  },

  







  setAddonProperties: function XPIDB_setAddonProperties(aAddon, aProperties) {
    function convertBoolean(value) {
      return value ? 1 : 0;
    }

    let stmt = this.getStatement("setAddonProperties");
    stmt.params.internal_id = aAddon._internal_id;

    ["userDisabled", "appDisabled", "softDisabled",
     "pendingUninstall"].forEach(function(aProp) {
      if (aProp in aProperties) {
        stmt.params[aProp] = convertBoolean(aProperties[aProp]);
        aAddon[aProp] = aProperties[aProp];
      }
      else {
        stmt.params[aProp] = convertBoolean(aAddon[aProp]);
      }
    });

    if ("applyBackgroundUpdates" in aProperties) {
      stmt.params.applyBackgroundUpdates = aProperties.applyBackgroundUpdates;
      aAddon.applyBackgroundUpdates = aProperties.applyBackgroundUpdates;
    }
    else {
      stmt.params.applyBackgroundUpdates = aAddon.applyBackgroundUpdates;
    }

    executeStatement(stmt);
  },

  







  setAddonSyncGUID: function XPIDB_setAddonSyncGUID(aAddon, aGUID) {
    let stmt = this.getStatement("setAddonSyncGUID");
    stmt.params.internal_id = aAddon._internal_id;
    stmt.params.syncGUID = aGUID;

    executeStatement(stmt);
  },

  







  setAddonDescriptor: function XPIDB_setAddonDescriptor(aAddon, aDescriptor) {
    let stmt = this.getStatement("setAddonDescriptor");
    stmt.params.internal_id = aAddon._internal_id;
    stmt.params.descriptor = aDescriptor;

    executeStatement(stmt);
  },

  





  updateAddonActive: function XPIDB_updateAddonActive(aAddon) {
    LOG("Updating add-on state");

    let stmt = this.getStatement("updateAddonActive");
    stmt.params.internal_id = aAddon._internal_id;
    stmt.params.active = aAddon.active ? 1 : 0;
    executeStatement(stmt);
  },

  


  updateActiveAddons: function XPIDB_updateActiveAddons() {
    LOG("Updating add-on states");
    let stmt = this.getStatement("setActiveAddons");
    executeStatement(stmt);

    
    
    
    
    this.addonCache = [];
  },

  


  writeAddonsList: function XPIDB_writeAddonsList() {
    Services.appinfo.invalidateCachesOnRestart();

    let addonsList = FileUtils.getFile(KEY_PROFILEDIR, [FILE_XPI_ADDONS_LIST],
                                       true);
    if (!this.connection) {
      try {
        addonsList.remove(false);
        LOG("Deleted add-ons list");
      }
      catch (e) {
      }

      Services.prefs.clearUserPref(PREF_EM_ENABLED_ADDONS);
      return;
    }

    let enabledAddons = [];
    let text = "[ExtensionDirs]\r\n";
    let count = 0;
    let fullCount = 0;

    let stmt = this.getStatement("getActiveAddons");

    for (let row in resultRows(stmt)) {
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

    if (dssEnabled) {
      stmt = this.getStatement("getThemes");
    }
    else {
      stmt = this.getStatement("getActiveTheme");
      stmt.params.internalName = XPIProvider.selectedSkin;
    }

    if (stmt) {
      count = 0;
      for (let row in resultRows(stmt)) {
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
