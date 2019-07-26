

















"use strict";

this.EXPORTED_SYMBOLS = ["FxAccountsMgmtService"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/ObjectWrapper.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");

XPCOMUtils.defineLazyModuleGetter(this, "FxAccountsManager",
  "resource://gre/modules/FxAccountsManager.jsm");

this.FxAccountsMgmtService = {

  _sendChromeEvent: function(aMsg) {
    if (!this._shell) {
      return;
    }
    this._shell.sendCustomEvent("mozFxAccountsChromeEvent", aMsg);
  },

  _onFullfill: function(aMsgId, aData) {
    this._sendChromeEvent({
      id: aMsgId,
      data: aData ? aData : null
    });
  },

  _onReject: function(aMsgId, aReason) {
    this._sendChromeEvent({
      id: aMsgId,
      error: aReason ? aReason : null
    });
  },

  init: function() {
    Services.obs.addObserver(this, "content-start", false);
  },

  observe: function(aSubject, aTopic, aData) {
    this._shell = Services.wm.getMostRecentWindow("navigator:browser").shell;
    let content = this._shell.contentBrowser.contentWindow;
    content.addEventListener("mozFxAccountsContentEvent",
                             FxAccountsMgmtService);
    Services.obs.removeObserver(this, "content-start");
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
            
            self._onFullfill(msg.id, account);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "logout":
        FxAccountsManager.signOut().then(
          () => {
            self._onFullfill(msg.id);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "queryAccount":
        FxAccountsManager.queryAccount(data.accountId).then(
          result => {
            self._onFullfill(msg.id, result);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "signIn":
      case "signUp":
        FxAccountsManager[data.method](data.accountId, data.password).then(
          user => {
            self._onFullfill(msg.id, user);
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
