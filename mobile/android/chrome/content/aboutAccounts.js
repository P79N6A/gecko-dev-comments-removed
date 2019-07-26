



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Services.jsm");




const PREF_LOG_LEVEL = "identity.fxaccounts.loglevel";


let log = Log.repository.getLogger("FirefoxAccounts");


try {
   if (log.ownAppenders.length == 0) {
    log.addAppender(new Log.DumpAppender());
  }
} catch (e) {
  log.addAppender(new Log.DumpAppender());
}

log.level = Log.Level.Error;
try {
  let level =
    Services.prefs.getPrefType(PREF_LOG_LEVEL) == Ci.nsIPrefBranch.PREF_STRING
    && Services.prefs.getCharPref(PREF_LOG_LEVEL);
  log.level = Log.Level[level] || Log.Level.Error;
} catch (e) {
  log.error(e);
}

let wrapper = {
  iframe: null,

  init: function () {
    log.info("about:accounts init");
    let iframe = document.getElementById("remote");
    this.iframe = iframe;
    iframe.addEventListener("load", this);
    log.debug("accountsURI " + this.accountsURI);
    iframe.src = this.accountsURI;
  },

  handleEvent: function (evt) {
    switch (evt.type) {
      case "load":
        this.iframe.contentWindow.addEventListener("FirefoxAccountsCommand", this);
        this.iframe.removeEventListener("load", this);
        break;
      case "FirefoxAccountsCommand":
        this.handleRemoteCommand(evt);
        break;
    }
  },

  onLogin: function (data) {
    log.debug("Received: 'login'. Data:" + JSON.stringify(data));
    this.injectData("message", { status: "login" });
  },

  onCreate: function (data) {
    log.debug("Received: 'create'. Data:" + JSON.stringify(data));
    this.injectData("message", { status: "create" });
  },

  onVerified: function (data) {
    log.debug("Received: 'verified'. Data:" + JSON.stringify(data));
    this.injectData("message", { status: "verified" });
  },

  get accountsURI() {
    delete this.accountsURI;
    return this.accountsURI = Services.urlFormatter.formatURLPref("firefox.accounts.remoteUrl");
  },

  handleRemoteCommand: function (evt) {
    log.debug('command: ' + evt.detail.command);
    let data = evt.detail.data;

    switch (evt.detail.command) {
      case "create":
        this.onCreate(data);
        break;
      case "login":
        this.onLogin(data);
        break;
      case "verified":
        this.onVerified(data);
        break;
      default:
        log.warn("Unexpected remote command received: " + evt.detail.command + ". Ignoring command.");
        break;
    }
  },

  injectData: function (type, content) {
    let authUrl = this.accountsURI;

    let data = {
      type: type,
      content: content
    };

    this.iframe.contentWindow.postMessage(data, authUrl);
  },
};

log.info("about:accounts initializing.");
wrapper.init();
log.info("about:accounts initialized.");
