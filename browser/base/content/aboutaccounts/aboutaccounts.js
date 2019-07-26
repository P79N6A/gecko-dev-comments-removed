



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FxAccounts.jsm");

let fxAccountsCommon = {};
Cu.import("resource://gre/modules/FxAccountsCommon.js", fxAccountsCommon);

const PREF_LAST_FXA_USER = "identity.fxaccounts.lastSignedInUserHash";
const PREF_SYNC_SHOW_CUSTOMIZATION = "services.sync.ui.showCustomizationDialog";

const OBSERVER_TOPICS = [
  fxAccountsCommon.ONVERIFIED_NOTIFICATION,
  fxAccountsCommon.ONLOGOUT_NOTIFICATION,
];

function log(msg) {
  
};

function error(msg) {
  console.log("Firefox Account Error: " + msg + "\n");
};

function getPreviousAccountNameHash() {
  try {
    return Services.prefs.getComplexValue(PREF_LAST_FXA_USER, Ci.nsISupportsString).data;
  } catch (_) {
    return "";
  }
}

function setPreviousAccountNameHash(acctName) {
  let string = Cc["@mozilla.org/supports-string;1"]
               .createInstance(Ci.nsISupportsString);
  string.data = sha256(acctName);
  Services.prefs.setComplexValue(PREF_LAST_FXA_USER, Ci.nsISupportsString, string);
}

function needRelinkWarning(acctName) {
  let prevAcctHash = getPreviousAccountNameHash();
  return prevAcctHash && prevAcctHash != sha256(acctName);
}


function sha256(str) {
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  
  let data = converter.convertToByteArray(str, {});
  let hasher = Cc["@mozilla.org/security/hash;1"]
                 .createInstance(Ci.nsICryptoHash);
  hasher.init(hasher.SHA256);
  hasher.update(data, data.length);

  return hasher.finish(true);
}

function promptForRelink(acctName) {
  let sb = Services.strings.createBundle("chrome://browser/locale/syncSetup.properties");
  let continueLabel = sb.GetStringFromName("continue.label");
  let title = sb.GetStringFromName("relinkVerify.title");
  let description = sb.formatStringFromName("relinkVerify.description",
                                            [acctName], 1);
  let body = sb.GetStringFromName("relinkVerify.heading") +
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






function shouldAllowRelink(acctName) {
  return !needRelinkWarning(acctName) || promptForRelink(acctName);
}

let wrapper = {
  iframe: null,

  init: function (url=null) {
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
      iframe.src = url || fxAccounts.getAccountsSignUpURI();
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
      Services.prefs.setBoolPref(PREF_SYNC_SHOW_CUSTOMIZATION, true);
      delete accountData.customizeSync;
    }

    
    let newAccountEmail = accountData.email;
    
    
    
    if (!accountData.verifiedCanLinkAccount && !shouldAllowRelink(newAccountEmail)) {
      
      
      this.injectData("message", { status: "login" });
      
      window.location = "about:accounts";
      return;
    }
    delete accountData.verifiedCanLinkAccount;

    
    setPreviousAccountNameHash(newAccountEmail);

    
    
    let xps = Cc["@mozilla.org/weave/service;1"]
              .getService(Ci.nsISupports)
              .wrappedJSObject;
    xps.whenLoaded().then(() => {
      return fxAccounts.setSignedInUser(accountData);
    }).then(() => {
      
      
      if (accountData.verified) {
        showManage();
      }
      this.injectData("message", { status: "login" });
      
      
      
      
      
      
      
    }, (err) => this.injectData("message", { status: "error", error: err })
    );
  },

  onCanLinkAccount: function(accountData) {
    
    let ok = shouldAllowRelink(accountData.email);
    this.injectData("message", { status: "can_link_account", data: { ok: ok } });
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
      case "can_link_account":
        this.onCanLinkAccount(data);
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
      authUrl = fxAccounts.getAccountsSignUpURI();
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
  let chromeWin = window
    .QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Ci.nsIWebNavigation)
    .QueryInterface(Ci.nsIDocShellTreeItem)
    .rootTreeItem
    .QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Ci.nsIDOMWindow)
    .QueryInterface(Ci.nsIDOMChromeWindow);
  let url = Services.urlFormatter.formatURLPref("app.support.baseURL") + "old-sync";
  chromeWin.switchToTabHavingURI(url, true);
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
  fxAccounts.getSignedInUser().then(user => {
    
    
    if (window.closed) {
      return;
    }
    if (window.location.href.contains("action=signin")) {
      if (user) {
        
        showManage();
      } else {
        show("remote");
        wrapper.init(fxAccounts.getAccountsSignInURI());
      }
    } else if (window.location.href.contains("action=signup")) {
      if (user) {
        
        showManage();
      } else {
        show("remote");
        wrapper.init();
      }
    } else if (window.location.href.contains("action=reauth")) {
      
      
      
      
      fxAccounts.promiseAccountsForceSigninURI().then(url => {
        show("remote");
        wrapper.init(url);
      });
    } else {
      
      if (user) {
        showManage();
        let sb = Services.strings.createBundle("chrome://browser/locale/syncSetup.properties");
        document.title = sb.GetStringFromName("manage.pageTitle");
      } else {
        show("stage");
        show("intro");
        
        wrapper.init();
      }
    }
  });
}

function show(id) {
  document.getElementById(id).style.display = 'block';
}
function hide(id) {
  document.getElementById(id).style.display = 'none';
}

function showManage() {
  show("stage");
  show("manage");
  hide("remote");
  hide("intro");
}

document.addEventListener("DOMContentLoaded", function onload() {
  document.removeEventListener("DOMContentLoaded", onload, true);
  init();
}, true);

function initObservers() {
  function observe(subject, topic, data) {
    log("about:accounts observed " + topic);
    if (topic == fxAccountsCommon.ONLOGOUT_NOTIFICATION) {
      
      window.location = "about:accounts?action=signin";
      return;
    }
    
    window.location = "about:accounts";
  }

  for (let topic of OBSERVER_TOPICS) {
    Services.obs.addObserver(observe, topic, false);
  }
  window.addEventListener("unload", function(event) {
    log("about:accounts unloading")
    for (let topic of OBSERVER_TOPICS) {
      Services.obs.removeObserver(observe, topic);
    }
  });
}
initObservers();
