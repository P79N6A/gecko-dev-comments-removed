





var loop = loop || {};
loop.store = loop.store || {};

loop.store.FxOSActiveRoomStore = (function() {
  "use strict";
  var sharedActions = loop.shared.actions;
  var ROOM_STATES = loop.store.ROOM_STATES;

  var FxOSActiveRoomStore = loop.store.createStore({
    actions: [
      "fetchServerData"
    ],

    initialize: function(options) {
      if (!options.mozLoop) {
        throw new Error("Missing option mozLoop");
      }
      this._mozLoop = options.mozLoop;
    },

    


    getInitialStoreState: function() {
      return {
        roomState: ROOM_STATES.INIT,
        audioMuted: false,
        videoMuted: false,
        failureReason: undefined
      };
    },

    



    _registerPostSetupActions: function() {
      this.dispatcher.register(this, [
        "joinRoom"
      ]);
    },

    







    fetchServerData: function(actionData) {
      if (actionData.windowType !== "room") {
        
        return;
      }

      this._registerPostSetupActions();

      this.setStoreState({
        roomToken: actionData.token,
        roomState: ROOM_STATES.READY
      });

    },

    


    joinRoom: function() {
      
      if (this.getStoreState().failureReason) {
        this.setStoreState({failureReason: undefined});
      }

      this._setupOutgoingRoom(true);
    },

    








    _setupOutgoingRoom: function(installApp) {
      var request = new MozActivity({
        name: "room-call",
        data: {
          type: "loop/rToken",
          token: this.getStoreState("roomToken")
        }
      });

      request.onsuccess = function() {};

      request.onerror = (function(event) {
        if (!installApp) {
          
          console.error(
           "Unexpected activity launch error after the app has been installed");
          return;
        }
        if (event.target.error.name !== "NO_PROVIDER") {
          console.error ("Unexpected " + event.target.error.name);
          return;
        }
        
        this.setStoreState({
            marketplaceSrc: loop.config.marketplaceUrl,
            onMarketplaceMessage: this._onMarketplaceMessage.bind(this)
        });
      }).bind(this);
    },

    






    _onMarketplaceMessage: function(event) {
      var message = event.data;
      switch (message.name) {
        case "loaded":
          var marketplace = window.document.getElementById("marketplace");
          
          
          
          marketplace.contentWindow.postMessage({
            "name": "install-package",
            "data": {
              "product": {
                "name": loop.config.fxosApp.name,
                "manifest_url": loop.config.fxosApp.manifestUrl,
                "is_packaged": true
              }
            }
          }, "*");
          break;
        case "install-package":
          window.removeEventListener("message", this.onMarketplaceMessage);
          if (message.error) {
            console.error(message.error.error);
            return;
          }
          
          
          this._setupOutgoingRoom(false);
          break;
      }
    }
  });

  return FxOSActiveRoomStore;
})();
