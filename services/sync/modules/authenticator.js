




































const EXPORTED_SYMBOLS = ["Authenticator"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let Authenticator = {
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


    __observerService : null, 
    get _observerService() {
        if (!this.__observerService)
            this.__observerService = Cc["@mozilla.org/observer-service;1"].
                                     getService(Ci.nsIObserverService);
        return this.__observerService;
    },

    get loginManager() {
      delete this.loginManager;
      return this.loginManager = Cc["@mozilla.org/login-manager;1"].
                                 getService(Ci.nsILoginManager);
    },

    
    
    __privateBrowsingService : undefined,
    get _privateBrowsingService() {
        if (this.__privateBrowsingService == undefined) {
            if ("@mozilla.org/privatebrowsing;1" in Cc)
                this.__privateBrowsingService = Cc["@mozilla.org/privatebrowsing;1"].
                                                getService(Ci.nsIPrivateBrowsingService);
            else
                this.__privateBrowsingService = null;
        }
        return this.__privateBrowsingService;
    },


    
    get _inPrivateBrowsing() {
        var pbSvc = this._privateBrowsingService;
        if (pbSvc)
            return pbSvc.privateBrowsingEnabled;
        else
            return false;
    },

    _prefBranch  : null, 
    _nsLoginInfo : null, 

    _remember : true,  
    _debug    : false, 


    








    init : function () {
        
        this._observer._pwmgr            = this;

        
        this._prefBranch = Cc["@mozilla.org/preferences-service;1"].
                           getService(Ci.nsIPrefService).getBranch("signon.");
        this._prefBranch.QueryInterface(Ci.nsIPrefBranch2);
        this._prefBranch.addObserver("", this._observer, false);

        
        this._debug = this._prefBranch.getBoolPref("debug");
        this._remember = this._prefBranch.getBoolPref("rememberSignons");

        
        this._nsLoginInfo = new Components.Constructor(
            "@mozilla.org/login-manager/loginInfo;1", Ci.nsILoginInfo);
    },

    




    log : function (message) {
        if (!this._debug)
            return;
        dump("Authenticator: " + message + "\n");
        this._logService.logStringMessage("Authenticator: " + message);
    },


    


    





    _observer : {
        _pwmgr : null,

        QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver, 
                                                Ci.nsISupportsWeakReference]),

        
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
            } else {
                this._pwmgr.log("Oops! Unexpected notification: " + topic);
            }
        }
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


    





    _isAutocompleteDisabled :  function (element) {
        if (element && element.hasAttribute("autocomplete") &&
            element.getAttribute("autocomplete").toLowerCase() == "off")
            return true;

        return false;
    },

    




    _getPasswordOrigin : function (uriString, allowJS) {
        var realm = "";
        try {
            var uri = this._ioService.newURI(uriString, null, null);

            if (allowJS && uri.scheme == "javascript")
                return "javascript:"

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

        return this._getPasswordOrigin(uriString, true);
    },


    





    _fillDocument : function (doc) {
        var forms = doc.forms;
        if (!forms || forms.length == 0)
            return [];

        var formOrigin = this._getPasswordOrigin(doc.documentURI);

        
        if (!this.loginManager.countLogins(formOrigin, "", null))
            return [];

        this.log("fillDocument processing " + forms.length +
                 " forms on " + doc.documentURI);

        var autofillForm = !this._inPrivateBrowsing &&
                           this._prefBranch.getBoolPref("autofillForms");
        var previousActionOrigin = null;
        var foundLogins = null;

        let formInfos = [];
        for (var i = 0; i < forms.length; i++) {
            var form = forms[i];

            
            
            var actionOrigin = this._getActionOrigin(form);
            if (actionOrigin != previousActionOrigin) {
                foundLogins = null;
                previousActionOrigin = actionOrigin;
            }
            this.log("_fillDocument processing form[" + i + "]");
            let formInfo = this._fillForm(form, autofillForm, false, foundLogins);
            foundLogins = formInfo.foundLogins;
            if (formInfo.canFillForm)
                formInfos.push(formInfo);
        } 
        return formInfos;
    },


    











    _fillForm : function (form, autofillForm, ignoreAutocomplete, foundLogins) {
        
        
        
        
        var [usernameField, passwordField, ignored] =
            this._getFormFields(form, false);

        
        if (passwordField == null)
            return { foundLogins: foundLogins };

        
        if (passwordField.disabled || passwordField.readOnly ||
            usernameField && (usernameField.disabled ||
                              usernameField.readOnly)) {
            this.log("not filling form, login fields disabled");
            return { foundLogins: foundLogins };
        }

        
        if (foundLogins == null) {
            var formOrigin = 
                this._getPasswordOrigin(form.ownerDocument.documentURI);
            var actionOrigin = this._getActionOrigin(form);
            foundLogins = this.loginManager.findLogins({}, formOrigin, actionOrigin, null);
            this.log("found " + foundLogins.length + " matching logins.");
        } else {
            this.log("reusing logins from last form.");
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
            return { foundLogins: foundLogins };



        
        
        
        
        

        
        if (passwordField.value)
            return { foundLogins: foundLogins };

        
        
        
        
        var isFormDisabled = false;
        if (!ignoreAutocomplete &&
            (this._isAutocompleteDisabled(form) ||
             this._isAutocompleteDisabled(usernameField) ||
             this._isAutocompleteDisabled(passwordField))) {

            isFormDisabled = true;
            this.log("form not filled, has autocomplete=off");
        }

        
        
        var selectedLogin = null;

        if (usernameField && usernameField.value) {
            
            
            var username = usernameField.value.toLowerCase();

            let matchingLogins = logins.filter(function(l)
                                     l.username.toLowerCase() == username);
            if (matchingLogins.length)
                selectedLogin = matchingLogins[0];
            else
                this.log("Password not filled. None of the stored " +
                         "logins match the username already present.");
        } else if (logins.length == 1) {
            selectedLogin = logins[0];
        } else {
            
            
            
            
            let matchingLogins;
            if (usernameField)
                matchingLogins = logins.filter(function(l) l.username);
            else
                matchingLogins = logins.filter(function(l) !l.username);
            if (matchingLogins.length == 1)
                selectedLogin = matchingLogins[0];
            else
                this.log("Multiple logins for form, so not filling any.");
        }

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        return { canFillForm:   true,
                 usernameField: usernameField,
                 passwordField: passwordField,
                 foundLogins:   foundLogins,
                 selectedLogin: selectedLogin };
    }

}; 

Authenticator.init();
