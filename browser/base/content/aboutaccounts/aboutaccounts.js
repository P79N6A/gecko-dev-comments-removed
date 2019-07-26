



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FxAccounts.jsm");

const PREF_LAST_FXA_USER = "identity.fxaccounts.lastSignedInUser";

function log(msg) {
  
};

function error(msg) {
  console.log("Firefox Account Error: " + msg + "\n");
};

function getPreviousAccountName() {
  try {
    return Services.prefs.getComplexValue(PREF_LAST_FXA_USER, Ci.nsISupportsString).data;
  } catch (_) {
    return "";
  }
}

function setPreviousAccountName(acctName) {
  let string = Cc["@mozilla.org/supports-string;1"]
               .createInstance(Ci.nsISupportsString);
  string.data = acctName;
  Services.prefs.setComplexValue(PREF_LAST_FXA_USER, Ci.nsISupportsString, string);
}

function needRelinkWarning(accountData) {
  let prevAcct = getPreviousAccountName();
  return prevAcct && prevAcct != accountData.email;
}

function promptForRelink() {
  let sb = Services.strings.createBundle("chrome://browser/locale/syncSetup.properties");
  let continueLabel = sb.GetStringFromName("continue.label");
  let title = sb.GetStringFromName("relink.verify.title");
  let description = sb.formatStringFromName("relink.verify.description",
                                            [Services.prefs.getCharPref(PREF_LAST_FXA_USER)], 1);
  let body = sb.GetStringFromName("relink.verify.heading") +
             "\n\n" + description;
  let ps = Services.prompt;
  let buttonFlags = (ps.BUTTON_POS_0 * ps.BUTTON_TITLE_IS_STRING) +
                    (ps.BUTTON_POS_1 * ps.BUTTON_TITLE_CANCEL) +
                    ps.BUTTON_POS_1_DEFAULT;
  let pressed = Services.prompt.confirmEx(window, title, body, buttonFlags,
                                     continueLabel, null, null, null,
                                     {});
  return pressed == 0; 
}

let wrapper = {
  iframe: null,

  init: function () {
    let weave = Cc["@mozilla.org/weave/service;1"]
                  .getService(Ci.nsISupports)
                  .wrappedJSObject;

    
    if (!weave.fxAccountsEnabled) {
      document.body.remove();
      return;
    }

    let iframe = document.getElementById("remote");
    this.iframe = iframe;
    iframe.addEventListener("load", this);

    try {
      iframe.src = fxAccounts.getAccountsURI();
    } catch (e) {
      error("Couldn't init Firefox Account wrapper: " + e.message);
    }
  },

  handleEvent: function (evt) {
    switch (evt.type) {
      case "load":
        this.iframe.contentWindow.addEventListener("FirefoxAccountsCommand", this);
        this.iframe.removeEventListener("load", this);
        break;
      case "FirefoxAccountsCommand":
        this.handleRemoteCommand(evt);
        break;
    }
  },

  





  onLogin: function (accountData) {
    log("Received: 'login'. Data:" + JSON.stringify(accountData));

    if (accountData.customizeSync) {
      Services.prefs.setBoolPref("services.sync.needsCustomization", true);
      delete accountData.customizeSync;
    }

    
    
    
    
    
    if (needRelinkWarning(accountData) && !promptForRelink()) {
      
      
      this.injectData("message", { status: "login" });
      
      window.location.reload();
      return;
    }

    
    setPreviousAccountName(accountData.email);

    fxAccounts.setSignedInUser(accountData).then(
      () => {
        this.injectData("message", { status: "login" });
        
        
        
        
        
        
        
      },
      (err) => this.injectData("message", { status: "error", error: err })
    );
  },

  



  onSessionStatus: function () {
    log("Received: 'session_status'.");

    fxAccounts.getSignedInUser().then(
      (accountData) => this.injectData("message", { status: "session_status", data: accountData }),
      (err) => this.injectData("message", { status: "error", error: err })
    );
  },

  


  onSignOut: function () {
    log("Received: 'sign_out'.");

    fxAccounts.signOut().then(
      () => this.injectData("message", { status: "sign_out" }),
      (err) => this.injectData("message", { status: "error", error: err })
    );
  },

  handleRemoteCommand: function (evt) {
    log('command: ' + evt.detail.command);
    let data = evt.detail.data;

    switch (evt.detail.command) {
      case "login":
        this.onLogin(data);
        break;
      case "session_status":
        this.onSessionStatus(data);
        break;
      case "sign_out":
        this.onSignOut(data);
        break;
      default:
        log("Unexpected remote command received: " + evt.detail.command + ". Ignoring command.");
        break;
    }
  },

  injectData: function (type, content) {
    let authUrl;
    try {
      authUrl = fxAccounts.getAccountsURI();
    } catch (e) {
      error("Couldn't inject data: " + e.message);
      return;
    }
    let data = {
      type: type,
      content: content
    };
    this.iframe.contentWindow.postMessage(data, authUrl);
  },
};



function handleOldSync() {
  
  window.location = Services.urlFormatter.formatURLPref("app.support.baseURL") + "old-sync";
}

function getStarted() {
  hide("intro");
  hide("stage");
  show("remote");
}

function openPrefs() {
  window.openPreferences("paneSync");
}

function init() {
  let signinQuery = window.location.href.match(/signin=true$/);

  if (signinQuery) {
    show("remote");
    wrapper.init();
  } else {
    
    fxAccounts.getSignedInUser().then(user => {
      if (user) {
        show("stage");
        show("manage");
        let sb = Services.strings.createBundle("chrome://browser/locale/syncSetup.properties");
        document.title = sb.GetStringFromName("manage.pageTitle");
      } else {
        show("stage");
        show("intro");
        
        wrapper.init();
      }
    });
  }
}

function show(id) {
  document.getElementById(id).style.display = 'block';
}
function hide(id) {
  document.getElementById(id).style.display = 'none';
}

document.addEventListener("DOMContentLoaded", function onload() {
  document.removeEventListener("DOMContentLoaded", onload, true);
  init();
}, true);
