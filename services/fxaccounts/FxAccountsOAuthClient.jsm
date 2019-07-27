








this.EXPORTED_SYMBOLS = ["FxAccountsOAuthClient"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
XPCOMUtils.defineLazyModuleGetter(this, "WebChannel",
                                  "resource://gre/modules/WebChannel.jsm");
Cu.importGlobalProperties(["URL"]);


























this.FxAccountsOAuthClient = function(options) {
  this._validateOptions(options);
  this.parameters = options.parameters;
  this._configureChannel();

  let authorizationEndpoint = options.authorizationEndpoint || "/authorization";

  try {
    this._fxaOAuthStartUrl = new URL(this.parameters.oauth_uri + authorizationEndpoint + "?");
  } catch (e) {
    throw new Error("Invalid OAuth Url");
  }

  let params = this._fxaOAuthStartUrl.searchParams;
  params.append("client_id", this.parameters.client_id);
  params.append("state", this.parameters.state);
  params.append("scope", this.parameters.scope || "");
  params.append("action", this.parameters.action || "signin");
  params.append("webChannelId", this._webChannelId);
  if (this.parameters.keys) {
    params.append("keys", "true");
  }

};

this.FxAccountsOAuthClient.prototype = {
  





  onComplete: null,
  




  onError: null,
  


  parameters: null,
  


  _channel: null,
  


  _complete: false,
  


  _fxaOAuthStartUrl: null,
  


  _webChannelId: null,
  


  _webChannelOrigin: null,
  



  launchWebFlow: function () {
    if (!this._channelCallback) {
      this._registerChannel();
    }

    if (this._complete) {
      throw new Error("This client already completed the OAuth flow");
    } else {
      let opener = Services.wm.getMostRecentWindow("navigator:browser").gBrowser;
      opener.selectedTab = opener.addTab(this._fxaOAuthStartUrl.href);
    }
  },

  


  tearDown: function() {
    this.onComplete = null;
    this.onError = null;
    this._complete = true;
    this._channel.stopListening();
    this._channel = null;
  },

  




  _configureChannel: function() {
    this._webChannelId = "oauth_" + this.parameters.client_id;

    
    try {
      this._webChannelOrigin = Services.io.newURI(this.parameters.content_uri, null, null);
    } catch (e) {
      throw e;
    }
  },

  



  _registerChannel: function() {
    










    let listener = function (webChannelId, message, sendingContext) {
      if (message) {
        let command = message.command;
        let data = message.data;
        let target = sendingContext && sendingContext.browser;

        switch (command) {
          case "oauth_complete":
            
            let result = null;
            let err = null;

            if (this.parameters.state !== data.state) {
              err = new Error("OAuth flow failed. State doesn't match");
            } else if (this.parameters.keys && !data.keys) {
              err = new Error("OAuth flow failed. Keys were not returned");
            } else {
              result = {
                code: data.code,
                state: data.state
              };
            }

            if (err) {
              log.debug(err.message);
              if (this.onError) {
                this.onError(err);
              }
            } else {
              log.debug("OAuth flow completed.");
              if (this.onComplete) {
                if (this.parameters.keys) {
                  this.onComplete(result, data.keys);
                } else {
                  this.onComplete(result);
                }
              }
            }

            
            
            this.tearDown();

            
            if (data.closeWindow && target) {
              
              let tabbrowser = target.getTabBrowser();

              if (tabbrowser) {
                let tab = tabbrowser.getTabForBrowser(target);

                if (tab) {
                  tabbrowser.removeTab(tab);
                  log.debug("OAuth flow closed the tab.");
                } else {
                  log.debug("OAuth flow failed to close the tab. Tab not found in TabBrowser.");
                }
              } else {
                log.debug("OAuth flow failed to close the tab. TabBrowser not found.");
              }
            }
            break;
        }
      }
    };

    this._channelCallback = listener.bind(this);
    this._channel = new WebChannel(this._webChannelId, this._webChannelOrigin);
    this._channel.listen(this._channelCallback);
    log.debug("Channel registered: " + this._webChannelId + " with origin " + this._webChannelOrigin.prePath);
  },

  






  _validateOptions: function (options) {
    if (!options || !options.parameters) {
      throw new Error("Missing 'parameters' configuration option");
    }

    ["oauth_uri", "client_id", "content_uri", "state"].forEach(option => {
      if (!options.parameters[option]) {
        throw new Error("Missing 'parameters." + option + "' parameter");
      }
    });
  },
};
