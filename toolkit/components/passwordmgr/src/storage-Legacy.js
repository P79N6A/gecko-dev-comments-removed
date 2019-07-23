




































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function LoginManagerStorage_legacy() { };

LoginManagerStorage_legacy.prototype = {

    classDescription  : "LoginManagerStorage_legacy",
    contractID : "@mozilla.org/login-manager/storage/legacy;1",
    classID : Components.ID("{e09e4ca6-276b-4bb4-8b71-0635a3a2a007}"),
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

    _prefBranch : null,  

    _signonsFile : null,  
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

        
        this._prefBranch = Cc["@mozilla.org/preferences-service;1"]
                                .getService(Ci.nsIPrefService);
        this._prefBranch = this._prefBranch.getBranch("signon.");
        this._prefBranch.QueryInterface(Ci.nsIPrefBranch2);

        this._debug = this._prefBranch.getBoolPref("debug");

        
        
        var tokenDB = Cc["@mozilla.org/security/pk11tokendb;1"]
                            .getService(Ci.nsIPK11TokenDB);

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

        
        this._readFile()

        
        if (importFile) {
            this._signonsFile = tmp;
            this._writeFile();
        }
    },


    



    addLogin : function (login) {
        var key = login.hostname;

        
        if (!this._logins[key])
            this._logins[key] = [];

        this._logins[key].push(login);

        this._writeFile();
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

        this._writeFile();
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


    




    removeAllLogins : function () {
        this._logins = {};
        

        this._writeFile();
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

        this._writeFile();
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




    




    






    _getSignonsFile : function() {
        var importFile = null;

        
        var DIR_SERVICE = new Components.Constructor(
                "@mozilla.org/file/directory_service;1", "nsIProperties");
        var pathname = (new DIR_SERVICE()).get("ProfD", Ci.nsIFile).path;


        
        var filename = this._prefBranch.getCharPref("SignonFileName2");

        var file = Cc["@mozilla.org/file/local;1"].
                   createInstance(Ci.nsILocalFile);
        file.initWithPath(pathname);
        file.append(filename);

        if (!file.exists()) {
            this.log("SignonFilename2 file does not exist. file=" +
                     filename + ", path=" + pathname);

            
            var oldname = this._prefBranch.getCharPref("SignonFileName");

            importFile = Cc["@mozilla.org/file/local;1"].
                         createInstance(Ci.nsILocalFile);
            importFile.initWithPath(pathname);
            importFile.append(oldname);

            if (!importFile.exists()) {
                this.log("SignonFilename1 file does not exist. file=" +
                        oldname + ", path=" + pathname);
                importFile = null;
            }
        }

        return [file, importFile];
    },


    



    _readFile : function () {
        var oldFormat = false;
        var writeOnFinish = false;

        this.log("Reading passwords from " + this._signonsFile.path);

        
        if (!this._signonsFile.exists()) {
            this.log("Creating new signons file...");
            this._writeFile();
            return;
        }

        var inputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                                .createInstance(Ci.nsIFileInputStream);
        
        inputStream.init(this._signonsFile, 0x01, -1, null);
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
            this._writeFile();

        return;
    },


    



    _writeFile : function () {
        function writeLine(data) {
            data += "\r\n";
            outputStream.write(data, data.length);
        }

        this.log("Writing passwords to " + this._signonsFile.path);

        var outputStream = Cc["@mozilla.org/network/safe-file-output-stream;1"]
                                .createInstance(Ci.nsIFileOutputStream);
        outputStream.QueryInterface(Ci.nsISafeOutputStream);

        
        outputStream.init(this._signonsFile, 0x02 | 0x08 | 0x20, 0600, null);

        
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

}; 

var component = [LoginManagerStorage_legacy];
function NSGetModule(compMgr, fileSpec) {
    return XPCOMUtils.generateModule(component);
}
