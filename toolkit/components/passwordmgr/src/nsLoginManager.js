




































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function LoginManager() {
    this.init();
}

LoginManager.prototype = {

    classDescription: "LoginManager",
    contractID: "@mozilla.org/login-manager;1",
    classID: Components.ID("{cb9e0de8-3598-4ed7-857b-827f011ad5d8}"),
    QueryInterface : XPCOMUtils.generateQI([Ci.nsILoginManager,
                                            Ci.nsISupportsWeakReference]),


    


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


    __formFillService : null, 
    get _formFillService() {
        if (!this.__formFillService)
            this.__formFillService =
                            Cc["@mozilla.org/satchel/form-fill-controller;1"].
                            getService(Ci.nsIFormFillController);
        return this.__formFillService;
    },


    __storage : null, 
    get _storage() {
        if (!this.__storage) {
            this.__storage = Cc["@mozilla.org/login-manager/storage/legacy;1"].
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

    _prefBranch  : null, 
    _nsLoginInfo : null, 

    _remember : true,  
    _debug    : false, 


    








    init : function () {

        
        this._webProgressListener._domEventListener = this._domEventListener;
        this._webProgressListener._pwmgr = this;
        this._domEventListener._pwmgr    = this;
        this._observer._pwmgr            = this;

        
        this._prefBranch = Cc["@mozilla.org/preferences-service;1"].
                           getService(Ci.nsIPrefService).getBranch("signon.");
        this._prefBranch.QueryInterface(Ci.nsIPrefBranch2);
        this._prefBranch.addObserver("", this._observer, false);

        
        this._debug = this._prefBranch.getBoolPref("debug");

        this._remember = this._prefBranch.getBoolPref("rememberSignons");


        
        this._nsLoginInfo = new Components.Constructor(
            "@mozilla.org/login-manager/loginInfo;1", Ci.nsILoginInfo);


        
        var observerService = Cc["@mozilla.org/observer-service;1"].
                              getService(Ci.nsIObserverService);
        observerService.addObserver(this._observer, "earlyformsubmit", false);
        observerService.addObserver(this._observer, "xpcom-shutdown", false);

        
        var progress = Cc["@mozilla.org/docloaderservice;1"].
                       getService(Ci.nsIWebProgress);
        progress.addProgressListener(this._webProgressListener,
                                     Ci.nsIWebProgress.NOTIFY_STATE_DOCUMENT);


    },


    




    log : function (message) {
        if (!this._debug)
            return;
        dump("Login Manager: " + message + "\n");
        this._logService.logStringMessage("Login Manager: " + message);
    },


    


    





    _observer : {
        _pwmgr : null,

        QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver, 
                                                Ci.nsIFormSubmitObserver,
                                                Ci.nsISupportsWeakReference]),


        
        notify : function (formElement, aWindow, actionURI) {
            this._pwmgr.log("observer notified for form submission.");

            
            

            try {
                this._pwmgr._onFormSubmit(formElement);
            } catch (e) {
                this._pwmgr.log("Caught error in onFormSubmit: " + e);
            }

            return true; 
        },

        
        observe : function (subject, topic, data) {

            if (topic == "nsPref:changed") {
                var prefName = data;
                this._pwmgr.log("got change to " + prefName + " preference");

                if (prefName == "debug") {
                    this._pwmgr._debug = 
                        this._pwmgr._prefBranch.getBoolPref("debug");
                } else if (prefName == "rememberSignons") {
                    this._pwmgr._remember =
                        this._pwmgr._prefBranch.getBoolPref("rememberSignons");
                } else {
                    this._pwmgr.log("Oops! Pref not handled, change ignored.");
                }
            } else if (topic == "xpcom-shutdown") {
                for (let i in this._pwmgr) {
                  try {
                    this._pwmgr[i] = null;
                  } catch(ex) {}
                }
                this._pwmgr = null;
            } else {
                this._pwmgr.log("Oops! Unexpected notification: " + topic);
            }
        }
    },


    






    _webProgressListener : {
        _pwmgr : null,
        _domEventListener : null,

        QueryInterface : XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                                Ci.nsISupportsWeakReference]),


        onStateChange : function (aWebProgress, aRequest,
                                  aStateFlags,  aStatus) {

            
            if (!(aStateFlags & Ci.nsIWebProgressListener.STATE_TRANSFERRING))
                return;

            if (!this._pwmgr._remember)
                return;

            var domWin = aWebProgress.DOMWindow;
            var domDoc = domWin.document;

            
            if (!(domDoc instanceof Ci.nsIDOMHTMLDocument))
                return;

            this._pwmgr.log("onStateChange accepted: req = " + (aRequest ?
                        aRequest.name : "(null)") + ", flags = " + aStateFlags);

            
            
            if (aStateFlags & Ci.nsIWebProgressListener.STATE_RESTORING) {
                this._pwmgr.log("onStateChange: restoring document");
                return this._pwmgr._fillDocument(domDoc);
            }

            
            this._pwmgr.log("onStateChange: adding dom listeners");
            domDoc.addEventListener("DOMContentLoaded",
                                    this._domEventListener, false);
            return;
        },

        
        onProgressChange : function() { throw "Unexpected onProgressChange"; },
        onLocationChange : function() { throw "Unexpected onLocationChange"; },
        onStatusChange   : function() { throw "Unexpected onStatusChange";   },
        onSecurityChange : function() { throw "Unexpected onSecurityChange"; }
    },


    





    _domEventListener : {
        _pwmgr : null,

        QueryInterface : XPCOMUtils.generateQI([Ci.nsIDOMEventListener,
                                                Ci.nsISupportsWeakReference]),


        handleEvent : function (event) {
            this._pwmgr.log("domEventListener: got event " + event.type);

            var doc, inputElement;
            switch (event.type) {
                case "DOMContentLoaded":
                    doc = event.target;
                    this._pwmgr._fillDocument(doc);
                    return;

                case "DOMAutoComplete":
                case "blur":
                    inputElement = event.target;
                    this._pwmgr._fillPassword(inputElement);
                    return;

                default:
                    this._pwmgr.log("Oops! This event unexpected.");
                    return;
            }
        }
    },




    




    




    addLogin : function (login) {
        
        if (login.hostname == null || login.hostname.length == 0)
            throw "Can't add a login with a null or empty hostname.";

        
        if (login.username == null)
            throw "Can't add a login with a null username.";

        if (login.password == null || login.password.length == 0)
            throw "Can't add a login with a null or empty password.";

        if (!login.httpRealm && !login.formSubmitURL)
            throw "Can't add a login without a httpRealm or formSubmitURL.";

        
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


    





    countLogins : function (hostname, formSubmitURL, httpRealm) {
        this.log("Counting logins matching host: " + hostname +
            ", formSubmitURL: " + formSubmitURL + ", httpRealm: " + httpRealm);

        return this._storage.countLogins(hostname, formSubmitURL, httpRealm);
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
            return false;

        this.log("AutoCompleteSearch invoked. Search is: " + aSearchString);

        var result = null;

        if (aPreviousResult) {
            this.log("Using previous autocomplete result");
            result = aPreviousResult;

            
            
            
            
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
                if (aSearchString.length <= username.length &&
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
    },




    




    








    _getPasswordFields : function (form, skipEmptyFields) {
        
        var pwFields = [];
        for (var i = 0; i < form.elements.length; i++) {
            if (form.elements[i].type != "password")
                continue;

            if (skipEmptyFields && !form.elements[i].value)
                continue;

            pwFields[pwFields.length] = {
                                            index   : i,
                                            element : form.elements[i]
                                        };
        }

        
        if (pwFields.length == 0) {
            this.log("(form ignored -- no password fields.)");
            return null;
        } else if (pwFields.length > 3) {
            this.log("(form ignored -- too many password fields. [got " +
                        pwFields.length + "])");
            return null;
        }

        return pwFields;
    },


    















    _getFormFields : function (form, isSubmission) {
        var usernameField = null;

        
        
        var pwFields = this._getPasswordFields(form, isSubmission);
        if (!pwFields)
            return [null, null, null];


        
        
        
        
        for (var i = pwFields[0].index - 1; i >= 0; i--) {
            if (form.elements[i].type == "text") {
                usernameField = form.elements[i];
                break;
            }
        }

        if (!usernameField)
            this.log("(form -- no username field found)");


        
        
        
        if (!isSubmission || pwFields.length == 1)
            return [usernameField, pwFields[0].element, null];


        
        var oldPasswordField, newPasswordField;
        var pw1 = pwFields[0].element.value;
        var pw2 = pwFields[1].element.value;
        var pw3 = (pwFields[2] ? pwFields[2].element.value : null);

        if (pwFields.length == 3) {
            

            if (pw1 == pw2 && pw2 == pw3) {
                
                newPasswordField = pwFields[0].element;
                oldPasswordField = null;
            } else if (pw1 == pw2) {
                newPasswordField = pwFields[0].element;
                oldPasswordField = pwFields[2].element;
            } else if (pw2 == pw3) {
                oldPasswordField = pwFields[0].element;
                newPasswordField = pwFields[2].element;
            } else  if (pw1 == pw3) {
                
                newPasswordField = pwFields[0].element;
                oldPasswordField = pwFields[1].element;
            } else {
                
                this.log("(form ignored -- all 3 pw fields differ)");
                return [null, null, null];
            }
        } else { 
            if (pw1 == pw2) {
                
                newPasswordField = pwFields[0].element;
                oldPasswordField = null;
            } else {
                
                oldPasswordField = pwFields[0].element;
                newPasswordField = pwFields[1].element;
            }
        }

        return [usernameField, newPasswordField, oldPasswordField];
    },


    







    _onFormSubmit : function (form) {

        
        function autocompleteDisabled(element) {
            if (element && element.hasAttribute("autocomplete") &&
                element.getAttribute("autocomplete").toLowerCase() == "off")
                return true;

           return false;
        };

        
        function getPrompter(aWindow) {
            var prompterSvc = Cc["@mozilla.org/login-manager/prompter;1"].
                              createInstance(Ci.nsILoginManagerPrompter);
            prompterSvc.init(aWindow);
            return prompterSvc;
        }

        var doc = form.ownerDocument;
        var win = doc.defaultView;

        
        if (!this._remember)
            return;

        var hostname      = this._getPasswordOrigin(doc.documentURI);
        var formSubmitURL = this._getActionOrigin(form)
        if (!this.getLoginSavingEnabled(hostname)) {
            this.log("(form submission ignored -- saving is " +
                     "disabled for: " + hostname + ")");
            return;
        }


        
        var [usernameField, newPasswordField, oldPasswordField] =
            this._getFormFields(form, true);

        
        if (newPasswordField == null)
                return;

        
        
        
        if (autocompleteDisabled(form) ||
            autocompleteDisabled(usernameField) ||
            autocompleteDisabled(newPasswordField) ||
            autocompleteDisabled(oldPasswordField)) {
                this.log("(form submission ignored -- autocomplete=off found)");
                return;
        }


        var formLogin = new this._nsLoginInfo();
        formLogin.init(hostname, formSubmitURL, null,
                    (usernameField ? usernameField.value : ""),
                    newPasswordField.value,
                    (usernameField ? usernameField.name  : ""),
                    newPasswordField.name);

        
        
        
        if (!usernameField && oldPasswordField) {

            var logins = this.findLogins({}, hostname, formSubmitURL, null);

            if (logins.length == 0) {
                
                
                this.log("(no logins for this host -- pwchange ignored)");
                return;
            }

            var prompter = getPrompter(win);

            if (logins.length == 1) {
                var oldLogin = logins[0];
                formLogin.username      = oldLogin.username;
                formLogin.usernameField = oldLogin.usernameField;

                prompter.promptToChangePassword(oldLogin, formLogin);
            } else {
                prompter.promptToChangePasswordWithUsernames(
                                    logins, logins.length, formLogin);
            }

            return;
        }


        
        var existingLogin = null;
        var logins = this.findLogins({}, hostname, formSubmitURL, null);

        for (var i = 0; i < logins.length; i++) {
            var same, login = logins[i];

            
            
            
            
            if (!login.username && formLogin.username) {
                var restoreMe = formLogin.username;
                formLogin.username = ""; 
                same = formLogin.matches(login);
                formLogin.username = restoreMe;
            } else if (!formLogin.username && login.username) {
                formLogin.username = login.username;
                same = formLogin.matches(login);
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
                if (formLogin.username) {
                    this.log("...Updating password for existing login.");
                    this.modifyLogin(existingLogin, formLogin);
                } else {
                    this.log("...passwords differ, prompting to change.");
                    prompter = getPrompter(win);
                    prompter.promptToChangePassword(existingLogin, formLogin);
                }
            }

            return;
        }


        
        prompter = getPrompter(win);
        prompter.promptToSavePassword(formLogin);
    },


    




    _getPasswordOrigin : function (uriString) {
        var realm = "";
        try {
            var uri = this._ioService.newURI(uriString, null, null);

            realm = uri.scheme + "://" + uri.host;

            
            
            var port = uri.port;
            if (port != -1) {
                var handler = this._ioService.getProtocolHandler(uri.scheme);
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

        return this._getPasswordOrigin(uriString);
    },


    





    _fillDocument : function (doc) {
        var forms = doc.forms;
        if (!forms || forms.length == 0)
            return;

        var formOrigin = this._getPasswordOrigin(doc.documentURI);

        
        if (!this.countLogins(formOrigin, "", null))
            return;

        this.log("fillDocument processing " + forms.length +
                 " forms on " + doc.documentURI);

        var autofillForm = this._prefBranch.getBoolPref("autofillForms");
        var previousActionOrigin = null;

        for (var i = 0; i < forms.length; i++) {
            var form = forms[i];

            
            
            
            
            var [usernameField, passwordField, ignored] =
                this._getFormFields(form, false);

            
            if (passwordField == null)
                continue;


            
            
            var actionOrigin = this._getActionOrigin(form);
            if (actionOrigin != previousActionOrigin) {
                var foundLogins =
                    this.findLogins({}, formOrigin, actionOrigin, null);

                this.log("form[" + i + "]: got " +
                         foundLogins.length + " logins.");

                previousActionOrigin = actionOrigin;
            } else {
                this.log("form[" + i + "]: using logins from last form.");
            }


            
            
            
            
            var maxUsernameLen = Number.MAX_VALUE;
            var maxPasswordLen = Number.MAX_VALUE;

            
            if (usernameField && usernameField.maxLength >= 0)
                maxUsernameLen = usernameField.maxLength;
            if (passwordField.maxLength >= 0)
                maxPasswordLen = passwordField.maxLength;

            logins = foundLogins.filter(function (l) {
                    var fit = (l.username.length <= maxUsernameLen &&
                               l.password.length <= maxPasswordLen);
                    if (!fit)
                        this.log("Ignored " + l.username + " login: won't fit");

                    return fit;
                }, this);


            
            if (logins.length == 0)
                continue;


            
            
            
            if (usernameField)
                this._attachToInput(usernameField);

            if (autofillForm) {

                if (usernameField && usernameField.value) {
                    
                    

                    var username = usernameField.value;

                    var matchingLogin;
                    var found = logins.some(function(l) {
                                                matchingLogin = l;
                                                return (l.username == username);
                                            });
                    if (found)
                        passwordField.value = matchingLogin.password;

                } else if (usernameField && logins.length == 2) {
                    
                    
                    
                    
                    
                    if (!logins[0].username && logins[1].username) {
                        usernameField.value = logins[1].username;
                        passwordField.value = logins[1].password;
                    } else if (!logins[1].username && logins[0].username) {
                        usernameField.value = logins[0].username;
                        passwordField.value = logins[0].password;
                    }
                } else if (logins.length == 1) {
                    if (usernameField)
                        usernameField.value = logins[0].username;
                    passwordField.value = logins[0].password;
                }
            }
        } 
    },


    






    _attachToInput : function (element) {
        this.log("attaching autocomplete stuff");
        element.addEventListener("blur",
                                this._domEventListener, false);
        element.addEventListener("DOMAutoComplete",
                                this._domEventListener, false);
        this._formFillService.markAsLoginManagerField(element);
    },


    




    _fillPassword : function (usernameField) {
        this.log("fillPassword autocomplete username: " + usernameField.value);

        var form = usernameField.form;
        var doc = form.ownerDocument;

        var hostname = this._getPasswordOrigin(doc.documentURI);
        var formSubmitURL = this._getActionOrigin(form)

        
        
        var pwFields = this._getPasswordFields(form, false);
        if (!pwFields) {
            const err = "No password field for autocomplete password fill.";

            
            if (!this._debug)
                dump(err);
            else
                this.log(err);

            return;
        }

        
        
        var passwordField = pwFields[0].element;

        
        var currentLogin = new this._nsLoginInfo();
        currentLogin.init(hostname, formSubmitURL, null,
                          usernameField.value, null,
                          usernameField.name, passwordField.name);

        
        var match = null;
        var logins = this.findLogins({}, hostname, formSubmitURL, null);

        if (!logins.some(function(l) {
                                match = l;
                                return currentLogin.matches(l, true);
                        }))
        {
            this.log("Can't find a login for this autocomplete result.");
            return;
        }

        this.log("Found a matching login, filling in password.");
        passwordField.value = match.password;
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
    },
};

var component = [LoginManager];
function NSGetModule (compMgr, fileSpec) {
    return XPCOMUtils.generateModule(component);
}
