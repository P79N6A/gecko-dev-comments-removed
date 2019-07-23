




































const Cc = Components.classes;
const Ci = Components.interfaces;

function LoginManagerStorage_legacy() { };

LoginManagerStorage_legacy.prototype = {

    QueryInterface : function (iid) {
        const interfaces = [Ci.nsILoginManagerStorage, Ci.nsISupports];
        if (!interfaces.some( function(v) { return iid.equals(v) } ))
            throw Components.results.NS_ERROR_NO_INTERFACE;
        return this;
    },

    __logService : null, 
    get _logService() {
        if (!this.__logService)
            this.__logService = Cc["@mozilla.org/consoleservice;1"]
                                    .getService(Ci.nsIConsoleService);
        return this.__logService;
    },

    __decoderRing : null,  
    get _decoderRing() {
        if (!this.__decoderRing)
            this.__decoderRing = Cc["@mozilla.org/security/sdr;1"]
                                .getService(Ci.nsISecretDecoderRing);
        return this.__decoderRing;
    },

    _prefBranch : null,  

    _datafile    : null,  
    _datapath    : null,  
    _debug       : false, 


    





    _logins        : null, 
    _disabledHosts : null,


    




    log : function (message) {
        if (!this._debug)
            return;
        dump("PwMgr Storage: " + message + "\n");
        this._logService.logStringMessage("PwMgr Storage: " + message);
    },




    




    initWithFile : function(aInputFile, aOutputFile) {
        this._datapath = aInputFile.parent.path;
        this._datafile = aInputFile.leafName;

        this.init();

        if (aOutputFile) {
            this._datapath = aOutputFile.parent.path;
            this._datafile = aOutputFile.leafName;
            this._writeFile(this._datapath, this._datafile);
        }
    },

    




    init : function () {
        this._logins  = {};
        this._disabledHosts = {};

        
        this._prefBranch = Cc["@mozilla.org/preferences-service;1"]
                                .getService(Ci.nsIPrefService);
        this._prefBranch = this._prefBranch.getBranch("signon.");
        this._prefBranch.QueryInterface(Ci.nsIPrefBranch2);

        if (this._prefBranch.prefHasUserValue("debug"))
            this._debug = this._prefBranch.getBoolPref("debug");

        
        
        var tokenDB = Cc["@mozilla.org/security/pk11tokendb;1"]
                            .getService(Ci.nsIPK11TokenDB);

        var token = tokenDB.getInternalKeyToken();
        if (token.needsUserInit) {
            this.log("Initializing key3.db with default blank password.");
            token.initPassword("");
        }

        
        if (!this._datapath) {
            var DIR_SERVICE = new Components.Constructor(
                    "@mozilla.org/file/directory_service;1", "nsIProperties");
            this._datapath = (new DIR_SERVICE()).get("ProfD", Ci.nsIFile).path;
        }

        if (!this._datafile)
            this._datafile = this._prefBranch.getCharPref("SignonFileName2");

        var importFile = null;
        if (!this._doesFileExist(this._datapath, this._datafile)) {
            this.log("SignonFilename2 file does not exist. (file=" +
                        this._datafile + ") path=(" + this._datapath + ")");

            
            importFile = this._prefBranch.getCharPref("SignonFileName");
            if (!this._doesFileExist(this._datapath, importFile)) {
                this.log("SignonFilename1 file does not exist. (file=" +
                            importFile + ") path=(" + this._datapath + ")");
                this.log("Creating new signons file...");
                importFile = null;
                this._writeFile(this._datapath, this._datafile);
            }
        }

        
        if (importFile) {
            this.log("Importing " + importFile);
            this._readFile(this._datapath, importFile);

            this._writeFile(this._datapath, this._datafile);
        } else {
            this._readFile(this._datapath, this._datafile);
        }
    },


    



    addLogin : function (login) {
        var key = login.hostname;

        
        if (!this._logins[key])
            this._logins[key] = [];

        this._logins[key].push(login);

        this._writeFile(this._datapath, this._datafile);
    },


    



    removeLogin : function (login) {
        var key = login.hostname;
        var logins = this._logins[key];

        if (!logins)
            throw "No logins found for hostname (" + key + ")";

        for (var i = 0; i < logins.length; i++) {
            if (logins[i].equals(login)) {
                logins.splice(i, 1); 
                break;
                
                
            }
        }

        
        if (logins.length == 0)
            delete this._logins[key];

        this._writeFile(this._datapath, this._datafile);
    },


    



    modifyLogin : function (oldLogin, newLogin) {
        this.removeLogin(oldLogin);
        this.addLogin(newLogin);
    },


    




    getAllLogins : function (count) {
        var result = [];

        
        for each (var hostLogins in this._logins) {
            result = result.concat(hostLogins);
        }

        count.value = result.length; 
        return result;
    },


    




    clearAllLogins : function () {
        this._logins = {};
        

        this._writeFile(this._datapath, this._datafile);
    },


    



    getAllDisabledHosts : function (count) {
        var result = [];

        for (var hostname in this._disabledHosts) {
            result.push(hostname);
        }

        count.value = result.length; 
        return result;
    },


    



    getLoginSavingEnabled : function (hostname) {
        return !this._disabledHosts[hostname];
    },


    



    setLoginSavingEnabled : function (hostname, enabled) {
        if (enabled)
            delete this._disabledHosts[hostname];
        else
            this._disabledHosts[hostname] = true;

        this._writeFile(this._datapath, this._datafile);
    },


    



    findLogins : function (count, hostname, formSubmitURL, httpRealm) {
        var hostLogins = this._logins[hostname];
        if (hostLogins == null) {
            count.value = 0;
            return [];
        }

        var result = [];

        for each (var login in hostLogins) {

            
            if (httpRealm != login.httpRealm)
                continue;

            
            
            
            if (formSubmitURL != login.formSubmitURL &&
                login.formSubmitURL != "")
                continue;

            result.push(login);
        }

        count.value = result.length; 
        return result;
    },




    




    



    _readFile : function (pathname, filename) {
        var oldFormat = false;
        var writeOnFinish = false;

        this.log("Reading passwords from " + pathname + "/" + filename);

        var file = Cc["@mozilla.org/file/local;1"]
                        .createInstance(Ci.nsILocalFile);
        file.initWithPath(pathname);
        file.append(filename);

        var inputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                                .createInstance(Ci.nsIFileInputStream);
        inputStream.init(file, 0x01, -1, null); 
        var lineStream = inputStream.QueryInterface(Ci.nsILineInputStream);
        var line = { value: "" };

        const STATE = { HEADER : 0, REJECT : 1, REALM : 2,
                        USERFIELD : 3, USERVALUE : 4,
                        PASSFIELD : 5, PASSVALUE : 6, ACTIONURL : 7 };
        var parseState = STATE.HEADER;

        var nsLoginInfo = new Components.Constructor(
                "@mozilla.org/login-manager/loginInfo;1", Ci.nsILoginInfo);
        var processEntry = false;

        do {
            var hasMore = lineStream.readLine(line);

            switch (parseState) {
                
                case STATE.HEADER:
                    if (line.value == "#2c") {
                        oldFormat = true;
                    } else if (line.value != "#2d") {
                        this.log("invalid file header (" + line.value + ")");
                        throw "invalid file header in " + filename;
                        
                        
                        
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
                        parseState = STATE.REALM;
                        break;
                    }

                    var entry = new nsLoginInfo();
                    entry.hostname  = hostname;
                    entry.httpRealm = httpRealm;

                    entry.usernameField = line.value;
                    parseState++;
                    break;

                
                case STATE.USERVALUE:
                    entry.username = this._decrypt(line.value);
                    parseState++;
                    break;

                
                
                case STATE.PASSFIELD:
                    entry.passwordField = line.value.substr(1);
                    parseState++;
                    break;

                
                case STATE.PASSVALUE:
                    entry.password = this._decrypt(line.value);
                    if (oldFormat) {
                        entry.formSubmitURL = "";
                        processEntry = true;
                        parseState = STATE.USERFIELD;
                    } else {
                        parseState++;
                    }
                    break;

                
                case STATE.ACTIONURL:
                    entry.formSubmitURL = line.value;
                    processEntry = true;
                    parseState = STATE.USERFIELD;
                    break;

            }

            if (processEntry) {
                if (entry.username == "" && entry.password == "") {
                    
                    writeOnFinish = true;
                } else {
                    if (!this._logins[hostname])
                        this._logins[hostname] = [];
                    this._logins[hostname].push(entry);
                }
                entry = null;
                processEntry = false;
            }
        } while (hasMore);

        lineStream.close();

        if (writeOnFinish)
            this._writeFile(pathname, filename);

        return;
    },


    



    _writeFile : function (pathname, filename) {
        function writeLine(data) {
            data += "\r\n";
            outputStream.write(data, data.length);
        }

        this.log("Writing passwords to " + pathname + "/" + filename);

        var file = Cc["@mozilla.org/file/local;1"]
                        .createInstance(Ci.nsILocalFile);
        file.initWithPath(pathname);
        file.append(filename);

        var outputStream = Cc["@mozilla.org/network/safe-file-output-stream;1"]
                                .createInstance(Ci.nsIFileOutputStream);
        outputStream.QueryInterface(Ci.nsISafeOutputStream);

        
        outputStream.init(file, 0x02 | 0x08 | 0x20, 0600, null);

        
        writeLine("#2d");

        
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

                var encUsername = this._encrypt(login.username);
                var encPassword = this._encrypt(login.password);

                writeLine((login.usernameField ?  login.usernameField : ""));
                writeLine(encUsername);
                writeLine("*" +
                    (login.passwordField ?  login.passwordField : ""));
                writeLine(encPassword);
                writeLine((login.formSubmitURL ? login.formSubmitURL : ""));

                lastRealm = login.httpRealm;
            }

            
            writeLine(".");
        }

        

        outputStream.finish();
    },


    



    _encrypt : function (plainText) {
        var converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                             .createInstance(Ci.nsIScriptableUnicodeConverter);
        converter.charset = "UTF-8";
        var plainOctet = converter.ConvertFromUnicode(plainText);
        plainOctet += converter.Finish();
        return this._decoderRing.encryptString(plainOctet);
    },


    



    _decrypt : function (cipherText) {
        var plainText = null;

        try {
            var plainOctet;
            if (cipherText.charAt(0) == '~') {
                
                
                
                plainOctet = atob(cipherText.substring(1));
            } else {
                plainOctet = this._decoderRing.decryptString(cipherText);
            }
            var converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                              .createInstance(Ci.nsIScriptableUnicodeConverter);
            converter.charset = "UTF-8";
            plainText = converter.ConvertToUnicode(plainOctet);
        } catch (e) {
            this.log("Failed to decrypt string: " + cipherText);
        }

        return plainText;
    },


    



    _doesFileExist : function (filepath, filename) {
        var file = Cc["@mozilla.org/file/local;1"]
                        .createInstance(Ci.nsILocalFile);
        file.initWithPath(filepath);
        file.append(filename);

        return file.exists();
    }
}; 





var gModule = {
    registerSelf: function(componentManager, fileSpec, location, type) {
        componentManager = componentManager.QueryInterface(
                                                Ci.nsIComponentRegistrar);
        for each (var obj in this._objects) 
            componentManager.registerFactoryLocation(obj.CID,
                    obj.className, obj.contractID,
                    fileSpec, location, type);
    },

    unregisterSelf: function (componentManager, location, type) {
        for each (var obj in this._objects) 
            componentManager.unregisterFactoryLocation(obj.CID, location);
    },
    
    getClassObject: function(componentManager, cid, iid) {
        if (!iid.equals(Ci.nsIFactory))
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  
        for (var key in this._objects) {
            if (cid.equals(this._objects[key].CID))
                return this._objects[key].factory;
        }
    
        throw Components.results.NS_ERROR_NO_INTERFACE;
    },
  
    _objects: {
        service: {
            CID : Components.ID("{e09e4ca6-276b-4bb4-8b71-0635a3a2a007}"),
            contractID : "@mozilla.org/login-manager/storage/legacy;1",
            className  : "LoginManagerStorage_legacy",
            factory    : aFactory = {
                createInstance: function(aOuter, aIID) {
                    if (aOuter != null)
                        throw Components.results.NS_ERROR_NO_AGGREGATION;
                    var svc = new LoginManagerStorage_legacy();
                    return svc.QueryInterface(aIID);
                }
            }
        }
    },
  
    canUnload: function(componentManager) {
        return true;
    }
};

function NSGetModule(compMgr, fileSpec) {
    return gModule;
}
