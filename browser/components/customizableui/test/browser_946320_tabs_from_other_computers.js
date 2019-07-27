



"use strict";

let Preferences = Cu.import("resource://gre/modules/Preferences.jsm", {}).Preferences;

let tmp = {};
Cu.import("resource://gre/modules/FxAccounts.jsm", tmp);
Cu.import("resource://gre/modules/FxAccountsCommon.js", tmp);
Cu.import("resource://services-sync/browserid_identity.js", tmp);
let {FxAccounts, BrowserIDManager, DATA_FORMAT_VERSION, CERT_LIFETIME} = tmp;
let fxaSyncIsEnabled = Weave.Service.identity instanceof BrowserIDManager;

add_task(function() {
  yield PanelUI.show({type: "command"});

  let historyButton = document.getElementById("history-panelmenu");
  let historySubview = document.getElementById("PanelUI-history");
  let subviewShownPromise = subviewShown(historySubview);
  historyButton.click();
  yield subviewShownPromise;

  let tabsFromOtherComputers = document.getElementById("sync-tabs-menuitem2");
  is(tabsFromOtherComputers.hidden, true, "The Tabs From Other Computers menuitem should be hidden when sync isn't enabled.");

  let hiddenPanelPromise = promisePanelHidden(window);
  PanelUI.hide();
  yield hiddenPanelPromise;

  
  yield configureIdentity();
  yield PanelUI.show({type: "command"});

  subviewShownPromise = subviewShown(historySubview);
  historyButton.click();
  yield subviewShownPromise;

  is(tabsFromOtherComputers.hidden, false, "The Tabs From Other Computers menuitem should be shown when sync is enabled.");

  let syncPrefBranch = new Preferences("services.sync.");
  syncPrefBranch.resetBranch("");
  Services.logins.removeAllLogins();

  hiddenPanelPromise = promisePanelHidden(window);
  PanelUI.toggle({type: "command"});
  yield hiddenPanelPromise;

  if (fxaSyncIsEnabled) {
    yield fxAccounts.signOut();
  }
});

function configureIdentity() {
  
  configureFxAccountIdentity();

  if (fxaSyncIsEnabled) {
    return Weave.Service.identity.initializeWithCurrentIdentity().then(() => {
      
      return Weave.Service.identity.whenReadyToAuthenticate.promise;
    });
  }

  Weave.Service.createAccount("john@doe.com", "mysecretpw",
                              "challenge", "response");
  Weave.Service.identity.account = "john@doe.com";
  Weave.Service.identity.basicPassword = "mysecretpw";
  Weave.Service.identity.syncKey = Weave.Utils.generatePassphrase();
  Weave.Svc.Prefs.set("firstSync", "newAccount");
  Weave.Service.persistLogin();
  return Promise.resolve();
}



function configureFxAccountIdentity() {
  let user = {
    assertion: "assertion",
    email: "email",
    kA: "kA",
    kB: "kB",
    sessionToken: "sessionToken",
    uid: "user_uid",
    verified: true,
  };

  let token = {
    endpoint: Weave.Svc.Prefs.get("tokenServerURI"),
    duration: 300,
    id: "id",
    key: "key",
    
  };

  let MockInternal = {};
  let mockTSC = { 
    getTokenFromBrowserIDAssertion: function(uri, assertion, cb) {
      token.uid = "username";
      cb(null, token);
    },
  };

  let authService = Weave.Service.identity;
  authService._fxaService = new FxAccounts(MockInternal);

  authService._fxaService.internal.currentAccountState.signedInUser = {
    version: DATA_FORMAT_VERSION,
    accountData: user
  }
  authService._fxaService.internal.currentAccountState.getCertificate = function(data, keyPair, mustBeValidUntil) {
    this.cert = {
      validUntil: authService._fxaService.internal.now() + CERT_LIFETIME,
      cert: "certificate",
    };
    return Promise.resolve(this.cert.cert);
  };

  authService._tokenServerClient = mockTSC;
  
  
  authService._account = user.email;
}
