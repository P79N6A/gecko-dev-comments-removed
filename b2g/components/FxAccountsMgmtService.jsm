

















"use strict";

this.EXPORTED_SYMBOLS = ["FxAccountsMgmtService"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");

XPCOMUtils.defineLazyModuleGetter(this, "FxAccountsManager",
  "resource://gre/modules/FxAccountsManager.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SystemAppProxy",
                                  "resource://gre/modules/SystemAppProxy.jsm");

this.FxAccountsMgmtService = {
  _onFulfill: function(aMsgId, aData) {
    SystemAppProxy._sendCustomEvent("mozFxAccountsChromeEvent", {
      id: aMsgId,
      data: aData ? aData : null
    });
  },

  _onReject: function(aMsgId, aReason) {
    SystemAppProxy._sendCustomEvent("mozFxAccountsChromeEvent", {
      id: aMsgId,
      error: aReason ? aReason : null
    });
  },

  init: function() {
    Services.obs.addObserver(this, "content-start", false);
    Services.obs.addObserver(this, ONLOGIN_NOTIFICATION, false);
    Services.obs.addObserver(this, ONVERIFIED_NOTIFICATION, false);
    Services.obs.addObserver(this, ONLOGOUT_NOTIFICATION, false);
  },

  observe: function(aSubject, aTopic, aData) {
    log.debug("Observed " + aTopic);
    switch (aTopic) {
      case "content-start":
        SystemAppProxy.addEventListener("mozFxAccountsContentEvent",
                                        FxAccountsMgmtService);
        Services.obs.removeObserver(this, "content-start");
        break;
      case ONLOGIN_NOTIFICATION:
      case ONVERIFIED_NOTIFICATION:
      case ONLOGOUT_NOTIFICATION:
        
        SystemAppProxy._sendCustomEvent("mozFxAccountsUnsolChromeEvent", {
          eventName: aTopic.substring(aTopic.indexOf(":") + 1)
        });
        break;
    }
  },

  handleEvent: function(aEvent) {
    let msg = aEvent.detail;
    log.debug("Got content msg " + JSON.stringify(msg));
    let self = FxAccountsMgmtService;

    if (!msg.id) {
      return;
    }

    let data = msg.data;
    if (!data) {
      return;
    }

    switch(data.method) {
      case "getAccounts":
        FxAccountsManager.getAccount().then(
          account => {
            
            self._onFulfill(msg.id, account);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "logout":
        FxAccountsManager.signOut().then(
          () => {
            self._onFulfill(msg.id);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "queryAccount":
        FxAccountsManager.queryAccount(data.accountId).then(
          result => {
            self._onFulfill(msg.id, result);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "signIn":
      case "signUp":
      case "refreshAuthentication":
        FxAccountsManager[data.method](data.accountId, data.password).then(
          user => {
            self._onFulfill(msg.id, user);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
    }
  }
};

FxAccountsMgmtService.init();
