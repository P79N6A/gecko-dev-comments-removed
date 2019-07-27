




const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/PrivateBrowsingUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LoginManagerContent",
                                  "resource://gre/modules/LoginManagerContent.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

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
        Services.obs.addObserver(this._observer, "passwordmgr-storage-replace",
                                 false);

        
        this._initStorage();
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
                for (let i in this._pwmgr) {
                  try {
                    this._pwmgr[i] = null;
                  } catch(ex) {}
                }
                this._pwmgr = null;
            } else if (topic == "passwordmgr-storage-replace") {
                Task.spawn(function () {
                  yield this._pwmgr._storage.terminate();
                  this._pwmgr._initStorage();
                  yield this._pwmgr.initializationPromise;
                  Services.obs.notifyObservers(null,
                               "passwordmgr-storage-replace-complete", null);
                }.bind(this));
            } else {
                log("Oops! Unexpected notification:", topic);
            }
        }
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


    









    autoCompleteSearch : function (aSearchString, aPreviousResult, aElement) {
        
        

        if (!this._remember)
            return null;

        log("AutoCompleteSearch invoked. Search is:", aSearchString);

        var result = null;

        if (aPreviousResult &&
                aSearchString.substr(0, aPreviousResult.searchString.length) == aPreviousResult.searchString) {
            log("Using previous autocomplete result");
            result = aPreviousResult;
            result.wrappedJSObject.searchString = aSearchString;

            
            
            
            
            for (var i = result.matchCount - 1; i >= 0; i--) {
                var match = result.getValueAt(i);

                
                if (aSearchString.length > match.length ||
                    aSearchString.toLowerCase() !=
                        match.substr(0, aSearchString.length).toLowerCase())
                {
                    log("Removing autocomplete entry:", match);
                    result.removeValueAt(i, false);
                }
            }
        } else {
            log("Creating new autocomplete search result.");

            var doc = aElement.ownerDocument;
            var origin = this._getPasswordOrigin(doc.documentURI);
            var actionOrigin = this._getActionOrigin(aElement.form);

            
            
            
            var logins = this.findLogins({}, origin, actionOrigin, null);
            var matchingLogins = [];

            
            
            
            for (i = 0; i < logins.length; i++) {
                var username = logins[i].username.toLowerCase();
                if (username &&
                    aSearchString.length <= username.length &&
                    aSearchString.toLowerCase() ==
                        username.substr(0, aSearchString.length))
                {
                    matchingLogins.push(logins[i]);
                }
            }
            log(matchingLogins.length, "autocomplete logins avail.");
            result = new UserAutoCompleteResult(aSearchString, matchingLogins);
        }

        return result;
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
        return LoginManagerContent._fillForm(form, true, true, false, false, null)[0];
    },

}; 





function UserAutoCompleteResult (aSearchString, matchingLogins) {
    function loginSort(a,b) {
        var userA = a.username.toLowerCase();
        var userB = b.username.toLowerCase();

        if (userA < userB)
            return -1;

        if (userB > userA)
            return  1;

        return 0;
    };

    this.searchString = aSearchString;
    this.logins = matchingLogins.sort(loginSort);
    this.matchCount = matchingLogins.length;

    if (this.matchCount > 0) {
        this.searchResult = Ci.nsIAutoCompleteResult.RESULT_SUCCESS;
        this.defaultIndex = 0;
    }
}

UserAutoCompleteResult.prototype = {
    QueryInterface : XPCOMUtils.generateQI([Ci.nsIAutoCompleteResult,
                                            Ci.nsISupportsWeakReference]),

    
    logins : null,

    
    
    get wrappedJSObject() {
        return this;
    },

    
    searchString : null,
    searchResult : Ci.nsIAutoCompleteResult.RESULT_NOMATCH,
    defaultIndex : -1,
    errorDescription : "",
    matchCount : 0,

    getValueAt : function (index) {
        if (index < 0 || index >= this.logins.length)
            throw "Index out of range.";

        return this.logins[index].username;
    },

    getLabelAt: function(index) {
        return this.getValueAt(index);
    },

    getCommentAt : function (index) {
        return "";
    },

    getStyleAt : function (index) {
        return "";
    },

    getImageAt : function (index) {
        return "";
    },

    getFinalCompleteValueAt : function (index) {
        return this.getValueAt(index);
    },

    removeValueAt : function (index, removeFromDB) {
        if (index < 0 || index >= this.logins.length)
            throw "Index out of range.";

        var [removedLogin] = this.logins.splice(index, 1);

        this.matchCount--;
        if (this.defaultIndex > this.logins.length)
            this.defaultIndex--;

        if (removeFromDB) {
            var pwmgr = Cc["@mozilla.org/login-manager;1"].
                        getService(Ci.nsILoginManager);
            pwmgr.removeLogin(removedLogin);
        }
    }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([LoginManager]);
