





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const DB_VERSION = 3;
const DAY_IN_MS  = 86400000; 

function FormHistory() {
    this.init();
}

FormHistory.prototype = {
    classID          : Components.ID("{0c1bb408-71a2-403f-854a-3a0659829ded}"),
    QueryInterface   : XPCOMUtils.generateQI([Ci.nsIFormHistory2,
                                              Ci.nsIObserver,
                                              Ci.nsIFrameMessageListener,
                                              ]),

    debug          : true,
    enabled        : true,
    saveHttpsForms : true,

    
    dbSchema : {
        tables : {
            moz_formhistory: {
                "id"        : "INTEGER PRIMARY KEY",
                "fieldname" : "TEXT NOT NULL",
                "value"     : "TEXT NOT NULL",
                "timesUsed" : "INTEGER",
                "firstUsed" : "INTEGER",
                "lastUsed"  : "INTEGER",
                "guid"      : "TEXT"
            },
        },
        indices : {
            moz_formhistory_index : {
                table   : "moz_formhistory",
                columns : ["fieldname"]
            },
            moz_formhistory_lastused_index : {
                table   : "moz_formhistory",
                columns : ["lastUsed"]
            },
            moz_formhistory_guid_index : {
                table   : "moz_formhistory",
                columns : ["guid"]
            },
        }
    },
    dbConnection : null,  
    dbStmts      : null,  
    dbFile       : null,

    _uuidService: null,
    get uuidService() {
        if (!this._uuidService)
            this._uuidService = Cc["@mozilla.org/uuid-generator;1"].
                                getService(Ci.nsIUUIDGenerator);
        return this._uuidService;
    },

    
    
    _privBrowsingSvc : undefined,
    get privBrowsingSvc() {
        if (this._privBrowsingSvc == undefined) {
            if ("@mozilla.org/privatebrowsing;1" in Cc)
                this._privBrowsingSvc = Cc["@mozilla.org/privatebrowsing;1"].
                                        getService(Ci.nsIPrivateBrowsingService);
            else
                this._privBrowsingSvc = null;
        }
        return this._privBrowsingSvc;
    },


    log : function (message) {
        if (!this.debug)
            return;
        dump("FormHistory: " + message + "\n");
        Services.console.logStringMessage("FormHistory: " + message);
    },


    init : function() {
        let self = this;

        Services.prefs.addObserver("browser.formfill.", this, false);

        this.updatePrefs();

        this.dbStmts = {};

        this.messageManager = Cc["@mozilla.org/globalmessagemanager;1"].
                              getService(Ci.nsIChromeFrameMessageManager);
        this.messageManager.loadFrameScript("chrome://satchel/content/formSubmitListener.js", true);
        this.messageManager.addMessageListener("FormHistory:FormSubmitEntries", this);

        
        Services.obs.addObserver(function() { self.expireOldEntries() }, "idle-daily", false);
        Services.obs.addObserver(function() { self.expireOldEntries() }, "formhistory-expire-now", false);

        try {
            this.dbFile = Services.dirsvc.get("ProfD", Ci.nsIFile).clone();
            this.dbFile.append("formhistory.sqlite");
            this.log("Opening database at " + this.dbFile.path);

            this.dbInit();
        } catch (e) {
            this.log("Initialization failed: " + e);
            
            if (e.result == Cr.NS_ERROR_FILE_CORRUPTED) {
                this.dbCleanup(true);
                this.dbInit();
            } else {
                throw "Initialization failed";
            }
        }
    },


    


    receiveMessage: function receiveMessage(message) {
        
        this.dbConnection.beginTransaction();

        try {
            let entries = message.json;
            for (let i = 0; i < entries.length; i++) {
                this.addEntry(entries[i].name, entries[i].value);
            }
        } finally {
            
            
            this.dbConnection.commitTransaction();
        }
    },


    


    get hasEntries() {
        return (this.countAllEntries() > 0);
    },


    addEntry : function (name, value) {
        if (!this.enabled ||
            this.privBrowsingSvc && this.privBrowsingSvc.privateBrowsingEnabled)
            return;

        this.log("addEntry for " + name + "=" + value);

        let now = Date.now() * 1000; 

        let [id, guid] = this.getExistingEntryID(name, value);
        let stmt;

        if (id != -1) {
            
            let query = "UPDATE moz_formhistory SET timesUsed = timesUsed + 1, lastUsed = :lastUsed WHERE id = :id";
            let params = {
                            lastUsed : now,
                            id       : id
                         };

            try {
                stmt = this.dbCreateStatement(query, params);
                stmt.execute();
                this.sendStringNotification("modifyEntry", name, value, guid);
            } catch (e) {
                this.log("addEntry (modify) failed: " + e);
                throw e;
            } finally {
                if (stmt) {
                    stmt.reset();
                }
            }

        } else {
            
            guid = this.generateGUID();

            let query = "INSERT INTO moz_formhistory (fieldname, value, timesUsed, firstUsed, lastUsed, guid) " +
                            "VALUES (:fieldname, :value, :timesUsed, :firstUsed, :lastUsed, :guid)";
            let params = {
                            fieldname : name,
                            value     : value,
                            timesUsed : 1,
                            firstUsed : now,
                            lastUsed  : now,
                            guid      : guid
                        };

            try {
                stmt = this.dbCreateStatement(query, params);
                stmt.execute();
                this.sendStringNotification("addEntry", name, value, guid);
            } catch (e) {
                this.log("addEntry (create) failed: " + e);
                throw e;
            } finally {
                if (stmt) {
                    stmt.reset();
                }
            }
        }
    },


    removeEntry : function (name, value) {
        this.log("removeEntry for " + name + "=" + value);

        let [id, guid] = this.getExistingEntryID(name, value);
        this.sendStringNotification("before-removeEntry", name, value, guid);

        let stmt;
        let query = "DELETE FROM moz_formhistory WHERE id = :id";
        let params = { id : id };

        try {
            stmt = this.dbCreateStatement(query, params);
            stmt.execute();
            this.sendStringNotification("removeEntry", name, value, guid);
        } catch (e) {
            this.log("removeEntry failed: " + e);
            throw e;
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }
    },


    removeEntriesForName : function (name) {
        this.log("removeEntriesForName with name=" + name);

        this.sendStringNotification("before-removeEntriesForName", name);

        let stmt;
        let query = "DELETE FROM moz_formhistory WHERE fieldname = :fieldname";
        let params = { fieldname : name };

        try {
            stmt = this.dbCreateStatement(query, params);
            stmt.execute();
            this.sendStringNotification("removeEntriesForName", name);
        } catch (e) {
            this.log("removeEntriesForName failed: " + e);
            throw e;
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }
    },


    removeAllEntries : function () {
        this.log("removeAllEntries");

        this.sendNotification("before-removeAllEntries", null);

        let stmt;
        let query = "DELETE FROM moz_formhistory";

        try {
            stmt = this.dbCreateStatement(query);
            stmt.execute();
            this.sendNotification("removeAllEntries", null);
        } catch (e) {
            this.log("removeEntriesForName failed: " + e);
            throw e;
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }

        
        let oldFile = Services.dirsvc.get("ProfD", Ci.nsIFile);
        oldFile.append("formhistory.dat");
        if (oldFile.exists())
            oldFile.remove(false);
    },


    nameExists : function (name) {
        this.log("nameExists for name=" + name);
        let stmt;
        let query = "SELECT COUNT(1) AS numEntries FROM moz_formhistory WHERE fieldname = :fieldname";
        let params = { fieldname : name };
        try {
            stmt = this.dbCreateStatement(query, params);
            stmt.executeStep();
            return (stmt.row.numEntries > 0);
        } catch (e) {
            this.log("nameExists failed: " + e);
            throw e;
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }
    },

    entryExists : function (name, value) {
        this.log("entryExists for " + name + "=" + value);
        let [id, guid] = this.getExistingEntryID(name, value);
        this.log("entryExists: id=" + id);
        return (id != -1);
    },

    removeEntriesByTimeframe : function (beginTime, endTime) {
        this.log("removeEntriesByTimeframe for " + beginTime + " to " + endTime);

        this.sendIntNotification("before-removeEntriesByTimeframe", beginTime, endTime);

        let stmt;
        let query = "DELETE FROM moz_formhistory WHERE firstUsed >= :beginTime AND firstUsed <= :endTime";
        let params = {
                        beginTime : beginTime,
                        endTime   : endTime
                     };
        try {
            stmt = this.dbCreateStatement(query, params);
            stmt.executeStep();
            this.sendIntNotification("removeEntriesByTimeframe", beginTime, endTime);
        } catch (e) {
            this.log("removeEntriesByTimeframe failed: " + e);
            throw e;
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }

    },


    get DBConnection() {
        return this.dbConnection;
    },


    


    observe : function (subject, topic, data) {
        if (topic == "nsPref:changed")
            this.updatePrefs();
        else
            this.log("Oops! Unexpected notification: " + topic);
    },


    


    generateGUID : function() {
        
        let uuid = this.uuidService.generateUUID().toString();
        let raw = ""; 
        let bytes = 0;
        for (let i = 1; bytes < 12 ; i+= 2) {
            
            if (uuid[i] == "-")
                i++;
            let hexVal = parseInt(uuid[i] + uuid[i + 1], 16);
            raw += String.fromCharCode(hexVal);
            bytes++;
        }
        return btoa(raw);
    },


    sendStringNotification : function (changeType, str1, str2, str3) {
        function wrapit(str) {
            let wrapper = Cc["@mozilla.org/supports-string;1"].
                          createInstance(Ci.nsISupportsString);
            wrapper.data = str;
            return wrapper;
        }

        let strData;
        if (arguments.length == 2) {
            
            strData = wrapit(str1);
        } else {
            
            strData = Cc["@mozilla.org/array;1"].
                      createInstance(Ci.nsIMutableArray);
            strData.appendElement(wrapit(str1), false);
            strData.appendElement(wrapit(str2), false);
            strData.appendElement(wrapit(str3), false);
        }
        this.sendNotification(changeType, strData);
    },


    sendIntNotification : function (changeType, int1, int2) {
        function wrapit(int) {
            let wrapper = Cc["@mozilla.org/supports-PRInt64;1"].
                          createInstance(Ci.nsISupportsPRInt64);
            wrapper.data = int;
            return wrapper;
        }

        let intData;
        if (arguments.length == 2) {
            
            intData = wrapit(int1);
        } else {
            
            intData = Cc["@mozilla.org/array;1"].
                      createInstance(Ci.nsIMutableArray);
            intData.appendElement(wrapit(int1), false);
            intData.appendElement(wrapit(int2), false);
        }
        this.sendNotification(changeType, intData);
    },


    sendNotification : function (changeType, data) {
        Services.obs.notifyObservers(data, "satchel-storage-changed", changeType);
    },


    getExistingEntryID : function (name, value) {
        let id = -1, guid = null;
        let stmt;
        let query = "SELECT id, guid FROM moz_formhistory WHERE fieldname = :fieldname AND value = :value";
        let params = {
                        fieldname : name,
                        value     : value
                     };
        try {
            stmt = this.dbCreateStatement(query, params);
            if (stmt.executeStep()) {
                id   = stmt.row.id;
                guid = stmt.row.guid;
            }
        } catch (e) {
            this.log("getExistingEntryID failed: " + e);
            throw e;
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }

        return [id, guid];
    },


    countAllEntries : function () {
        let query = "SELECT COUNT(1) AS numEntries FROM moz_formhistory";

        let stmt, numEntries;
        try {
            stmt = this.dbCreateStatement(query, null);
            stmt.executeStep();
            numEntries = stmt.row.numEntries;
        } catch (e) {
            this.log("countAllEntries failed: " + e);
            throw e;
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }

        this.log("countAllEntries: counted entries: " + numEntries);
        return numEntries;
    },


    expireOldEntries : function () {
        this.log("expireOldEntries");

        
        let expireDays = 180;
        try {
            expireDays = Services.prefs.getIntPref("browser.formfill.expire_days");
        } catch (e) {  }

        let expireTime = Date.now() - expireDays * DAY_IN_MS;
        expireTime *= 1000; 

        this.sendIntNotification("before-expireOldEntries", expireTime);

        let beginningCount = this.countAllEntries();

        
        let stmt;
        let query = "DELETE FROM moz_formhistory WHERE lastUsed <= :expireTime";
        let params = { expireTime : expireTime };

        try {
            stmt = this.dbCreateStatement(query, params);
            stmt.execute();
        } catch (e) {
            this.log("expireOldEntries failed: " + e);
            throw e;
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }

        let endingCount = this.countAllEntries();

        
        
        
        
        if (beginningCount - endingCount > 500)
            this.dbConnection.executeSimpleSQL("VACUUM");

        this.sendIntNotification("expireOldEntries", expireTime);
    },


    updatePrefs : function () {
        this.debug          = Services.prefs.getBoolPref("browser.formfill.debug");
        this.enabled        = Services.prefs.getBoolPref("browser.formfill.enable");
        this.saveHttpsForms = Services.prefs.getBoolPref("browser.formfill.saveHttpsForms");
    },


    

    





    dbCreateStatement : function (query, params) {
        let stmt = this.dbStmts[query];
        
        if (!stmt) {
            this.log("Creating new statement for query: " + query);
            stmt = this.dbConnection.createStatement(query);
            this.dbStmts[query] = stmt;
        }
        
        if (params)
            for (let i in params)
                stmt.params[i] = params[i];
        return stmt;
    },


    





    dbInit : function () {
        this.log("Initializing Database");

        let storage = Cc["@mozilla.org/storage/service;1"].
                      getService(Ci.mozIStorageService);
        this.dbConnection = storage.openDatabase(this.dbFile);
        let version = this.dbConnection.schemaVersion;

        
        
        if (version == 0 && !this.dbConnection.tableExists("moz_formhistory"))
            this.dbCreate();
        else if (version != DB_VERSION)
            this.dbMigrate(version);
    },


    dbCreate: function () {
        this.log("Creating DB -- tables");
        for (let name in this.dbSchema.tables) {
            let table = this.dbSchema.tables[name];
            let tSQL = [[col, table[col]].join(" ") for (col in table)].join(", ");
            this.dbConnection.createTable(name, tSQL);
        }

        this.log("Creating DB -- indices");
        for (let name in this.dbSchema.indices) {
            let index = this.dbSchema.indices[name];
            let statement = "CREATE INDEX IF NOT EXISTS " + name + " ON " + index.table +
                            "(" + index.columns.join(", ") + ")";
            this.dbConnection.executeSimpleSQL(statement);
        }

        this.dbConnection.schemaVersion = DB_VERSION;
    },


    dbMigrate : function (oldVersion) {
        this.log("Attempting to migrate from version " + oldVersion);

        if (oldVersion > DB_VERSION) {
            this.log("Downgrading to version " + DB_VERSION);
            
            
            
            
            

            if (!this.dbAreExpectedColumnsPresent())
                throw Components.Exception("DB is missing expected columns",
                                           Cr.NS_ERROR_FILE_CORRUPTED);

            
            
            
            this.dbConnection.schemaVersion = DB_VERSION;
            return;
        }

        

        this.dbConnection.beginTransaction();

        try {
            for (let v = oldVersion + 1; v <= DB_VERSION; v++) {
                this.log("Upgrading to version " + v + "...");
                let migrateFunction = "dbMigrateToVersion" + v;
                this[migrateFunction]();
            }
        } catch (e) {
            this.log("Migration failed: "  + e);
            this.dbConnection.rollbackTransaction();
            throw e;
        }

        this.dbConnection.schemaVersion = DB_VERSION;
        this.dbConnection.commitTransaction();
        this.log("DB migration completed.");
    },


    





    dbMigrateToVersion1 : function () {
        
        
        let query;
        ["timesUsed", "firstUsed", "lastUsed"].forEach(function(column) {
            if (!this.dbColumnExists(column)) {
                query = "ALTER TABLE moz_formhistory ADD COLUMN " + column + " INTEGER";
                this.dbConnection.executeSimpleSQL(query);
            }
        }, this);

        
        
        
        
        
        
        let stmt;
        query = "UPDATE moz_formhistory " +
                "SET timesUsed = 1, firstUsed = :time, lastUsed = :time " +
                "WHERE timesUsed isnull OR firstUsed isnull or lastUsed isnull";
        let params = { time: (Date.now() - DAY_IN_MS) * 1000 }
        try {
            stmt = this.dbCreateStatement(query, params);
            stmt.execute();
        } catch (e) {
            this.log("Failed setting timestamps: " + e);
            throw e;
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }
    },


    





    dbMigrateToVersion2 : function () {
        let query = "DROP TABLE IF EXISTS moz_dummy_table";
        this.dbConnection.executeSimpleSQL(query);

        query = "CREATE INDEX IF NOT EXISTS moz_formhistory_lastused_index ON moz_formhistory (lastUsed)";
        this.dbConnection.executeSimpleSQL(query);
    },


    





    dbMigrateToVersion3 : function () {
        
        let query;
        if (!this.dbColumnExists("guid")) {
            query = "ALTER TABLE moz_formhistory ADD COLUMN guid TEXT";
            this.dbConnection.executeSimpleSQL(query);

            query = "CREATE INDEX IF NOT EXISTS moz_formhistory_guid_index ON moz_formhistory (guid)";
            this.dbConnection.executeSimpleSQL(query);
        }

        
        let ids = [];
        query = "SELECT id FROM moz_formhistory WHERE guid isnull";
        let stmt;
        try {
            stmt = this.dbCreateStatement(query);
            while (stmt.executeStep())
                ids.push(stmt.row.id);
        } catch (e) {
            this.log("Failed getting IDs: " + e);
            throw e;
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }

        
        query = "UPDATE moz_formhistory SET guid = :guid WHERE id = :id";
        for each (let id in ids) {
            let params = {
                id   : id,
                guid : this.generateGUID()
            };

            try {
                stmt = this.dbCreateStatement(query, params);
                stmt.execute();
            } catch (e) {
                this.log("Failed setting GUID: " + e);
                throw e;
            } finally {
                if (stmt) {
                    stmt.reset();
                }
            }
        }
    },


    





    dbAreExpectedColumnsPresent : function () {
        for (let name in this.dbSchema.tables) {
            let table = this.dbSchema.tables[name];
            let query = "SELECT " +
                        [col for (col in table)].join(", ") +
                        " FROM " + name;
            try {
                let stmt = this.dbConnection.createStatement(query);
                
                stmt.finalize();
            } catch (e) {
                return false;
            }
        }

        this.log("verified that expected columns are present in DB.");
        return true;
    },


    




    dbColumnExists : function (columnName) {
        let query = "SELECT " + columnName + " FROM moz_formhistory";
        try {
            let stmt = this.dbConnection.createStatement(query);
            
            stmt.finalize();
            return true;
        } catch (e) {
            return false;
        }
    },


    





    dbCleanup : function (backup) {
        this.log("Cleaning up DB file - close & remove & backup=" + backup)

        
        if (backup) {
            let storage = Cc["@mozilla.org/storage/service;1"].
                          getService(Ci.mozIStorageService);

            let backupFile = this.dbFile.leafName + ".corrupt";
            storage.backupDatabaseFile(this.dbFile, backupFile);
        }

        
        for each (let stmt in this.dbStmts)
            stmt.finalize();
        this.dbStmts = [];

        
        try { this.dbConnection.close() } catch(e) {}
        this.dbFile.remove(false);
    }
};

let component = [FormHistory];
var NSGetFactory = XPCOMUtils.generateNSGetFactory(component);
