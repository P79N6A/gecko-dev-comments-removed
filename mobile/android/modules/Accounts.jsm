



"use strict";

this.EXPORTED_SYMBOLS = ["Accounts"];

const { utils: Cu } = Components;

Cu.import("resource://gre/modules/Messaging.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");




















let Accounts = Object.freeze({
  _accountsExist: function (kind) {
    return Messaging.sendRequestForResult({
      type: "Accounts:Exist",
      kind: kind
    }).then(data => data.exists);
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
      extras: extras
    });
  },

  








  createFirefoxAccountFromJSON: function (json) {
    return Messaging.sendRequestForResponse({
      type: "Accounts:CreateFirefoxAccountFromJSON",
      json: json
    });
  }
});
