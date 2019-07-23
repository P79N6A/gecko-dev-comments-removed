




































const Cc = Components.classes;
const Ci = Components.interfaces;









function LoginManagerPromptFactory() {}

LoginManagerPromptFactory.prototype = {

    QueryInterface : function (iid) {
        const interfaces = [Ci.nsIPromptFactory, Ci.nsISupports];
        if (!interfaces.some( function(v) { return iid.equals(v) } ))
            throw Components.results.NS_ERROR_NO_INTERFACE;
        return this;
    },

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

        if (!aIID.equals(Ci.nsIAuthPrompt2))
            throw Components.results.NS_ERROR_NO_INTERFACE;

        var prompter = new LoginManagerPrompter();
        prompter.init(this._pwmgr, this._promptService, aWindow);
        prompter.QueryInterface(Ci.nsIAuthPrompt2);

        return prompter;
    }
}; 

















function LoginManagerPrompter() {}
LoginManagerPrompter.prototype = {

    QueryInterface : function (iid) {
        var interfaces = [Ci.nsIAuthPrompt2, Ci.nsISupports];
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

    _promptService : null,
    _pwmgr         : null,
    _window        : null,

    _debug         : false,


    init : function (aPWManager, aPromptService, aWindow) {
        this._pwmgr = aPWManager;
        this._promptService = aPromptService;
        this._window = aWindow;

        this.log("===== initialized =====");
    },


    




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
            CID : Components.ID("{447fc780-1d28-412a-91a1-466d48129c65}"),
            contractID : "@mozilla.org/passwordmanager/authpromptfactory;1",
            className  : "LoginManagerPromptFactory",
            factory    : LoginManagerPromptFactory_Factory = {
                singleton : null,
                createInstance: function (aOuter, aIID) {
                    if (aOuter != null)
                        throw Components.results.NS_ERROR_NO_AGGREGATION;

                    if (this.singleton == null) {
                        var svc = new LoginManagerPromptFactory();
                        this.singleton = svc;
                    } else {
                        svc = this.singleton;
                    }
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
