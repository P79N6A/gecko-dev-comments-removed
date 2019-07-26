



"use strict";

this.EXPORTED_SYMBOLS = [
  "FxAccountsHelper",
];

const { utils: Cu } = Components;

Cu.import("resource://gre/modules/FxAccountsClient.jsm");
Cu.import("resource://services-common/async.js");
Cu.import("resource://services-sync/main.js");
Cu.import("resource://tps/logger.jsm");





var FxAccountsHelper = {

  







  signIn: function signIn(email, password) {
    let cb = Async.makeSpinningCallback();

    var client = new FxAccountsClient();
    client.signIn(email, password).then(credentials => {
      
      credentials.kA = 'foo';
      credentials.kB = 'bar';

      Weave.Service.identity._fxaService.setSignedInUser(credentials).then(() => {
        cb(null);
      }, err => {
        cb(err);
      });
    }, (err) => {
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
