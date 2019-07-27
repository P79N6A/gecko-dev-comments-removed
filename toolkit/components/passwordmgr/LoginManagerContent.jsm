



"use strict";

this.EXPORTED_SYMBOLS = [ "LoginManagerContent",
                          "UserAutoCompleteResult" ];

const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LoginRecipesContent",
                                  "resource://gre/modules/LoginRecipes.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LoginHelper",
                                  "resource://gre/modules/LoginHelper.jsm");

XPCOMUtils.defineLazyGetter(this, "log", () => {
  let logger = LoginHelper.createLogger("LoginManagerContent");
  return logger.log.bind(logger);
});


var gEnabled, gAutofillForms, gStoreWhenAutocompleteOff;

var observer = {
  QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver,
                                          Ci.nsIFormSubmitObserver,
                                          Ci.nsISupportsWeakReference]),

  
  notify : function (formElement, aWindow, actionURI) {
    log("observer notified for form submission.");

    
    

    try {
      let formLike = FormLikeFactory.createFromForm(formElement);
      LoginManagerContent._onFormSubmit(formLike);
    } catch (e) {
      log("Caught error in onFormSubmit(", e.lineNumber, "):", e.message);
      Cu.reportError(e);
    }

    return true; 
  },

  onPrefChange : function() {
    gEnabled = Services.prefs.getBoolPref("signon.rememberSignons");
    gAutofillForms = Services.prefs.getBoolPref("signon.autofillForms");
    gStoreWhenAutocompleteOff = Services.prefs.getBoolPref("signon.storeWhenAutocompleteOff");
  },
};

Services.obs.addObserver(observer, "earlyformsubmit", false);
var prefBranch = Services.prefs.getBranch("signon.");
prefBranch.addObserver("", observer.onPrefChange, false);

observer.onPrefChange(); 


function messageManagerFromWindow(win) {
  return win.QueryInterface(Ci.nsIInterfaceRequestor)
            .getInterface(Ci.nsIWebNavigation)
            .QueryInterface(Ci.nsIDocShell)
            .QueryInterface(Ci.nsIInterfaceRequestor)
            .getInterface(Ci.nsIContentFrameMessageManager)
}


var LoginManagerContent = {

  __formFillService : null, 
  get _formFillService() {
    if (!this.__formFillService)
      this.__formFillService =
                      Cc["@mozilla.org/satchel/form-fill-controller;1"].
                      getService(Ci.nsIFormFillController);
    return this.__formFillService;
  },

  _getRandomId: function() {
    return Cc["@mozilla.org/uuid-generator;1"]
             .getService(Ci.nsIUUIDGenerator).generateUUID().toString();
  },

  _messages: [ "RemoteLogins:loginsFound",
               "RemoteLogins:loginsAutoCompleted" ],

  
  _requests: new Map(),

  
  _managers: new Map(),

  _takeRequest: function(msg) {
    let data = msg.data;
    let request = this._requests.get(data.requestId);

    this._requests.delete(data.requestId);

    let count = this._managers.get(msg.target);
    if (--count === 0) {
      this._managers.delete(msg.target);

      for (let message of this._messages)
        msg.target.removeMessageListener(message, this);
    } else {
      this._managers.set(msg.target, count);
    }

    return request;
  },

  _sendRequest: function(messageManager, requestData,
                         name, messageData) {
    let count;
    if (!(count = this._managers.get(messageManager))) {
      this._managers.set(messageManager, 1);

      for (let message of this._messages)
        messageManager.addMessageListener(message, this);
    } else {
      this._managers.set(messageManager, ++count);
    }

    let requestId = this._getRandomId();
    messageData.requestId = requestId;

    messageManager.sendAsyncMessage(name, messageData);

    let deferred = Promise.defer();
    requestData.promise = deferred;
    this._requests.set(requestId, requestData);
    return deferred.promise;
  },

  receiveMessage: function (msg, window) {
    
    
    function jsLoginsToXPCOM(logins) {
      return logins.map(login => {
        var formLogin = Cc["@mozilla.org/login-manager/loginInfo;1"].
                      createInstance(Ci.nsILoginInfo);
        formLogin.init(login.hostname, login.formSubmitURL,
                       login.httpRealm, login.username,
                       login.password, login.usernameField,
                       login.passwordField);
        return formLogin;
      });
    }

    if (msg.name == "RemoteLogins:fillForm") {
      this.fillForm({
        topDocument: window.document,
        loginFormOrigin: msg.data.loginFormOrigin,
        loginsFound: jsLoginsToXPCOM(msg.data.logins),
        recipes: msg.data.recipes,
      });
      return;
    }

    let request = this._takeRequest(msg);
    switch (msg.name) {
      case "RemoteLogins:loginsFound": {
        let loginsFound = jsLoginsToXPCOM(msg.data.logins);
        request.promise.resolve({
          form: request.form,
          loginsFound: loginsFound,
          recipes: msg.data.recipes,
        });
        break;
      }

      case "RemoteLogins:loginsAutoCompleted": {
        let loginsFound = jsLoginsToXPCOM(msg.data.logins);
        request.promise.resolve(loginsFound);
        break;
      }
    }
  },

  






  _getLoginDataFromParent: function(form, options) {
    let doc = form.ownerDocument;
    let win = doc.defaultView;

    let formOrigin = LoginUtils._getPasswordOrigin(doc.documentURI);
    let actionOrigin = LoginUtils._getActionOrigin(form);

    let messageManager = messageManagerFromWindow(win);

    
    let requestData = { form: form };
    let messageData = { formOrigin: formOrigin,
                        actionOrigin: actionOrigin,
                        options: options };

    return this._sendRequest(messageManager, requestData,
                             "RemoteLogins:findLogins",
                             messageData);
  },

  _autoCompleteSearchAsync: function(aSearchString, aPreviousResult,
                                     aElement, aRect) {
    let doc = aElement.ownerDocument;
    let form = aElement.form;
    let win = doc.defaultView;

    let formOrigin = LoginUtils._getPasswordOrigin(doc.documentURI);
    let actionOrigin = LoginUtils._getActionOrigin(form);

    let messageManager = messageManagerFromWindow(win);

    let remote = (Services.appinfo.processType ===
                  Services.appinfo.PROCESS_TYPE_CONTENT);

    let requestData = {};
    let messageData = { formOrigin: formOrigin,
                        actionOrigin: actionOrigin,
                        searchString: aSearchString,
                        previousResult: aPreviousResult,
                        rect: aRect,
                        remote: remote };

    return this._sendRequest(messageManager, requestData,
                             "RemoteLogins:autoCompleteLogins",
                             messageData);
  },

  onDOMFormHasPassword(event, window) {
    if (!event.isTrusted) {
      return;
    }

    let form = event.target;

    
    this.stateForDocument(form.ownerDocument).loginForm = form;

    this._updateLoginFormPresence(window);

    let messageManager = messageManagerFromWindow(window);
    messageManager.sendAsyncMessage("LoginStats:LoginEncountered");

    if (!gEnabled) {
      return;
    }

    log("onDOMFormHasPassword for", form.ownerDocument.documentURI);
    this._getLoginDataFromParent(form, { showMasterPassword: true })
        .then(this.loginsFound.bind(this))
        .then(null, Cu.reportError);
  },

  onPageShow(event, window) {
    this._updateLoginFormPresence(window);
  },

  



  loginFormStateByDocument: new WeakMap(),

  



  stateForDocument(document) {
    let loginFormState = this.loginFormStateByDocument.get(document);
    if (!loginFormState) {
      loginFormState = {};
      this.loginFormStateByDocument.set(document, loginFormState);
    }
    return loginFormState;
  },

  




  _updateLoginFormPresence(topWindow) {
    
    
    
    let loginFormOrigin =
        LoginUtils._getPasswordOrigin(topWindow.document.documentURI);

    
    
    let getFirstLoginForm = thisWindow => {
      let loginForm = this.stateForDocument(thisWindow.document).loginForm;
      if (loginForm) {
        return loginForm;
      }
      for (let i = 0; i < thisWindow.frames.length; i++) {
        let frame = thisWindow.frames[i];
        if (LoginUtils._getPasswordOrigin(frame.document.documentURI) !=
            loginFormOrigin) {
          continue;
        }
        let loginForm = getFirstLoginForm(frame);
        if (loginForm) {
          return loginForm;
        }
      }
      return null;
    };

    
    let topState = this.stateForDocument(topWindow.document);
    topState.loginFormForFill = getFirstLoginForm(topWindow);

    
    let messageManager = messageManagerFromWindow(topWindow);
    messageManager.sendAsyncMessage("RemoteLogins:updateLoginFormPresence", {
      loginFormOrigin,
      loginFormPresent: !!topState.loginFormForFill,
    });
  },

  





















  fillForm({ topDocument, loginFormOrigin, loginsFound, recipes }) {
    let topState = this.stateForDocument(topDocument);
    if (!topState.loginFormForFill) {
      log("fillForm: There is no login form anymore. The form may have been",
          "removed or the document may have changed.");
      return;
    }
    if (LoginUtils._getPasswordOrigin(topDocument.documentURI) !=
        loginFormOrigin) {
      log("fillForm: The requested origin doesn't match the one form the",
          "document. This may mean we navigated to a document from a different",
          "site before we had a chance to indicate this change in the user",
          "interface.");
      return;
    }
    this._fillForm(topState.loginFormForFill, true, true, true, true,
                   loginsFound, recipes);
  },

  loginsFound: function({ form, loginsFound, recipes }) {
    let doc = form.ownerDocument;
    let autofillForm = gAutofillForms && !PrivateBrowsingUtils.isContentWindowPrivate(doc.defaultView);

    this._fillForm(form, autofillForm, false, false, false, loginsFound, recipes);
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
      this._getLoginDataFromParent(acForm, { showMasterPassword: false })
          .then(({ form, loginsFound, recipes }) => {
            this._fillForm(form, true, false, true, true, loginsFound, recipes);
          })
          .then(null, Cu.reportError);
    } else {
      
    }
  },

  








  _getPasswordFields(form, skipEmptyFields = false) {
    
    let pwFields = [];
    for (let i = 0; i < form.elements.length; i++) {
      let element = form.elements[i];
      if (!(element instanceof Ci.nsIDOMHTMLInputElement) ||
          element.type != "password") {
        continue;
      }

      if (skipEmptyFields && !element.value) {
        continue;
      }

      pwFields[pwFields.length] = {
                                    index   : i,
                                    element : element
                                  };
    }

    
    if (pwFields.length == 0) {
      log("(form ignored -- no password fields.)");
      return null;
    } else if (pwFields.length > 3) {
      log("(form ignored -- too many password fields. [ got ", pwFields.length, "])");
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


  
















  _getFormFields : function (form, isSubmission, recipes) {
    var usernameField = null;
    var pwFields = null;
    var fieldOverrideRecipe = LoginRecipesContent.getFieldOverrides(recipes, form);
    if (fieldOverrideRecipe) {
      var pwOverrideField = LoginRecipesContent.queryLoginField(
        form,
        fieldOverrideRecipe.passwordSelector
      );
      if (pwOverrideField) {
        pwFields = [{
          index   : [...pwOverrideField.form.elements].indexOf(pwOverrideField),
          element : pwOverrideField,
        }];
      }

      var usernameOverrideField = LoginRecipesContent.queryLoginField(
        form,
        fieldOverrideRecipe.usernameSelector
      );
      if (usernameOverrideField) {
        usernameField = usernameOverrideField;
      }
    }

    if (!pwFields) {
      
      
      pwFields = this._getPasswordFields(form, isSubmission);
    }

    if (!pwFields) {
      return [null, null, null];
    }

    if (!usernameField) {
      
      
      
      
      for (var i = pwFields[0].index - 1; i >= 0; i--) {
        var element = form.elements[i];
        if (this._isUsernameFieldType(element)) {
          usernameField = element;
          break;
        }
      }
    }

    if (!usernameField)
      log("(form -- no username field found)");
    else
      log("Username field ", usernameField, "has name/value:",
          usernameField.name, "/", usernameField.value);

    
    
    
    if (!isSubmission || pwFields.length == 1) {
      var passwordField = pwFields[0].element;
      log("Password field", passwordField, "has name: ", passwordField.name);
      return [usernameField, passwordField, null];
    }


    
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

    log("Password field (new) id/name is: ", newPasswordField.id, " / ", newPasswordField.name);
    log("Password field (old) id/name is: ", oldPasswordField.id, " / ", oldPasswordField.name);
    return [usernameField, newPasswordField, oldPasswordField];
  },


  



  _isAutocompleteDisabled(element) {
    return element && element.autocomplete == "off";
  },

  







  _onFormSubmit(form) {
    var doc = form.ownerDocument;
    var win = doc.defaultView;

    if (PrivateBrowsingUtils.isContentWindowPrivate(win)) {
      
      
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

    let formSubmitURL = LoginUtils._getActionOrigin(form);
    let messageManager = messageManagerFromWindow(win);

    let recipesArray = messageManager.sendSyncMessage("RemoteLogins:findRecipes", {
      formOrigin: hostname,
    })[0];
    let recipes = new Set(recipesArray);

    
    var [usernameField, newPasswordField, oldPasswordField] =
          this._getFormFields(form, true, recipes);

    
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

    
    let mockUsername = usernameField ?
                         { name: usernameField.name,
                           value: usernameField.value } :
                         null;
    let mockPassword = { name: newPasswordField.name,
                         value: newPasswordField.value };
    let mockOldPassword = oldPasswordField ?
                            { name: oldPasswordField.name,
                              value: oldPasswordField.value } :
                            null;

    
    let opener = win.opener ? win.opener.top : null;

    messageManager.sendAsyncMessage("RemoteLogins:onFormSubmit",
                                    { hostname: hostname,
                                      formSubmitURL: formSubmitURL,
                                      usernameField: mockUsername,
                                      newPasswordField: mockPassword,
                                      oldPasswordField: mockOldPassword },
                                    { openerWin: opener });
  },

  














  _fillForm : function (form, autofillForm, clobberUsername, clobberPassword,
                        userTriggered, foundLogins, recipes) {
    let ignoreAutocomplete = true;
    const AUTOFILL_RESULT = {
      FILLED: 0,
      NO_PASSWORD_FIELD: 1,
      PASSWORD_DISABLED_READONLY: 2,
      NO_LOGINS_FIT: 3,
      NO_SAVED_LOGINS: 4,
      EXISTING_PASSWORD: 5,
      EXISTING_USERNAME: 6,
      MULTIPLE_LOGINS: 7,
      NO_AUTOFILL_FORMS: 8,
      AUTOCOMPLETE_OFF: 9,
    };

    function recordAutofillResult(result) {
      if (userTriggered) {
        
        return;
      }
      const autofillResultHist = Services.telemetry.getHistogramById("PWMGR_FORM_AUTOFILL_RESULT");
      autofillResultHist.add(result);
    }

    try {
      
      if (foundLogins.length == 0) {
        
        recordAutofillResult(AUTOFILL_RESULT.NO_SAVED_LOGINS);
        return;
      }

      
      
      
      
      var [usernameField, passwordField, ignored] =
            this._getFormFields(form, false, recipes);

      
      if (passwordField == null) {
        log("not filling form, no password field found");
        recordAutofillResult(AUTOFILL_RESULT.NO_PASSWORD_FIELD);
        return;
      }

      
      if (passwordField.disabled || passwordField.readOnly) {
        log("not filling form, password field disabled or read-only");
        recordAutofillResult(AUTOFILL_RESULT.PASSWORD_DISABLED_READONLY);
        return;
      }

      var isAutocompleteOff = false;
      if (this._isAutocompleteDisabled(form) ||
          this._isAutocompleteDisabled(usernameField) ||
          this._isAutocompleteDisabled(passwordField)) {
        isAutocompleteOff = true;
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

      if (logins.length == 0) {
        log("form not filled, none of the logins fit in the field");
        recordAutofillResult(AUTOFILL_RESULT.NO_LOGINS_FIT);
        return;
      }

      
      
      
      if (usernameField)
        this._formFillService.markAsLoginManagerField(usernameField);

      
      if (passwordField.value && !clobberPassword) {
        log("form not filled, the password field was already filled");
        recordAutofillResult(AUTOFILL_RESULT.EXISTING_PASSWORD);
        return;
      }

      
      var selectedLogin;
      if (!clobberUsername && usernameField && (usernameField.value ||
                                                usernameField.disabled ||
                                                usernameField.readOnly)) {
        
        
        var username = usernameField.value.toLowerCase();

        let matchingLogins = logins.filter(function(l)
                                           l.username.toLowerCase() == username);
        if (matchingLogins.length == 0) {
          log("Password not filled. None of the stored logins match the username already present.");
          recordAutofillResult(AUTOFILL_RESULT.EXISTING_USERNAME);
          return;
        }

        
        for (let l of matchingLogins) {
          if (l.username == usernameField.value) {
            selectedLogin = l;
          }
        }
        
        if (!selectedLogin) {
          selectedLogin = matchingLogins[0];
        }
      } else if (logins.length == 1) {
        selectedLogin = logins[0];
      } else {
        
        
        
        
        let matchingLogins;
        if (usernameField)
          matchingLogins = logins.filter(function(l) l.username);
        else
          matchingLogins = logins.filter(function(l) !l.username);

        if (matchingLogins.length != 1) {
          log("Multiple logins for form, so not filling any.");
          recordAutofillResult(AUTOFILL_RESULT.MULTIPLE_LOGINS);
          return;
        }

        selectedLogin = matchingLogins[0];
      }

      

      if (!autofillForm) {
        log("autofillForms=false but form can be filled");
        recordAutofillResult(AUTOFILL_RESULT.NO_AUTOFILL_FORMS);
        return;
      }

      if (isAutocompleteOff && !ignoreAutocomplete) {
        log("Not filling the login because we're respecting autocomplete=off");
        recordAutofillResult(AUTOFILL_RESULT.AUTOCOMPLETE_OFF);
        return;
      }

      

      if (usernameField) {
      
        let disabledOrReadOnly = usernameField.disabled || usernameField.readOnly;

        let userNameDiffers = selectedLogin.username != usernameField.value;
        
        
        
        let userEnteredDifferentCase = userTriggered && userNameDiffers &&
               usernameField.value.toLowerCase() == selectedLogin.username.toLowerCase();

        if (!disabledOrReadOnly && !userEnteredDifferentCase && userNameDiffers) {
          usernameField.setUserInput(selectedLogin.username);
        }
      }
      if (passwordField.value != selectedLogin.password) {
        passwordField.setUserInput(selectedLogin.password);
      }

      log("_fillForm succeeded");
      recordAutofillResult(AUTOFILL_RESULT.FILLED);
      let doc = form.ownerDocument;
      let win = doc.defaultView;
      let messageManager = messageManagerFromWindow(win);
      messageManager.sendAsyncMessage("LoginStats:LoginFillSuccessful");
    } finally {
      Services.obs.notifyObservers(form, "passwordmgr-processed-form", null);
    }
  },

};

var LoginUtils = {
  




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
      throw new Error("Index out of range.");

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
        throw new Error("Index out of range.");

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





let FormLikeFactory = {
  _propsFromForm: [
    "autocomplete",
    "ownerDocument",
  ],

  






  createFromForm(aForm) {
    if (!(aForm instanceof Ci.nsIDOMHTMLFormElement)) {
      throw new Error("createFromForm: aForm must be a nsIDOMHTMLFormElement");
    }

    let formLike = {
      action: LoginUtils._getActionOrigin(aForm),
      elements: [...aForm.elements],
      rootElement: aForm,
    };

    for (let prop of this._propsFromForm) {
      formLike[prop] = aForm[prop];
    }

    this._addToJSONProperty(formLike);
    return formLike;
  },

  












  createFromPasswordField(aPasswordField) {
    if (!(aPasswordField instanceof Ci.nsIDOMHTMLInputElement) ||
        aPasswordField.type != "password" ||
        !aPasswordField.ownerDocument) {
      throw new Error("createFromPasswordField requires a password field in a document");
    }

    if (aPasswordField.form) {
      return this.createFromForm(aPasswordField.form);
    }

    let doc = aPasswordField.ownerDocument;
    log("Created non-form FormLike for rootElement:", doc.documentElement);
    let formLike = {
      action: LoginUtils._getPasswordOrigin(doc.baseURI),
      autocomplete: "on",
      
      
      elements: [for (el of doc.documentElement.querySelectorAll("input")) if (!el.form) el],
      ownerDocument: doc,
      rootElement: doc.documentElement,
    };

    this._addToJSONProperty(formLike);
    return formLike;
  },

  



  _addToJSONProperty(aFormLike) {
    function prettyElementOutput(aElement) {
      let idText = aElement.id ? "#" + aElement.id : "";
      let classText = [for (className of aElement.classList) "." + className].join("");
      return `<${aElement.nodeName + idText + classText}>`;
    }

    Object.defineProperty(aFormLike, "toJSON", {
      value: () => {
        let cleansed = {};
        for (let key of Object.keys(aFormLike)) {
          let value = aFormLike[key];
          let cleansedValue = value;

          switch (key) {
            case "elements": {
              cleansedValue = [for (element of value) prettyElementOutput(element)];
              break;
            }

            case "ownerDocument": {
              cleansedValue = {
                location: {
                  href: value.location.href,
                },
              };
              break;
            }

            case "rootElement": {
              cleansedValue = prettyElementOutput(value);
              break;
            }
          }

          cleansed[key] = cleansedValue;
        }
        return cleansed;
      }
    });
  },
};
