



"use strict";

this.EXPORTED_SYMBOLS = [
  "Authentication",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/FxAccounts.jsm");
Cu.import("resource://gre/modules/FxAccountsClient.jsm");
Cu.import("resource://services-common/async.js");
Cu.import("resource://services-sync/main.js");
Cu.import("resource://tps/logger.jsm");





var Authentication = {

  


  get isLoggedIn() {
    return !!this.getSignedInUser();
  },

  




  getSignedInUser: function getSignedInUser() {
    let cb = Async.makeSpinningCallback();

    fxAccounts.getSignedInUser().then(user => {
      cb(null, user);
    }, error => {
      cb(error);
    })

    try {
      return cb.wait();
    } catch (error) {
      Logger.logError("getSignedInUser() failed with: " + JSON.stringify(error));
      throw error;
    }
  },

  









  signIn: function signIn(account) {
    let cb = Async.makeSpinningCallback();

    Logger.AssertTrue(account["username"], "Username has been found");
    Logger.AssertTrue(account["password"], "Password has been found");

    Logger.logInfo("Login user: " + account["username"] + '\n');

    let client = new FxAccountsClient();
    client.signIn(account["username"], account["password"], true).then(credentials => {
      return fxAccounts.setSignedInUser(credentials);
    }).then(() => {
      cb(null, true);
    }, error => {
      cb(error, false);
    });

    try {
      return cb.wait();
    } catch (error) {
      throw new Error("signIn() failed with: " + JSON.stringify(error));
    }
  }
};
