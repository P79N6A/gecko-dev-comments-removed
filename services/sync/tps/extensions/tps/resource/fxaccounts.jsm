



"use strict";

this.EXPORTED_SYMBOLS = [
  "FxAccountsHelper",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/FxAccounts.jsm");
Cu.import("resource://gre/modules/FxAccountsClient.jsm");
Cu.import("resource://services-common/async.js");
Cu.import("resource://services-sync/main.js");
Cu.import("resource://tps/logger.jsm");





var FxAccountsHelper = {

  







  signIn: function signIn(email, password) {
    let cb = Async.makeSpinningCallback();

    var client = new FxAccountsClient();
    client.signIn(email, password, true).then(credentials => {
      return fxAccounts.setSignedInUser(credentials);
    }).then(() => {
      cb(null);
    }, err => {
      cb(err);
    });

    try {
      cb.wait();
    } catch (err) {
      Logger.logError("signIn() failed with: " + JSON.stringify(err));
      throw err;
    }
  }
};
