



"use strict";

let Preferences = Cu.import("resource://gre/modules/Preferences.jsm", {}).Preferences;

const {FxAccounts, AccountState} = Cu.import("resource://gre/modules/FxAccounts.jsm", {});

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

  yield fxAccounts.signOut(true);
});

function configureIdentity() {
  
  return configureFxAccountIdentity().then(() => {
    Weave.Service.identity.whenReadyToAuthenticate.promise
  });
}


function configureFxAccountIdentity() {
  
  function MockFxaStorageManager() {
  }

  MockFxaStorageManager.prototype = {
    promiseInitialized: Promise.resolve(),

    initialize(accountData) {
      this.accountData = accountData;
    },

    finalize() {
      return Promise.resolve();
    },

    getAccountData() {
      return Promise.resolve(this.accountData);
    },

    updateAccountData(updatedFields) {
      for (let [name, value] of Iterator(updatedFields)) {
        if (value == null) {
          delete this.accountData[name];
        } else {
          this.accountData[name] = value;
        }
      }
      return Promise.resolve();
    },

    deleteAccountData() {
      this.accountData = null;
      return Promise.resolve();
    }
  }

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

  let MockInternal = {
    newAccountState(credentials) {
      let storageManager = new MockFxaStorageManager();
      storageManager.initialize(credentials);
      return new AccountState(this, storageManager);
    },
    getCertificate(data, keyPair, mustBeValidUntil) {
      this.cert = {
        validUntil: this.now() + 10000,
        cert: "certificate",
      };
      return Promise.resolve(this.cert.cert);
    },
  };
  let mockTSC = { 
    getTokenFromBrowserIDAssertion: function(uri, assertion, cb) {
      token.uid = "username";
      cb(null, token);
    },
  };

  let fxa = new FxAccounts(MockInternal);
  Weave.Service.identity._fxaService = fxa;
  Weave.Service.identity._tokenServerClient = mockTSC;
  
  
  Weave.Service.identity._account = user.email;
  return fxa.setSignedInUser(user);
}
