




































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function LoginManagerStorage_legacy() { };

LoginManagerStorage_legacy.prototype = {

    classDescription  : "LoginManagerStorage_legacy",
    contractID : "@mozilla.org/login-manager/storage/legacy;1",
    classID : Components.ID("{e09e4ca6-276b-4bb4-8b71-0635a3a2a007}"),
    QueryInterface : XPCOMUtils.generateQI([Ci.nsILoginManagerStorage,
                                    Ci.nsILoginManagerIEMigrationHelper]),

    __logService : null, 
    get _logService() {
        if (!this.__logService)
            this.__logService = Cc["@mozilla.org/consoleservice;1"].
                                getService(Ci.nsIConsoleService);
        return this.__logService;
    },

    __ioService: null, 
    get _ioService() {
        if (!this.__ioService)
            this.__ioService = Cc["@mozilla.org/network/io-service;1"].
                               getService(Ci.nsIIOService);
        return this.__ioService;
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
        if (!this.__profileDir) {
            var dirService = Cc["@mozilla.org/file/directory_service;1"].
                             getService(Ci.nsIProperties);
            this.__profileDir = dirService.get("ProfD", Ci.nsIFile);
        }
        return this.__profileDir;
    },

    __nsLoginInfo: null,  
    get _nsLoginInfo() {
        if (!this.__nsLoginInfo)
            this.__nsLoginInfo = new Components.Constructor(
                "@mozilla.org/login-manager/loginInfo;1", Ci.nsILoginInfo);
        return this.__nsLoginInfo;
    },

    _prefBranch : null,  

    _signonsFile : null,  
    _debug       : false, 

    




    _filenamePrefs : ["SignonFileName3", "SignonFileName2", "SignonFileName"],

    





    _logins        : null, 
    _disabledHosts : null,


    




    log : function (message) {
        if (!this._debug)
            return;
        dump("PwMgr Storage: " + message + "\n");
        this._logService.logStringMessage("PwMgr Storage: " + message);
    },




    




    initWithFile : function(aInputFile, aOutputFile) {
        this._signonsFile = aInputFile;

        this.init();

        if (aOutputFile) {
            this._signonsFile = aOutputFile;
            this._writeFile();
        }
    },

    




    init : function () {
        this._logins  = {};
        this._disabledHosts = {};

        
        this._prefBranch = Cc["@mozilla.org/preferences-service;1"].
                           getService(Ci.nsIPrefService);
        this._prefBranch = this._prefBranch.getBranch("signon.");
        this._prefBranch.QueryInterface(Ci.nsIPrefBranch2);

        this._debug = this._prefBranch.getBoolPref("debug");

        
        
        var tokenDB = Cc["@mozilla.org/security/pk11tokendb;1"].
                      getService(Ci.nsIPK11TokenDB);

        var token = tokenDB.getInternalKeyToken();
        if (token.needsUserInit) {
            this.log("Initializing key3.db with default blank password.");
            token.initPassword("");
        }

        var importFile = null;
        
        if (!this._signonsFile)
            [this._signonsFile, importFile] = this._getSignonsFile();

        
        if (importFile) {
            this.log("Importing " + importFile.path);

            var tmp = this._signonsFile;
            this._signonsFile = importFile;
        }

        
        this._readFile();

        
        if (importFile) {
            this._signonsFile = tmp;
            this._writeFile();
        }
    },


    



    addLogin : function (login) {
        
        this._checkLoginValues(login);

        
        
        
        
        var clone = new this._nsLoginInfo();
        clone.init(login.hostname, login.formSubmitURL, login.httpRealm,
                   login.username,      login.password,
                   login.usernameField, login.passwordField);
        login = clone;

        var key = login.hostname;

        
        var rollback;
        if (!this._logins[key]) {
            this._logins[key] = [];
            rollback = null;
        } else {
            rollback = this._logins[key].concat(); 
        }

        this._logins[key].push(login);

        var ok = this._writeFile();

        
        if (!ok) {
            if (rollback)
                this._logins[key] = rollback;
            else
                delete this._logins[key];

            throw "Couldn't write to file, login not added.";
        }
    },


    



    removeLogin : function (login) {
        var key = login.hostname;
        var logins = this._logins[key];

        if (!logins)
            throw "No logins found for hostname (" + key + ")";

        var rollback = this._logins[key].concat(); 

        
        
        
        
        for (var i = 0; i < logins.length; i++) {

            var [[decryptedLogin], userCanceled] =
                        this._decryptLogins([logins[i]]);

            if (userCanceled)
                throw "User canceled master password entry, login not removed.";

            if (!decryptedLogin)
                continue;

            if (decryptedLogin.equals(login)) {
                logins.splice(i, 1); 
                break;
                
                
            }
        }

        
        if (logins.length == 0)
            delete this._logins[key];

        var ok = this._writeFile();

        
        if (!ok) {
            this._logins[key] = rollback;
            throw "Couldn't write to file, login not removed.";
        }
    },


    



    modifyLogin : function (oldLogin, newLogin) {
        if (newLogin instanceof Ci.nsIPropertyBag)
            throw "legacy modifyLogin with propertybag not implemented.";
        newLogin.QueryInterface(Ci.nsILoginInfo);
        
        this._checkLoginValues(newLogin);

        this.removeLogin(oldLogin);
        this.addLogin(newLogin);
    },


    




    getAllLogins : function (count) {
        var result = [], userCanceled;

        
        for each (var hostLogins in this._logins) {
            result = result.concat(hostLogins);
        }

        
        [result, userCanceled] = this._decryptLogins(result);

        if (userCanceled)
            throw "User canceled Master Password entry";

        if (count)
            count.value = result.length; 
        return result;
    },


    




    getAllEncryptedLogins : function (count) {
        var result = [];

        
        for each (var hostLogins in this._logins) {
            
            
            for each (var login in hostLogins) {
                var clone = new this._nsLoginInfo();
                clone.init(login.hostname, login.formSubmitURL, login.httpRealm,
                           login.wrappedJSObject.encryptedUsername,
                           login.wrappedJSObject.encryptedPassword,
                           login.usernameField, login.passwordField);
                result.push(clone);
            }
        }

        if (count)
            count.value = result.length; 
        return result;
    },


    






    searchLogins : function (count, matchData) {
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
    },


    




    removeAllLogins : function () {
        
        this._removeOldSignonsFiles();

        
        this._logins = {};
        this._writeFile();
    },


    



    getAllDisabledHosts : function (count) {
        var result = [];

        for (var hostname in this._disabledHosts) {
            result.push(hostname);
        }

        if (count)
            count.value = result.length; 
        return result;
    },


    



    getLoginSavingEnabled : function (hostname) {
        return !this._disabledHosts[hostname];
    },


    



    setLoginSavingEnabled : function (hostname, enabled) {
        
        
        if (hostname == "." ||
            hostname.indexOf("\r") != -1 ||
            hostname.indexOf("\n") != -1 ||
            hostname.indexOf("\0") != -1)
            throw "Invalid hostname";

        if (enabled)
            delete this._disabledHosts[hostname];
        else
            this._disabledHosts[hostname] = true;

        this._writeFile();
    },


    



    findLogins : function (count, hostname, formSubmitURL, httpRealm) {
        var userCanceled;

        var logins = this._searchLogins(hostname, formSubmitURL, httpRealm);

        
        [logins, userCanceled] = this._decryptLogins(logins);

        
        
        
        if (userCanceled)
            throw "User canceled Master Password entry";

        count.value = logins.length; 
        return logins;
    },

    
    



    countLogins : function (aHostname, aFormSubmitURL, aHttpRealm) {
        var logins;

        
        if (aHostname) {
            logins = this._searchLogins(aHostname, aFormSubmitURL, aHttpRealm);
            return logins.length
        } 

        
        if (aHostname == null)
            return 0;

        
        var count = 0;
        for (var hostname in this._logins) {
            logins = this._searchLogins(hostname, aFormSubmitURL, aHttpRealm);
            count += logins.length;
        }

        return count;
    },




    




    



    _searchLogins : function (hostname, formSubmitURL, httpRealm) {
        var hostLogins = this._logins[hostname];
        if (hostLogins == null)
            return [];

        var result = [], userCanceled;

        for each (var login in hostLogins) {

            
            
            
            if (httpRealm == null) {
                if (login.httpRealm != null)
                    continue;
            } else if (httpRealm != "") {
                
                
                if (httpRealm != login.httpRealm)
                    continue;
            }

            
            
            
            if (formSubmitURL == null) {
                if (login.formSubmitURL != null)
                    continue;
            } else if (formSubmitURL != "") {
                
                
                
                if (login.formSubmitURL != "" &&
                    formSubmitURL != login.formSubmitURL)
                    continue;
            }

            result.push(login);
        }

        return result;
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


    






    _getSignonsFile : function() {
        var destFile = null, importFile = null;

        
        
        
        for (var i = 0; i < this._filenamePrefs.length; i++) {
            var prefname = this._filenamePrefs[i];
            var filename = this._prefBranch.getCharPref(prefname);
            var file = this._profileDir.clone();
            file.append(filename);

            this.log("Checking file " + filename + " (" + prefname + ")");

            
            if (!destFile)
                destFile = file;
            else
                importFile = file;

            if (file.exists())
                return [destFile, importFile];
        }

        
        return [destFile, null];
    },


    




    _removeOldSignonsFiles : function() {
        
        
        for (var i = 1; i < this._filenamePrefs.length; i++) {
            var prefname = this._filenamePrefs[i];
            var filename = this._prefBranch.getCharPref(prefname);
            var file = this._profileDir.clone();
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


    






    _upgrade_entry_to_2E : function (aLogin) {
        var upgradedLogins = [aLogin];

        













        if (aLogin.hostname.indexOf("://") == -1) {
            var oldHost = aLogin.hostname;

            
            
            
            if (!/:\d+$/.test(aLogin.hostname)) {
                this.log("2E upgrade: no port, skipping " + aLogin.hostname);
                return upgradedLogins;
            }

            
            try {
                
                
                
                var uri = this._ioService.newURI("http://" + aLogin.hostname,
                                                 null, null);
                var host = uri.host;
                var port = uri.port;
            } catch (e) {
                this.log("2E upgrade: Can't parse hostname " + aLogin.hostname);
                return upgradedLogins;
            }

            if (port == 80 || port == -1)
                aLogin.hostname = "http://" + host;
            else if (port == 443)
                aLogin.hostname = "https://" + host;
            else {
                
                
                
                
                this.log("2E upgrade: Cloning login for " + aLogin.hostname);

                aLogin.hostname = "http://" + host + ":" + port;

                var extraLogin = new this._nsLoginInfo();
                extraLogin.init("https://" + host + ":" + port,
                                null, aLogin.httpRealm,
                                aLogin.username, aLogin.password, "", "");
                
                
                extraLogin.wrappedJSObject.encryptedPassword = 
                    aLogin.wrappedJSObject.encryptedPassword;
                extraLogin.wrappedJSObject.encryptedUsername = 
                    aLogin.wrappedJSObject.encryptedUsername;

                if (extraLogin.httpRealm == "")
                    extraLogin.httpRealm = extraLogin.hostname;
                
                upgradedLogins.push(extraLogin);
            }

            
            
            if (aLogin.httpRealm == "")
                aLogin.httpRealm = aLogin.hostname;

            this.log("2E upgrade: " + oldHost + " ---> " + aLogin.hostname);

            return upgradedLogins;
        }


        














        
        var ioService = this._ioService;
        var log = this.log;

        function cleanupURL(aURL, allowJS) {
            var newURL, username = null, pathname = "";

            try {
                var uri = ioService.newURI(aURL, null, null);
                var scheme = uri.scheme;

                if (allowJS && scheme == "javascript")
                    return ["javascript:", null, ""];

                newURL = scheme + "://" + uri.host;

                
                
                port = uri.port;
                if (port != -1) {
                    var handler = ioService.getProtocolHandler(scheme);
                    if (port != handler.defaultPort)
                        newURL += ":" + port;
                }

                
                if (scheme != "http" && scheme != "https" && uri.username)
                    username = uri.username;

                if (uri.path != "/")
                    pathname = uri.path;

            } catch (e) {
                log("Can't cleanup URL: " + aURL + " e: " + e);
                newURL = aURL;
            }

            if (newURL != aURL)
                log("2E upgrade: " + aURL + " ---> " + newURL);

            return [newURL, username, pathname];
        }

        const isMailNews = /^(ldaps?|smtp|imap|news|mailbox):\/\//;

        
        
        var isFormLogin = (aLogin.formSubmitURL ||
                           aLogin.usernameField ||
                           aLogin.passwordField) &&
                          !isMailNews.test(aLogin.hostname);

        var [hostname, username, pathname] = cleanupURL(aLogin.hostname);
        aLogin.hostname = hostname;

        
        
        
        if (username && !isFormLogin) {
            if (isMailNews.test(aLogin.hostname))
                username = decodeURIComponent(username);

            var [encUsername, userCanceled] = this._encrypt(username);
            if (!userCanceled)
                aLogin.wrappedJSObject.encryptedUsername = encUsername;
        }


        if (aLogin.formSubmitURL) {
            [hostname, username, pathname] = cleanupURL(aLogin.formSubmitURL,
                                                        true);
            aLogin.formSubmitURL = hostname;
            
        }


        










        const isHTTP = /^https?:\/\//;
        const isLDAP = /^ldaps?:\/\//;
        const isNews = /^news?:\/\//;
        if (!isHTTP.test(aLogin.hostname) && !isFormLogin) {
            
            if (isLDAP.test(aLogin.hostname) ||
                isNews.test(aLogin.hostname))
                aLogin.httpRealm = aLogin.hostname + pathname;
            else
                aLogin.httpRealm = aLogin.hostname;

            aLogin.formSubmitURL = null;

            
            
            if (isMailNews.test(aLogin.hostname)) {
                aLogin.usernameField = "";
                aLogin.passwordField = "";
            }

            this.log("2E upgrade: set empty realm to " + aLogin.httpRealm);
        }

        return upgradedLogins;
    },


    



    _readFile : function () {
        var formatVersion;

        this.log("Reading passwords from " + this._signonsFile.path);

        
        if (!this._signonsFile.exists()) {
            this.log("No existing signons file found.");
            return;
        }

        var inputStream = Cc["@mozilla.org/network/file-input-stream;1"].
                          createInstance(Ci.nsIFileInputStream);
        
        inputStream.init(this._signonsFile, 0x01, -1, null);
        var lineStream = inputStream.QueryInterface(Ci.nsILineInputStream);
        var line = { value: "" };

        const STATE = { HEADER : 0, REJECT : 1, REALM : 2,
                        USERFIELD : 3, USERVALUE : 4,
                        PASSFIELD : 5, PASSVALUE : 6, ACTIONURL : 7,
                        FILLER : 8 };
        var parseState = STATE.HEADER;

        var processEntry = false;
        var discardEntry = false;

        do {
            var hasMore = lineStream.readLine(line);
            try {
              line.value = this._utfConverter.ConvertToUnicode(line.value);
            } catch (e) {
              this.log("Bad UTF8 conversion: " + line.value);
              this._utfConverterReset();
            }

            switch (parseState) {
                
                case STATE.HEADER:
                    if (line.value == "#2c") {
                        formatVersion = 0x2c;
                    } else if (line.value == "#2d") {
                        formatVersion = 0x2d;
                    } else if (line.value == "#2e") {
                        formatVersion = 0x2e;
                    } else {
                        this.log("invalid file header (" + line.value + ")");
                        throw "invalid file header in signons file";
                        
                        
                        
                        return;
                    }
                    parseState++;
                    break;

                
                case STATE.REJECT:
                    if (line.value == ".") {
                        parseState++;
                        break;
                    }

                    this._disabledHosts[line.value] = true;

                    break;

                
                case STATE.REALM:
                    var hostrealm = line.value;

                    
                    
                    const realmFormat = /^(.+?)( \(.*\))?$/;
                    var matches = realmFormat.exec(hostrealm);
                    var hostname, httpRealm;
                    if (matches && matches.length == 3) {
                        hostname  = matches[1];
                        httpRealm = matches[2] ?
                                        matches[2].slice(2, -1) : null;
                    } else {
                        if (hostrealm != "") {
                            
                            this.log("Error parsing host/realm: " + hostrealm);
                        }
                        hostname = hostrealm;
                        httpRealm = null;
                    }

                    parseState++;
                    break;

                
                
                case STATE.USERFIELD:
                    if (line.value == ".") {
                        discardEntry = false;
                        parseState = STATE.REALM;
                        break;
                    }

                    
                    
                    if (discardEntry)
                        break;

                    var entry = new this._nsLoginInfo();
                    entry.hostname  = hostname;
                    entry.httpRealm = httpRealm;

                    entry.usernameField = line.value;
                    parseState++;
                    break;

                
                case STATE.USERVALUE:
                    entry.wrappedJSObject.encryptedUsername = line.value;
                    parseState++;
                    break;

                
                
                case STATE.PASSFIELD:
                    if (line.value.charAt(0) != '*') {
                        discardEntry = true;
                        entry = null;
                        parseState = STATE.USERFIELD;
                        break;
                    }
                    entry.passwordField = line.value.substr(1);
                    parseState++;
                    break;

                
                case STATE.PASSVALUE:
                    entry.wrappedJSObject.encryptedPassword = line.value;

                    
                    
                    if (formatVersion < 0x2d)
                        processEntry = true;

                    parseState++;
                    break;

                
                case STATE.ACTIONURL:
                    var formSubmitURL = line.value;
                    if (!formSubmitURL && entry.httpRealm != null)
                        entry.formSubmitURL = null;
                    else
                        entry.formSubmitURL = formSubmitURL;

                    
                    
                    if (formatVersion < 0x2e)
                        processEntry = true;

                    parseState++;
                    break;

                
                case STATE.FILLER:
                    
                    
                    entry.wrappedJSObject.filler = line.value;
                    processEntry = true;

                    parseState++;
                    break;
            }

            
            
            if (processEntry) {
                if (formatVersion < 0x2d) {
                    
                    if (entry.httpRealm != null)
                        entry.formSubmitURL = null;
                    else
                        entry.formSubmitURL = "";
                }

                
                
                var entries = [entry];
                if (formatVersion < 0x2e)
                    entries = this._upgrade_entry_to_2E(entry);


                for each (var e in entries) {
                    if (!this._logins[e.hostname])
                        this._logins[e.hostname] = [];
                    this._logins[e.hostname].push(e);
                }

                entry = null;
                processEntry = false;
                parseState = STATE.USERFIELD;
            }
        } while (hasMore);

        lineStream.close();

        return;
    },


    






    _writeFile : function () {
        var converter = this._utfConverter;
        function writeLine(data) {
            data = converter.ConvertFromUnicode(data);
            data += converter.Finish();
            data += "\r\n";
            outputStream.write(data, data.length);
        }

        this.log("Writing passwords to " + this._signonsFile.path);

        var safeStream = Cc["@mozilla.org/network/safe-file-output-stream;1"].
                         createInstance(Ci.nsIFileOutputStream);
        
        safeStream.init(this._signonsFile, 0x02 | 0x08 | 0x20, 0600, null);

        var outputStream = Cc["@mozilla.org/network/buffered-output-stream;1"].
                           createInstance(Ci.nsIBufferedOutputStream);
        outputStream.init(safeStream, 8192);
        outputStream.QueryInterface(Ci.nsISafeOutputStream); 


        
        writeLine("#2e");

        
        for (var hostname in this._disabledHosts) {
            writeLine(hostname);
        }

        
        writeLine(".");

        for (var hostname in this._logins) {
            function sortByRealm(a,b) {
                a = a.httpRealm;
                b = b.httpRealm;

                if (!a && !b)
                    return  0;

                if (!a || a < b)
                    return -1;

                if (!b || b > a)
                    return  1;

                return 0; 
            }

            
            
            this._logins[hostname].sort(sortByRealm);


            
            var lastRealm = null;
            var firstEntry = true;
            var userCanceled = false;
            for each (var login in this._logins[hostname]) {

                
                if (login.httpRealm != lastRealm || firstEntry) {
                    
                    if (!firstEntry)
                        writeLine(".");

                    var hostrealm = login.hostname;
                    if (login.httpRealm)
                        hostrealm += " (" + login.httpRealm + ")";

                    writeLine(hostrealm);
                }

                firstEntry = false;

                
                
                var encUsername = login.wrappedJSObject.encryptedUsername;
                if (!encUsername) {
                    [encUsername, userCanceled] = this._encrypt(login.username);
                    login.wrappedJSObject.encryptedUsername = encUsername;
                }

                if (userCanceled)
                    break;

                
                
                var encPassword = login.wrappedJSObject.encryptedPassword;
                if (!encPassword) {
                    [encPassword, userCanceled] = this._encrypt(login.password);
                    login.wrappedJSObject.encryptedPassword = encPassword;
                }

                if (userCanceled)
                    break;


                writeLine((login.usernameField ?  login.usernameField : ""));
                writeLine(encUsername);
                writeLine("*" +
                    (login.passwordField ?  login.passwordField : ""));
                writeLine(encPassword);
                writeLine((login.formSubmitURL ? login.formSubmitURL : ""));
                if (login.wrappedJSObject.filler)
                    writeLine(login.wrappedJSObject.filler);
                else
                    writeLine("---");

                lastRealm = login.httpRealm;
            }

            if (userCanceled) {
                this.log("User canceled Master Password, aborting write.");
                
                outputStream.close();
                return false;
            }

            
            writeLine(".");
        }

        

        outputStream.finish();
        return true;
    },


    














    _decryptLogins : function (logins) {
        var result = [], userCanceled = false;

        for each (var login in logins) {
            var decryptedUsername, decryptedPassword;

            [decryptedUsername, userCanceled] =
                this._decrypt(login.wrappedJSObject.encryptedUsername);

            if (userCanceled)
                break;

            [decryptedPassword, userCanceled] =
                this._decrypt(login.wrappedJSObject.encryptedPassword);

            
            if (userCanceled)
                break;

            
            
            if (decryptedUsername == null || !decryptedPassword)
                continue;

            
            
            
            var clone = new this._nsLoginInfo();
            clone.init(login.hostname, login.formSubmitURL, login.httpRealm,
                       decryptedUsername, decryptedPassword,
                       login.usernameField, login.passwordField);

            
            
            var recrypted;
            if (login.wrappedJSObject.encryptedUsername &&
                login.wrappedJSObject.encryptedUsername.charAt(0) == '~') {
                  [recrypted, userCanceled] = this._encrypt(decryptedUsername);

                  if (userCanceled)
                    break;

                  login.wrappedJSObject.encryptedUsername = recrypted;
            }

            if (login.wrappedJSObject.encryptedPassword &&
                login.wrappedJSObject.encryptedPassword.charAt(0) == '~') {
                  [recrypted, userCanceled] = this._encrypt(decryptedPassword);

                  if (userCanceled)
                    break;

                  login.wrappedJSObject.encryptedPassword = recrypted;
            }

            result.push(clone);
        }

        return [result, userCanceled];
    },


    













    _encrypt : function (plainText) {
        var cipherText = null, userCanceled = false;

        try {
            var plainOctet = this._utfConverter.ConvertFromUnicode(plainText);
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
        var plainText = null, userCanceled = false;

        try {
            var plainOctet;
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




    




    _migrationLoginManager : null,

    














































    migrateAndAddLogin : function (aLogin) {
        
        if (!this._migrationLoginManager) {
            
            this._prefBranch = Cc["@mozilla.org/preferences-service;1"].
                               getService(Ci.nsIPrefService);
            this._prefBranch = this._prefBranch.getBranch("signon.");
            this._prefBranch.QueryInterface(Ci.nsIPrefBranch2);

            this._debug = this._prefBranch.getBoolPref("debug");

            this._migrationLoginManager = Cc["@mozilla.org/login-manager;1"].
                                          getService(Ci.nsILoginManager);
        }

        this.log("Migrating login for " + aLogin.hostname);

        
        
        var logins = this._upgrade_entry_to_2E(aLogin);

        
        
        for each (var login in logins)
            this._migrationLoginManager.addLogin(login);
    }
}; 

var component = [LoginManagerStorage_legacy];
function NSGetModule(compMgr, fileSpec) {
    return XPCOMUtils.generateModule(component);
}
