





var loop = loop || {};
loop.store = loop.store || {};

(function() {
  "use strict";

  



  var roomSchema = {
    roomToken: String,
    roomUrl:   String,
    roomName:  String,
    maxSize:   Number,
    currSize:  Number,
    ctime:     Number
  };

  





  var temporaryRawRoomList = [{
    roomToken: "_nxD4V4FflQ",
    roomUrl: "http://sample/_nxD4V4FflQ",
    roomName: "First Room Name",
    maxSize: 2,
    currSize: 0,
    ctime: 1405517546
  }, {
    roomToken: "QzBbvGmIZWU",
    roomUrl: "http://sample/QzBbvGmIZWU",
    roomName: "Second Room Name",
    maxSize: 2,
    currSize: 0,
    ctime: 1405517418
  }, {
    roomToken: "3jKS_Els9IU",
    roomUrl: "http://sample/3jKS_Els9IU",
    roomName: "Third Room Name",
    maxSize: 3,
    clientMaxSize: 2,
    currSize: 1,
    ctime: 1405518241
  }];

  




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
      "openRoom"
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

    





    _fetchRoomList: function(cb) {
      
      if (!this.mozLoop.hasOwnProperty("rooms")) {
        cb(null, temporaryRawRoomList);
        return;
      }
      this.mozLoop.rooms.getAll(cb);
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
      this._fetchRoomList(function(err, rawRoomList) {
        this.setStoreState({
          error: err,
          rooms: this._processRawRoomList(rawRoomList)
        });
      }.bind(this));
    }
  }, Backbone.Events);

  loop.store.RoomListStore = RoomListStore;
})();
