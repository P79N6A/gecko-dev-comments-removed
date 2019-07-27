



"use strict";

this.EXPORTED_SYMBOLS = ["Accounts"];

const { utils: Cu } = Components;

Cu.import("resource://gre/modules/Messaging.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");




















let Accounts = Object.freeze({
  _accountsExist: function (kind) {
    let deferred = Promise.defer();

    sendMessageToJava({
      type: "Accounts:Exist",
      kind: kind,
    }, (data, error) => {
      if (error) {
        deferred.reject(error);
      } else {
        deferred.resolve(data.exists);
      }
    });

    return deferred.promise;
  },

  firefoxAccountsExist: function () {
    return this._accountsExist("fxa");
  },

  syncAccountsExist: function () {
    return this._accountsExist("sync11");
  },

  anySyncAccountsExist: function () {
    return this._accountsExist("any");
  },

  









  launchSetup: function (extras) {
    Messaging.sendRequest({
      type: "Accounts:Create",
      extras: extras,
    });
  },
});
