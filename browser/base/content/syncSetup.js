







































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;



const INTRO_PAGE                    = 0;
const NEW_ACCOUNT_START_PAGE        = 1;
const NEW_ACCOUNT_PP_PAGE           = 2;
const NEW_ACCOUNT_PREFS_PAGE        = 3;
const NEW_ACCOUNT_CAPTCHA_PAGE      = 4;
const EXISTING_ACCOUNT_LOGIN_PAGE   = 5;
const EXISTING_ACCOUNT_PP_PAGE      = 6;
const EXISTING_ACCOUNT_MERGE_PAGE   = 7;
const EXISTING_ACCOUNT_CONFIRM_PAGE = 8;
const SETUP_SUCCESS_PAGE            = 9;

Cu.import("resource://services-sync/main.js");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

var gSyncSetup = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference]),

  captchaBrowser: null,
  wizard: null,
  _disabledSites: [],
  _remoteSites: [Weave.Service.serverURL, "https://api-secure.recaptcha.net"],

  status: {
    username: false,
    password: false,
    email: false,
    server: false
  },

  get _usingMainServers() {
    if (this._settingUpNew)
      return document.getElementById("serverType").selectedItem.value == "main";

    return document.getElementById("existingServerType").selectedItem.value == "main";
  },

  init: function () {
    let obs = [
      ["weave:service:changepph:finish", "onResetPassphrase"],
      ["weave:service:verify-login:start",  "onLoginStart"],
      ["weave:service:verify-login:error",  "onLoginEnd"],
      ["weave:service:verify-login:finish", "onLoginEnd"]];

    
    let self = this;
    let addRem = function(add) {
      obs.forEach(function([topic, func]) {
        
        
        if (add)
          Weave.Svc.Obs.add(topic, self[func], self);
        else
          Weave.Svc.Obs.remove(topic, self[func], self);
      });
    };
    addRem(true);
    window.addEventListener("unload", function() addRem(false), false);

    this.captchaBrowser = document.getElementById("captcha");
    this.wizard = document.getElementById("accountSetup");

    if (window.arguments && window.arguments[0] == true) {
      
      this._resettingSync = true;
      this.wizard.pageIndex = EXISTING_ACCOUNT_MERGE_PAGE;
    }
    else {
      this.wizard.canAdvance = false;
      this.captchaBrowser.addProgressListener(this);
      Weave.Svc.Prefs.set("firstSync", "notReady");
    }
  },

  updateSyncPrefs: function () {
    let syncEverything = document.getElementById("weaveSyncMode").selectedItem.value == "syncEverything";
    document.getElementById("syncModeOptions").selectedIndex = syncEverything ? 0 : 1;

    if (syncEverything) {
      document.getElementById("engine.bookmarks").checked = true;
      document.getElementById("engine.passwords").checked = true;
      document.getElementById("engine.history").checked   = true;
      document.getElementById("engine.tabs").checked      = true;
      document.getElementById("engine.prefs").checked     = true;
    }
  },

  startNewAccountSetup: function () {
    this._settingUpNew = true;
    this.wizard.pageIndex = NEW_ACCOUNT_START_PAGE;
  },

  useExistingAccount: function () {
    this._settingUpNew = false;
    this.wizard.pageIndex = EXISTING_ACCOUNT_LOGIN_PAGE;
  },

  onResetPassphrase: function () {
    document.getElementById("existingPassphrase").value = Weave.Service.passphrase;
    this.wizard.advance();
  },

  onLoginStart: function () {
    this.toggleLoginFeedback(false);
  },

  onLoginEnd: function () {
    this.toggleLoginFeedback(true);
  },

  toggleLoginFeedback: function (stop) {
    switch (this.wizard.pageIndex) {
      case EXISTING_ACCOUNT_LOGIN_PAGE:
        document.getElementById("connect-throbber").hidden = stop;
        let feedback = document.getElementById("existingPasswordFeedbackRow");
        if (stop) {
          let success = Weave.Status.login == Weave.LOGIN_SUCCEEDED ||
                        Weave.Status.login == Weave.LOGIN_FAILED_INVALID_PASSPHRASE;
          this._setFeedbackMessage(feedback, success, Weave.Status.login);
        }
        else
          this._setFeedbackMessage(feedback, true);
        break;
      case EXISTING_ACCOUNT_PP_PAGE:
        document.getElementById("passphrase-throbber").hidden = stop;
        feedback = document.getElementById("existingPassphraseFeedbackBox");
        if (stop) {
          let success = Weave.Status.login == Weave.LOGIN_SUCCEEDED;
          this._setFeedbackMessage(feedback, success, Weave.Status.login);
          document.getElementById("passphraseHelpBox").hidden = success;
        }
        else
          this._setFeedbackMessage(feedback, true);

        break;
    }
  },

  handleExpanderClick: function (event) {
    let expander = document.getElementById("setupAccountExpander");
    let expand = expander.className == "expander-down";
    expander.className =
       expand ? "expander-up" : "expander-down";
    document.getElementById("signInBox").hidden = !expand;
  },

  setupInitialSync: function () {
    let action = document.getElementById("mergeChoiceRadio").selectedItem.id;
    switch (action) {
      case "resetClient":
        
        
        if (!this._resettingSync)
          return;
        
      case "wipeClient":
      case "wipeRemote":
        Weave.Svc.Prefs.set("firstSync", action);
        break;
    }
  },

  
  checkFields: function () {
    this.wizard.canAdvance = this.readyToAdvance();
  },

  readyToAdvance: function () {
    switch (this.wizard.pageIndex) {
      case INTRO_PAGE:
        return false;
      case NEW_ACCOUNT_START_PAGE:
        for (i in this.status) {
          if (!this.status[i])
            return false;
        }
        if (this._usingMainServers)
          return document.getElementById("tos").checked;

        return true;
      case NEW_ACCOUNT_PP_PAGE:
        return this.onPassphraseChange();
      case EXISTING_ACCOUNT_LOGIN_PAGE:
        let hasUser = document.getElementById("existingUsername").value != "";
        let hasPass = document.getElementById("existingPassword").value != "";
        if (hasUser && hasPass) {
          if (this._usingMainServers)
            return true;

          if (this._validateServer(document.getElementById("existingServerURL"), false))
            return true;
        }
        return false;
      case EXISTING_ACCOUNT_PP_PAGE:
        return document.getElementById("existingPassphrase").value != "";
    }
    
    return true;
  },

  onUsernameChange: function () {
    let feedback = document.getElementById("usernameFeedbackRow");
    let val = document.getElementById("weaveUsername").value;
    let availCheck = "", str = "";
    let available = true;
    if (val) {
      availCheck = Weave.Service.checkUsername(val);
      available = availCheck == "available";
    }

    if (!available) {
      if (availCheck == "notAvailable")
        str = "usernameNotAvailable.label";
      else
        str = availCheck;
    }

    this._setFeedbackMessage(feedback, available, str);

    this.status.username = val && available;
    if (available)
      Weave.Service.username = val;

    this.checkFields();
  },

  onPasswordChange: function () {
    let password = document.getElementById("weavePassword");
    let valid, str;
    if (password.value == document.getElementById("weavePassphrase").value) {
      
      valid = false;
      errorString = Weave.Utils.getErrorString("change.password.pwSameAsPassphrase");
    }
    else {
      let pwconfirm = document.getElementById("weavePasswordConfirm");
      [valid, errorString] = gSyncUtils.validatePassword(password, pwconfirm);
    }

    let feedback = document.getElementById("passwordFeedbackRow");
    this._setFeedback(feedback, valid, errorString);

    this.status.password = valid;
    this.checkFields();
  },

  onEmailChange: function () {
    
    let re = /^(([^<>()[\]\\.,;:\s@\"]+(\.[^<>()[\]\\.,;:\s@\"]+)*)|(\".+\"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$/;
    this.status.email = re.test(document.getElementById("weaveEmail").value);

    this._setFeedbackMessage(document.getElementById("emailFeedbackRow"),
                             this.status.email,
                             "invalidEmail.label");

    this.checkFields();
  },

  onPassphraseChange: function () {
    let el1 = document.getElementById("weavePassphrase");
    let valid, str;
    
    if (el1.value == document.getElementById("weavePassword").value) {
      valid = false;
      str = Weave.Utils.getErrorString("change.passphrase.ppSameAsPassword");
    }
    else {
      let el2 = document.getElementById("weavePassphraseConfirm");
      [valid, str] = gSyncUtils.validatePassphrase(el1, el2);
    }

    let feedback = document.getElementById("passphraseFeedbackRow");
    this._setFeedback(feedback, valid, str);
    return valid;
  },

  onPageShow: function() {
    switch (this.wizard.pageIndex) {
      case INTRO_PAGE:
        this.wizard.getButton("next").hidden = true;
        this.wizard.getButton("back").hidden = true;
        this.wizard.getButton("cancel").label =
          this._stringBundle.GetStringFromName("cancelSetup.label");
        break;
      case NEW_ACCOUNT_PP_PAGE:
        this.checkFields();
        break;
      case NEW_ACCOUNT_START_PAGE:
        this.onServerChange();
        this.checkFields(); 
      case EXISTING_ACCOUNT_LOGIN_PAGE:
      case EXISTING_ACCOUNT_MERGE_PAGE:
        this.wizard.getButton("next").hidden = false;
        this.wizard.getButton("back").hidden = false;
        this.wizard.canRewind = !this._resettingSync;
        break;
      case SETUP_SUCCESS_PAGE:
        this.wizard.canRewind = false;
        this.wizard.getButton("back").hidden = true;
        this.wizard.getButton("cancel").hidden = true;
        break;
    }
  },

  onWizardAdvance: function () {
    if (!this.wizard.pageIndex)
      return true;

    switch (this.wizard.pageIndex) {
      case NEW_ACCOUNT_PREFS_PAGE:
        if (this._settingUpNew) {
          
          
          this._handleNoScript(true);
          this.captchaBrowser.loadURI(Weave.Service.miscAPI + "captcha_html");
          return true;
        }

        this.wizard.pageIndex = SETUP_SUCCESS_PAGE;
        return false;
      case NEW_ACCOUNT_CAPTCHA_PAGE:
        let doc = this.captchaBrowser.contentDocument;
        let getField = function getField(field) {
          let node = doc.getElementById("recaptcha_" + field + "_field");
          return node && node.value;
        };

        this.startThrobber(true);
        let username = document.getElementById("weaveUsername").value;
        let password = document.getElementById("weavePassword").value;
        let email    = document.getElementById("weaveEmail").value;
        let challenge = getField("challenge");
        let response = getField("response");

        let error = Weave.Service.createAccount(username, password, email,
                                                challenge, response);
        this.startThrobber(false);

        if (error == null) {
          Weave.Service.username = username;
          Weave.Service.password = password;
          this._handleNoScript(false);
          this.wizard.pageIndex = SETUP_SUCCESS_PAGE;
          return true;
        }

        
        Weave.Svc.Prompt.alert(window,
                               this._stringBundle.GetStringFromName("errorCreatingAccount.title"),
                               Weave.Utils.getErrorString(error));
        return false;
      case NEW_ACCOUNT_PP_PAGE:
        Weave.Service.passphrase = document.getElementById("weavePassphrase").value;
        document.getElementById("syncComputerName").value = Weave.Clients.localName;
        break;
      case EXISTING_ACCOUNT_LOGIN_PAGE:
        Weave.Service.username = document.getElementById("existingUsername").value;
        Weave.Service.password = document.getElementById("existingPassword").value;
        Weave.Service.passphrase = document.getElementById("existingPassphrase").value;
        
        
        
        if (!Weave.Service.verifyLogin()
            && Weave.Status.login != Weave.LOGIN_FAILED_NO_PASSPHRASE
            && Weave.Status.login != Weave.LOGIN_FAILED_INVALID_PASSPHRASE) {
          let feedback = document.getElementById("existingPasswordFeedbackRow");
          this._setFeedbackMessage(feedback, false, Weave.Status.login);
          return false;
        }
        break;
      case EXISTING_ACCOUNT_PP_PAGE:
        Weave.Service.passphrase = document.getElementById("existingPassphrase").value;
        if (Weave.Service.login())
          return true;

        return false;
      case EXISTING_ACCOUNT_MERGE_PAGE:
        return this._handleChoice();
      case EXISTING_ACCOUNT_CONFIRM_PAGE:
        this.setupInitialSync();
        if (this._resettingSync) {
          this.onWizardFinish();
          window.close();
          return false;
        }

        this.wizard.pageIndex = NEW_ACCOUNT_PREFS_PAGE;
        document.getElementById("syncComputerName").value = Weave.Clients.localName;
        return false;
    }
    return true;
  },

  onWizardBack: function () {
    switch (this.wizard.pageIndex) {
      case NEW_ACCOUNT_START_PAGE:
      case EXISTING_ACCOUNT_LOGIN_PAGE:
        this.wizard.pageIndex = INTRO_PAGE;
        return false;
      case EXISTING_ACCOUNT_PP_PAGE: 
        this.wizard.pageIndex = EXISTING_ACCOUNT_LOGIN_PAGE;
        return false;
      case NEW_ACCOUNT_PREFS_PAGE:
        if (this._settingUpNew)
          return true;

        this.wizard.pageIndex = EXISTING_ACCOUNT_CONFIRM_PAGE;
        return false;
    }
    return true;
  },

  onWizardFinish: function () {
    Weave.Status.service == Weave.STATUS_OK;

    if (!this._resettingSync) {
      function isChecked(element) {
        return document.getElementById(element).hasAttribute("checked");
      }

      let prefs = ["engine.bookmarks", "engine.passwords", "engine.history", "engine.tabs", "engine.prefs"];
      for (let i = 0;i < prefs.length;i++) {
        Weave.Svc.Prefs.set(prefs[i], isChecked(prefs[i]));
      }
      this._handleNoScript(false);
      if (Weave.Svc.Prefs.get("firstSync", "") == "notReady")
        Weave.Svc.Prefs.reset("firstSync");

      Weave.Service.persistLogin();
      Weave.Svc.Obs.notify("weave:service:setup-complete");
      if (this._settingUpNew)
        gSyncUtils.openFirstClientFirstrun();
      else
        gSyncUtils.openAddedClientFirstrun();
    }

    if (!Weave.Service.isLoggedIn)
      Weave.Service.login();

    Weave.Service.syncOnIdle(1);
  },

  onWizardCancel: function () {
    if (this._resettingSync)
      return;

    if (this.wizard.pageIndex == 9) {
      this.onWizardFinish();
      return;
    }
    this._handleNoScript(false);
    Weave.Service.startOver();
  },

  
  
  
  _handleNoScript: function (addExceptions) {
    
    let ns = Cc["@maone.net/noscript-service;1"];
    if (ns == null)
      return;

    ns = ns.getService().wrappedJSObject;
    if (addExceptions) {
      this._remoteSites.forEach(function(site) {
        site = ns.getSite(site);
        if (!ns.isJSEnabled(site)) {
          this._disabledSites.push(site); 
          ns.setJSEnabled(site, true); 
        }
      }, this);
    }
    else {
      this._disabledSites.forEach(function(site) {
        ns.setJSEnabled(site, false);
      });
      this._disabledSites = [];
    }
  },

  startThrobber: function (start) {
    
  },

  onServerChange: function () {
    if (this.wizard.pageIndex == EXISTING_ACCOUNT_LOGIN_PAGE) {
      if (this._usingMainServers)
        Weave.Svc.Prefs.reset("serverURL");
      document.getElementById("existingServerRow").hidden = this._usingMainServers;
      this.checkFields();
      return;
    }

    document.getElementById("serverRow").hidden = this._usingMainServers;
    document.getElementById("TOSRow").hidden = !this._usingMainServers;
    let valid = false;
    let feedback = document.getElementById("serverFeedbackRow");

    if (this._usingMainServers) {
      Weave.Svc.Prefs.reset("serverURL");
      valid = true;
      feedback.hidden = true;
    }
    else {
      let el = document.getElementById("weaveServerURL");
      let str = "";
      if (el.value) {
        valid = this._validateServer(el, true);
        let str = valid ? "" : "serverInvalid.label";
        this._setFeedbackMessage(feedback, valid, str);
      }
      else
        this._setFeedbackMessage(feedback, true);
    }

    
    if (valid)
      this.onUsernameChange();

    this.status.server = valid;
    this.checkFields();
  },

  
  
  _validateServer: function (element, checkRemote) {
    let valid = false;
    let val = element.value;
    if (!val)
      return false;

    let uri = Weave.Utils.makeURI(val);

    if (!uri)
      uri = Weave.Utils.makeURI("https://" + val);

    if (uri && checkRemote) {
      function isValid(uri) {
        Weave.Service.serverURL = uri.spec;
        let check = Weave.Service.checkUsername("a");
        return (check == "available" || check == "notAvailable");
      }

      if (uri.schemeIs("http")) {
        uri.scheme = "https";
        if (isValid(uri))
          valid = true;
        else
          
          uri.scheme = "http";
      }
      if (!valid)
        valid = isValid(uri);
    }
    else if (uri) {
      valid = true;
      Weave.Service.serverURL = uri.spec;
    }

    if (valid)
      element.value = Weave.Service.serverURL;
    else
      Weave.Svc.Prefs.reset("serverURL");

    return valid;
  },

  _handleChoice: function () {
    let desc = document.getElementById("mergeChoiceRadio").selectedIndex;
    document.getElementById("chosenActionDeck").selectedIndex = desc;
    switch (desc) {
      case 1:
        if (this._case1Setup)
          break;

        
        let db = Weave.Svc.History.DBConnection;

        let daysOfHistory = 0;
        let stm = db.createStatement(
          "SELECT ROUND(( " +
            "strftime('%s','now','localtime','utc') - " +
            "( " +
              "SELECT visit_date FROM moz_historyvisits " +
              "UNION ALL " +
              "SELECT visit_date FROM moz_historyvisits_temp " +
              "ORDER BY visit_date ASC LIMIT 1 " +
              ")/1000000 " +
            ")/86400) AS daysOfHistory ");

        if (stm.step())
          daysOfHistory = stm.getInt32(0);
        document.getElementById("historyCount").value =
          this._stringBundle.formatStringFromName("historyCount.label",  [daysOfHistory], 1);

        
        let bookmarks = 0;
        stm = db.createStatement(
          "SELECT count(*) AS bookmarks " +
          "FROM moz_bookmarks b " +
          "LEFT JOIN moz_bookmarks t ON " +
          "b.parent = t.id WHERE b.type = 1 AND t.parent <> :tag");
        stm.params.tag = Weave.Svc.Bookmark.tagsFolder;
        if (stm.executeStep())
          bookmarks = stm.row.bookmarks;
        document.getElementById("bookmarkCount").value =
          this._stringBundle.formatStringFromName("bookmarkCount.label", [bookmarks], 1);

        
        let logins = Weave.Svc.Login.getAllLogins({});
        document.getElementById("passwordCount").value =
          this._stringBundle.formatStringFromName("passwordCount.label",  [logins.length], 1);
        this._case1Setup = true;
        break;
      case 2:
        if (this._case2Setup)
          break;
        let count = 0;
        function appendNode(label) {
          let box = document.getElementById("clientList");
          let node = document.createElement("label");
          node.setAttribute("value", label);
          node.setAttribute("class", "data indent");
          box.appendChild(node);
        }

        for each (let name in Weave.Clients.stats.names) {
          
          if (name == Weave.Clients.localName)
            continue;

          
          if (++count <= 5)
            appendNode(name);
        }
        if (count > 5) {
          let label =
            this._stringBundle.formatStringFromName("additionalClients.label", [count - 5], 1);
          appendNode(label);
        }
        this._case2Setup = true;
        break;
    }

    return true;
  },

  
  
  _setFeedback: function (element, success, string) {
    element.hidden = success || !string;
    let class = success ? "success" : "error";
    let image = element.firstChild.nextSibling.firstChild;
    image.setAttribute("status", class);
    let label = image.nextSibling;
    label.value = string;
  },

  
  _setFeedbackMessage: function (element, success, string) {
    let str = "";
    if (string) {
      try {
        str = this._stringBundle.GetStringFromName(string);
      } catch(e) {}

      if (!str)
        str = Weave.Utils.getErrorString(string);
    }
    this._setFeedback(element, success, str);
  },


  onStateChange: function(webProgress, request, stateFlags, status) {
    
    if ((stateFlags & Ci.nsIWebProgressListener.STATE_STOP) == 0)
      return;
    if ((stateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK) == 0)
      return;
    if ((stateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW) == 0)
      return;

    
    if (request.QueryInterface(Ci.nsIHttpChannel).responseStatus == 404)
      this.onWizardAdvance();
  },
  onProgressChange: function() {},
  onStatusChange: function() {},
  onSecurityChange: function() {},
  onLocationChange: function () {}
}



XPCOMUtils.defineLazyGetter(gSyncSetup, "wizard", function() {
  return document.getElementById("accountSetup");
});
XPCOMUtils.defineLazyGetter(gSyncSetup, "_stringBundle", function() {
  return Services.strings.createBundle("chrome://browser/locale/syncSetup.properties");
});
