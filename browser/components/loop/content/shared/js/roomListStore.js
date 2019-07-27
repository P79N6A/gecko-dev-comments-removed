





var loop = loop || {};
loop.store = loop.store || {};

(function() {
  "use strict";

  



  var sharedActions = loop.shared.actions;

  



  var roomSchema = {
    roomToken:    String,
    roomUrl:      String,
    roomName:     String,
    maxSize:      Number,
    participants: Array,
    ctime:        Number
  };

  




  function Room(values) {
    var validatedData = new loop.validate.Validator(roomSchema || {})
                                         .validate(values || {});
    for (var prop in validatedData) {
      this[prop] = validatedData[prop];
    }
  }

  loop.store.Room = Room;

  










  function RoomListStore(options) {
    options = options || {};
    this.storeState = {error: null, rooms: []};

    if (!options.dispatcher) {
      throw new Error("Missing option dispatcher");
    }
    this.dispatcher = options.dispatcher;

    if (!options.mozLoop) {
      throw new Error("Missing option mozLoop");
    }
    this.mozLoop = options.mozLoop;

    this.dispatcher.register(this, [
      "getAllRooms",
      "getAllRoomsError",
      "openRoom",
      "updateRoomList"
    ]);
  }

  RoomListStore.prototype = _.extend({
    




    getStoreState: function() {
      return this.storeState;
    },

    




    setStoreState: function(state) {
      this.storeState = state;
      this.trigger("change");
    },

    





    _processRawRoomList: function(rawRoomList) {
      if (!rawRoomList) {
        return [];
      }
      return rawRoomList
        .map(function(rawRoom) {
          return new Room(rawRoom);
        })
        .slice()
        .sort(function(a, b) {
          return b.ctime - a.ctime;
        });
    },

    


    getAllRooms: function() {
      this.mozLoop.rooms.getAll(function(err, rawRoomList) {
        var action;
        if (err) {
          action = new sharedActions.GetAllRoomsError({error: err});
        } else {
          action = new sharedActions.UpdateRoomList({roomList: rawRoomList});
        }
        this.dispatcher.dispatch(action);
      }.bind(this));
    },

    




    getAllRoomsError: function(actionData) {
      this.setStoreState({error: actionData.error});
    },

    




    updateRoomList: function(actionData) {
      this.setStoreState({
        error: undefined,
        rooms: this._processRawRoomList(actionData.roomList)
      });
    },
  }, Backbone.Events);

  loop.store.RoomListStore = RoomListStore;
})();
