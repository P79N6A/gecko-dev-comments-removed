





var loop = loop || {};
loop.store = loop.store || {};





loop.store.StandaloneAppStore = (function() {
  "use strict";

  var sharedActions = loop.shared.actions;
  var sharedUtils = loop.shared.utils;

  var CALL_REGEXP = /\/c\/([\w\-]+)$/;
  var ROOM_REGEXP = /\/([\w\-]+)$/;

  




  var StandaloneAppStore = function(options) {
    if (!options.dispatcher) {
      throw new Error("Missing option dispatcher");
    }
    if (!options.sdk) {
      throw new Error("Missing option sdk");
    }
    if (!options.conversation) {
      throw new Error("Missing option conversation");
    }

    this._dispatcher = options.dispatcher;
    this._storeState = {};
    this._sdk = options.sdk;
    this._conversation = options.conversation;

    this._dispatcher.register(this, [
      "extractTokenInfo"
    ]);
  };

  StandaloneAppStore.prototype = _.extend({
    




    getStoreState: function() {
      return this._storeState;
    },

    




    setStoreState: function(state) {
      this._storeState = state;
      this.trigger("change");
    },

    _extractWindowDataFromPath: function(windowPath) {
      var match;
      var windowType = "home";

      function extractId(path, regexp) {
        var match = path.match(regexp);
        if (match && match[1]) {
          return match;
        }
        return null;
      }

      if (windowPath) {
        match = extractId(windowPath, CALL_REGEXP);

        if (match) {
          windowType = "outgoing";
        } else {
          
          match = extractId(windowPath, ROOM_REGEXP);

          if (match) {
            windowType = "room";
          }
        }
      }
      return [windowType, match && match[1] ? match[1] : null];
    },

    


    _extractCryptoKey: function(windowHash) {
      if (windowHash && windowHash[0] === "#") {
        return windowHash.substring(1, windowHash.length);
      }

      return null;
    },

    






    extractTokenInfo: function(actionData) {
      var windowType = "home";
      var token;

      
      var unsupportedPlatform =
        sharedUtils.getUnsupportedPlatform(navigator.platform);

      if (unsupportedPlatform) {
        windowType = "unsupportedDevice";
      } else if (!this._sdk.checkSystemRequirements()) {
        windowType = "unsupportedBrowser";
      } else if (actionData.windowPath) {
        
        var result = this._extractWindowDataFromPath(actionData.windowPath);
        windowType = result[0];
        token = result[1];
      }
      

      if (token) {
        this._conversation.set({loopToken: token});
      }

      this.setStoreState({
        windowType: windowType,
        isFirefox: sharedUtils.isFirefox(navigator.userAgent),
        unsupportedPlatform: unsupportedPlatform
      });

      
      
      if (token) {
        this._dispatcher.dispatch(new loop.shared.actions.FetchServerData({
          cryptoKey: this._extractCryptoKey(actionData.windowHash),
          token: token,
          windowType: windowType
        }));
      }
    }
  }, Backbone.Events);

  return StandaloneAppStore;
})();
