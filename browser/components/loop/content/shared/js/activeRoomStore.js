





var loop = loop || {};
loop.store = loop.store || {};
loop.store.ActiveRoomStore = (function() {
  "use strict";

  var sharedActions = loop.shared.actions;

  











  function ActiveRoomStore(options) {
    options = options || {};

    if (!options.dispatcher) {
      throw new Error("Missing option dispatcher");
    }
    this.dispatcher = options.dispatcher;

    if (!options.mozLoop) {
      throw new Error("Missing option mozLoop");
    }
    this.mozLoop = options.mozLoop;

    this.dispatcher.register(this, [
      "setupWindowData"
    ]);
  }

  ActiveRoomStore.prototype = _.extend({

    











    _storeState: {
    },

    getStoreState: function() {
      return this._storeState;
    },

    setStoreState: function(state) {
      this._storeState = state;
      this.trigger("change");
    },

    











    setupWindowData: function(actionData) {
      if (actionData.type !== "room") {
        
        return;
      }

      this.mozLoop.rooms.get(actionData.roomToken,
        function(error, roomData) {
          this.setStoreState({
            error: error,
            roomToken: actionData.roomToken,
            serverData: roomData
          });
        }.bind(this));
    }

  }, Backbone.Events);

  return ActiveRoomStore;

})();
