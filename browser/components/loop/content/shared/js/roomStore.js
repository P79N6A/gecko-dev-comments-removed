





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

  












  function RoomStore(options) {
    options = options || {};

    if (!options.dispatcher) {
      throw new Error("Missing option dispatcher");
    }
    this._dispatcher = options.dispatcher;

    if (!options.mozLoop) {
      throw new Error("Missing option mozLoop");
    }
    this._mozLoop = options.mozLoop;

    if (options.activeRoomStore) {
      this.activeRoomStore = options.activeRoomStore;
      this.setStoreState({activeRoom: this.activeRoomStore.getStoreState()});
      this.activeRoomStore.on("change",
                              this._onActiveRoomStoreChange.bind(this));
    }

    this._dispatcher.register(this, [
      "createRoom",
      "createRoomError",
      "copyRoomUrl",
      "deleteRoom",
      "deleteRoomError",
      "emailRoomUrl",
      "getAllRooms",
      "getAllRoomsError",
      "openRoom",
      "renameRoom",
      "updateRoomList"
    ]);
  }

  RoomStore.prototype = _.extend({
    





    maxRoomCreationSize: 2,

    



    defaultExpiresIn: 24 * 7 * 8,

    




    _storeState: {
      activeRoom: {},
      error: null,
      pendingCreation: false,
      pendingInitialRetrieval: false,
      rooms: []
    },

    














    getStoreState: function(key) {
      if (key) {
        return this._storeState[key];
      }
      return this._storeState;
    },

    











    setStoreState: function(newState) {
      for (var key in newState) {
        this._storeState[key] = newState[key];
        this.trigger("change:" + key);
      }
      this.trigger("change");
    },

    


    startListeningToRoomEvents: function() {
      
      this._mozLoop.rooms.on("add", this._onRoomAdded.bind(this));
      this._mozLoop.rooms.on("update", this._onRoomUpdated.bind(this));
      this._mozLoop.rooms.on("delete", this._onRoomRemoved.bind(this));
    },

    


    _onActiveRoomStoreChange: function() {
      this.setStoreState({activeRoom: this.activeRoomStore.getStoreState()});
    },

    




    _dispatchAction: function(action) {
      this._dispatcher.dispatch(action);
    },

    





    _onRoomAdded: function(eventName, addedRoomData) {
      addedRoomData.participants = [];
      addedRoomData.ctime = new Date().getTime();
      this._dispatchAction(new sharedActions.UpdateRoomList({
        roomList: this._storeState.rooms.concat(new Room(addedRoomData))
      }));
    },

    





    _onRoomUpdated: function(eventName, updatedRoomData) {
      this._dispatchAction(new sharedActions.UpdateRoomList({
        roomList: this._storeState.rooms.map(function(room) {
          return room.roomToken === updatedRoomData.roomToken ?
                 updatedRoomData : room;
        })
      }));
    },

    





    _onRoomRemoved: function(eventName, removedRoomData) {
      this._dispatchAction(new sharedActions.UpdateRoomList({
        roomList: this._storeState.rooms.filter(function(room) {
          return room.roomToken !== removedRoomData.roomToken;
        })
      }));
    },

    





    _processRoomList: function(rawRoomList) {
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

    






    findNextAvailableRoomNumber: function(nameTemplate) {
      var searchTemplate = nameTemplate.replace("{{conversationLabel}}", "");
      var searchRegExp = new RegExp("^" + searchTemplate + "(\\d+)$");

      var roomNumbers = this._storeState.rooms.map(function(room) {
        var match = searchRegExp.exec(room.roomName);
        return match && match[1] ? parseInt(match[1], 10) : 0;
      });

      if (!roomNumbers.length) {
        return 1;
      }

      return Math.max.apply(null, roomNumbers) + 1;
    },

    





    _generateNewRoomName: function(nameTemplate) {
      var roomLabel = this.findNextAvailableRoomNumber(nameTemplate);
      return nameTemplate.replace("{{conversationLabel}}", roomLabel);
    },

    




    createRoom: function(actionData) {
      this.setStoreState({pendingCreation: true});

      var roomCreationData = {
        roomName:  this._generateNewRoomName(actionData.nameTemplate),
        roomOwner: actionData.roomOwner,
        maxSize:   this.maxRoomCreationSize,
        expiresIn: this.defaultExpiresIn
      };

      this._mozLoop.rooms.create(roomCreationData, function(err) {
        this.setStoreState({pendingCreation: false});
        if (err) {
          this._dispatchAction(new sharedActions.CreateRoomError({error: err}));
        }
      }.bind(this));
    },

    




    createRoomError: function(actionData) {
      this.setStoreState({
        error: actionData.error,
        pendingCreation: false
      });
    },

    




    copyRoomUrl: function(actionData) {
      this._mozLoop.copyString(actionData.roomUrl);
    },

    




    emailRoomUrl: function(actionData) {
      loop.shared.utils.composeCallUrlEmail(actionData.roomUrl);
    },

    




    deleteRoom: function(actionData) {
      this._mozLoop.rooms.delete(actionData.roomToken, function(err) {
        if (err) {
         this._dispatchAction(new sharedActions.DeleteRoomError({error: err}));
        }
      }.bind(this));
    },

    




    deleteRoomError: function(actionData) {
      this.setStoreState({error: actionData.error});
    },

    


    getAllRooms: function() {
      this.setStoreState({pendingInitialRetrieval: true});
      this._mozLoop.rooms.getAll(null, function(err, rawRoomList) {
        var action;

        this.setStoreState({pendingInitialRetrieval: false});

        if (err) {
          action = new sharedActions.GetAllRoomsError({error: err});
        } else {
          action = new sharedActions.UpdateRoomList({roomList: rawRoomList});
        }

        this._dispatchAction(action);

        
        
        this.startListeningToRoomEvents();
      }.bind(this));
    },

    




    getAllRoomsError: function(actionData) {
      this.setStoreState({error: actionData.error});
    },

    




    updateRoomList: function(actionData) {
      this.setStoreState({
        error: undefined,
        rooms: this._processRoomList(actionData.roomList)
      });
    },

    




    openRoom: function(actionData) {
      this._mozLoop.rooms.open(actionData.roomToken);
    },

    




    renameRoom: function(actionData) {
      this._mozLoop.rooms.rename(actionData.roomToken, actionData.newRoomName,
        function(err) {
          if (err) {
            
            console.error("Failed to rename the room", err);
          }
        });
    }
  }, Backbone.Events);

  loop.store.RoomStore = RoomStore;
})();
