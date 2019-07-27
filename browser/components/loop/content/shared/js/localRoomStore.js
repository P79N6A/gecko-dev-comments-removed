





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

    this.dispatcher.register(this, ["setupEmptyRoom"]);
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

    







    _fetchRoomData: function(actionData, cb) {
      if (this.mozLoop.rooms && this.mozLoop.rooms.getRoomData) {
        this.mozLoop.rooms.getRoomData(actionData.localRoomId, cb);
      } else {
        cb(null, {roomName: "Donkeys"});
      }
    },

    











    setupEmptyRoom: function(actionData) {
      this._fetchRoomData(actionData, function(error, roomData) {
        this.setStoreState({
          error: error,
          localRoomId: actionData.localRoomId,
          serverData: roomData
        });
      }.bind(this));
    }

  }, Backbone.Events);

  return LocalRoomStore;

})();
