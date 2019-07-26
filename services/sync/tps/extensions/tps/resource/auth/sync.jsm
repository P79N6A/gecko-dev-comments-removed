



"use strict";

this.EXPORTED_SYMBOLS = [
  "Authentication",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://services-sync/main.js");
Cu.import("resource://tps/logger.jsm");





var Authentication = {

  


  get isLoggedIn() {
    return !!this.getSignedInUser();
  },

  




  getSignedInUser: function getSignedInUser() {
    let user = null;

    if (Weave.Service.isLoggedIn) {
      user = {
        email: Weave.Service.identity.account,
        password: Weave.Service.identity.basicPassword,
        passphrase: Weave.Service.identity.syncKey
      };
    }

    return user;
  },

  











  signIn: function signIn(account) {
    Logger.AssertTrue(account["username"], "Username has been found");
    Logger.AssertTrue(account["password"], "Password has been found");
    Logger.AssertTrue(account["passphrase"], "Passphrase has been found");

    Logger.logInfo("Logging in user: " + account["username"]);

    Weave.Service.identity.account = account["username"];
    Weave.Service.identity.basicPassword = account["password"];
    Weave.Service.identity.syncKey = account["passphrase"];

    
    Weave.Service.login();
    Weave.Svc.Obs.notify("weave:service:setup-complete");

    return true;
  }
};
