





































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

function LoginManager() {
    this.init();
}

LoginManager.prototype = {

    classID: Components.ID("{f9a0edde-2a8d-4bfd-a08c-3f9333213a85}"),
    QueryInterface : XPCOMUtils.generateQI([Ci.nsILoginManager,
                                            Ci.nsIObserver,
                                            Ci.nsISupportsWeakReference]),


    


    __storage : null, 
    get _storage() {
        if (!this.__storage) {

            var contractID = "@mozilla.org/login-manager/storage/mozStorage;1";
            try {
                var catMan = Cc["@mozilla.org/categorymanager;1"].
                             getService(Ci.nsICategoryManager);
                contractID = catMan.getCategoryEntry("login-manager-storage",
                                                     "nsILoginManagerStorage");
                this.log("Found alternate nsILoginManagerStorage with " +
                         "contract ID: " + contractID);
            } catch (e) {
                this.log("No alternate nsILoginManagerStorage registered");
            }

            this.__storage = Cc[contractID].
                             createInstance(Ci.nsILoginManagerStorage);
            try {
                this.__storage.init();
            } catch (e) {
                this.log("Initialization of storage component failed: " + e);
                this.__storage = null;
            }
        }

        return this.__storage;
    },


    _nsLoginInfo : null, 
    _debug    : false, 
    _remember : true,  


    








    init : function () {
        
        var messageManager = Cc["@mozilla.org/globalmessagemanager;1"].
                             getService(Ci.nsIChromeFrameMessageManager);
        messageManager.loadFrameScript("chrome://browser/content/LoginManagerChild.js", true);
        messageManager.addMessageListener("PasswordMgr:FormSubmitted", this);
        messageManager.addMessageListener("PasswordMgr:GetPasswords", this);

        
        this._nsLoginInfo = new Components.Constructor(
            "@mozilla.org/login-manager/loginInfo;1", Ci.nsILoginInfo);

        
        Services.prefs.addObserver("signon.", this, false);

        
        this._debug = Services.prefs.getBoolPref("signon.debug");
        this._remember = Services.prefs.getBoolPref("signon.rememberSignons");

        
        Services.obs.addObserver(this, "xpcom-shutdown", false);
    },

    




    log : function (message) {
        if (!this._debug)
            return;
        dump("PasswordUtils: " + message + "\n");
        Services.console.logStringMessage("PasswordUtils: " + message);
    },

    




    observe : function (subject, topic, data) {
        if (topic == "nsPref:changed") {
            this._pwmgr._debug    = Services.prefs.getBoolPref("signon.debug");
            this._pwmgr._remember = Services.prefs.getBoolPref("signon.rememberSignons");
        } else if (topic == "xpcom-shutdown") {
            
            
            this._formFillService = null;
        } else {
            this._pwmgr.log("Oops! Unexpected notification: " + topic);
        }
    },

    




    receiveMessage: function (message) {
        
        function getPrompter(aBrowser) {
            var prompterSvc = Cc["@mozilla.org/login-manager/prompter;1"].
                              createInstance(Ci.nsILoginManagerPrompter);
            prompterSvc.init(aBrowser);
            return prompterSvc;
        }

        switch (message.name) {
            case "PasswordMgr:GetPasswords":
                
                if (!this.countLogins(message.json.formOrigin, "", null))
                    return { foundLogins: {} };

                var foundLogins = {};

                if (!this.uiBusy) {
                    for (var i = 0; i < message.json.actionOrigins.length; i++) {
                        var actionOrigin = message.json.actionOrigins[i];
                        var logins = this.findLogins({}, message.json.formOrigin, actionOrigin, null);
                        if (logins.length) {
                            foundLogins[actionOrigin] = logins;
                        }
                    }
                }

                return {
                    uiBusy: this.uiBusy,
                    foundLogins: foundLogins
                };

            case "PasswordMgr:FormSubmitted":
                var json = message.json;
                var hostname = json.hostname;
                var formSubmitURL = json.formSubmitURL;

                if (!this.getLoginSavingEnabled(hostname)) {
                    this.log("(form submission ignored -- saving is " +
                             "disabled for: " + hostname + ")");
                    return {};
                }

                var browser = message.target;

                var formLogin = new this._nsLoginInfo();

                formLogin.init(hostname, formSubmitURL, null,
                            json.usernameValue,
                            json.passwordValue,
                            json.usernameField,
                            json.passwordField);

                
                
                
                if (!json.usernameField && json.hasOldPasswordField) {

                    var logins = this.findLogins({}, hostname, formSubmitURL, null);

                    if (logins.length == 0) {
                        
                        
                        this.log("(no logins for this host -- pwchange ignored)");
                        return {};
                    }

                    var prompter = getPrompter(browser);

                    if (logins.length == 1) {
                        var oldLogin = logins[0];
                        formLogin.username      = oldLogin.username;
                        formLogin.usernameField = oldLogin.usernameField;

                        prompter.promptToChangePassword(oldLogin, formLogin);
                    } else {
                        prompter.promptToChangePasswordWithUsernames(
                                            logins, logins.length, formLogin);
                    }

                } else {

                    
                    var existingLogin = null;
                    var logins = this.findLogins({}, hostname, formSubmitURL, null);

                    for (var i = 0; i < logins.length; i++) {
                        var same, login = logins[i];

                        
                        
                        
                        
                        if (!login.username && formLogin.username) {
                            var restoreMe = formLogin.username;
                            formLogin.username = ""; 
                            same = formLogin.matches(login, false);
                            formLogin.username = restoreMe;
                        } else if (!formLogin.username && login.username) {
                            formLogin.username = login.username;
                            same = formLogin.matches(login, false);
                            formLogin.username = ""; 
                        } else {
                            same = formLogin.matches(login, true);
                        }

                        if (same) {
                            existingLogin = login;
                            break;
                        }
                    }

                    if (existingLogin) {
                        this.log("Found an existing login matching this form submission");

                        
                        if (existingLogin.password != formLogin.password) {
                            this.log("...passwords differ, prompting to change.");
                            prompter = getPrompter(browser);
                            prompter.promptToChangePassword(existingLogin, formLogin);
                        } else {
                            
                            var propBag = Cc["@mozilla.org/hash-property-bag;1"].
                                          createInstance(Ci.nsIWritablePropertyBag);
                            propBag.setProperty("timeLastUsed", Date.now());
                            propBag.setProperty("timesUsedIncrement", 1);
                            this.modifyLogin(existingLogin, propBag);
                        }

                        return {};
                    }


                    
                    prompter = getPrompter(browser);
                    prompter.promptToSavePassword(formLogin);
                }
                return {};

            default:
                throw "Unexpected message " + message.name;
        }
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
            
            
            this.log("Couldn't parse origin for " + uriString);
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
        
        return false;
    },


    




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

        this.log("Adding login: " + login);
        return this._storage.addLogin(login);
    },


    




    removeLogin : function (login) {
        this.log("Removing login: " + login);
        return this._storage.removeLogin(login);
    },


    




    modifyLogin : function (oldLogin, newLogin) {
        this.log("Modifying oldLogin: " + oldLogin + " newLogin: " + newLogin);
        return this._storage.modifyLogin(oldLogin, newLogin);
    },


    








    getAllLogins : function (count) {
        this.log("Getting a list of all logins");
        return this._storage.getAllLogins(count);
    },


    




    removeAllLogins : function () {
        this.log("Removing all logins");
        this._storage.removeAllLogins();
    },

    









    getAllDisabledHosts : function (count) {
        this.log("Getting a list of all disabled hosts");
        return this._storage.getAllDisabledHosts(count);
    },


    




    findLogins : function (count, hostname, formSubmitURL, httpRealm) {
        this.log("Searching for logins matching host: " + hostname +
            ", formSubmitURL: " + formSubmitURL + ", httpRealm: " + httpRealm);

        return this._storage.findLogins(count, hostname, formSubmitURL,
                                        httpRealm);
    },


    







    searchLogins : function(count, matchData) {
       this.log("Searching for logins");

        return this._storage.searchLogins(count, matchData);
    },


    





    countLogins : function (hostname, formSubmitURL, httpRealm) {
        this.log("Counting logins matching host: " + hostname +
            ", formSubmitURL: " + formSubmitURL + ", httpRealm: " + httpRealm);

        return this._storage.countLogins(hostname, formSubmitURL, httpRealm);
    },


    


    get uiBusy() {
        return this._storage.uiBusy;
    },


    




    getLoginSavingEnabled : function (host) {
        this.log("Checking if logins to " + host + " can be saved.");
        if (!this._remember)
            return false;

        return this._storage.getLoginSavingEnabled(host);
    },


    




    setLoginSavingEnabled : function (hostname, enabled) {
        
        if (hostname.indexOf("\0") != -1)
            throw "Invalid hostname";

        this.log("Saving logins for " + hostname + " enabled? " + enabled);
        return this._storage.setLoginSavingEnabled(hostname, enabled);
    },


    









    autoCompleteSearch : function (aSearchString, aPreviousResult, aElement) {
        
        

        if (!this._remember)
            return null;

        this.log("AutoCompleteSearch invoked. Search is: " + aSearchString);

        var result = null;

        if (aPreviousResult &&
                aSearchString.substr(0, aPreviousResult.searchString.length) == aPreviousResult.searchString) {
            this.log("Using previous autocomplete result");
            result = aPreviousResult;
            result.wrappedJSObject.searchString = aSearchString;

            
            
            
            
            for (var i = result.matchCount - 1; i >= 0; i--) {
                var match = result.getValueAt(i);

                
                if (aSearchString.length > match.length ||
                    aSearchString.toLowerCase() !=
                        match.substr(0, aSearchString.length).toLowerCase())
                {
                    this.log("Removing autocomplete entry '" + match + "'");
                    result.removeValueAt(i, false);
                }
            }
        } else {
            this.log("Creating new autocomplete search result.");

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
            this.log(matchingLogins.length + " autocomplete logins avail.");
            result = new UserAutoCompleteResult(aSearchString, matchingLogins);
        }

        return result;
    }
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

    getLabelAt : function (index) {
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

var NSGetFactory = XPCOMUtils.generateNSGetFactory([LoginManager]);
