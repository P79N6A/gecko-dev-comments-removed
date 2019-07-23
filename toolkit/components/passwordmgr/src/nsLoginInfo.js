




































const Cc = Components.classes;
const Ci = Components.interfaces;

function nsLoginInfo() {}

nsLoginInfo.prototype = {

    QueryInterface : function (iid) {
        var interfaces = [Ci.nsILoginInfo, Ci.nsISupports];
        if (!interfaces.some( function(v) { return iid.equals(v) } ))
            throw Components.results.NS_ERROR_NO_INTERFACE;
        return this;
    },


    hostname      : null,
    formSubmitURL : null,
    httpRealm     : null,
    username      : null,
    password      : null,
    usernameField : null,
    passwordField : null,

    init : function (aHostname, aFormSubmitURL, aHttpRealm,
                     aUsername,      aPassword,
                     aUsernameField, aPasswordField) {
        this.hostname      = aHostname;
        this.formSubmitURL = aFormSubmitURL;
        this.httpRealm     = aHttpRealm;
        this.username      = aUsername;
        this.password      = aPassword;
        this.usernameField = aUsernameField;
        this.passwordField = aPasswordField;
    },

    equalsIgnorePassword : function (aLogin) {
        if (this.hostname != aLogin.hostname)
            return false;

        
        if (this.formSubmitURL != "" && aLogin.formSubmitURL != "" &&
            this.formSubmitURL != aLogin.formSubmitURL)
            return false;

        if (this.httpRealm != aLogin.httpRealm)
            return false;

        if (this.username != aLogin.username)
            return false;

        if (this.usernameField != aLogin.usernameField)
            return false;

        

        return true;
    },

    equals : function (aLogin) {
        if (!this.equalsIgnorePassword(aLogin) ||
            this.password      != aLogin.password   ||
            this.passwordField != aLogin.passwordField)
            return false;

        return true;
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
            CID : Components.ID("{0f2f347c-1e4f-40cc-8efd-792dea70a85e}"),
            contractID : "@mozilla.org/login-manager/loginInfo;1",
            className  : "LoginInfo",
            factory    : LoginInfoFactory = {
                createInstance: function(aOuter, aIID) {
                    if (aOuter != null)
                        throw Components.results.NS_ERROR_NO_AGGREGATION;
                    var svc = new nsLoginInfo();
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
