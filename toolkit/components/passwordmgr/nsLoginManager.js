




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");
Cu.import("resource://gre/modules/LoginManagerContent.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "BrowserUtils",
                                  "resource://gre/modules/BrowserUtils.jsm");

var debug = false;
function log(...pieces) {
    function generateLogMessage(args) {
        let strings = ['Login Manager:'];

        args.forEach(function(arg) {
            if (typeof arg === 'string') {
                strings.push(arg);
            } else if (typeof arg === 'undefined') {
                strings.push('undefined');
            } else if (arg === null) {
                strings.push('null');
            } else {
                try {
                  strings.push(JSON.stringify(arg, null, 2));
                } catch(err) {
                  strings.push("<<something>>");
                }
            }
        });
        return strings.join(' ');
    }

    if (!debug)
        return;

    let message = generateLogMessage(pieces);
    dump(message + "\n");
    Services.console.logStringMessage(message);
}

function LoginManager() {
    this.init();
}

LoginManager.prototype = {

    classID: Components.ID("{cb9e0de8-3598-4ed7-857b-827f011ad5d8}"),
    QueryInterface : XPCOMUtils.generateQI([Ci.nsILoginManager,
                                            Ci.nsISupportsWeakReference,
                                            Ci.nsIInterfaceRequestor]),
    getInterface : function(aIID) {
        if (aIID.equals(Ci.mozIStorageConnection) && this._storage) {
          let ir = this._storage.QueryInterface(Ci.nsIInterfaceRequestor);
          return ir.getInterface(aIID);
        }

        if (aIID.equals(Ci.nsIVariant)) {
            
            return this;
        }

        throw Cr.NS_ERROR_NO_INTERFACE;
    },


    


    __formFillService : null, 
    get _formFillService() {
        if (!this.__formFillService)
            this.__formFillService =
                            Cc["@mozilla.org/satchel/form-fill-controller;1"].
                            getService(Ci.nsIFormFillController);
        return this.__formFillService;
    },


    _storage : null, 
    _prefBranch  : null, 
    _remember : true,  


    








    init : function () {

        
        this._observer._pwmgr            = this;

        
        this._prefBranch = Services.prefs.getBranch("signon.");
        this._prefBranch.addObserver("", this._observer, false);

        
        debug = this._prefBranch.getBoolPref("debug");

        this._remember = this._prefBranch.getBoolPref("rememberSignons");

        
        Services.obs.addObserver(this._observer, "xpcom-shutdown", false);

        
        
        if (Services.appinfo.processType ===
            Services.appinfo.PROCESS_TYPE_DEFAULT) {
            Services.obs.addObserver(this._observer, "passwordmgr-storage-replace",
                                     false);

            
            this._initStorage();
        }

        Services.obs.addObserver(this._observer, "gather-telemetry", false);
    },


    _initStorage : function () {
#ifdef ANDROID
        var contractID = "@mozilla.org/login-manager/storage/mozStorage;1";
#else
        var contractID = "@mozilla.org/login-manager/storage/json;1";
#endif
        try {
            var catMan = Cc["@mozilla.org/categorymanager;1"].
                         getService(Ci.nsICategoryManager);
            contractID = catMan.getCategoryEntry("login-manager-storage",
                                                 "nsILoginManagerStorage");
            log("Found alternate nsILoginManagerStorage with contract ID:", contractID);
        } catch (e) {
            log("No alternate nsILoginManagerStorage registered");
        }

        this._storage = Cc[contractID].
                        createInstance(Ci.nsILoginManagerStorage);
        this.initializationPromise = this._storage.initialize();
    },


    


    





    _observer : {
        _pwmgr : null,

        QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver,
                                                Ci.nsISupportsWeakReference]),

        
        observe : function (subject, topic, data) {

            if (topic == "nsPref:changed") {
                var prefName = data;
                log("got change to", prefName, "preference");

                if (prefName == "debug") {
                    debug = this._pwmgr._prefBranch.getBoolPref("debug");
                } else if (prefName == "rememberSignons") {
                    this._pwmgr._remember =
                        this._pwmgr._prefBranch.getBoolPref("rememberSignons");
                } else {
                    log("Oops! Pref not handled, change ignored.");
                }
            } else if (topic == "xpcom-shutdown") {
                delete this._pwmgr.__formFillService;
                delete this._pwmgr._storage;
                delete this._pwmgr._prefBranch;
                this._pwmgr = null;
            } else if (topic == "passwordmgr-storage-replace") {
                Task.spawn(function () {
                  yield this._pwmgr._storage.terminate();
                  this._pwmgr._initStorage();
                  yield this._pwmgr.initializationPromise;
                  Services.obs.notifyObservers(null,
                               "passwordmgr-storage-replace-complete", null);
                }.bind(this));
            } else if (topic == "gather-telemetry") {
                this._pwmgr._gatherTelemetry();
            } else {
                log("Oops! Unexpected notification:", topic);
            }
        }
    },

    _gatherTelemetry : function() {
      let numPasswordsHist = Services.telemetry.getHistogramById("PWMGR_NUM_SAVED_PASSWORDS");
      numPasswordsHist.clear();
      numPasswordsHist.add(this.countLogins("", "", ""));

      let isPwdSavedEnabledHist = Services.telemetry.getHistogramById("PWMGR_SAVING_ENABLED");
      isPwdSavedEnabledHist.clear();
      isPwdSavedEnabledHist.add(this._remember);
    },





    




    





    initializationPromise : null,


    




    addLogin : function (login) {
        
        if (login.hostname == null || login.hostname.length == 0)
            throw "Can't add a login with a null or empty hostname.";

        
        if (login.username == null)
            throw "Can't add a login with a null username.";

        if (login.password == null || login.password.length == 0)
            throw "Can't add a login with a null or empty password.";

        if (login.formSubmitURL || login.formSubmitURL == "") {
            
            if (login.httpRealm != null)
                throw "Can't add a login with both a httpRealm and formSubmitURL.";
        } else if (login.httpRealm) {
            
            if (login.formSubmitURL != null)
                throw "Can't add a login with both a httpRealm and formSubmitURL.";
        } else {
            
            throw "Can't add a login without a httpRealm or formSubmitURL.";
        }


        
        var logins = this.findLogins({}, login.hostname, login.formSubmitURL,
                                     login.httpRealm);

        if (logins.some(function(l) login.matches(l, true)))
            throw "This login already exists.";

        log("Adding login");
        return this._storage.addLogin(login);
    },


    




    removeLogin : function (login) {
        log("Removing login");
        return this._storage.removeLogin(login);
    },


    




    modifyLogin : function (oldLogin, newLogin) {
        log("Modifying login");
        return this._storage.modifyLogin(oldLogin, newLogin);
    },


    








    getAllLogins : function (count) {
        log("Getting a list of all logins");
        return this._storage.getAllLogins(count);
    },


    




    removeAllLogins : function () {
        log("Removing all logins");
        this._storage.removeAllLogins();
    },

    









    getAllDisabledHosts : function (count) {
        log("Getting a list of all disabled hosts");
        return this._storage.getAllDisabledHosts(count);
    },


    




    findLogins : function (count, hostname, formSubmitURL, httpRealm) {
        log("Searching for logins matching host:", hostname,
            "formSubmitURL:", formSubmitURL, "httpRealm:", httpRealm);

        return this._storage.findLogins(count, hostname, formSubmitURL,
                                        httpRealm);
    },


    







    searchLogins : function(count, matchData) {
       log("Searching for logins");

        return this._storage.searchLogins(count, matchData);
    },


    





    countLogins : function (hostname, formSubmitURL, httpRealm) {
        log("Counting logins matching host:", hostname,
            "formSubmitURL:", formSubmitURL, "httpRealm:", httpRealm);

        return this._storage.countLogins(hostname, formSubmitURL, httpRealm);
    },


    


    get uiBusy() {
        return this._storage.uiBusy;
    },


    


    get isLoggedIn() {
        return this._storage.isLoggedIn;
    },


    




    getLoginSavingEnabled : function (host) {
        log("Checking if logins to", host, "can be saved.");
        if (!this._remember)
            return false;

        return this._storage.getLoginSavingEnabled(host);
    },


    




    setLoginSavingEnabled : function (hostname, enabled) {
        
        if (hostname.indexOf("\0") != -1)
            throw "Invalid hostname";

        log("Login saving for", hostname, "now enabled?", enabled);
        return this._storage.setLoginSavingEnabled(hostname, enabled);
    },

    









    autoCompleteSearchAsync : function (aSearchString, aPreviousResult,
                                        aElement, aCallback) {
        
        

        if (!this._remember) {
            setTimeout(function() {
                aCallback.onSearchCompletion(new UserAutoCompleteResult(aSearchString, []));
            }, 0);
            return;
        }

        log("AutoCompleteSearch invoked. Search is:", aSearchString);

        var previousResult;
        if (aPreviousResult) {
            previousResult = { searchString: aPreviousResult.searchString,
                               logins: aPreviousResult.wrappedJSObject.logins };
        } else {
            previousResult = null;
        }

        let rect = BrowserUtils.getElementBoundingScreenRect(aElement);
        LoginManagerContent._autoCompleteSearchAsync(aSearchString, previousResult,
                                                     aElement, rect)
                           .then(function(logins) {
                               let results =
                                   new UserAutoCompleteResult(aSearchString, logins);
                               aCallback.onSearchCompletion(results);
                           })
                           .then(null, Cu.reportError);
    },


    


    




    _getPasswordOrigin : function (uriString, allowJS) {
        var realm = "";
        try {
            var uri = Services.io.newURI(uriString, null, null);

            if (allowJS && uri.scheme == "javascript")
                return "javascript:"

            realm = uri.scheme + "://" + uri.host;

            
            
            var port = uri.port;
            if (port != -1) {
                var handler = Services.io.getProtocolHandler(uri.scheme);
                if (port != handler.defaultPort)
                    realm += ":" + port;
            }

        } catch (e) {
            
            
            log("Couldn't parse origin for", uriString);
            realm = null;
        }

        return realm;
    },

    _getActionOrigin : function (form) {
        var uriString = form.action;

        
        if (uriString == "")
            uriString = form.baseURI; 

        return this._getPasswordOrigin(uriString, true);
    },


    




    fillForm : function (form) {
        log("fillForm processing form[ id:", form.id, "]");
        return LoginManagerContent._asyncFindLogins(form, { showMasterPassword: true })
                                  .then(function({ form, loginsFound }) {
                   return LoginManagerContent._fillForm(form, true, true,
                                                        false, false, loginsFound)[0];
               });
    },

}; 

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([LoginManager]);
