



this.EXPORTED_SYMBOLS = ["LoginManagerContent"];

const Ci = Components.interfaces;
const Cr = Components.results;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");


var gEnabled, gDebug, gAutofillForms, gStoreWhenAutocompleteOff;

function log(...pieces) {
    function generateLogMessage(args) {
        let strings = ['Login Manager (content):'];

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

    if (!gDebug)
        return;

    let message = generateLogMessage(pieces);
    dump(message + "\n");
    Services.console.logStringMessage(message);
}


var observer = {
    QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver,
                                            Ci.nsIFormSubmitObserver,
                                            Ci.nsISupportsWeakReference]),

    
    notify : function (formElement, aWindow, actionURI) {
        log("observer notified for form submission.");

        
        

        try {
            LoginManagerContent._onFormSubmit(formElement);
        } catch (e) {
            log("Caught error in onFormSubmit:", e);
        }

        return true; 
    },

    onPrefChange : function() {
        gDebug = Services.prefs.getBoolPref("signon.debug");
        gEnabled = Services.prefs.getBoolPref("signon.rememberSignons");
        gAutofillForms = Services.prefs.getBoolPref("signon.autofillForms");
        gStoreWhenAutocompleteOff = Services.prefs.getBoolPref("signon.storeWhenAutocompleteOff");
    },
};

Services.obs.addObserver(observer, "earlyformsubmit", false);
var prefBranch = Services.prefs.getBranch("signon.");
prefBranch.addObserver("", observer.onPrefChange, false);

observer.onPrefChange(); 


var LoginManagerContent = {

    __formFillService : null, 
    get _formFillService() {
        if (!this.__formFillService)
            this.__formFillService =
                            Cc["@mozilla.org/satchel/form-fill-controller;1"].
                            getService(Ci.nsIFormFillController);
        return this.__formFillService;
    },

    




    onFormPassword: function (event) {
      if (!event.isTrusted)
          return;

      if (!gEnabled)
          return;

      let form = event.target;
      let doc = form.ownerDocument;

      log("onFormPassword for", doc.documentURI);

      
      let formOrigin = LoginUtils._getPasswordOrigin(doc.documentURI);
      if (!Services.logins.countLogins(formOrigin, "", null))
          return;

      
      
      if (Services.logins.uiBusy) {
        log("deferring onFormPassword for", doc.documentURI);
        let self = this;
        let observer = {
            QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupportsWeakReference]),

            observe: function (subject, topic, data) {
                log("Got deferred onFormPassword notification:", topic);
                
                Services.obs.removeObserver(this, "passwordmgr-crypto-login");
                Services.obs.removeObserver(this, "passwordmgr-crypto-loginCanceled");
                if (topic == "passwordmgr-crypto-loginCanceled")
                    return;
                self.onFormPassword(event);
            },
            handleEvent : function (event) {
                
            }
        };
        
        
        
        
        Services.obs.addObserver(observer, "passwordmgr-crypto-login", true);
        Services.obs.addObserver(observer, "passwordmgr-crypto-loginCanceled", true);
        form.addEventListener("mozCleverClosureHack", observer);
        return;
      }

      let autofillForm = gAutofillForms && !PrivateBrowsingUtils.isWindowPrivate(doc.defaultView);

      this._fillForm(form, autofillForm, false, false, false, null);
    },


    




    onUsernameInput : function(event) {
        if (!event.isTrusted)
            return;

        if (!gEnabled)
            return;

        var acInputField = event.target;

        
        if (!(acInputField.ownerDocument instanceof Ci.nsIDOMHTMLDocument))
            return;

        if (!this._isUsernameFieldType(acInputField))
            return;

        var acForm = acInputField.form;
        if (!acForm)
            return;

        
        
        
        if (!acInputField.value)
            return;

        log("onUsernameInput from", event.type);

        
        
        var [usernameField, passwordField, ignored] =
            this._getFormFields(acForm, false);
        if (usernameField == acInputField && passwordField) {
            
            
            if (!Services.logins.isLoggedIn)
                return;

            this._fillForm(acForm, true, true, true, true, null);
        } else {
            
        }
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
            log("(form ignored -- too many password fields. [ got ",
                        pwFields.length, "])");
            return null;
        }

        return pwFields;
    },


    _isUsernameFieldType: function(element) {
        if (!(element instanceof Ci.nsIDOMHTMLInputElement))
            return false;

        let fieldType = (element.hasAttribute("type") ?
                         element.getAttribute("type").toLowerCase() :
                         element.type);
        if (fieldType == "text"  ||
            fieldType == "email" ||
            fieldType == "url"   ||
            fieldType == "tel"   ||
            fieldType == "number") {
            return true;
        }
        return false;
    },


    















    _getFormFields : function (form, isSubmission) {
        var usernameField = null;

        
        
        var pwFields = this._getPasswordFields(form, isSubmission);
        if (!pwFields)
            return [null, null, null];


        
        
        
        
        for (var i = pwFields[0].index - 1; i >= 0; i--) {
            var element = form.elements[i];
            if (this._isUsernameFieldType(element)) {
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

        
        if (!gEnabled)
            return;

        var hostname = LoginUtils._getPasswordOrigin(doc.documentURI);
        if (!hostname) {
            log("(form submission ignored -- invalid hostname)");
            return;
        }

        
        
        let topWin = win.top;
        if (/^about:accounts($|\?)/i.test(topWin.document.documentURI)) {
            log("(form submission ignored -- about:accounts)");
            return;
        }

        var formSubmitURL = LoginUtils._getActionOrigin(form)
        if (!Services.logins.getLoginSavingEnabled(hostname)) {
            log("(form submission ignored -- saving is disabled for:", hostname, ")");
            return;
        }


        
        var [usernameField, newPasswordField, oldPasswordField] =
            this._getFormFields(form, true);

        
        if (newPasswordField == null)
                return;

        
        
        
        
        if ((this._isAutocompleteDisabled(form) ||
             this._isAutocompleteDisabled(usernameField) ||
             this._isAutocompleteDisabled(newPasswordField) ||
             this._isAutocompleteDisabled(oldPasswordField)) &&
            !gStoreWhenAutocompleteOff) {
                log("(form submission ignored -- autocomplete=off found)");
                return;
        }


        var formLogin = Cc["@mozilla.org/login-manager/loginInfo;1"].
                        createInstance(Ci.nsILoginInfo);
        formLogin.init(hostname, formSubmitURL, null,
                    (usernameField ? usernameField.value : ""),
                    newPasswordField.value,
                    (usernameField ? usernameField.name  : ""),
                    newPasswordField.name);

        
        
        
        if (!usernameField && oldPasswordField) {

            var logins = Services.logins.findLogins({}, hostname, formSubmitURL, null);

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
        var logins = Services.logins.findLogins({}, hostname, formSubmitURL, null);

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
                Services.logins.modifyLogin(existingLogin, propBag);
            }

            return;
        }


        
        prompter = getPrompter(win);
        prompter.promptToSavePassword(formLogin);
    },


    















    _fillForm : function (form, autofillForm, ignoreAutocomplete,
                          clobberPassword, userTriggered, foundLogins) {
        
        
        
        
        var [usernameField, passwordField, ignored] =
            this._getFormFields(form, false);

        
        if (passwordField == null)
            return [false, foundLogins];

        
        if (passwordField.disabled || passwordField.readOnly) {
            log("not filling form, password field disabled or read-only");
            return [false, foundLogins];
        }

        
        if (foundLogins == null) {
            var formOrigin =
                LoginUtils._getPasswordOrigin(form.ownerDocument.documentURI);
            var actionOrigin = LoginUtils._getActionOrigin(form);
            foundLogins = Services.logins.findLogins({}, formOrigin, actionOrigin, null);
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
            this._formFillService.markAsLoginManagerField(usernameField);

        
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

        if (usernameField && (usernameField.value || usernameField.disabled || usernameField.readOnly)) {
            
            
            var username = usernameField.value.toLowerCase();

            let matchingLogins = logins.filter(function(l)
                                     l.username.toLowerCase() == username);
            if (matchingLogins.length) {
                
                for (let l of matchingLogins) {
                    if (l.username == usernameField.value) {
                        selectedLogin = l;
                    }
                }
                
                if (!selectedLogin) {
                  selectedLogin = matchingLogins[0];
                }
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
            

            if (usernameField) {
                
                let disabledOrReadOnly = usernameField.disabled || usernameField.readOnly;

                
                
                
                dump("field value: " + usernameField.value + "\n");
                dump("selectedLogin value: " + selectedLogin.username + "\n");
                let userEnteredDifferentCase = userTriggered &&
                      (usernameField.value != selectedLogin.username &&
                       usernameField.value.toLowerCase() == selectedLogin.username.toLowerCase());
    
                if (!disabledOrReadOnly && !userEnteredDifferentCase) {
                    usernameField.value = selectedLogin.username;
                }
            }
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

};




LoginUtils = {
    




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

};
