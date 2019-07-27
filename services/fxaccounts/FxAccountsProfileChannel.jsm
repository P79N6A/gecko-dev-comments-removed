








this.EXPORTED_SYMBOLS = ["FxAccountsProfileChannel"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");

XPCOMUtils.defineLazyModuleGetter(this, "WebChannel",
                                  "resource://gre/modules/WebChannel.jsm");

const PROFILE_CHANGE_COMMAND = "profile:change";










this.FxAccountsProfileChannel = function(options) {
  if (!options) {
    throw new Error("Missing configuration options");
  }
  if (!options["content_uri"]) {
    throw new Error("Missing 'content_uri' option");
  }
  this.parameters = options;

  this._setupChannel();
};

this.FxAccountsProfileChannel.prototype = {
  


  parameters: null,
  


  _channel: null,
  


  _webChannelOrigin: null,

  


  tearDown: function() {
    this._channel.stopListening();
    this._channel = null;
    this._channelCallback = null;
  },

  




  _setupChannel: function() {
    
    try {
      this._webChannelOrigin = Services.io.newURI(this.parameters.content_uri, null, null);
      this._registerChannel();
    } catch (e) {
      log.error(e);
      throw e;
    }
  },

  



  _registerChannel: function() {
    










    let listener = (webChannelId, message, target) => {
      if (message) {
        let command = message.command;
        let data = message.data;
        switch (command) {
          case PROFILE_CHANGE_COMMAND:
            Services.obs.notifyObservers(null, ON_PROFILE_CHANGE_NOTIFICATION, data.uid);
          break;
        }
      }
    };

    this._channelCallback = listener;
    this._channel = new WebChannel(PROFILE_WEBCHANNEL_ID, this._webChannelOrigin);
    this._channel.listen(this._channelCallback);
    log.debug("Channel registered: " + PROFILE_WEBCHANNEL_ID + " with origin " + this._webChannelOrigin.prePath);
  }

};
