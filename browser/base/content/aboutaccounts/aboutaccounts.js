



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FxAccounts.jsm");

let fxAccountsCommon = {};
Cu.import("resource://gre/modules/FxAccountsCommon.js", fxAccountsCommon);


Cu.import("resource://services-sync/util.js");

const PREF_LAST_FXA_USER = "identity.fxaccounts.lastSignedInUserHash";
const PREF_SYNC_SHOW_CUSTOMIZATION = "services.sync-setup.ui.showCustomizationDialog";

const ACTION_URL_PARAM = "action";

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

  init: function (url, urlParams) {
    
    
    
    
    Utils.ensureMPUnlocked();

    let iframe = document.getElementById("remote");
    this.iframe = iframe;
    iframe.addEventListener("load", this);

    
    
    let urlParamStr = urlParams.toString();
    if (urlParamStr) {
      url += (url.includes("?") ? "&" : "?") + urlParamStr;
    }
    iframe.src = url;
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
        show("stage", "manage");
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

    
    
    let urlParams = new URLSearchParams(document.URL.split("?")[1] || "");
    let action = urlParams.get(ACTION_URL_PARAM);
    urlParams.delete(ACTION_URL_PARAM);

    switch (action) {
    case "signin":
      if (user) {
        
        show("stage", "manage");
      } else {
        show("remote");
        wrapper.init(fxAccounts.getAccountsSignInURI(), urlParams);
      }
      break;
    case "signup":
      if (user) {
        
        show("stage", "manage");
      } else {
        show("remote");
        wrapper.init(fxAccounts.getAccountsSignUpURI(), urlParams);
      }
      break;
    case "reauth":
      
      
      
      
      fxAccounts.promiseAccountsForceSigninURI().then(url => {
        show("remote");
        wrapper.init(url, urlParams);
      });
      break;
    default:
      
      if (user) {
        show("stage", "manage");
        let sb = Services.strings.createBundle("chrome://browser/locale/syncSetup.properties");
        document.title = sb.GetStringFromName("manage.pageTitle");
      } else {
        
        
        migrateToDevEdition(urlParams).then(migrated => {
          if (!migrated) {
            show("stage", "intro");
            
            wrapper.init(fxAccounts.getAccountsSignUpURI(), urlParams);
          }
        });
      }
      break;
    }
  });
}




function show(id, childId) {
  
  let allTop = document.querySelectorAll("body > div, iframe");
  for (let elt of allTop) {
    if (elt.getAttribute("id") == id) {
      elt.style.display = 'block';
    } else {
      elt.style.display = 'none';
    }
  }
  if (childId) {
    
    let allSecond = document.querySelectorAll("#" + id + " > div");
    for (let elt of allSecond) {
      if (elt.getAttribute("id") == childId) {
        elt.style.display = 'block';
      } else {
        elt.style.display = 'none';
      }
    }
  }
}




function migrateToDevEdition(urlParams) {
  let defaultProfilePath;
  try {
    defaultProfilePath = window.getDefaultProfilePath();
  } catch (e) {} 
  let migrateSyncCreds = false;
  if (defaultProfilePath) {
    try {
      migrateSyncCreds = Services.prefs.getBoolPref("identity.fxaccounts.migrateToDevEdition");
    } catch (e) {}
  }

  if (!migrateSyncCreds) {
    return Promise.resolve(false);
  }

  Cu.import("resource://gre/modules/osfile.jsm");
  let fxAccountsStorage = OS.Path.join(defaultProfilePath, fxAccountsCommon.DEFAULT_STORAGE_FILENAME);
  return OS.File.read(fxAccountsStorage, { encoding: "utf-8" }).then(text => {
    let accountData = JSON.parse(text).accountData;
    return fxAccounts.setSignedInUser(accountData);
  }).then(() => {
    return fxAccounts.promiseAccountsForceSigninURI().then(url => {
      show("remote");
      wrapper.init(url, urlParams);
    });
  }).then(null, error => {
    log("Failed to migrate FX Account: " + error);
    show("stage", "intro");
    
    wrapper.init(fxAccounts.getAccountsSignUpURI(), urlParams);
  }).then(() => {
    
    Services.prefs.setBoolPref("identity.fxaccounts.migrateToDevEdition", false);
    return true;
  }).then(null, err => {
    Cu.reportError("Failed to reset the migrateToDevEdition pref: " + err);
    return false;
  });
}



function getDefaultProfilePath() {
  let defaultProfile = Cc["@mozilla.org/toolkit/profile-service;1"]
                        .getService(Ci.nsIToolkitProfileService)
                        .defaultProfile;
  return defaultProfile.rootDir.path;
}

document.addEventListener("DOMContentLoaded", function onload() {
  document.removeEventListener("DOMContentLoaded", onload, true);
  init();
  var buttonGetStarted = document.getElementById('buttonGetStarted');
  buttonGetStarted.addEventListener('click', getStarted);

  var oldsync = document.getElementById('oldsync');
  oldsync.addEventListener('click', handleOldSync);

  var buttonOpenPrefs = document.getElementById('buttonOpenPrefs')
  buttonOpenPrefs.addEventListener('click', openPrefs);
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
