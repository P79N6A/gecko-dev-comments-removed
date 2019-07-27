





var loop = loop || {};
loop.store = loop.store || {};





loop.store.StandaloneAppStore = (function() {
  "use strict";

  var sharedActions = loop.shared.actions;
  var sharedUtils = loop.shared.utils;

  var OLD_STYLE_CALL_REGEXP = /\#call\/(.*)/;
  var NEW_STYLE_CALL_REGEXP = /\/c\/([\w\-]+)$/;
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
        
        match = extractId(windowPath, OLD_STYLE_CALL_REGEXP) ||
                extractId(windowPath, NEW_STYLE_CALL_REGEXP);

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

    





    extractTokenInfo: function(actionData) {
      var windowType = "home";
      var token;

      
      if (sharedUtils.isIOS(navigator.platform)) {
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
        isFirefox: sharedUtils.isFirefox(navigator.userAgent)
      });

      
      
      if (token) {
        this._dispatcher.dispatch(new loop.shared.actions.FetchServerData({
          token: token,
          windowType: windowType
        }));
      }
    }
  }, Backbone.Events);

  return StandaloneAppStore;
})();
