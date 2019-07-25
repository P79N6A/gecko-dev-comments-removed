




































var Cc = Components.classes;
var Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

var loginManager = {

    


    get _formFillService() {
        return this._formFillService =
                        Cc["@mozilla.org/satchel/form-fill-controller;1"].
                        getService(Ci.nsIFormFillController);
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

    _nsLoginInfo : null, 
    _debug    : false, 
    _remember : true,  

    init : function () {
        
        this._domEventListener._pwmgr    = this;
        this._observer._pwmgr            = this;

        
        Services.obs.addObserver(this._observer, "earlyformsubmit", false);

        
        addEventListener("pageshow", this._domEventListener, false);
        addEventListener("unload", this._domEventListener, false);

        
        this._nsLoginInfo = new Components.Constructor(
            "@mozilla.org/login-manager/loginInfo;1", Ci.nsILoginInfo);

        
        Services.prefs.addObserver("signon.", this._observer, false);

        
        this._debug = Services.prefs.getBoolPref("signon.debug");
        this._remember = Services.prefs.getBoolPref("signon.rememberSignons");
    },


    




    log : function (message) {
        if (!this._debug)
            return;
        dump("PasswordUtils: " + message + "\n");
        Services.console.logStringMessage("PasswordUtils: " + message);
    },


    



    fillForm: function (form) {
        this._fillForm(form, true, true, false, null);
    },


    







    _fillForm : function (form, autofillForm, ignoreAutocomplete,
                         clobberPassword, foundLogins) {
        
        
        
        
        var [usernameField, passwordField, ignored] =
            this._getFormFields(form, false);

        
        if (passwordField == null)
            return false;

        
        if (passwordField.disabled || passwordField.readOnly ||
            usernameField && (usernameField.disabled ||
                              usernameField.readOnly)) {
            this.log("not filling form, login fields disabled");
            return false;
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
                    this.log("Ignored " + l.username + " login: won't fit");

                return fit;
            }, this);


        
        if (logins.length == 0)
            return false;


        
        
        
        var didntFillReason = null;

        
        
        
        if (usernameField)
            this._attachToInput(usernameField);

        
        if (passwordField.value && !clobberPassword) {
            didntFillReason = "existingPassword";
            this._notifyFoundLogins(didntFillReason, usernameField,
                                    passwordField, foundLogins, null);
            return false;
        }

        
        
        
        
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
            if (matchingLogins.length) {
                selectedLogin = matchingLogins[0];
            } else {
                didntFillReason = "existingUsername";
                this.log("Password not filled. None of the stored " +
                         "logins match the username already present.");
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
                this.log("Multiple logins for form, so not filling any.");
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
            this.log("autofillForms=false but form can be filled; notified observers");
        } else if (selectedLogin && isFormDisabled) {
            
            
            didntFillReason = "autocompleteOff";
            Services.obs.notifyObservers(form, "passwordmgr-found-form", didntFillReason);
            this.log("autocomplete=off but form can be filled; notified observers");
        }

        this._notifyFoundLogins(didntFillReason, usernameField, passwordField,
                                foundLogins, selectedLogin);

        return didFillForm;
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


    





    _isAutocompleteDisabled :  function (element) {
        if (element && element.hasAttribute("autocomplete") &&
            element.getAttribute("autocomplete").toLowerCase() == "off")
            return true;

        return false;
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
            this.log("(form ignored -- no password fields.)");
            return null;
        } else if (pwFields.length > 3) {
            this.log("(form ignored -- too many password fields. [got " +
                        pwFields.length + "])");
            return null;
        }

        return pwFields;
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

    






    _attachToInput : function (element) {
        this.log("attaching autocomplete stuff");
        element.addEventListener("blur",
                                this._domEventListener, false);
        element.addEventListener("DOMAutoComplete",
                                this._domEventListener, false);
        this._formFillService.markAsLoginManagerField(element);
    },

    






    _fillDocument : function (doc) {
        var forms = doc.forms;
        if (!forms || forms.length == 0)
            return;

        this.log("_fillDocument processing " + forms.length +
                 " forms on " + doc.documentURI);

        var autofillForm = !this._inPrivateBrowsing &&
                           Services.prefs.getBoolPref("signon.autofillForms");

        
        
        
        var actionOrigins = [];

        for (var i = 0; i < forms.length; i++) {
            var form = forms[i];
            let [, passwordField, ] = this._getFormFields(form, false);
            if (passwordField) {
                var actionOrigin = this._getActionOrigin(form);
                actionOrigins.push(actionOrigin);
            }
        } 

        if (!actionOrigins.length)
            return;

        var formOrigin = this._getPasswordOrigin(doc.documentURI);
        var foundLogins = this._getPasswords(actionOrigins, formOrigin);

        for (var i = 0; i < forms.length; i++) {
            var form = forms[i];
            var actionOrigin = this._getActionOrigin(form);
            if (foundLogins[actionOrigin]) {
                this.log("_fillDocument processing form[" + i + "]");
                this._fillForm(form, autofillForm, false, false,
                               foundLogins[actionOrigin]);
            }
        } 
    },

    





    _getPasswords: function(actionOrigins, formOrigin) {
        
        var message = sendSyncMessage("PasswordMgr:GetPasswords", {
            actionOrigins: actionOrigins,
            formOrigin: formOrigin
        })[0];

        
        

        var foundLogins = message.foundLogins;

        
        
        for (var key in foundLogins) {
            var logins = foundLogins[key];
            for (var i = 0; i < logins.length; i++) {
                var obj = logins[i];
                logins[i] = new this._nsLoginInfo();
                logins[i].init(obj.hostname, obj.formSubmitURL, obj.httpRealm,
                               obj.username, obj.password,
                               obj.usernameField, obj.passwordField);
            }
        }

        return foundLogins;
    },

    







    _onFormSubmit : function (form) {
        if (this._inPrivateBrowsing) {
            
            
            this.log("(form submission ignored in private browsing mode)");
            return;
        }

        var doc = form.ownerDocument;

        
        if (!this._remember)
            return;

        var hostname      = this._getPasswordOrigin(doc.documentURI);
        var formSubmitURL = this._getActionOrigin(form);


        
        var [usernameField, newPasswordField, oldPasswordField] =
            this._getFormFields(form, true);

        
        if (newPasswordField == null)
            return;

        
        
        
        
        if (this._isAutocompleteDisabled(form) ||
            this._isAutocompleteDisabled(usernameField) ||
            this._isAutocompleteDisabled(newPasswordField) ||
            this._isAutocompleteDisabled(oldPasswordField)) {
                this.log("(form submission ignored -- autocomplete=off found)");
                return;
        }

        sendSyncMessage("PasswordMgr:FormSubmitted", {
            hostname: hostname,
            formSubmitURL: formSubmitURL,
            usernameField: usernameField ? usernameField.name : "",
            usernameValue: usernameField ? usernameField.value : "",
            passwordField: newPasswordField.name,
            passwordValue: newPasswordField.value,
            hasOldPasswordField: !!oldPasswordField
        });
    },

    

    





    _observer : {
        _pwmgr : null,

        QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver,
                                                Ci.nsIFormSubmitObserver,
                                                Ci.nsISupportsWeakReference]),


        
        notify : function (formElement, aWindow, actionURI) {
            
            
            if (aWindow.top != content)
                return true;

            this._pwmgr.log("observer notified for form submission.");

            
            

            try {
                this._pwmgr._onFormSubmit(formElement);
            } catch (e) {
                this._pwmgr.log("Caught error in onFormSubmit: " + e);
            }

            return true; 
        },

        observe : function (aSubject, aTopic, aData) {
          this._pwmgr._debug    = Services.prefs.getBoolPref("signon.debug");
          this._pwmgr._remember = Services.prefs.getBoolPref("signon.rememberSignons");
        }
    },


    





    _domEventListener : {
        _pwmgr : null,

        QueryInterface : XPCOMUtils.generateQI([Ci.nsIDOMEventListener,
                                                Ci.nsISupportsWeakReference]),


        handleEvent : function (event) {
            if (!event.isTrusted)
                return;

            this._pwmgr.log("domEventListener: got event " + event.type);

            switch (event.type) {
                case "DOMAutoComplete":
                case "blur":
                    var acInputField = event.target;
                    var acForm = acInputField.form;

                    
                    
                    
                    if (!acInputField.value)
                        return;

                    
                    
                    
                    var [usernameField, passwordField, ignored] =
                        this._pwmgr._getFormFields(acForm, false);
                    if (usernameField == acInputField && passwordField) {
                        var actionOrigin = this._pwmgr._getActionOrigin(acForm);
                        var formOrigin = this._pwmgr._getPasswordOrigin(acForm.ownerDocument.documentURI);
                        var foundLogins = this._pwmgr._getPasswords([actionOrigin], formOrigin);
                        this._pwmgr._fillForm(acForm, true, true, true, foundLogins[actionOrigin]);
                    } else {
                        this._pwmgr.log("Oops, form changed before AC invoked");
                    }
                    return;

                case "pageshow":
                    
                    if (this._pwmgr._remember && event.target instanceof Ci.nsIDOMHTMLDocument)
                        this._pwmgr._fillDocument(event.target);
                    break;

                case "unload":
                    Services.prefs.removeObserver("signon.", this._pwmgr._observer);
                    Services.obs.removeObserver(this._pwmgr._observer, "earlyformsubmit");
                    break;

                default:
                    this._pwmgr.log("Oops! This event unexpected.");
                    return;
            }
        }
    }
};

loginManager.init();
