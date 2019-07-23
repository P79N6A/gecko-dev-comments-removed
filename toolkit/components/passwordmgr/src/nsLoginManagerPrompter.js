




































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");









function LoginManagerPromptFactory() {}

LoginManagerPromptFactory.prototype = {

    classDescription : "LoginManagerPromptFactory",
    contractID : "@mozilla.org/passwordmanager/authpromptfactory;1",
    classID : Components.ID("{447fc780-1d28-412a-91a1-466d48129c65}"),
    QueryInterface : XPCOMUtils.generateQI([Ci.nsIPromptFactory]),

    _promptService : null,
    _pwmgr         : null,

    _initialized : false,

    getPrompt : function (aWindow, aIID) {

        if (!this._initialized) {
            
            this._pwmgr = Cc["@mozilla.org/login-manager;1"]
                                .getService(Ci.nsILoginManager);

            
            this._promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"]
                                    .getService(Ci.nsIPromptService2);

            this._initialized = true;
        }

        return new LoginManagerPrompter(this._pwmgr, this._promptService,
                                        aWindow).QueryInterface(aIID);
    }
}; 

















function LoginManagerPrompter(pwmgr, promptService, window) {
  this._pwmgr = pwmgr;
  this._promptService = promptService;
  this._window = window;

  this.log("===== initialized =====");
}
LoginManagerPrompter.prototype = {

    QueryInterface : XPCOMUtils.generateQI([Ci.nsIAuthPrompt2]),

    __logService : null, 
    get _logService() {
        if (!this.__logService)
            this.__logService = Cc["@mozilla.org/consoleservice;1"]
                                    .getService(Ci.nsIConsoleService);
        return this.__logService;
    },

    _promptService : null,
    _pwmgr         : null,
    _window        : null,

    _debug         : false,


    




    log : function (message) {
        if (!this._debug)
            return;

        dump("Pwmgr Prompter: " + message + "\n");
        this._logService.logStringMessage("Pwmgr Prompter: " + message);
    },


    









    promptAuth : function (aChannel, aLevel, aAuthInfo, aConfirm) {
        var rememberLogin = false;
        var selectedLogin = null;
        var checkboxLabel = null;

        this.log("===== promptAuth called =====");

        var hostname, httpRealm;
        [hostname, httpRealm] = this._GetAuthKey(aChannel, aAuthInfo);

        if (this._pwmgr.getLoginSavingEnabled(hostname)) {
            checkboxLabel = this.getLocalizedString("rememberPassword");


            var foundLogins = this._pwmgr.findLogins({},
                                            hostname, null, httpRealm);

            
            
            if (foundLogins.length > 0) {
                selectedLogin = foundLogins[0];
                this._SetAuthInfo(aAuthInfo, selectedLogin.username,
                                             selectedLogin.password);
                rememberLogin = true;
            }
        }

        
        var checkbox = { value : rememberLogin };

        var ok = this._promptService.promptAuth(this._window, aChannel,
                                aLevel, aAuthInfo, checkboxLabel, checkbox);
        rememberLogin = checkbox.value;

        if (ok && rememberLogin) {
            var newLogin = Cc["@mozilla.org/login-manager/loginInfo;1"]
                                .createInstance(Ci.nsILoginInfo);
            newLogin.init(hostname, null, httpRealm,
                            aAuthInfo.username, aAuthInfo.password,
                            "", "");

            
            
            if (!selectedLogin ||
                aAuthInfo.username != selectedLogin.username) {

                
                this.log("Adding login for " + aAuthInfo.username +
                         " @ " + hostname + " (" + httpRealm + ")");
                this._pwmgr.addLogin(newLogin);

            } else if (selectedLogin &&
                       aAuthInfo.password != selectedLogin.password) {

                this.log("Updating password for " + aAuthInfo.username +
                         " @ " + hostname + " (" + httpRealm + ")");
                
                this._pwmgr.modifyLogin(foundLogins[0], newLogin);

            } else {
                this.log("Login unchanged, no further action needed.");
                return ok;
            }
        }

        return ok;
    },

    asyncPromptAuth : function () {
        return NS_ERROR_NOT_IMPLEMENTED;
    },

    
    




    _GetRealPort : function (aURI) {
        var port = aURI.port;

        if (port != -1)
            return port; 

        
        
        var scheme = aURI.scheme;

        var ioService = Cc["@mozilla.org/network/io-service;1"]
                            .getService(Ci.nsIIOService);

        var handler = ioService.getProtocolHandler(scheme);
        port = handler.defaultPort;

        return port;
    },


    
    
    _GetAuthHostPort : function (aChannel, aAuthInfo) {

        
        var flags = aAuthInfo.flags;

        if (flags & (Ci.nsIAuthInformation.AUTH_PROXY)) {
            
            var proxied = aChannel.QueryInterface(Ci.nsIProxiedChannel);
            if (!proxied)
                throw "proxy auth needs nsIProxiedChannel";

            var info = proxied.proxyInfo;
            if (!info)
                throw "proxy auth needs nsIProxyInfo";

            var idnhost = info.host;
            var port    = info.port;

            var idnService = Cc["@mozilla.org/network/idn-service;1"]
                                .getService(Ci.nsIIDNService);
            host = idnService.convertUTF8toACE(idnhost);
        } else {
            var host = aChannel.URI.host;
            var port = this._GetRealPort(aChannel.URI);
        }

        return [host, port];
    },


    
    
    _GetAuthKey : function (aChannel, aAuthInfo) {
        var key = "";
        
        var http = aChannel.QueryInterface(Ci.nsIHttpChannel);
        if (!http) {
            key = aChannel.URI.prePath;
            this.log("_GetAuthKey: got http channel, key is: " + key);
            return key;
        }

        var [host, port] = this._GetAuthHostPort(aChannel, aAuthInfo);

        var realm = aAuthInfo.realm;

        key += host;
        key += ':';
        key += port;

        this.log("_GetAuthKey got host: " + key + " and realm: " + realm);

        return [key, realm];
    },


    
    
    




    _SetAuthInfo : function (aAuthInfo, username, password) {
        var flags = aAuthInfo.flags;
        if (flags & Ci.nsIAuthInformation.NEED_DOMAIN) {
            
            var idx = username.indexOf("\\");
            if (idx == -1) {
                aAuthInfo.username = username;
            } else {
                aAuthInfo.domain   =  username.substring(0, idx);
                aAuthInfo.username =  username.substring(idx+1);
            }
        } else {
            aAuthInfo.username = username;
        }
        aAuthInfo.password = password;
    },


    _bundle : null,
    getLocalizedString : function (key) {

        if (!this._bundle) {
            var bunService = Cc["@mozilla.org/intl/stringbundle;1"]
                                .getService(Ci.nsIStringBundleService);
            this._bundle = bunService.createBundle(
                        "chrome://passwordmgr/locale/passwordmgr.properties");

            if (!this._bundle)
                throw "String bundle not present!";
        }

        return this._bundle.GetStringFromName(key);
    }
}; 

var component = [LoginManagerPromptFactory];
function NSGetModule(compMgr, fileSpec) {
    return XPCOMUtils.generateModule(component);
}
