



var loop = loop || {};
loop.store = loop.store || {};

(function(mozL10n) {
  "use strict";

  



  var sharedActions = loop.shared.actions;

  





  var MAX_ROOM_CREATION_SIZE = loop.store.MAX_ROOM_CREATION_SIZE = 2;

  



  var roomSchema = {
    roomToken: String,
    roomUrl: String,
    
    
    maxSize: Number,
    participants: Array,
    ctime: Number
  };

  




  function Room(values) {
    var validatedData = new loop.validate.Validator(roomSchema || {})
                                         .validate(values || {});
    for (var prop in validatedData) {
      this[prop] = validatedData[prop];
    }
  }

  loop.store.Room = Room;

  











  loop.store.RoomStore = loop.store.createStore({
    





    maxRoomCreationSize: MAX_ROOM_CREATION_SIZE,

    



    actions: [
      "addSocialShareProvider",
      "createRoom",
      "createdRoom",
      "createRoomError",
      "copyRoomUrl",
      "deleteRoom",
      "deleteRoomError",
      "emailRoomUrl",
      "getAllRooms",
      "getAllRoomsError",
      "openRoom",
      "shareRoomUrl",
      "updateRoomContext",
      "updateRoomContextDone",
      "updateRoomContextError",
      "updateRoomList"
    ],

    initialize: function(options) {
      if (!options.mozLoop) {
        throw new Error("Missing option mozLoop");
      }
      this._mozLoop = options.mozLoop;
      this._notifications = options.notifications;

      if (options.activeRoomStore) {
        this.activeRoomStore = options.activeRoomStore;
        this.activeRoomStore.on("change",
                                this._onActiveRoomStoreChange.bind(this));
      }
    },

    getInitialStoreState: function() {
      return {
        activeRoom: this.activeRoomStore ? this.activeRoomStore.getStoreState() : {},
        error: null,
        pendingCreation: false,
        pendingInitialRetrieval: false,
        rooms: [],
        savingContext: false
      };
    },

    


    startListeningToRoomEvents: function() {
      
      this._mozLoop.rooms.on("add", this._onRoomAdded.bind(this));
      this._mozLoop.rooms.on("update", this._onRoomUpdated.bind(this));
      this._mozLoop.rooms.on("delete", this._onRoomRemoved.bind(this));
      this._mozLoop.rooms.on("refresh", this._onRoomsRefresh.bind(this));
    },

    


    _onActiveRoomStoreChange: function() {
      this.setStoreState({activeRoom: this.activeRoomStore.getStoreState()});
    },

    





    _onRoomAdded: function(eventName, addedRoomData) {
      addedRoomData.participants = addedRoomData.participants || [];
      addedRoomData.ctime = addedRoomData.ctime || new Date().getTime();
      this.dispatchAction(new sharedActions.UpdateRoomList({
        
        roomList: this._storeState.rooms.filter(function(room) {
          return addedRoomData.roomToken !== room.roomToken;
        }).concat(new Room(addedRoomData))
      }));
    },

    





    _onRoomUpdated: function(eventName, updatedRoomData) {
      this.dispatchAction(new sharedActions.UpdateRoomList({
        roomList: this._storeState.rooms.map(function(room) {
          return room.roomToken === updatedRoomData.roomToken ?
                 updatedRoomData : room;
        })
      }));
    },

    





    _onRoomRemoved: function(eventName, removedRoomData) {
      this.dispatchAction(new sharedActions.UpdateRoomList({
        roomList: this._storeState.rooms.filter(function(room) {
          return room.roomToken !== removedRoomData.roomToken;
        })
      }));
    },

    




    _onRoomsRefresh: function(eventName) {
      this.dispatchAction(new sharedActions.UpdateRoomList({
        roomList: []
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
        var match = searchRegExp.exec(room.decryptedContext.roomName);
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
      this.setStoreState({
        pendingCreation: true,
        error: null
      });

      var roomCreationData = {
        decryptedContext: {
          roomName: this._generateNewRoomName(actionData.nameTemplate)
        },
        roomOwner: actionData.roomOwner,
        maxSize: this.maxRoomCreationSize
      };

      if ("urls" in actionData) {
        roomCreationData.decryptedContext.urls = actionData.urls;
      }

      this._notifications.remove("create-room-error");

      this._mozLoop.rooms.create(roomCreationData, function(err, createdRoom) {
        if (err) {
          this.dispatchAction(new sharedActions.CreateRoomError({error: err}));
          return;
        }

        this.dispatchAction(new sharedActions.CreatedRoom({
          roomToken: createdRoom.roomToken
        }));
      }.bind(this));
    },

    


    createdRoom: function(actionData) {
      this.setStoreState({pendingCreation: false});

      
      this.dispatchAction(new sharedActions.OpenRoom({
        roomToken: actionData.roomToken
      }));
    },

    




    createRoomError: function(actionData) {
      this.setStoreState({
        error: actionData.error,
        pendingCreation: false
      });

      
      this._notifications.set({
        id: "create-room-error",
        level: "error",
        message: mozL10n.get("generic_failure_title")
      });
    },

    




    copyRoomUrl: function(actionData) {
      this._mozLoop.copyString(actionData.roomUrl);
      this._mozLoop.notifyUITour("Loop:RoomURLCopied");
    },

    




    emailRoomUrl: function(actionData) {
      loop.shared.utils.composeCallUrlEmail(actionData.roomUrl, null,
        actionData.roomDescription);
      this._mozLoop.notifyUITour("Loop:RoomURLEmailed");
    },

    




    shareRoomUrl: function(actionData) {
      var providerOrigin = new URL(actionData.provider.origin).hostname;
      var shareTitle = "";
      var shareBody = null;

      switch (providerOrigin) {
        case "mail.google.com":
          shareTitle = mozL10n.get("share_email_subject5", {
            clientShortname2: mozL10n.get("clientShortname2")
          });
          shareBody = mozL10n.get("share_email_body5", {
            callUrl: actionData.roomUrl,
            brandShortname: mozL10n.get("brandShortname"),
            clientShortname2: mozL10n.get("clientShortname2"),
            clientSuperShortname: mozL10n.get("clientSuperShortname"),
            learnMoreUrl: this._mozLoop.getLoopPref("learnMoreUrl")
          });
          break;
        case "twitter.com":
        default:
          shareTitle = mozL10n.get("share_tweet", {
            clientShortname2: mozL10n.get("clientShortname2")
          });
          break;
      }

      this._mozLoop.socialShareRoom(actionData.provider.origin, actionData.roomUrl,
        shareTitle, shareBody);
      this._mozLoop.notifyUITour("Loop:RoomURLShared");
    },

    




    addSocialShareProvider: function(actionData) {
      this._mozLoop.addSocialShareProvider();
    },

    




    deleteRoom: function(actionData) {
      this._mozLoop.rooms.delete(actionData.roomToken, function(err) {
        if (err) {
         this.dispatchAction(new sharedActions.DeleteRoomError({error: err}));
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

        this.dispatchAction(action);

        
        
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

    




    updateRoomContext: function(actionData) {
      this.setStoreState({ savingContext: true });
      this._mozLoop.rooms.get(actionData.roomToken, function(err, room) {
        if (err) {
          this.dispatchAction(new sharedActions.UpdateRoomContextError({
            error: err
          }));
          return;
        }

        var roomData = {};
        var context = room.decryptedContext;
        var oldRoomName = context.roomName;
        var newRoomName = actionData.newRoomName.trim();
        if (newRoomName && oldRoomName != newRoomName) {
          roomData.roomName = newRoomName;
        }
        var oldRoomURLs = context.urls;
        var oldRoomURL = oldRoomURLs && oldRoomURLs[0];
        
        
        var newRoomURL = loop.shared.utils.stripFalsyValues({
          location: actionData.newRoomURL ? actionData.newRoomURL.trim() : "",
          thumbnail: actionData.newRoomURL ? actionData.newRoomThumbnail.trim() : "",
          description: actionData.newRoomDescription ?
            actionData.newRoomDescription.trim() : ""
        });
        
        
        
        
        var diff = loop.shared.utils.objectDiff(oldRoomURL, newRoomURL);
        if (diff.added.length || diff.updated.length) {
          newRoomURL = _.extend(oldRoomURL || {}, newRoomURL);
          var isValidURL = false;
          try {
            isValidURL = new URL(newRoomURL.location);
          } catch(ex) {}
          if (isValidURL) {
            roomData.urls = [newRoomURL];
          }
        }
        
        
        

        
        
        if (!Object.getOwnPropertyNames(roomData).length) {
          this.dispatchAction(new sharedActions.UpdateRoomContextDone());
          return;
        }

        this.setStoreState({error: null});
        this._mozLoop.rooms.update(actionData.roomToken, roomData,
          function(err, data) {
            var action = err ?
              new sharedActions.UpdateRoomContextError({ error: err }) :
              new sharedActions.UpdateRoomContextDone();
            this.dispatchAction(action);
          }.bind(this));
      }.bind(this));
    },

    


    updateRoomContextDone: function() {
      this.setStoreState({ savingContext: false });
    },

    




    updateRoomContextError: function(actionData) {
      this.setStoreState({
        error: actionData.error,
        savingContext: false
      });
    }
  });
})(document.mozL10n || navigator.mozL10n);
