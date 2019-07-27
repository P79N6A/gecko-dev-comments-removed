






const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DB_VERSION = 5; 

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/Promise.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LoginHelper",
                                  "resource://gre/modules/LoginHelper.jsm");








function Transaction(aDatabase) {
    this._db = aDatabase;

    this._hasTransaction = false;
    try {
        this._db.beginTransaction();
        this._hasTransaction = true;
    }
    catch(e) {  }
}

Transaction.prototype = {
    commit : function() {
        if (this._hasTransaction)
            this._db.commitTransaction();
    },

    rollback : function() {
        if (this._hasTransaction)
            this._db.rollbackTransaction();
    },
};


function LoginManagerStorage_mozStorage() { };

LoginManagerStorage_mozStorage.prototype = {

    classID : Components.ID("{8c2023b9-175c-477e-9761-44ae7b549756}"),
    QueryInterface : XPCOMUtils.generateQI([Ci.nsILoginManagerStorage,
                                            Ci.nsIInterfaceRequestor]),
    getInterface : function(aIID) {
        if (aIID.equals(Ci.nsIVariant)) {
            
            return this;
        }

        if (aIID.equals(Ci.mozIStorageConnection)) {
            return this._dbConnection;
        }

        throw Cr.NS_ERROR_NO_INTERFACE;
    },

    __crypto : null,  
    get _crypto() {
        if (!this.__crypto)
            this.__crypto = Cc["@mozilla.org/login-manager/crypto/SDR;1"].
                            getService(Ci.nsILoginManagerCrypto);
        return this.__crypto;
    },

    __profileDir: null,  
    get _profileDir() {
        if (!this.__profileDir)
            this.__profileDir = Services.dirsvc.get("ProfD", Ci.nsIFile);
        return this.__profileDir;
    },

    __storageService: null, 
    get _storageService() {
        if (!this.__storageService)
            this.__storageService = Cc["@mozilla.org/storage/service;1"].
                                    getService(Ci.mozIStorageService);
        return this.__storageService;
    },

    __uuidService: null,
    get _uuidService() {
        if (!this.__uuidService)
            this.__uuidService = Cc["@mozilla.org/uuid-generator;1"].
                                 getService(Ci.nsIUUIDGenerator);
        return this.__uuidService;
    },


    
    _dbSchema: {
        tables: {
            moz_logins:         "id                  INTEGER PRIMARY KEY," +
                                "hostname            TEXT NOT NULL,"       +
                                "httpRealm           TEXT,"                +
                                "formSubmitURL       TEXT,"                +
                                "usernameField       TEXT NOT NULL,"       +
                                "passwordField       TEXT NOT NULL,"       +
                                "encryptedUsername   TEXT NOT NULL,"       +
                                "encryptedPassword   TEXT NOT NULL,"       +
                                "guid                TEXT,"                +
                                "encType             INTEGER,"             +
                                "timeCreated         INTEGER,"             +
                                "timeLastUsed        INTEGER,"             +
                                "timePasswordChanged INTEGER,"             +
                                "timesUsed           INTEGER",
            
            

            moz_disabledHosts:  "id                 INTEGER PRIMARY KEY," +
                                "hostname           TEXT UNIQUE ON CONFLICT REPLACE",

            moz_deleted_logins: "id                  INTEGER PRIMARY KEY," +
                                "guid                TEXT,"                +
                                "timeDeleted         INTEGER",
        },
        indices: {
          moz_logins_hostname_index: {
            table: "moz_logins",
            columns: ["hostname"]
          },
          moz_logins_hostname_formSubmitURL_index: {
            table: "moz_logins",
            columns: ["hostname", "formSubmitURL"]
          },
          moz_logins_hostname_httpRealm_index: {
              table: "moz_logins",
              columns: ["hostname", "httpRealm"]
          },
          moz_logins_guid_index: {
              table: "moz_logins",
              columns: ["guid"]
          },
          moz_logins_encType_index: {
              table: "moz_logins",
              columns: ["encType"]
          }
        }
    },
    _dbConnection : null,  
    _dbStmts      : null,  

    _prefBranch   : null,  
    _signonsFile  : null,  
    _debug        : false, 


    




    log : function (message) {
        if (!this._debug)
            return;
        dump("PwMgr mozStorage: " + message + "\n");
        Services.console.logStringMessage("PwMgr mozStorage: " + message);
    },


    



    initWithFile : function(aDBFile) {
        if (aDBFile)
            this._signonsFile = aDBFile;

        this.initialize();
    },


    



    initialize : function () {
        this._dbStmts = {};

        
        this._prefBranch = Services.prefs.getBranch("signon.");
        this._debug = this._prefBranch.getBoolPref("debug");

        let isFirstRun;
        try {
            
            
            this._crypto;

            
            if (!this._signonsFile) {
                
                this._signonsFile = this._profileDir.clone();
                this._signonsFile.append("signons.sqlite");
            }
            this.log("Opening database at " + this._signonsFile.path);

            
            isFirstRun = this._dbInit();

            this._initialized = true;

            return Promise.resolve();
        } catch (e) {
            this.log("Initialization failed: " + e);
            
            if (isFirstRun && e == "Import failed")
                this._dbCleanup(false);
            throw "Initialization failed";
        }
    },


    





    terminate : function () {
        return Promise.resolve();
    },


    



    addLogin : function (login) {
        
        LoginHelper.checkLoginValues(login);

        let [encUsername, encPassword, encType] = this._encryptLogin(login);

        
        let loginClone = login.clone();

        
        loginClone.QueryInterface(Ci.nsILoginMetaInfo);
        if (loginClone.guid) {
            if (!this._isGuidUnique(loginClone.guid))
                throw "specified GUID already exists";
        } else {
            loginClone.guid = this._uuidService.generateUUID().toString();
        }

        
        let currentTime = Date.now();
        if (!loginClone.timeCreated)
            loginClone.timeCreated = currentTime;
        if (!loginClone.timeLastUsed)
            loginClone.timeLastUsed = currentTime;
        if (!loginClone.timePasswordChanged)
            loginClone.timePasswordChanged = currentTime;
        if (!loginClone.timesUsed)
            loginClone.timesUsed = 1;

        let query =
            "INSERT INTO moz_logins " +
            "(hostname, httpRealm, formSubmitURL, usernameField, " +
             "passwordField, encryptedUsername, encryptedPassword, " +
             "guid, encType, timeCreated, timeLastUsed, timePasswordChanged, " +
             "timesUsed) " +
            "VALUES (:hostname, :httpRealm, :formSubmitURL, :usernameField, " +
                    ":passwordField, :encryptedUsername, :encryptedPassword, " +
                    ":guid, :encType, :timeCreated, :timeLastUsed, " +
                    ":timePasswordChanged, :timesUsed)";

        let params = {
            hostname:            loginClone.hostname,
            httpRealm:           loginClone.httpRealm,
            formSubmitURL:       loginClone.formSubmitURL,
            usernameField:       loginClone.usernameField,
            passwordField:       loginClone.passwordField,
            encryptedUsername:   encUsername,
            encryptedPassword:   encPassword,
            guid:                loginClone.guid,
            encType:             encType,
            timeCreated:         loginClone.timeCreated,
            timeLastUsed:        loginClone.timeLastUsed,
            timePasswordChanged: loginClone.timePasswordChanged,
            timesUsed:           loginClone.timesUsed
        };

        let stmt;
        try {
            stmt = this._dbCreateStatement(query, params);
            stmt.execute();
        } catch (e) {
            this.log("addLogin failed: " + e.name + " : " + e.message);
            throw "Couldn't write to database, login not added.";
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }

        
        this._sendNotification("addLogin", loginClone);
    },


    



    removeLogin : function (login) {
        let [idToDelete, storedLogin] = this._getIdForLogin(login);
        if (!idToDelete)
            throw "No matching logins";

        
        let query  = "DELETE FROM moz_logins WHERE id = :id";
        let params = { id: idToDelete };
        let stmt;
        let transaction = new Transaction(this._dbConnection);
        try {
            stmt = this._dbCreateStatement(query, params);
            stmt.execute();
            this.storeDeletedLogin(storedLogin);
            transaction.commit();
        } catch (e) {
            this.log("_removeLogin failed: " + e.name + " : " + e.message);
            throw "Couldn't write to database, login not removed.";
            transaction.rollback();
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }
        this._sendNotification("removeLogin", storedLogin);
    },


    



    modifyLogin : function (oldLogin, newLoginData) {
        let [idToModify, oldStoredLogin] = this._getIdForLogin(oldLogin);
        if (!idToModify)
            throw "No matching logins";

        let newLogin = LoginHelper.buildModifiedLogin(oldStoredLogin, newLoginData);

        
        if (newLogin.guid != oldStoredLogin.guid &&
            !this._isGuidUnique(newLogin.guid)) 
        {
            throw "specified GUID already exists";
        }

        
        if (!newLogin.matches(oldLogin, true)) {
            let logins = this.findLogins({}, newLogin.hostname,
                                         newLogin.formSubmitURL,
                                         newLogin.httpRealm);

            if (logins.some(login => newLogin.matches(login, true)))
                throw "This login already exists.";
        }

        
        let [encUsername, encPassword, encType] = this._encryptLogin(newLogin);

        let query =
            "UPDATE moz_logins " +
            "SET hostname = :hostname, " +
                "httpRealm = :httpRealm, " +
                "formSubmitURL = :formSubmitURL, " +
                "usernameField = :usernameField, " +
                "passwordField = :passwordField, " +
                "encryptedUsername = :encryptedUsername, " +
                "encryptedPassword = :encryptedPassword, " +
                "guid = :guid, " +
                "encType = :encType, " +
                "timeCreated = :timeCreated, " +
                "timeLastUsed = :timeLastUsed, " +
                "timePasswordChanged = :timePasswordChanged, " +
                "timesUsed = :timesUsed " +
            "WHERE id = :id";

        let params = {
            id:                  idToModify,
            hostname:            newLogin.hostname,
            httpRealm:           newLogin.httpRealm,
            formSubmitURL:       newLogin.formSubmitURL,
            usernameField:       newLogin.usernameField,
            passwordField:       newLogin.passwordField,
            encryptedUsername:   encUsername,
            encryptedPassword:   encPassword,
            guid:                newLogin.guid,
            encType:             encType,
            timeCreated:         newLogin.timeCreated,
            timeLastUsed:        newLogin.timeLastUsed,
            timePasswordChanged: newLogin.timePasswordChanged,
            timesUsed:           newLogin.timesUsed
        };

        let stmt;
        try {
            stmt = this._dbCreateStatement(query, params);
            stmt.execute();
        } catch (e) {
            this.log("modifyLogin failed: " + e.name + " : " + e.message);
            throw "Couldn't write to database, login not modified.";
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }

        this._sendNotification("modifyLogin", [oldStoredLogin, newLogin]);
    },


    




    getAllLogins : function (count) {
        let [logins, ids] = this._searchLogins({});

        
        logins = this._decryptLogins(logins);

        this.log("_getAllLogins: returning " + logins.length + " logins.");
        if (count)
            count.value = logins.length; 
        return logins;
    },


    







    searchLogins : function(count, matchData) {
        let realMatchData = {};
        
        let propEnum = matchData.enumerator;
        while (propEnum.hasMoreElements()) {
            let prop = propEnum.getNext().QueryInterface(Ci.nsIProperty);
            realMatchData[prop.name] = prop.value;
        }

        let [logins, ids] = this._searchLogins(realMatchData);

        
        logins = this._decryptLogins(logins);

        count.value = logins.length; 
        return logins;
    },


    









    _searchLogins : function (matchData) {
        let conditions = [], params = {};

        for (let field in matchData) {
            let value = matchData[field];
            switch (field) {
                
                case "formSubmitURL":
                    if (value != null) {
                        conditions.push("formSubmitURL = :formSubmitURL OR formSubmitURL = ''");
                        params["formSubmitURL"] = value;
                        break;
                    }
                
                case "hostname":
                case "httpRealm":
                case "id":
                case "usernameField":
                case "passwordField":
                case "encryptedUsername":
                case "encryptedPassword":
                case "guid":
                case "encType":
                case "timeCreated":
                case "timeLastUsed":
                case "timePasswordChanged":
                case "timesUsed":
                    if (value == null) {
                        conditions.push(field + " isnull");
                    } else {
                        conditions.push(field + " = :" + field);
                        params[field] = value;
                    }
                    break;
                
                default:
                    throw "Unexpected field: " + field;
            }
        }

        
        let query = "SELECT * FROM moz_logins";
        if (conditions.length) {
            conditions = conditions.map(function(c) "(" + c + ")");
            query += " WHERE " + conditions.join(" AND ");
        }

        let stmt;
        let logins = [], ids = [];
        try {
            stmt = this._dbCreateStatement(query, params);
            
            while (stmt.executeStep()) {
                
                let login = Cc["@mozilla.org/login-manager/loginInfo;1"].
                            createInstance(Ci.nsILoginInfo);
                login.init(stmt.row.hostname, stmt.row.formSubmitURL,
                           stmt.row.httpRealm, stmt.row.encryptedUsername,
                           stmt.row.encryptedPassword, stmt.row.usernameField,
                           stmt.row.passwordField);
                
                login.QueryInterface(Ci.nsILoginMetaInfo);
                login.guid = stmt.row.guid;
                login.timeCreated = stmt.row.timeCreated;
                login.timeLastUsed = stmt.row.timeLastUsed;
                login.timePasswordChanged = stmt.row.timePasswordChanged;
                login.timesUsed = stmt.row.timesUsed;
                logins.push(login);
                ids.push(stmt.row.id);
            }
        } catch (e) {
            this.log("_searchLogins failed: " + e.name + " : " + e.message);
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }

        this.log("_searchLogins: returning " + logins.length + " logins");
        return [logins, ids];
    },

    




     storeDeletedLogin : function(aLogin) {
          let stmt = null; 
          try {
              this.log("Storing " + aLogin.guid + " in deleted passwords\n");
              let query = "INSERT INTO moz_deleted_logins (guid, timeDeleted) VALUES (:guid, :timeDeleted)";
              let params = { guid: aLogin.guid,
                             timeDeleted: Date.now() };
              let stmt = this._dbCreateStatement(query, params);
              stmt.execute();
          } catch(ex) {
              throw ex;
          } finally {
              if (stmt)
                  stmt.reset();
          }		
     },


    




    removeAllLogins : function () {
        this.log("Removing all logins");
        let query;
        let stmt;
        let transaction = new Transaction(this._dbConnection);
 
        
        
        
        query = "DELETE FROM moz_logins";
        try {
            stmt = this._dbCreateStatement(query);
            stmt.execute();
            transaction.commit();
        } catch (e) {
            this.log("_removeAllLogins failed: " + e.name + " : " + e.message);
            transaction.rollback();
            throw "Couldn't write to database";
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }

        this._sendNotification("removeAllLogins", null);
    },


    



    getAllDisabledHosts : function (count) {
        let disabledHosts = this._queryDisabledHosts(null);

        this.log("_getAllDisabledHosts: returning " + disabledHosts.length + " disabled hosts.");
        if (count)
            count.value = disabledHosts.length; 
        return disabledHosts;
    },


    



    getLoginSavingEnabled : function (hostname) {
        this.log("Getting login saving is enabled for " + hostname);
        return this._queryDisabledHosts(hostname).length == 0
    },


    



    setLoginSavingEnabled : function (hostname, enabled) {
        
        LoginHelper.checkHostnameValue(hostname);

        this.log("Setting login saving enabled for " + hostname + " to " + enabled);
        let query;
        if (enabled)
            query = "DELETE FROM moz_disabledHosts " +
                    "WHERE hostname = :hostname";
        else
            query = "INSERT INTO moz_disabledHosts " +
                    "(hostname) VALUES (:hostname)";
        let params = { hostname: hostname };

        let stmt
        try {
            stmt = this._dbCreateStatement(query, params);
            stmt.execute();
        } catch (e) {
            this.log("setLoginSavingEnabled failed: " + e.name + " : " + e.message);
            throw "Couldn't write to database"
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }

        this._sendNotification(enabled ? "hostSavingEnabled" : "hostSavingDisabled", hostname);
    },


    



    findLogins : function (count, hostname, formSubmitURL, httpRealm) {
        let loginData = {
            hostname: hostname,
            formSubmitURL: formSubmitURL,
            httpRealm: httpRealm
        };
        let matchData = { };
        for each (let field in ["hostname", "formSubmitURL", "httpRealm"])
            if (loginData[field] != '')
                matchData[field] = loginData[field];
        let [logins, ids] = this._searchLogins(matchData);

        
        logins = this._decryptLogins(logins);

        this.log("_findLogins: returning " + logins.length + " logins");
        count.value = logins.length; 
        return logins;
    },


    



    countLogins : function (hostname, formSubmitURL, httpRealm) {
        
        let [conditions, params] =
            this._buildConditionsAndParams(hostname, formSubmitURL, httpRealm);

        let query = "SELECT COUNT(1) AS numLogins FROM moz_logins";
        if (conditions.length) {
            conditions = conditions.map(function(c) "(" + c + ")");
            query += " WHERE " + conditions.join(" AND ");
        }

        let stmt, numLogins;
        try {
            stmt = this._dbCreateStatement(query, params);
            stmt.executeStep();
            numLogins = stmt.row.numLogins;
        } catch (e) {
            this.log("_countLogins failed: " + e.name + " : " + e.message);
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }

        this.log("_countLogins: counted logins: " + numLogins);
        return numLogins;
    },


    


    get uiBusy() {
        return this._crypto.uiBusy;
    },


    


    get isLoggedIn() {
        return this._crypto.isLoggedIn;
    },


    




    _sendNotification : function (changeType, data) {
        let dataObject = data;
        
        if (data instanceof Array) {
            dataObject = Cc["@mozilla.org/array;1"].
                         createInstance(Ci.nsIMutableArray);
            for (let i = 0; i < data.length; i++)
                dataObject.appendElement(data[i], false);
        } else if (typeof(data) == "string") {
            dataObject = Cc["@mozilla.org/supports-string;1"].
                         createInstance(Ci.nsISupportsString);
            dataObject.data = data;
        }
        Services.obs.notifyObservers(dataObject, "passwordmgr-storage-changed", changeType);
    },


    






    _getIdForLogin : function (login) {
        let matchData = { };
        for each (let field in ["hostname", "formSubmitURL", "httpRealm"])
            if (login[field] != '')
                matchData[field] = login[field];
        let [logins, ids] = this._searchLogins(matchData);

        let id = null;
        let foundLogin = null;

        
        
        
        
        for (let i = 0; i < logins.length; i++) {
            let [decryptedLogin] = this._decryptLogins([logins[i]]);

            if (!decryptedLogin || !decryptedLogin.equals(login))
                continue;

            
            foundLogin = decryptedLogin;
            id = ids[i];
            break;
        }

        return [id, foundLogin];
    },


    






    _queryDisabledHosts : function (hostname) {
        let disabledHosts = [];

        let query = "SELECT hostname FROM moz_disabledHosts";
        let params = {};
        if (hostname) {
            query += " WHERE hostname = :hostname";
            params = { hostname: hostname };
        }

        let stmt;
        try {
            stmt = this._dbCreateStatement(query, params);
            while (stmt.executeStep())
                disabledHosts.push(stmt.row.hostname);
        } catch (e) {
            this.log("_queryDisabledHosts failed: " + e.name + " : " + e.message);
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }

        return disabledHosts;
    },


    






    _buildConditionsAndParams : function (hostname, formSubmitURL, httpRealm) {
        let conditions = [], params = {};

        if (hostname == null) {
            conditions.push("hostname isnull");
        } else if (hostname != '') {
            conditions.push("hostname = :hostname");
            params["hostname"] = hostname;
        }

        if (formSubmitURL == null) {
            conditions.push("formSubmitURL isnull");
        } else if (formSubmitURL != '') {
            conditions.push("formSubmitURL = :formSubmitURL OR formSubmitURL = ''");
            params["formSubmitURL"] = formSubmitURL;
        }

        if (httpRealm == null) {
            conditions.push("httpRealm isnull");
        } else if (httpRealm != '') {
            conditions.push("httpRealm = :httpRealm");
            params["httpRealm"] = httpRealm;
        }

        return [conditions, params];
    },


    




    _isGuidUnique : function (guid) {
        let query = "SELECT COUNT(1) AS numLogins FROM moz_logins WHERE guid = :guid";
        let params = { guid: guid };

        let stmt, numLogins;
        try {
            stmt = this._dbCreateStatement(query, params);
            stmt.executeStep();
            numLogins = stmt.row.numLogins;
        } catch (e) {
            this.log("_isGuidUnique failed: " + e.name + " : " + e.message);
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }

        return (numLogins == 0);
    },


    





    _encryptLogin : function (login) {
        let encUsername = this._crypto.encrypt(login.username);
        let encPassword = this._crypto.encrypt(login.password);
        let encType     = this._crypto.defaultEncType;

        return [encUsername, encPassword, encType];
    },


    












    _decryptLogins : function (logins) {
        let result = [];

        for each (let login in logins) {
            try {
                login.username = this._crypto.decrypt(login.username);
                login.password = this._crypto.decrypt(login.password);
            } catch (e) {
                
                
                if (e.result == Cr.NS_ERROR_FAILURE)
                    continue;
                throw e;
            }
            result.push(login);
        }

        return result;
    },


    
    

    






    _dbCreateStatement : function (query, params) {
        let wrappedStmt = this._dbStmts[query];
        
        if (!wrappedStmt) {
            this.log("Creating new statement for query: " + query);
            wrappedStmt = this._dbConnection.createStatement(query);
            this._dbStmts[query] = wrappedStmt;
        }
        
        if (params)
            for (let i in params)
                wrappedStmt.params[i] = params[i];
        return wrappedStmt;
    },


    





    _dbInit : function () {
        this.log("Initializing Database");
        let isFirstRun = false;
        try {
            this._dbConnection = this._storageService.openDatabase(this._signonsFile);
            
            
            let version = this._dbConnection.schemaVersion;
            if (version == 0) {
                this._dbCreate();
                isFirstRun = true;
            } else if (version != DB_VERSION) {
                this._dbMigrate(version);
            }
        } catch (e if e.result == Cr.NS_ERROR_FILE_CORRUPTED) {
            
            
            this._dbCleanup(true);
            throw e;
        }

        Services.obs.addObserver(this, "profile-before-change", false);
        return isFirstRun;
    },

    observe: function (subject, topic, data) {
        switch (topic) {
            case "profile-before-change":
                Services.obs.removeObserver(this, "profile-before-change");
                this._dbClose();
            break;
        }
    },

    _dbCreate: function () {
        this.log("Creating Database");
        this._dbCreateSchema();
        this._dbConnection.schemaVersion = DB_VERSION;
    },


    _dbCreateSchema : function () {
        this._dbCreateTables();
        this._dbCreateIndices();
    },


    _dbCreateTables : function () {
        this.log("Creating Tables");
        for (let name in this._dbSchema.tables)
            this._dbConnection.createTable(name, this._dbSchema.tables[name]);
    },


    _dbCreateIndices : function () {
        this.log("Creating Indices");
        for (let name in this._dbSchema.indices) {
            let index = this._dbSchema.indices[name];
            let statement = "CREATE INDEX IF NOT EXISTS " + name + " ON " + index.table +
                            "(" + index.columns.join(", ") + ")";
            this._dbConnection.executeSimpleSQL(statement);
        }
    },


    _dbMigrate : function (oldVersion) {
        this.log("Attempting to migrate from version " + oldVersion);

        if (oldVersion > DB_VERSION) {
            this.log("Downgrading to version " + DB_VERSION);
            
            
            
            
            

            if (!this._dbAreExpectedColumnsPresent())
                throw Components.Exception("DB is missing expected columns",
                                           Cr.NS_ERROR_FILE_CORRUPTED);

            
            
            
            this._dbConnection.schemaVersion = DB_VERSION;
            return;
        }

        

        let transaction = new Transaction(this._dbConnection);

        try {
            for (let v = oldVersion + 1; v <= DB_VERSION; v++) {
                this.log("Upgrading to version " + v + "...");
                let migrateFunction = "_dbMigrateToVersion" + v;
                this[migrateFunction]();
            }
        } catch (e) {
            this.log("Migration failed: "  + e);
            transaction.rollback();
            throw e;
        }

        this._dbConnection.schemaVersion = DB_VERSION;
        transaction.commit();
        this.log("DB migration completed.");
    },


    




    _dbMigrateToVersion2 : function () {
        
        let query;
        if (!this._dbColumnExists("guid")) {
            query = "ALTER TABLE moz_logins ADD COLUMN guid TEXT";
            this._dbConnection.executeSimpleSQL(query);

            query = "CREATE INDEX IF NOT EXISTS moz_logins_guid_index ON moz_logins (guid)";
            this._dbConnection.executeSimpleSQL(query);
        }

        
        let ids = [];
        query = "SELECT id FROM moz_logins WHERE guid isnull";
        let stmt;
        try {
            stmt = this._dbCreateStatement(query);
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

        
        query = "UPDATE moz_logins SET guid = :guid WHERE id = :id";
        for each (let id in ids) {
            let params = {
                id:   id,
                guid: this._uuidService.generateUUID().toString()
            };

            try {
                stmt = this._dbCreateStatement(query, params);
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


    




    _dbMigrateToVersion3 : function () {
        
        let query;
        if (!this._dbColumnExists("encType")) {
            query = "ALTER TABLE moz_logins ADD COLUMN encType INTEGER";
            this._dbConnection.executeSimpleSQL(query);

            query = "CREATE INDEX IF NOT EXISTS " +
                        "moz_logins_encType_index ON moz_logins (encType)";
            this._dbConnection.executeSimpleSQL(query);
        }

        
        let logins = [];
        let stmt;
        query = "SELECT id, encryptedUsername, encryptedPassword " +
                    "FROM moz_logins WHERE encType isnull";
        try {
            stmt = this._dbCreateStatement(query);
            while (stmt.executeStep()) {
                let params = { id: stmt.row.id };
                
                if (stmt.row.encryptedUsername.charAt(0) == '~' ||
                    stmt.row.encryptedPassword.charAt(0) == '~')
                    params.encType = Ci.nsILoginManagerCrypto.ENCTYPE_BASE64;
                else
                    params.encType = Ci.nsILoginManagerCrypto.ENCTYPE_SDR;
                logins.push(params);
            }
        } catch (e) {
            this.log("Failed getting logins: " + e);
            throw e;
        } finally {
            if (stmt) {
                stmt.reset();
            }
        }

        
        query = "UPDATE moz_logins SET encType = :encType WHERE id = :id";
        for each (let params in logins) {
            try {
                stmt = this._dbCreateStatement(query, params);
                stmt.execute();
            } catch (e) {
                this.log("Failed setting encType: " + e);
                throw e;
            } finally {
                if (stmt) {
                    stmt.reset();
                }
            }
        }
    },


    





    _dbMigrateToVersion4 : function () {
        let query;
        
        for each (let column in ["timeCreated", "timeLastUsed", "timePasswordChanged", "timesUsed"]) {
            if (!this._dbColumnExists(column)) {
                query = "ALTER TABLE moz_logins ADD COLUMN " + column + " INTEGER";
                this._dbConnection.executeSimpleSQL(query);
            }
        }

        
        let ids = [];
        let stmt;
        query = "SELECT id FROM moz_logins WHERE timeCreated isnull OR " +
                "timeLastUsed isnull OR timePasswordChanged isnull OR timesUsed isnull";
        try {
            stmt = this._dbCreateStatement(query);
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

        
        query = "UPDATE moz_logins SET timeCreated = :initTime, timeLastUsed = :initTime, " +
                "timePasswordChanged = :initTime, timesUsed = 1 WHERE id = :id";
        let params = {
            id:       null,
            initTime: Date.now()
        };
        for each (let id in ids) {
            params.id = id;
            try {
                stmt = this._dbCreateStatement(query, params);
                stmt.execute();
            } catch (e) {
                this.log("Failed setting timestamps: " + e);
                throw e;
            } finally {
                if (stmt) {
                    stmt.reset();
                }
            }
        }
    },


    




    _dbMigrateToVersion5 : function () {
        if (!this._dbConnection.tableExists("moz_deleted_logins")) {
          this._dbConnection.createTable("moz_deleted_logins", this._dbSchema.tables.moz_deleted_logins);
        }
    },

    





    _dbAreExpectedColumnsPresent : function () {
        let query = "SELECT " +
                       "id, " +
                       "hostname, " +
                       "httpRealm, " +
                       "formSubmitURL, " +
                       "usernameField, " +
                       "passwordField, " +
                       "encryptedUsername, " +
                       "encryptedPassword, " +
                       "guid, " +
                       "encType, " +
                       "timeCreated, " +
                       "timeLastUsed, " +
                       "timePasswordChanged, " +
                       "timesUsed " +
                    "FROM moz_logins";
        try {
            let stmt = this._dbConnection.createStatement(query);
            
            stmt.finalize();
        } catch (e) {
            return false;
        }

        query = "SELECT " +
                   "id, " +
                   "hostname " +
                "FROM moz_disabledHosts";
        try {
            let stmt = this._dbConnection.createStatement(query);
            
            stmt.finalize();
        } catch (e) {
            return false;
        }

        this.log("verified that expected columns are present in DB.");
        return true;
    },


    




    _dbColumnExists : function (columnName) {
        let query = "SELECT " + columnName + " FROM moz_logins";
        try {
            let stmt = this._dbConnection.createStatement(query);
            
            stmt.finalize();
            return true;
        } catch (e) {
            return false;
        }
    },

    _dbClose : function () {
        this.log("Closing the DB connection.");
        
        for each (let stmt in this._dbStmts) {
            stmt.finalize();
        }
        this._dbStmts = {};

        if (this._dbConnection !== null) {
            try {
                this._dbConnection.close();
            } catch (e) {
                Components.utils.reportError(e);
            }
        }
        this._dbConnection = null;
    },

    





    _dbCleanup : function (backup) {
        this.log("Cleaning up DB file - close & remove & backup=" + backup)

        
        if (backup) {
            let backupFile = this._signonsFile.leafName + ".corrupt";
            this._storageService.backupDatabaseFile(this._signonsFile, backupFile);
        }

        this._dbClose();
        this._signonsFile.remove(false);
    }

}; 

let component = [LoginManagerStorage_mozStorage];
this.NSGetFactory = XPCOMUtils.generateNSGetFactory(component);
