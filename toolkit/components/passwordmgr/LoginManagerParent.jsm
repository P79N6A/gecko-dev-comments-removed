



"use strict";

const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

Cu.importGlobalProperties(["URL"]);
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "UserAutoCompleteResult",
                                  "resource://gre/modules/LoginManagerContent.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AutoCompleteE10S",
                                  "resource://gre/modules/AutoCompleteE10S.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DeferredTask",
                                  "resource://gre/modules/DeferredTask.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "LoginDoorhangers",
                                  "resource://gre/modules/LoginDoorhangers.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "LoginHelper",
                                  "resource://gre/modules/LoginHelper.jsm");

XPCOMUtils.defineLazyGetter(this, "log", () => {
  let logger = LoginHelper.createLogger("LoginManagerParent");
  return logger.log.bind(logger);
});

this.EXPORTED_SYMBOLS = [ "LoginManagerParent", "PasswordsMetricsProvider" ];

#ifndef ANDROID
#ifdef MOZ_SERVICES_HEALTHREPORT
XPCOMUtils.defineLazyModuleGetter(this, "Metrics",
                                  "resource://gre/modules/Metrics.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

function recordFHRDailyCounter(aField) {
    let reporter = Cc["@mozilla.org/datareporting/service;1"]
                      .getService()
                      .wrappedJSObject
                      .healthReporter;
    
    
    if (!reporter) {
      return;
    }
      reporter.onInit().then(() => reporter.getProvider("org.mozilla.passwordmgr")
        .recordDailyCounter(aField));
  }

this.PasswordsMetricsProvider = function() {
  Metrics.Provider.call(this);
};

PasswordsMetricsProvider.prototype = Object.freeze({
  __proto__: Metrics.Provider.prototype,

  name: "org.mozilla.passwordmgr",

  measurementTypes: [
    PasswordsMeasurement1,
    PasswordsMeasurement2,
  ],

  collectDailyData: function () {
    return this.storage.enqueueTransaction(this._recordDailyPasswordData.bind(this));
  },

  _recordDailyPasswordData: function *() {
    let m = this.getMeasurement(PasswordsMeasurement2.prototype.name,
                                PasswordsMeasurement2.prototype.version);
    let enabled = Services.prefs.getBoolPref("signon.rememberSignons");
    yield m.setDailyLastNumeric("enabled", enabled ? 1 : 0);

    let loginsCount = Services.logins.countLogins("", "", "");
    yield m.setDailyLastNumeric("numSavedPasswords", loginsCount);

  },

  recordDailyCounter: function(aField) {
    let m = this.getMeasurement(PasswordsMeasurement2.prototype.name,
                                PasswordsMeasurement2.prototype.version);
    if (this.storage.hasFieldFromMeasurement(m.id, aField,
                                             Metrics.Storage.FIELD_DAILY_COUNTER)) {
      let fieldID = this.storage.fieldIDFromMeasurement(m.id, aField, Metrics.Storage.FIELD_DAILY_COUNTER);
      return this.enqueueStorageOperation(() => m.incrementDailyCounter(aField));
    }

    
    return this.enqueueStorageOperation (() => this.storage.registerField(m.id, aField, 
      Metrics.Storage.FIELD_DAILY_COUNTER).then(() => m.incrementDailyCounter(aField)));
  },
});

function PasswordsMeasurement1() {
  Metrics.Measurement.call(this);
}

PasswordsMeasurement1.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,
  name: "passwordmgr",
  version: 1,
  fields: {
    enabled: {type: Metrics.Storage.FIELD_DAILY_LAST_NUMERIC},
    numSavedPasswords: {type: Metrics.Storage.FIELD_DAILY_LAST_NUMERIC},
  },
});

function PasswordsMeasurement2() {
  Metrics.Measurement.call(this);
}
PasswordsMeasurement2.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,
  name: "passwordmgr",
  version: 2,
  fields: {
    enabled: {type: Metrics.Storage.FIELD_DAILY_LAST_NUMERIC},
    numSavedPasswords: {type: Metrics.Storage.FIELD_DAILY_LAST_NUMERIC},
    numSuccessfulFills: {type: Metrics.Storage.FIELD_DAILY_COUNTER},
    numNewSavedPasswordsInSession: {type: Metrics.Storage.FIELD_DAILY_COUNTER},
    numTotalLoginsEncountered: {type: Metrics.Storage.FIELD_DAILY_COUNTER},
  },
});

#endif
#endif

var LoginManagerParent = {
  







  _recipeManager: null,

  init: function() {
    let mm = Cc["@mozilla.org/globalmessagemanager;1"]
               .getService(Ci.nsIMessageListenerManager);
    mm.addMessageListener("RemoteLogins:findLogins", this);
    mm.addMessageListener("RemoteLogins:findRecipes", this);
    mm.addMessageListener("RemoteLogins:onFormSubmit", this);
    mm.addMessageListener("RemoteLogins:autoCompleteLogins", this);
    mm.addMessageListener("RemoteLogins:updateLoginFormPresence", this);
    mm.addMessageListener("LoginStats:LoginEncountered", this);
    mm.addMessageListener("LoginStats:LoginFillSuccessful", this);
    Services.obs.addObserver(this, "LoginStats:NewSavedPassword", false);

    XPCOMUtils.defineLazyGetter(this, "recipeParentPromise", () => {
      const { LoginRecipesParent } = Cu.import("resource://gre/modules/LoginRecipes.jsm", {});
      this._recipeManager = new LoginRecipesParent();
      return this._recipeManager.initializationPromise;
    });

  },

  observe: function (aSubject, aTopic, aData) {
#ifndef ANDROID
#ifdef MOZ_SERVICES_HEALTHREPORT
    if (aTopic == "LoginStats:NewSavedPassword") {
      recordFHRDailyCounter("numNewSavedPasswordsInSession");

    }
#endif
#endif
  },

  receiveMessage: function (msg) {
    let data = msg.data;
    switch (msg.name) {
      case "RemoteLogins:findLogins": {
        
        this.sendLoginDataToChild(data.options.showMasterPassword,
                                  data.formOrigin,
                                  data.actionOrigin,
                                  data.requestId,
                                  msg.target.messageManager);
        break;
      }

      case "RemoteLogins:findRecipes": {
        let formHost = (new URL(data.formOrigin)).host;
        return this._recipeManager.getRecipesForHost(formHost);
      }

      case "RemoteLogins:onFormSubmit": {
        
        this.onFormSubmit(data.hostname,
                          data.formSubmitURL,
                          data.usernameField,
                          data.newPasswordField,
                          data.oldPasswordField,
                          msg.objects.openerWin,
                          msg.target);
        break;
      }

      case "RemoteLogins:updateLoginFormPresence": {
        this.updateLoginFormPresence(msg.target, data);
        break;
      }

      case "RemoteLogins:autoCompleteLogins": {
        this.doAutocompleteSearch(data, msg.target);
        break;
      }

      case "LoginStats:LoginFillSuccessful": {
#ifndef ANDROID
#ifdef MOZ_SERVICES_HEALTHREPORT
        recordFHRDailyCounter("numSuccessfulFills");
#endif
#endif
        break;
      }

      case "LoginStats:LoginEncountered": {
#ifndef ANDROID
#ifdef MOZ_SERVICES_HEALTHREPORT
        recordFHRDailyCounter("numTotalLoginsEncountered");
#endif
#endif
        break;
      }
    }
  },

  



  fillForm: Task.async(function* ({ browser, loginFormOrigin, login }) {
    let recipes = [];
    if (loginFormOrigin) {
      let formHost;
      try {
        formHost = (new URL(loginFormOrigin)).host;
        let recipeManager = yield this.recipeParentPromise;
        recipes = recipeManager.getRecipesForHost(formHost);
      } catch (ex) {
        
      }
    }

    
    
    let jsLogins = JSON.parse(JSON.stringify([login]));
    browser.messageManager.sendAsyncMessage("RemoteLogins:fillForm", {
      loginFormOrigin,
      logins: jsLogins,
      recipes,
    });
  }),

  


  sendLoginDataToChild: Task.async(function*(showMasterPassword, formOrigin, actionOrigin,
                                             requestId, target) {
    let recipes = [];
    if (formOrigin) {
      let formHost;
      try {
        formHost = (new URL(formOrigin)).host;
        let recipeManager = yield this.recipeParentPromise;
        recipes = recipeManager.getRecipesForHost(formHost);
      } catch (ex) {
        
      }
    }

    if (!showMasterPassword && !Services.logins.isLoggedIn) {
      target.sendAsyncMessage("RemoteLogins:loginsFound", {
        requestId: requestId,
        logins: [],
        recipes,
      });
      return;
    }

    let allLoginsCount = Services.logins.countLogins(formOrigin, "", null);
    
    if (!allLoginsCount) {
      target.sendAsyncMessage("RemoteLogins:loginsFound", {
        requestId: requestId,
        logins: [],
        recipes,
      });
      return;
    }

    
    
    if (Services.logins.uiBusy) {
      log("deferring sendLoginDataToChild for", formOrigin);
      let self = this;
      let observer = {
        QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                               Ci.nsISupportsWeakReference]),

        observe: function (subject, topic, data) {
          log("Got deferred sendLoginDataToChild notification:", topic);
          
          Services.obs.removeObserver(this, "passwordmgr-crypto-login");
          Services.obs.removeObserver(this, "passwordmgr-crypto-loginCanceled");
          if (topic == "passwordmgr-crypto-loginCanceled") {
            target.sendAsyncMessage("RemoteLogins:loginsFound", {
              requestId: requestId,
              logins: [],
              recipes,
            });
            return;
          }

          self.sendLoginDataToChild(showMasterPassword, formOrigin, actionOrigin,
                                    requestId, target);
        },
      };

      
      
      
      
      
      Services.obs.addObserver(observer, "passwordmgr-crypto-login", false);
      Services.obs.addObserver(observer, "passwordmgr-crypto-loginCanceled", false);
      return;
    }

    var logins = Services.logins.findLogins({}, formOrigin, actionOrigin, null);
    
    
    var jsLogins = JSON.parse(JSON.stringify(logins));
    target.sendAsyncMessage("RemoteLogins:loginsFound", {
      requestId: requestId,
      logins: jsLogins,
      recipes,
    });

    const PWMGR_FORM_ACTION_EFFECT =  Services.telemetry.getHistogramById("PWMGR_FORM_ACTION_EFFECT");
    if (logins.length == 0) {
      PWMGR_FORM_ACTION_EFFECT.add(2);
    } else if (logins.length == allLoginsCount) {
      PWMGR_FORM_ACTION_EFFECT.add(0);
    } else {
      
      PWMGR_FORM_ACTION_EFFECT.add(1);
    }
  }),

  doAutocompleteSearch: function({ formOrigin, actionOrigin,
                                   searchString, previousResult,
                                   rect, requestId, remote }, target) {
    
    
    var result;

    let searchStringLower = searchString.toLowerCase();
    let logins;
    if (previousResult &&
        searchStringLower.startsWith(previousResult.searchString.toLowerCase())) {
      log("Using previous autocomplete result");

      
      
      logins = previousResult.logins;
    } else {
      log("Creating new autocomplete search result.");

      
      logins = Services.logins.findLogins({}, formOrigin, actionOrigin, null);
    }

    let matchingLogins = logins.filter(function(fullMatch) {
      let match = fullMatch.username;

      
      
      return match && match.toLowerCase().startsWith(searchStringLower);
    });

    
    
    
    
    
    if (remote) {
      result = new UserAutoCompleteResult(searchString, matchingLogins);
      AutoCompleteE10S.showPopupWithResults(target.ownerDocument.defaultView, rect, result);
    }

    
    
    var jsLogins = JSON.parse(JSON.stringify(matchingLogins));
    target.messageManager.sendAsyncMessage("RemoteLogins:loginsAutoCompleted", {
      requestId: requestId,
      logins: jsLogins,
    });
  },

  onFormSubmit: function(hostname, formSubmitURL,
                         usernameField, newPasswordField,
                         oldPasswordField, opener,
                         target) {
    function getPrompter() {
      var prompterSvc = Cc["@mozilla.org/login-manager/prompter;1"].
                        createInstance(Ci.nsILoginManagerPrompter);
      
      
      
      
      
      prompterSvc.init(target.isRemoteBrowser ?
                          target.ownerDocument.defaultView :
                          target.contentWindow);
      if (target.isRemoteBrowser)
        prompterSvc.setE10sData(target, opener);
      return prompterSvc;
    }

    function recordLoginUse(login) {
      
      let propBag = Cc["@mozilla.org/hash-property-bag;1"].
                    createInstance(Ci.nsIWritablePropertyBag);
      propBag.setProperty("timeLastUsed", Date.now());
      propBag.setProperty("timesUsedIncrement", 1);
      Services.logins.modifyLogin(login, propBag);
    }

    if (!Services.logins.getLoginSavingEnabled(hostname)) {
      log("(form submission ignored -- saving is disabled for:", hostname, ")");
      return;
    }

    var formLogin = Cc["@mozilla.org/login-manager/loginInfo;1"].
                    createInstance(Ci.nsILoginInfo);
    formLogin.init(hostname, formSubmitURL, null,
                   (usernameField ? usernameField.value : ""),
                   newPasswordField.value,
                   (usernameField ? usernameField.name  : ""),
                   newPasswordField.name);

    let logins = Services.logins.findLogins({}, hostname, formSubmitURL, null);

    
    
    
    if (!usernameField && oldPasswordField && logins.length > 0) {
      var prompter = getPrompter();

      if (logins.length == 1) {
        var oldLogin = logins[0];

        if (oldLogin.password == formLogin.password) {
          recordLoginUse(oldLogin);
          log("(Not prompting to save/change since we have no username and the " +
              "only saved password matches the new password)");
          return;
        }

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
        prompter = getPrompter();
        prompter.promptToChangePassword(existingLogin, formLogin);
      } else {
        recordLoginUse(existingLogin);
      }

      return;
    }


    
    prompter = getPrompter();
    prompter.promptToSavePassword(formLogin);
  },

  













  loginFormStateByBrowser: new WeakMap(),

  



  stateForBrowser(browser) {
    let loginFormState = this.loginFormStateByBrowser.get(browser);
    if (!loginFormState) {
      loginFormState = {};
      this.loginFormStateByBrowser.set(browser, loginFormState);
    }
    return loginFormState;
  },

  




  updateLoginFormPresence(browser, { loginFormOrigin, loginFormPresent }) {
    const ANCHOR_DELAY_MS = 200;

    let state = this.stateForBrowser(browser);

    
    
    state.loginFormOrigin = loginFormOrigin;
    state.loginFormPresent = loginFormPresent;

    
    if (!state.anchorDeferredTask) {
      state.anchorDeferredTask = new DeferredTask(
        () => this.updateLoginAnchor(browser),
        ANCHOR_DELAY_MS
      );
    }
    state.anchorDeferredTask.arm();
  },
  updateLoginAnchor: Task.async(function* (browser) {
    
    
    
    let { loginFormOrigin, loginFormPresent } = this.stateForBrowser(browser);

    yield Services.logins.initializationPromise;

    
    let hasLogins = loginFormOrigin &&
                    Services.logins.countLogins(loginFormOrigin, "", null) > 0;

    
    
    if (!Services.prefs.getBoolPref("signon.ui.experimental")) {
      return;
    }

    let showLoginAnchor = loginFormPresent || hasLogins;

    let fillDoorhanger = LoginDoorhangers.FillDoorhanger.find({ browser });
    if (fillDoorhanger) {
      if (!showLoginAnchor) {
        fillDoorhanger.remove();
        return;
      }
      
      yield fillDoorhanger.promiseHidden;
      fillDoorhanger.loginFormPresent = loginFormPresent;
      fillDoorhanger.loginFormOrigin = loginFormOrigin;
      fillDoorhanger.filterString = loginFormOrigin;
      return;
    }
    if (showLoginAnchor) {
      fillDoorhanger = new LoginDoorhangers.FillDoorhanger({
        browser,
        loginFormPresent,
        loginFormOrigin,
        filterString: loginFormOrigin,
      });
    }
  }),
};
