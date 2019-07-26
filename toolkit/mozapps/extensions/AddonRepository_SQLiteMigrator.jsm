



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/AddonManager.jsm");
Components.utils.import("resource://gre/modules/FileUtils.jsm");

const KEY_PROFILEDIR  = "ProfD";
const FILE_DATABASE   = "addons.sqlite";
const LAST_DB_SCHEMA   = 4;


const PROP_SINGLE = ["id", "type", "name", "version", "creator", "description",
                     "fullDescription", "developerComments", "eula",
                     "homepageURL", "supportURL", "contributionURL",
                     "contributionAmount", "averageRating", "reviewCount",
                     "reviewURL", "totalDownloads", "weeklyDownloads",
                     "dailyUsers", "sourceURI", "repositoryStatus", "size",
                     "updateDate"];


["LOG", "WARN", "ERROR"].forEach(function(aName) {
  this.__defineGetter__(aName, function logFuncGetter() {
    Components.utils.import("resource://gre/modules/AddonLogging.jsm");

    LogManager.getLogger("addons.repository.sqlmigrator", this);
    return this[aName];
  });
}, this);


this.EXPORTED_SYMBOLS = ["AddonRepository_SQLiteMigrator"];


this.AddonRepository_SQLiteMigrator = {

  











  migrate: function(aCallback) {
    if (!this._openConnection()) {
      this._closeConnection();
      aCallback([]);
      return false;
    }

    LOG("Importing addon repository from previous " + FILE_DATABASE + " storage.");

    this._retrieveStoredData((results) => {
      this._closeConnection();
      let resultArray = [addon for ([,addon] of Iterator(results))];
      LOG(resultArray.length + " addons imported.")
      aCallback(resultArray);
    });

    return true;
  },

  




  _openConnection: function AD_openConnection() {
    delete this.connection;

    let dbfile = FileUtils.getFile(KEY_PROFILEDIR, [FILE_DATABASE], true);
    if (!dbfile.exists())
      return false;

    try {
      this.connection = Services.storage.openUnsharedDatabase(dbfile);
    } catch (e) {
      return false;
    }

    this.connection.executeSimpleSQL("PRAGMA locking_mode = EXCLUSIVE");

    
    try {
      this.connection.beginTransaction();

      switch (this.connection.schemaVersion) {
        case 0:
          return false;

        case 1:
          LOG("Upgrading database schema to version 2");
          this.connection.executeSimpleSQL("ALTER TABLE screenshot ADD COLUMN width INTEGER");
          this.connection.executeSimpleSQL("ALTER TABLE screenshot ADD COLUMN height INTEGER");
          this.connection.executeSimpleSQL("ALTER TABLE screenshot ADD COLUMN thumbnailWidth INTEGER");
          this.connection.executeSimpleSQL("ALTER TABLE screenshot ADD COLUMN thumbnailHeight INTEGER");
        case 2:
          LOG("Upgrading database schema to version 3");
          this.connection.createTable("compatibility_override",
                                      "addon_internal_id INTEGER, " +
                                      "num INTEGER, " +
                                      "type TEXT, " +
                                      "minVersion TEXT, " +
                                      "maxVersion TEXT, " +
                                      "appID TEXT, " +
                                      "appMinVersion TEXT, " +
                                      "appMaxVersion TEXT, " +
                                      "PRIMARY KEY (addon_internal_id, num)");
        case 3:
          LOG("Upgrading database schema to version 4");
          this.connection.createTable("icon",
                                      "addon_internal_id INTEGER, " +
                                      "size INTEGER, " +
                                      "url TEXT, " +
                                      "PRIMARY KEY (addon_internal_id, size)");
          this._createIndices();
          this._createTriggers();
          this.connection.schemaVersion = LAST_DB_SCHEMA;
        case LAST_DB_SCHEMA:
          break;
        default:
          return false;
      }
      this.connection.commitTransaction();
    } catch (e) {
      ERROR("Failed to open " + FILE_DATABASE + ". Data import will not happen.", e);
      this.logSQLError(this.connection.lastError, this.connection.lastErrorString);
      this.connection.rollbackTransaction();
      return false;
    }

    return true;
  },

  _closeConnection: function() {
    for each (let stmt in this.asyncStatementsCache)
      stmt.finalize();
    this.asyncStatementsCache = {};

    if (this.connection)
      this.connection.asyncClose();

    delete this.connection;
  },

  






  _retrieveStoredData: function AD_retrieveStoredData(aCallback) {
    let self = this;
    let addons = {};

    
    function getAllAddons() {
      self.getAsyncStatement("getAllAddons").executeAsync({
        handleResult: function getAllAddons_handleResult(aResults) {
          let row = null;
          while ((row = aResults.getNextRow())) {
            let internal_id = row.getResultByName("internal_id");
            addons[internal_id] = self._makeAddonFromAsyncRow(row);
          }
        },

        handleError: self.asyncErrorLogger,

        handleCompletion: function getAllAddons_handleCompletion(aReason) {
          if (aReason != Ci.mozIStorageStatementCallback.REASON_FINISHED) {
            ERROR("Error retrieving add-ons from database. Returning empty results");
            aCallback({});
            return;
          }

          getAllDevelopers();
        }
      });
    }

    
    function getAllDevelopers() {
      self.getAsyncStatement("getAllDevelopers").executeAsync({
        handleResult: function getAllDevelopers_handleResult(aResults) {
          let row = null;
          while ((row = aResults.getNextRow())) {
            let addon_internal_id = row.getResultByName("addon_internal_id");
            if (!(addon_internal_id in addons)) {
              WARN("Found a developer not linked to an add-on in database");
              continue;
            }

            let addon = addons[addon_internal_id];
            if (!addon.developers)
              addon.developers = [];

            addon.developers.push(self._makeDeveloperFromAsyncRow(row));
          }
        },

        handleError: self.asyncErrorLogger,

        handleCompletion: function getAllDevelopers_handleCompletion(aReason) {
          if (aReason != Ci.mozIStorageStatementCallback.REASON_FINISHED) {
            ERROR("Error retrieving developers from database. Returning empty results");
            aCallback({});
            return;
          }

          getAllScreenshots();
        }
      });
    }

    
    function getAllScreenshots() {
      self.getAsyncStatement("getAllScreenshots").executeAsync({
        handleResult: function getAllScreenshots_handleResult(aResults) {
          let row = null;
          while ((row = aResults.getNextRow())) {
            let addon_internal_id = row.getResultByName("addon_internal_id");
            if (!(addon_internal_id in addons)) {
              WARN("Found a screenshot not linked to an add-on in database");
              continue;
            }

            let addon = addons[addon_internal_id];
            if (!addon.screenshots)
              addon.screenshots = [];
            addon.screenshots.push(self._makeScreenshotFromAsyncRow(row));
          }
        },

        handleError: self.asyncErrorLogger,

        handleCompletion: function getAllScreenshots_handleCompletion(aReason) {
          if (aReason != Ci.mozIStorageStatementCallback.REASON_FINISHED) {
            ERROR("Error retrieving screenshots from database. Returning empty results");
            aCallback({});
            return;
          }

          getAllCompatOverrides();
        }
      });
    }

    function getAllCompatOverrides() {
      self.getAsyncStatement("getAllCompatOverrides").executeAsync({
        handleResult: function getAllCompatOverrides_handleResult(aResults) {
          let row = null;
          while ((row = aResults.getNextRow())) {
            let addon_internal_id = row.getResultByName("addon_internal_id");
            if (!(addon_internal_id in addons)) {
              WARN("Found a compatibility override not linked to an add-on in database");
              continue;
            }

            let addon = addons[addon_internal_id];
            if (!addon.compatibilityOverrides)
              addon.compatibilityOverrides = [];
            addon.compatibilityOverrides.push(self._makeCompatOverrideFromAsyncRow(row));
          }
        },

        handleError: self.asyncErrorLogger,

        handleCompletion: function getAllCompatOverrides_handleCompletion(aReason) {
          if (aReason != Ci.mozIStorageStatementCallback.REASON_FINISHED) {
            ERROR("Error retrieving compatibility overrides from database. Returning empty results");
            aCallback({});
            return;
          }

          getAllIcons();
        }
      });
    }

    function getAllIcons() {
      self.getAsyncStatement("getAllIcons").executeAsync({
        handleResult: function getAllIcons_handleResult(aResults) {
          let row = null;
          while ((row = aResults.getNextRow())) {
            let addon_internal_id = row.getResultByName("addon_internal_id");
            if (!(addon_internal_id in addons)) {
              WARN("Found an icon not linked to an add-on in database");
              continue;
            }

            let addon = addons[addon_internal_id];
            let { size, url } = self._makeIconFromAsyncRow(row);
            addon.icons[size] = url;
            if (size == 32)
              addon.iconURL = url;
          }
        },

        handleError: self.asyncErrorLogger,

        handleCompletion: function getAllIcons_handleCompletion(aReason) {
          if (aReason != Ci.mozIStorageStatementCallback.REASON_FINISHED) {
            ERROR("Error retrieving icons from database. Returning empty results");
            aCallback({});
            return;
          }

          let returnedAddons = {};
          for each (let addon in addons)
            returnedAddons[addon.id] = addon;
          aCallback(returnedAddons);
        }
      });
    }

    
    getAllAddons();
  },

  
  asyncStatementsCache: {},

  








  getAsyncStatement: function AD_getAsyncStatement(aKey) {
    if (aKey in this.asyncStatementsCache)
      return this.asyncStatementsCache[aKey];

    let sql = this.queries[aKey];
    try {
      return this.asyncStatementsCache[aKey] = this.connection.createAsyncStatement(sql);
    } catch (e) {
      ERROR("Error creating statement " + aKey + " (" + sql + ")");
      throw Components.Exception("Error creating statement " + aKey + " (" + sql + "): " + e,
                                 e.result);
    }
  },

  
  queries: {
    getAllAddons: "SELECT internal_id, id, type, name, version, " +
                  "creator, creatorURL, description, fullDescription, " +
                  "developerComments, eula, homepageURL, supportURL, " +
                  "contributionURL, contributionAmount, averageRating, " +
                  "reviewCount, reviewURL, totalDownloads, weeklyDownloads, " +
                  "dailyUsers, sourceURI, repositoryStatus, size, updateDate " +
                  "FROM addon",

    getAllDevelopers: "SELECT addon_internal_id, name, url FROM developer " +
                      "ORDER BY addon_internal_id, num",

    getAllScreenshots: "SELECT addon_internal_id, url, width, height, " +
                       "thumbnailURL, thumbnailWidth, thumbnailHeight, caption " +
                       "FROM screenshot ORDER BY addon_internal_id, num",

    getAllCompatOverrides: "SELECT addon_internal_id, type, minVersion, " +
                           "maxVersion, appID, appMinVersion, appMaxVersion " +
                           "FROM compatibility_override " +
                           "ORDER BY addon_internal_id, num",

    getAllIcons: "SELECT addon_internal_id, size, url FROM icon " +
                 "ORDER BY addon_internal_id, size",
  },

  






  _makeAddonFromAsyncRow: function AD__makeAddonFromAsyncRow(aRow) {
    
    
    

    let addon = { icons: {} };

    for (let prop of PROP_SINGLE) {
      addon[prop] = aRow.getResultByName(prop)
    };

    return addon;
  },

  






  _makeDeveloperFromAsyncRow: function AD__makeDeveloperFromAsyncRow(aRow) {
    let name = aRow.getResultByName("name");
    let url = aRow.getResultByName("url")
    return new AddonManagerPrivate.AddonAuthor(name, url);
  },

  






  _makeScreenshotFromAsyncRow: function AD__makeScreenshotFromAsyncRow(aRow) {
    let url = aRow.getResultByName("url");
    let width = aRow.getResultByName("width");
    let height = aRow.getResultByName("height");
    let thumbnailURL = aRow.getResultByName("thumbnailURL");
    let thumbnailWidth = aRow.getResultByName("thumbnailWidth");
    let thumbnailHeight = aRow.getResultByName("thumbnailHeight");
    let caption = aRow.getResultByName("caption");
    return new AddonManagerPrivate.AddonScreenshot(url, width, height, thumbnailURL,
                                                   thumbnailWidth, thumbnailHeight, caption);
  },

  






  _makeCompatOverrideFromAsyncRow: function AD_makeCompatOverrideFromAsyncRow(aRow) {
    let type = aRow.getResultByName("type");
    let minVersion = aRow.getResultByName("minVersion");
    let maxVersion = aRow.getResultByName("maxVersion");
    let appID = aRow.getResultByName("appID");
    let appMinVersion = aRow.getResultByName("appMinVersion");
    let appMaxVersion = aRow.getResultByName("appMaxVersion");
    return new AddonManagerPrivate.AddonCompatibilityOverride(type,
                                                              minVersion,
                                                              maxVersion,
                                                              appID,
                                                              appMinVersion,
                                                              appMaxVersion);
  },

  






  _makeIconFromAsyncRow: function AD_makeIconFromAsyncRow(aRow) {
    let size = aRow.getResultByName("size");
    let url = aRow.getResultByName("url");
    return { size: size, url: url };
  },

  







  logSQLError: function AD_logSQLError(aError, aErrorString) {
    ERROR("SQL error " + aError + ": " + aErrorString);
  },

  





  asyncErrorLogger: function AD_asyncErrorLogger(aError) {
    ERROR("Async SQL error " + aError.result + ": " + aError.message);
  },

  


  _createTriggers: function AD__createTriggers() {
    this.connection.executeSimpleSQL("DROP TRIGGER IF EXISTS delete_addon");
    this.connection.executeSimpleSQL("CREATE TRIGGER delete_addon AFTER DELETE " +
      "ON addon BEGIN " +
      "DELETE FROM developer WHERE addon_internal_id=old.internal_id; " +
      "DELETE FROM screenshot WHERE addon_internal_id=old.internal_id; " +
      "DELETE FROM compatibility_override WHERE addon_internal_id=old.internal_id; " +
      "DELETE FROM icon WHERE addon_internal_id=old.internal_id; " +
      "END");
  },

  


  _createIndices: function AD__createIndices() {
    this.connection.executeSimpleSQL("CREATE INDEX IF NOT EXISTS developer_idx " +
                                     "ON developer (addon_internal_id)");
    this.connection.executeSimpleSQL("CREATE INDEX IF NOT EXISTS screenshot_idx " +
                                     "ON screenshot (addon_internal_id)");
    this.connection.executeSimpleSQL("CREATE INDEX IF NOT EXISTS compatibility_override_idx " +
                                     "ON compatibility_override (addon_internal_id)");
    this.connection.executeSimpleSQL("CREATE INDEX IF NOT EXISTS icon_idx " +
                                     "ON icon (addon_internal_id)");
  }
}
