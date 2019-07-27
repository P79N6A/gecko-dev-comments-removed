





var loop = loop || {};
loop.store = loop.store || {};
loop.store.LocalRoomStore = (function() {
  "use strict";

  var sharedActions = loop.shared.actions;

  











  function LocalRoomStore(options) {
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

  LocalRoomStore.prototype = _.extend({

    














    _storeState: {
    },

    getStoreState: function() {
      return this._storeState;
    },

    setStoreState: function(state) {
      this._storeState = state;
      this.trigger("change");
    },

    







    _fetchRoomData: function(roomId, cb) {
      
      if (!this.mozLoop.getLoopBoolPref("test.alwaysUseRooms")) {
        this.mozLoop.rooms.getRoomData(roomId, cb);
      } else {
        cb(null, {roomName: "Donkeys"});
      }
    },

    











    setupWindowData: function(actionData) {
      if (actionData.windowData.type !== "room") {
        
        return;
      }

      this._fetchRoomData(actionData.windowData.localRoomId,
        function(error, roomData) {
          this.setStoreState({
            error: error,
            localRoomId: actionData.windowData.localRoomId,
            serverData: roomData
          });
        }.bind(this));
    }

  }, Backbone.Events);

  return LocalRoomStore;

})();
