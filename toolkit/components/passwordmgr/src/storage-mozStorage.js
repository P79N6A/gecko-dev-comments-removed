







































const Cc = Components.classes;
const Ci = Components.interfaces;

const DB_VERSION = 3; 

const ENCTYPE_BASE64 = 0;
const ENCTYPE_SDR = 1;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function LoginManagerStorage_mozStorage() { };

LoginManagerStorage_mozStorage.prototype = {

    classDescription  : "LoginManagerStorage_mozStorage",
    contractID : "@mozilla.org/login-manager/storage/mozStorage;1",
    classID : Components.ID("{8c2023b9-175c-477e-9761-44ae7b549756}"),
    QueryInterface : XPCOMUtils.generateQI([Ci.nsILoginManagerStorage]),

    __logService : null, 
    get _logService() {
        if (!this.__logService)
            this.__logService = Cc["@mozilla.org/consoleservice;1"].
                                getService(Ci.nsIConsoleService);
        return this.__logService;
    },

    __decoderRing : null,  
    get _decoderRing() {
        if (!this.__decoderRing)
            this.__decoderRing = Cc["@mozilla.org/security/sdr;1"].
                                 getService(Ci.nsISecretDecoderRing);
        return this.__decoderRing;
    },

    __utfConverter : null, 
    get _utfConverter() {
        if (!this.__utfConverter) {
            this.__utfConverter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                                  createInstance(Ci.nsIScriptableUnicodeConverter);
            this.__utfConverter.charset = "UTF-8";
        }
        return this.__utfConverter;
    },

    _utfConverterReset : function() {
        this.__utfConverter = null;
    },

    __profileDir: null,  
    get _profileDir() {
        if (!this.__profileDir)
            this.__profileDir = Cc["@mozilla.org/file/directory_service;1"].
                                getService(Ci.nsIProperties).
                                get("ProfD", Ci.nsIFile);
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

    __observerService : null,
    get _observerService() {
        if (!this.__observerService)
            this.__observerService = Cc["@mozilla.org/observer-service;1"].
                                     getService(Ci.nsIObserverService);
        return this.__observerService;
    },


    
    _dbSchema: {
        tables: {
            moz_logins:         "id                 INTEGER PRIMARY KEY," +
                                "hostname           TEXT NOT NULL,"       +
                                "httpRealm          TEXT,"                +
                                "formSubmitURL      TEXT,"                +
                                "usernameField      TEXT NOT NULL,"       +
                                "passwordField      TEXT NOT NULL,"       +
                                "encryptedUsername  TEXT NOT NULL,"       +
                                "encryptedPassword  TEXT NOT NULL,"       +
                                "guid               TEXT,"                +
                                "encType            INTEGER",
            
            
            moz_disabledHosts:  "id                 INTEGER PRIMARY KEY," +
                                "hostname           TEXT UNIQUE ON CONFLICT REPLACE",
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
    _importFile   : null,  
    _debug        : false, 
    _base64checked : false,


    




    log : function (message) {
        if (!this._debug)
            return;
        dump("PwMgr mozStorage: " + message + "\n");
        this._logService.logStringMessage("PwMgr mozStorage: " + message);
    },


    






    initWithFile : function(aImportFile, aDBFile) {
        if (aImportFile)
            this._importFile = aImportFile;
        if (aDBFile)
            this._signonsFile = aDBFile;

        this.init();
    },


    





    init : function () {
        this._dbStmts = [];

        
        this._prefBranch = Cc["@mozilla.org/preferences-service;1"].
                           getService(Ci.nsIPrefService);
        this._prefBranch = this._prefBranch.getBranch("signon.");
        this._prefBranch.QueryInterface(Ci.nsIPrefBranch2);

        this._debug = this._prefBranch.getBoolPref("debug");

        
        
        let tokenDB = Cc["@mozilla.org/security/pk11tokendb;1"].
                      getService(Ci.nsIPK11TokenDB);

        let token = tokenDB.getInternalKeyToken();
        if (token.needsUserInit) {
            this.log("Initializing key3.db with default blank password.");
            token.initPassword("");
        }

        let isFirstRun;
        try {
            
            if (!this._signonsFile) {
                
                this._signonsFile = this._profileDir.clone();
                this._signonsFile.append("signons.sqlite");
            }
            this.log("Opening database at " + this._signonsFile.path);

            
            isFirstRun = this._dbInit();

            
            
            if (isFirstRun && !this._importFile)
                this._importLegacySignons();
            else if (this._importFile)
                this._importLegacySignons(this._importFile);

            this._initialized = true;
        } catch (e) {
            this.log("Initialization failed: " + e);
            
            if (isFirstRun && e == "Import failed")
                this._dbCleanup(false);
            throw "Initialization failed";
        }
    },


    



    addLogin : function (login) {
        this._addLogin(login, false);
    },


    




    _addLogin : function (login, isEncrypted) {
        let userCanceled, encUsername, encPassword;

        
        this._checkLoginValues(login);

        if (isEncrypted) {
            [encUsername, encPassword] = [login.username, login.password];
        } else {
            
            [encUsername, encPassword, userCanceled] = this._encryptLogin(login);
            if (userCanceled)
                throw "User canceled master password entry, login not added.";
        }

        
        let loginClone = login.clone();

        
        loginClone.QueryInterface(Ci.nsILoginMetaInfo);
        if (loginClone.guid) {
            if (!this._isGuidUnique(loginClone.guid))
                throw "specified GUID already exists";
        } else {
            loginClone.guid = this._uuidService.generateUUID().toString();
        }

        
        let encType = ENCTYPE_SDR;
        if (isEncrypted &&
            (encUsername.charAt(0) == '~' || encPassword.charAt(0) == '~'))
            encType = ENCTYPE_BASE64;

        let query =
            "INSERT INTO moz_logins " +
            "(hostname, httpRealm, formSubmitURL, usernameField, " +
             "passwordField, encryptedUsername, encryptedPassword, " +
             "guid, encType) " +
            "VALUES (:hostname, :httpRealm, :formSubmitURL, :usernameField, " +
                    ":passwordField, :encryptedUsername, :encryptedPassword, " +
                    ":guid, :encType)";

        let params = {
            hostname:          loginClone.hostname,
            httpRealm:         loginClone.httpRealm,
            formSubmitURL:     loginClone.formSubmitURL,
            usernameField:     loginClone.usernameField,
            passwordField:     loginClone.passwordField,
            encryptedUsername: encUsername,
            encryptedPassword: encPassword,
            guid:              loginClone.guid,
            encType:           encType
        };

        let stmt;
        try {
            stmt = this._dbCreateStatement(query, params);
            stmt.execute();
        } catch (e) {
            this.log("_addLogin failed: " + e.name + " : " + e.message);
            throw "Couldn't write to database, login not added.";
        } finally {
            stmt.reset();
        }

        
        if (!isEncrypted)
            this._sendNotification("addLogin", loginClone);
    },


    



    removeLogin : function (login) {
        let [idToDelete, storedLogin] = this._getIdForLogin(login);
        if (!idToDelete)
            throw "No matching logins";

        
        let query  = "DELETE FROM moz_logins WHERE id = :id";
        let params = { id: idToDelete };
        let stmt;
        try {
            stmt = this._dbCreateStatement(query, params);
            stmt.execute();
        } catch (e) {
            this.log("_removeLogin failed: " + e.name + " : " + e.message);
            throw "Couldn't write to database, login not removed.";
        } finally {
            stmt.reset();
        }

        this._sendNotification("removeLogin", storedLogin);
    },


    



    modifyLogin : function (oldLogin, newLoginData) {
        let [idToModify, oldStoredLogin] = this._getIdForLogin(oldLogin);
        if (!idToModify)
            throw "No matching logins";
        oldStoredLogin.QueryInterface(Ci.nsILoginMetaInfo);

        let newLogin;
        if (newLoginData instanceof Ci.nsILoginInfo) {
            
            
            newLogin = oldStoredLogin.clone();
            newLogin.init(newLoginData.hostname,
                          newLoginData.formSubmitURL, newLoginData.httpRealm,
                          newLoginData.username, newLoginData.password,
                          newLoginData.usernameField, newLoginData.passwordField);
            newLogin.QueryInterface(Ci.nsILoginMetaInfo);
        } else if (newLoginData instanceof Ci.nsIPropertyBag) {
            
            newLogin = oldStoredLogin.clone();
            newLogin.QueryInterface(Ci.nsILoginMetaInfo);

            let propEnum = newLoginData.enumerator;
            while (propEnum.hasMoreElements()) {
                let prop = propEnum.getNext().QueryInterface(Ci.nsIProperty);
                switch (prop.name) {
                    
                    case "hostname":
                    case "httpRealm":
                    case "formSubmitURL":
                    case "username":
                    case "password":
                    case "usernameField":
                    case "passwordField":
                        newLogin[prop.name] = prop.value;
                        break;

                    
                    case "guid":
                        newLogin.guid = prop.value;
                        if (!this._isGuidUnique(newLogin.guid))
                            throw "specified GUID already exists";
                        break;

                    
                    default:
                        throw "Unexpected propertybag item: " + prop.name;
                }
            }
        } else {
            throw "newLoginData needs an expected interface!";
        }

        
        this._checkLoginValues(newLogin);

        
        let [encUsername, encPassword, userCanceled] = this._encryptLogin(newLogin);
        if (userCanceled)
            throw "User canceled master password entry, login not modified.";

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
                "encType = :encType " +
            "WHERE id = :id";

        let params = {
            id:                idToModify,
            hostname:          newLogin.hostname,
            httpRealm:         newLogin.httpRealm,
            formSubmitURL:     newLogin.formSubmitURL,
            usernameField:     newLogin.usernameField,
            passwordField:     newLogin.passwordField,
            encryptedUsername: encUsername,
            encryptedPassword: encPassword,
            guid:              newLogin.guid,
            encType:           ENCTYPE_SDR
        };

        let stmt;
        try {
            stmt = this._dbCreateStatement(query, params);
            stmt.execute();
        } catch (e) {
            this.log("modifyLogin failed: " + e.name + " : " + e.message);
            throw "Couldn't write to database, login not modified.";
        } finally {
            stmt.reset();
        }

        this._sendNotification("modifyLogin", [oldStoredLogin, newLogin]);
    },


    




    getAllLogins : function (count) {
        let userCanceled;
        let [logins, ids] = this._searchLogins({});

        
        [logins, userCanceled] = this._decryptLogins(logins);

        if (userCanceled)
            throw "User canceled Master Password entry";

        this.log("_getAllLogins: returning " + logins.length + " logins.");
        count.value = logins.length; 
        return logins;
    },


    







    getAllEncryptedLogins : function (count) {
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
    },


    







    searchLogins : function(count, matchData) {
        let realMatchData = {};
        
        let propEnum = matchData.enumerator;
        while (propEnum.hasMoreElements()) {
            let prop = propEnum.getNext().QueryInterface(Ci.nsIProperty);
            realMatchData[prop.name] = prop.value;
        }

        let [logins, ids] = this._searchLogins(realMatchData);

        let userCanceled;
        
        [logins, userCanceled] = this._decryptLogins(logins);

        if (userCanceled)
        throw "User canceled Master Password entry";

        count.value = logins.length; 
        return logins;
    },


    









    _searchLogins : function (matchData) {
        let conditions = [], params = {};

        for (field in matchData) {
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
                logins.push(login);
                ids.push(stmt.row.id);
            }
        } catch (e) {
            this.log("_searchLogins failed: " + e.name + " : " + e.message);
        } finally {
            stmt.reset();
        }

        this.log("_searchLogins: returning " + logins.length + " logins");
        return [logins, ids];
    },


    




    removeAllLogins : function () {
        this.log("Removing all logins");
        
        this._removeOldSignonsFiles();

        
        let query = "DELETE FROM moz_logins";
        let stmt;
        try {
            stmt = this._dbCreateStatement(query);
            stmt.execute();
        } catch (e) {
            this.log("_removeAllLogins failed: " + e.name + " : " + e.message);
            throw "Couldn't write to database";
        } finally {
            stmt.reset();
        }

        this._sendNotification("removeAllLogins", null);
    },


    



    getAllDisabledHosts : function (count) {
        let disabledHosts = this._queryDisabledHosts(null);

        this.log("_getAllDisabledHosts: returning " + disabledHosts.length + " disabled hosts.");
        count.value = disabledHosts.length; 
        return disabledHosts;
    },


    



    getLoginSavingEnabled : function (hostname) {
        this.log("Getting login saving is enabled for " + hostname);
        return this._queryDisabledHosts(hostname).length == 0
    },


    



    setLoginSavingEnabled : function (hostname, enabled) {
        
        this._checkHostnameValue(hostname);

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
            stmt.reset();
        }

        this._sendNotification(enabled ? "hostSavingEnabled" : "hostSavingDisabled", hostname);
    },


    



    findLogins : function (count, hostname, formSubmitURL, httpRealm) {
        let userCanceled;
        let loginData = {
            hostname: hostname,
            formSubmitURL: formSubmitURL,
            httpRealm: httpRealm
        };
        let matchData = { };
        for each (field in ["hostname", "formSubmitURL", "httpRealm"])
          if (loginData[field] != '')
              matchData[field] = loginData[field];
        let [logins, ids] = this._searchLogins(matchData);

        
        [logins, userCanceled] = this._decryptLogins(logins);

        
        
        
        if (userCanceled)
            throw "User canceled Master Password entry";

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
            stmt.reset();
        }

        this.log("_countLogins: counted logins: " + numLogins);
        return numLogins;
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
        this._observerService.notifyObservers(dataObject, "passwordmgr-storage-changed", changeType);
    },


    






    _getIdForLogin : function (login) {
        let matchData = { };
        for each (field in ["hostname", "formSubmitURL", "httpRealm"])
            if (login[field] != '')
                matchData[field] = login[field];
        let [logins, ids] = this._searchLogins(matchData);

        let id = null;
        let foundLogin = null;

        
        
        
        
        for (let i = 0; i < logins.length; i++) {
            let [[decryptedLogin], userCanceled] =
                        this._decryptLogins([logins[i]]);

            if (userCanceled)
                throw "User canceled master password entry.";

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
            stmt.reset();
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


    






    _checkLoginValues : function (aLogin) {
        function badCharacterPresent(l, c) {
            return ((l.formSubmitURL && l.formSubmitURL.indexOf(c) != -1) ||
                    (l.httpRealm     && l.httpRealm.indexOf(c)     != -1) ||
                                        l.hostname.indexOf(c)      != -1  ||
                                        l.usernameField.indexOf(c) != -1  ||
                                        l.passwordField.indexOf(c) != -1);
        }

        
        
        if (badCharacterPresent(aLogin, "\0"))
            throw "login values can't contain nulls";

        
        
        
        
        if (aLogin.username.indexOf("\0") != -1 ||
            aLogin.password.indexOf("\0") != -1)
            throw "login values can't contain nulls";

        
        if (badCharacterPresent(aLogin, "\r") ||
            badCharacterPresent(aLogin, "\n"))
            throw "login values can't contain newlines";

        
        if (aLogin.usernameField == "." ||
            aLogin.formSubmitURL == ".")
            throw "login values can't be periods";

        
        
        
        if (aLogin.hostname.indexOf(" (") != -1)
            throw "bad parens in hostname";
    },


    





    _checkHostnameValue : function (hostname) {
        
        
        if (hostname == "." ||
            hostname.indexOf("\r") != -1 ||
            hostname.indexOf("\n") != -1 ||
            hostname.indexOf("\0") != -1)
            throw "Invalid hostname";
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
            stmt.reset();
        }

        return (numLogins == 0);
    },


    






    _importLegacySignons : function (importFile) {
        this.log("Importing " + (importFile ? importFile.path : "legacy storage"));

        let legacy = Cc["@mozilla.org/login-manager/storage/legacy;1"].
                     createInstance(Ci.nsILoginManagerStorage);

        
        try {
            if (importFile)
                legacy.initWithFile(importFile, null);
            else
                legacy.init();

            
            let logins = legacy.getAllEncryptedLogins({});

            
            this._dbConnection.beginTransaction();
            for each (let login in logins) {
                try {
                    this._addLogin(login, true);
                } catch (e) {
                    this.log("_importLegacySignons failed to add login: " + e);
                }
            }
            let disabledHosts = legacy.getAllDisabledHosts({});
            for each (let hostname in disabledHosts)
                this.setLoginSavingEnabled(hostname, false);
            this._dbConnection.commitTransaction();
        } catch (e) {
            this.log("_importLegacySignons failed: " + e.name + " : " + e.message);
            throw "Import failed";
        }
    },


    




    _removeOldSignonsFiles : function () {
        
        
        
        filenamePrefs = ["SignonFileName3", "SignonFileName2", "SignonFileName"];
        for each (let prefname in filenamePrefs) {
            let filename = this._prefBranch.getCharPref(prefname);
            let file = this._profileDir.clone();
            file.append(filename);

            if (file.exists()) {
                this.log("Deleting old " + filename + " (" + prefname + ")");
                try {
                    file.remove(false);
                } catch (e) {
                    this.log("NOTICE: Couldn't delete " + filename + ": " + e);
                }
            }
        }
    },


    






    _encryptLogin : function (login) {
        let encUsername, encPassword, userCanceled;
        [encUsername, userCanceled] = this._encrypt(login.username);
        if (userCanceled)
            return [null, null, true];

        [encPassword, userCanceled] = this._encrypt(login.password);
        
        if (userCanceled)
            return [null, null, true];

        if (!this._base64checked)
            this._reencryptBase64Logins();

        return [encUsername, encPassword, false];
    },


    












    _decryptLogins : function (logins) {
        let result = [], userCanceled = false;

        for each (let login in logins) {
            let decryptedUsername, decryptedPassword;

            [decryptedUsername, userCanceled] = this._decrypt(login.username);

            if (userCanceled)
                break;

            [decryptedPassword, userCanceled] = this._decrypt(login.password);

            
            if (userCanceled)
                break;

            
            
            if (decryptedUsername == null || !decryptedPassword)
                continue;

            login.username = decryptedUsername;
            login.password = decryptedPassword;

            result.push(login);
        }

        if (!this._base64checked && !userCanceled)
            this._reencryptBase64Logins();

        return [result, userCanceled];
    },


    








    _reencryptBase64Logins : function () {
        this._base64checked = true;
        

        try {
            let [logins, ids] = this._searchLogins({ encType: 0 });

            if (!logins.length)
                return;

            let userCancelled;
            [logins, userCanceled] = this._decryptLogins(logins);
            if (userCanceled)
                return;

            for each (let login in logins)
                this.modifyLogin(login, login);
        } catch (e) {
            this.log("_reencryptBase64Logins caught error: " + e);
        }
    },


    













    _encrypt : function (plainText) {
        let cipherText = null, userCanceled = false;

        try {
            let plainOctet = this._utfConverter.ConvertFromUnicode(plainText);
            plainOctet += this._utfConverter.Finish();
            cipherText = this._decoderRing.encryptString(plainOctet);
        } catch (e) {
            this.log("Failed to encrypt string. (" + e.name + ")");
            
            
            if (e.result == Components.results.NS_ERROR_FAILURE)
                userCanceled = true;
        }

        return [cipherText, userCanceled];
    },


    













    _decrypt : function (cipherText) {
        let plainText = null, userCanceled = false;

        try {
            let plainOctet;
            if (cipherText.charAt(0) == '~') {
                
                
                
                plainOctet = atob(cipherText.substring(1));
            } else {
                plainOctet = this._decoderRing.decryptString(cipherText);
            }
            plainText = this._utfConverter.ConvertToUnicode(plainOctet);
        } catch (e) {
            this.log("Failed to decrypt string: " + cipherText +
                " (" + e.name + ")");

            
            this._utfConverterReset();

            
            
            
            
            if (e.result == Components.results.NS_ERROR_NOT_AVAILABLE)
                userCanceled = true;
        }

        return [plainText, userCanceled];
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
        } catch (e if e.result == Components.results.NS_ERROR_FILE_CORRUPTED) {
            
            
            this._dbCleanup(true);
            throw e;
        }
        return isFirstRun;
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
                                           Components.results.NS_ERROR_FILE_CORRUPTED);

            
            
            
            this._dbConnection.schemaVersion = DB_VERSION;
            return;
        }

        

        this._dbConnection.beginTransaction();

        try {
            for (let v = oldVersion + 1; v <= DB_VERSION; v++) {
                this.log("Upgrading to version " + v + "...");
                let migrateFunction = "_dbMigrateToVersion" + v;
                this[migrateFunction]();
            }
        } catch (e) {
            this.log("Migration failed: "  + e);
            this._dbConnection.rollbackTransaction();
            throw e;
        }

        this._dbConnection.schemaVersion = DB_VERSION;
        this._dbConnection.commitTransaction();
        this.log("DB migration completed.");
    },


    




    _dbMigrateToVersion2 : function () {
        
        let exists = true;
        try { 
            let stmt = this._dbConnection.createStatement(
                           "SELECT guid FROM moz_logins");
            
            stmt.finalize();
        } catch (e) {
            exists = false;
        }

        
        if (!exists) {
            this._dbConnection.executeSimpleSQL(
                "ALTER TABLE moz_logins ADD COLUMN guid TEXT");

            this._dbConnection.executeSimpleSQL(
                "CREATE INDEX IF NOT EXISTS " +
                    "moz_logins_guid_index ON moz_logins (guid)");
        }

        
        let ids = [];
        let query = "SELECT id FROM moz_logins WHERE guid isnull";
        let stmt;
        try {
            stmt = this._dbCreateStatement(query);
            while (stmt.executeStep())
                ids.push(stmt.row.id);
        } catch (e) {
            this.log("Failed getting IDs: " + e);
            throw e;
        } finally {
            stmt.reset();
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
                stmt.reset();
            }
        }
    },


    




    _dbMigrateToVersion3 : function () {
        
        let exists = true;
        let query = "SELECT encType FROM moz_logins";
        let stmt;
        try { 
            stmt = this._dbConnection.createStatement(query);
            
            stmt.finalize();
        } catch (e) {
            exists = false;
        }

        
        if (!exists) {
            query = "ALTER TABLE moz_logins ADD COLUMN encType INTEGER";
            this._dbConnection.executeSimpleSQL(query);

            query = "CREATE INDEX IF NOT EXISTS " +
                        "moz_logins_encType_index ON moz_logins (encType)";
            this._dbConnection.executeSimpleSQL(query);
        }

        
        let logins = [];
        query = "SELECT id, encryptedUsername, encryptedPassword " +
                    "FROM moz_logins WHERE encType isnull";
        try {
            stmt = this._dbCreateStatement(query);
            while (stmt.executeStep()) {
                let params = { id: stmt.row.id };
                if (stmt.row.encryptedUsername.charAt(0) == '~' ||
                    stmt.row.encryptedPassword.charAt(0) == '~')
                    params.encType = ENCTYPE_BASE64;
                else
                    params.encType = ENCTYPE_SDR;
                logins.push(params);
            }
        } catch (e) {
            this.log("Failed getting logins: " + e);
            throw e;
        } finally {
            stmt.reset();
        }

        
        query = "UPDATE moz_logins SET encType = :encType WHERE id = :id";
        for each (params in logins) {
            try {
                stmt = this._dbCreateStatement(query, params);
                stmt.execute();
            } catch (e) {
                this.log("Failed setting encType: " + e);
                throw e;
            } finally {
                stmt.reset();
            }
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
                       "encType " +
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


    





    _dbCleanup : function (backup) {
        this.log("Cleaning up DB file - close & remove & backup=" + backup)

        
        if (backup) {
            let backupFile = this._signonsFile.leafName + ".corrupt";
            this._storageService.backupDatabaseFile(this._signonsFile, backupFile);
        }

        
        for each (let stmt in this._dbStmts)
            stmt.finalize();
        this._dbStmts = [];

        
        try { this._dbConnection.close() } catch(e) {}
        this._signonsFile.remove(false);
    }

}; 

let component = [LoginManagerStorage_mozStorage];
function NSGetModule(compMgr, fileSpec) {
    return XPCOMUtils.generateModule(component);
}
