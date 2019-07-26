




const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/PrivateBrowsingUtils.jsm");

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


    __storage : null, 
    get _storage() {
        if (!this.__storage) {

            var contractID = "@mozilla.org/login-manager/storage/mozStorage;1";
            try {
                var catMan = Cc["@mozilla.org/categorymanager;1"].
                             getService(Ci.nsICategoryManager);
                contractID = catMan.getCategoryEntry("login-manager-storage",
                                                     "nsILoginManagerStorage");
                log("Found alternate nsILoginManagerStorage with contract ID:", contractID);
            } catch (e) {
                log("No alternate nsILoginManagerStorage registered");
            }

            this.__storage = Cc[contractID].
                             createInstance(Ci.nsILoginManagerStorage);
            try {
                this.__storage.init();
            } catch (e) {
                log("Initialization of storage component failed:", e);
                this.__storage = null;
            }
        }

        return this.__storage;
    },

    _prefBranch  : null, 
    _nsLoginInfo : null, 

    _remember : true,  


    








    init : function () {

        
        this._webProgressListener._domEventListener = this._domEventListener;
        this._webProgressListener._pwmgr = this;
        this._domEventListener._pwmgr    = this;
        this._observer._pwmgr            = this;

        
        this._prefBranch = Services.prefs.getBranch("signon.");
        this._prefBranch.addObserver("", this._observer, false);

        
        debug = this._prefBranch.getBoolPref("debug");

        this._remember = this._prefBranch.getBoolPref("rememberSignons");


        
        this._nsLoginInfo = new Components.Constructor(
            "@mozilla.org/login-manager/loginInfo;1", Ci.nsILoginInfo);


        
        Services.obs.addObserver(this._observer, "earlyformsubmit", false);
        Services.obs.addObserver(this._observer, "xpcom-shutdown", false);

        
        var progress = Cc["@mozilla.org/docloaderservice;1"].
                       getService(Ci.nsIWebProgress);
        progress.addProgressListener(this._webProgressListener,
                                     Ci.nsIWebProgress.NOTIFY_STATE_DOCUMENT);
    },


    


    





    _observer : {
        _pwmgr : null,

        QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver,
                                                Ci.nsIFormSubmitObserver,
                                                Ci.nsISupportsWeakReference]),


        
        notify : function (formElement, aWindow, actionURI) {
            log("observer notified for form submission.");

            
            

            try {
                this._pwmgr._onFormSubmit(formElement);
            } catch (e) {
                log("Caught error in onFormSubmit:", e);
            }

            return true; 
        },

        
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
            } else {
                log("Oops! Unexpected notification:", topic);
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
            if (debug) {
                let requestName = "(null)";
                if (aRequest) {
                    try {
                        requestName = aRequest.name;
                    } catch (ex if ex.result == Components.results.NS_ERROR_NOT_IMPLEMENTED) {
                        
                    }
                }
                log("onStateChange accepted: req =", requestName);
            }

            
            if (aStateFlags & Ci.nsIWebProgressListener.STATE_RESTORING) {
                log("onStateChange: restoring document");
                return this._pwmgr._fillDocument(domDoc);
            }

            
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
            if (!event.isTrusted)
                return;

            log("domEventListener: got event", event.type);

            switch (event.type) {
                case "DOMContentLoaded":
                    this._pwmgr._fillDocument(event.target);
                    return;

                case "DOMAutoComplete":
                case "blur":
                    var acInputField = event.target;
                    var acForm = acInputField.form;

                    
                    
                    
                    if (!acInputField.value)
                        return;

                    
                    
                    
                    var [usernameField, passwordField, ignored] =
                        this._pwmgr._getFormFields(acForm, false);
                    if (usernameField == acInputField && passwordField) {
                        
                        
                        
                        this._pwmgr._fillForm(acForm, true, true, true, null);
                    } else {
                        log("Oops, form changed before AC invoked");
                    }
                    return;

                default:
                    log("Oops! This event unexpected.");
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




    




    








    _getPasswordFields : function (form, skipEmptyFields) {
        
        var pwFields = [];
        for (var i = 0; i < form.elements.length; i++) {
            var element = form.elements[i];
            if (!(element instanceof Ci.nsIDOMHTMLInputElement) ||
                element.type != "password")
                continue;

            if (skipEmptyFields && !element.value)
                continue;

            pwFields[pwFields.length] = {
                                            index   : i,
                                            element : element
                                        };
        }

        
        if (pwFields.length == 0) {
            log("(form ignored -- no password fields.)");
            return null;
        } else if (pwFields.length > 3) {
            log("(form ignored -- too many password fields. [ got",
                   pwFields.length, "])");
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
            var element = form.elements[i];
            var fieldType = (element.hasAttribute("type") ?
                             element.getAttribute("type").toLowerCase() :
                             element.type);
            if (fieldType == "text"  ||
                fieldType == "email" ||
                fieldType == "url"   ||
                fieldType == "tel"   ||
                fieldType == "number") {
                usernameField = element;
                break;
            }
        }

        if (!usernameField)
            log("(form -- no username field found)");


        
        
        
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
                
                log("(form ignored -- all 3 pw fields differ)");
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


    





    _isAutocompleteDisabled :  function (element) {
        if (element && element.hasAttribute("autocomplete") &&
            element.getAttribute("autocomplete").toLowerCase() == "off")
            return true;

        return false;
    },

    







    _onFormSubmit : function (form) {

        
        function getPrompter(aWindow) {
            var prompterSvc = Cc["@mozilla.org/login-manager/prompter;1"].
                              createInstance(Ci.nsILoginManagerPrompter);
            prompterSvc.init(aWindow);
            return prompterSvc;
        }

        var doc = form.ownerDocument;
        var win = doc.defaultView;

        if (PrivateBrowsingUtils.isWindowPrivate(win)) {
            
            
            log("(form submission ignored in private browsing mode)");
            return;
        }

        
        if (!this._remember)
            return;

        var hostname = this._getPasswordOrigin(doc.documentURI);
        if (!hostname) {
            log("(form submission ignored -- invalid hostname)");
            return;
        }

        var formSubmitURL = this._getActionOrigin(form)
        if (!this.getLoginSavingEnabled(hostname)) {
            log("(form submission ignored -- saving is disabled for:", hostname, ")");
            return;
        }


        
        var [usernameField, newPasswordField, oldPasswordField] =
            this._getFormFields(form, true);

        
        if (newPasswordField == null)
                return;

        
        
        
        
        if (this._isAutocompleteDisabled(form) ||
            this._isAutocompleteDisabled(usernameField) ||
            this._isAutocompleteDisabled(newPasswordField) ||
            this._isAutocompleteDisabled(oldPasswordField)) {
                log("(form submission ignored -- autocomplete=off found)");
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
                
                
                log("(no logins for this host -- pwchange ignored)");
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
            log("Found an existing login matching this form submission");

            
            if (existingLogin.password != formLogin.password) {
                log("...passwords differ, prompting to change.");
                prompter = getPrompter(win);
                prompter.promptToChangePassword(existingLogin, formLogin);
            } else {
                
                var propBag = Cc["@mozilla.org/hash-property-bag;1"].
                              createInstance(Ci.nsIWritablePropertyBag);
                propBag.setProperty("timeLastUsed", Date.now());
                propBag.setProperty("timesUsedIncrement", 1);
                this.modifyLogin(existingLogin, propBag);
            }

            return;
        }


        
        prompter = getPrompter(win);
        prompter.promptToSavePassword(formLogin);
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


    





    _fillDocument : function (doc) {
        var forms = doc.forms;
        if (!forms || forms.length == 0)
            return;

        var formOrigin = this._getPasswordOrigin(doc.documentURI);

        
        if (!this.countLogins(formOrigin, "", null))
            return;

        
        
        if (this.uiBusy) {
            log("deferring fillDoc for", doc.documentURI);
            let self = this;
            let observer = {
                QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupportsWeakReference]),

                observe: function (subject, topic, data) {
                    log("Got deferred fillDoc notification:", topic);
                    
                    Services.obs.removeObserver(this, "passwordmgr-crypto-login");
                    Services.obs.removeObserver(this, "passwordmgr-crypto-loginCanceled");
                    if (topic == "passwordmgr-crypto-loginCanceled")
                        return;
                    self._fillDocument(doc);
                },
                handleEvent : function (event) {
                    
                }
            };
            
            
            
            
            Services.obs.addObserver(observer, "passwordmgr-crypto-login", true);
            Services.obs.addObserver(observer, "passwordmgr-crypto-loginCanceled", true);
            doc.addEventListener("mozCleverClosureHack", observer, false);
            return;
        }

        log("fillDocument processing", forms.length, "forms on", doc.documentURI);

        var autofillForm = !PrivateBrowsingUtils.isWindowPrivate(doc.defaultView) &&
                           this._prefBranch.getBoolPref("autofillForms");
        var previousActionOrigin = null;
        var foundLogins = null;

        
        
        const MAX_FORMS = 40; 
        var skip_from = -1, skip_to = -1;
        if (forms.length > MAX_FORMS) {
            log("fillDocument limiting number of forms filled to", MAX_FORMS);
            let chunk_size = MAX_FORMS / 2;
            skip_from = chunk_size;
            skip_to   = forms.length - chunk_size;
        }

        for (var i = 0; i < forms.length; i++) {
            
            if (i == skip_from)
              i = skip_to;

            var form = forms[i];

            
            
            var actionOrigin = this._getActionOrigin(form);
            if (actionOrigin != previousActionOrigin) {
                foundLogins = null;
                previousActionOrigin = actionOrigin;
            }
            log("_fillDocument processing form[", i, "]");
            foundLogins = this._fillForm(form, autofillForm, false, false, foundLogins)[1];
        } 
    },


    











    _fillForm : function (form, autofillForm, ignoreAutocomplete,
                          clobberPassword, foundLogins) {
        
        
        
        
        var [usernameField, passwordField, ignored] =
            this._getFormFields(form, false);

        
        if (passwordField == null)
            return [false, foundLogins];

        
        if (passwordField.disabled || passwordField.readOnly ||
            usernameField && (usernameField.disabled ||
                              usernameField.readOnly)) {
            log("not filling form, login fields disabled");
            return [false, foundLogins];
        }

        
        if (foundLogins == null) {
            var formOrigin =
                this._getPasswordOrigin(form.ownerDocument.documentURI);
            var actionOrigin = this._getActionOrigin(form);
            foundLogins = this.findLogins({}, formOrigin, actionOrigin, null);
            log("found", foundLogins.length, "matching logins.");
        } else {
            log("reusing logins from last form.");
        }

        
        
        
        
        var maxUsernameLen = Number.MAX_VALUE;
        var maxPasswordLen = Number.MAX_VALUE;

        
        if (usernameField && usernameField.maxLength >= 0)
            maxUsernameLen = usernameField.maxLength;
        if (passwordField.maxLength >= 0)
            maxPasswordLen = passwordField.maxLength;

        var logins = foundLogins.filter(function (l) {
                var fit = (l.username.length <= maxUsernameLen &&
                           l.password.length <= maxPasswordLen);
                if (!fit)
                    log("Ignored", l.username, "login: won't fit");

                return fit;
            }, this);


        
        if (logins.length == 0)
            return [false, foundLogins];


        
        
        
        var didntFillReason = null;

        
        
        
        if (usernameField)
            this._attachToInput(usernameField);

        
        if (passwordField.value && !clobberPassword) {
            didntFillReason = "existingPassword";
            this._notifyFoundLogins(didntFillReason, usernameField,
                                    passwordField, foundLogins, null);
            return [false, foundLogins];
        }

        
        
        
        
        var isFormDisabled = false;
        if (!ignoreAutocomplete &&
            (this._isAutocompleteDisabled(form) ||
             this._isAutocompleteDisabled(usernameField) ||
             this._isAutocompleteDisabled(passwordField))) {

            isFormDisabled = true;
            log("form not filled, has autocomplete=off");
        }

        
        
        var selectedLogin = null;

        if (usernameField && usernameField.value) {
            
            
            var username = usernameField.value.toLowerCase();

            let matchingLogins = logins.filter(function(l)
                                     l.username.toLowerCase() == username);
            if (matchingLogins.length) {
                selectedLogin = matchingLogins[0];
            } else {
                didntFillReason = "existingUsername";
                log("Password not filled. None of the stored logins match the username already present.");
            }
        } else if (logins.length == 1) {
            selectedLogin = logins[0];
        } else {
            
            
            
            
            let matchingLogins;
            if (usernameField)
                matchingLogins = logins.filter(function(l) l.username);
            else
                matchingLogins = logins.filter(function(l) !l.username);
            if (matchingLogins.length == 1) {
                selectedLogin = matchingLogins[0];
            } else {
                didntFillReason = "multipleLogins";
                log("Multiple logins for form, so not filling any.");
            }
        }

        var didFillForm = false;
        if (selectedLogin && autofillForm && !isFormDisabled) {
            
            if (usernameField)
                usernameField.value = selectedLogin.username;
            passwordField.value = selectedLogin.password;
            didFillForm = true;
        } else if (selectedLogin && !autofillForm) {
            
            
            didntFillReason = "noAutofillForms";
            Services.obs.notifyObservers(form, "passwordmgr-found-form", didntFillReason);
            log("autofillForms=false but form can be filled; notified observers");
        } else if (selectedLogin && isFormDisabled) {
            
            
            didntFillReason = "autocompleteOff";
            Services.obs.notifyObservers(form, "passwordmgr-found-form", didntFillReason);
            log("autocomplete=off but form can be filled; notified observers");
        }

        this._notifyFoundLogins(didntFillReason, usernameField, passwordField,
                                foundLogins, selectedLogin);

        return [didFillForm, foundLogins];
    },

    



































    _notifyFoundLogins : function (didntFillReason, usernameField,
                                   passwordField, foundLogins, selectedLogin) {
        
        
        
        let formInfo = Cc["@mozilla.org/hash-property-bag;1"].
                       createInstance(Ci.nsIWritablePropertyBag2).
                       QueryInterface(Ci.nsIWritablePropertyBag);

        formInfo.setPropertyAsACString("didntFillReason", didntFillReason);
        formInfo.setPropertyAsInterface("usernameField", usernameField);
        formInfo.setPropertyAsInterface("passwordField", passwordField);
        formInfo.setProperty("foundLogins", foundLogins.concat());
        formInfo.setPropertyAsInterface("selectedLogin", selectedLogin);

        Services.obs.notifyObservers(formInfo, "passwordmgr-found-logins", null);
    },

    




    fillForm : function (form) {
        log("fillForm processing form[ id:", form.id, "]");
        return this._fillForm(form, true, true, false, null)[0];
    },


    






    _attachToInput : function (element) {
        log("attaching autocomplete stuff");
        element.addEventListener("blur",
                                this._domEventListener, false);
        element.addEventListener("DOMAutoComplete",
                                this._domEventListener, false);
        this._formFillService.markAsLoginManagerField(element);
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
