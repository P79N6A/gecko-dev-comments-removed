










this.EXPORTED_SYMBOLS = ["FxAccountsWebChannel"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "WebChannel",
                                  "resource://gre/modules/WebChannel.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "fxAccounts",
                                  "resource://gre/modules/FxAccounts.jsm");

const COMMAND_PROFILE_CHANGE       = "profile:change";
const COMMAND_CAN_LINK_ACCOUNT     = "fxaccounts:can_link_account";
const COMMAND_LOGIN                = "fxaccounts:login";

const PREF_LAST_FXA_USER           = "identity.fxaccounts.lastSignedInUserHash";
const PREF_SYNC_SHOW_CUSTOMIZATION = "services.sync-setup.ui.showCustomizationDialog";














this.FxAccountsWebChannel = function(options) {
  if (!options) {
    throw new Error("Missing configuration options");
  }
  if (!options["content_uri"]) {
    throw new Error("Missing 'content_uri' option");
  }
  this._contentUri = options.content_uri;

  if (!options["channel_id"]) {
    throw new Error("Missing 'channel_id' option");
  }
  this._webChannelId = options.channel_id;

  
  this._helpers = options.helpers || new FxAccountsWebChannelHelpers(options);

  this._setupChannel();
};

this.FxAccountsWebChannel.prototype = {
  


  _channel: null,

  


  _helpers: null,

  


  _webChannelId: null,
  


  _webChannelOrigin: null,

  


  tearDown() {
    this._channel.stopListening();
    this._channel = null;
    this._channelCallback = null;
  },

  




  _setupChannel() {
    
    try {
      this._webChannelOrigin = Services.io.newURI(this._contentUri, null, null);
      this._registerChannel();
    } catch (e) {
      log.error(e);
      throw e;
    }
  },

  



  _registerChannel() {
    


















    let listener = (webChannelId, message, sendingContext) => {
      if (message) {
        log.debug("FxAccountsWebChannel message received", message);
        let command = message.command;
        let data = message.data;

        switch (command) {
          case COMMAND_PROFILE_CHANGE:
            Services.obs.notifyObservers(null, ON_PROFILE_CHANGE_NOTIFICATION, data.uid);
            break;
          case COMMAND_LOGIN:
            this._helpers.login(data);
            break;
          case COMMAND_CAN_LINK_ACCOUNT:
            let canLinkAccount = this._helpers.shouldAllowRelink(data.email);

            let response = {
              command: command,
              messageId: message.messageId,
              data: { ok: canLinkAccount }
            };

            log.debug("FxAccountsWebChannel response", response);
            this._channel.send(response, sendingContext);
            break;
          default:
            log.warn("Unrecognized FxAccountsWebChannel command", command);
            break;
        }
      }
    };

    this._channelCallback = listener;
    this._channel = new WebChannel(this._webChannelId, this._webChannelOrigin);
    this._channel.listen(listener);
    log.debug("FxAccountsWebChannel registered: " + this._webChannelId + " with origin " + this._webChannelOrigin.prePath);
  }
};

this.FxAccountsWebChannelHelpers = function(options) {
  options = options || {};

  this._fxAccounts = options.fxAccounts || fxAccounts;
};

this.FxAccountsWebChannelHelpers.prototype = {
  
  
  
  
  
  shouldAllowRelink(acctName) {
    return !this._needRelinkWarning(acctName) ||
            this._promptForRelink(acctName);
  },

  







  setShowCustomizeSyncPref(showCustomizeSyncPref) {
    Services.prefs.setBoolPref(PREF_SYNC_SHOW_CUSTOMIZATION, showCustomizeSyncPref);
  },

  getShowCustomizeSyncPref(showCustomizeSyncPref) {
    return Services.prefs.getBoolPref(PREF_SYNC_SHOW_CUSTOMIZATION);
  },

  




  login(accountData) {
    if (accountData.customizeSync) {
      this.setShowCustomizeSyncPref(true);
      delete accountData.customizeSync;
    }

    
    
    delete accountData.verifiedCanLinkAccount;

    
    this.setPreviousAccountNameHashPref(accountData.email);

    
    
    let xps = Cc["@mozilla.org/weave/service;1"]
              .getService(Ci.nsISupports)
              .wrappedJSObject;
    return xps.whenLoaded().then(() => {
      return this._fxAccounts.setSignedInUser(accountData);
    });
  },

  


  getPreviousAccountNameHashPref() {
    try {
      return Services.prefs.getComplexValue(PREF_LAST_FXA_USER, Ci.nsISupportsString).data;
    } catch (_) {
      return "";
    }
  },

  




  setPreviousAccountNameHashPref(acctName) {
    let string = Cc["@mozilla.org/supports-string;1"]
                 .createInstance(Ci.nsISupportsString);
    string.data = this.sha256(acctName);
    Services.prefs.setComplexValue(PREF_LAST_FXA_USER, Ci.nsISupportsString, string);
  },

  


  sha256(str) {
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                      .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    
    let data = converter.convertToByteArray(str, {});
    let hasher = Cc["@mozilla.org/security/hash;1"]
                   .createInstance(Ci.nsICryptoHash);
    hasher.init(hasher.SHA256);
    hasher.update(data, data.length);

    return hasher.finish(true);
  },

  






  _needRelinkWarning(acctName) {
    let prevAcctHash = this.getPreviousAccountNameHashPref();
    return prevAcctHash && prevAcctHash != this.sha256(acctName);
  },

  





  _promptForRelink(acctName) {
    let sb = Services.strings.createBundle("chrome://browser/locale/syncSetup.properties");
    let continueLabel = sb.GetStringFromName("continue.label");
    let title = sb.GetStringFromName("relinkVerify.title");
    let description = sb.formatStringFromName("relinkVerify.description",
                                              [acctName], 1);
    let body = sb.GetStringFromName("relinkVerify.heading") +
               "\n\n" + description;
    let ps = Services.prompt;
    let buttonFlags = (ps.BUTTON_POS_0 * ps.BUTTON_TITLE_IS_STRING) +
                      (ps.BUTTON_POS_1 * ps.BUTTON_TITLE_CANCEL) +
                      ps.BUTTON_POS_1_DEFAULT;

    
    var targetWindow = typeof window === 'undefined' ? null : window;
    let pressed = Services.prompt.confirmEx(targetWindow, title, body, buttonFlags,
                                       continueLabel, null, null, null,
                                       {});
    return pressed === 0; 
  }
};
