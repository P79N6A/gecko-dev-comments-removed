


var expect = chai.expect;

describe("loop.store.Room", function () {
  "use strict";

  describe("#constructor", function() {
    it("should validate room values", function() {
      expect(function() {
        new loop.store.Room();
      }).to.Throw(Error, /missing required/);
    });
  });
});

describe("loop.store.RoomListStore", function () {
  "use strict";

  var sharedActions = loop.shared.actions;
  var sandbox, dispatcher;

  var fakeRoomList = [{
    roomToken: "_nxD4V4FflQ",
    roomUrl: "http://sample/_nxD4V4FflQ",
    roomName: "First Room Name",
    maxSize: 2,
    participants: [],
    ctime: 1405517546
  }, {
    roomToken: "QzBbvGmIZWU",
    roomUrl: "http://sample/QzBbvGmIZWU",
    roomName: "Second Room Name",
    maxSize: 2,
    participants: [],
    ctime: 1405517418
  }, {
    roomToken: "3jKS_Els9IU",
    roomUrl: "http://sample/3jKS_Els9IU",
    roomName: "Third Room Name",
    maxSize: 3,
    clientMaxSize: 2,
    participants: [],
    ctime: 1405518241
  }];

  beforeEach(function() {
    sandbox = sinon.sandbox.create();
    dispatcher = new loop.Dispatcher();
  });

  afterEach(function() {
    sandbox.restore();
  });

  describe("#constructor", function() {
    it("should throw an error if the dispatcher is missing", function() {
      expect(function() {
        new loop.store.RoomListStore({mozLoop: {}});
      }).to.Throw(/dispatcher/);
    });

    it("should throw an error if mozLoop is missing", function() {
      expect(function() {
        new loop.store.RoomListStore({dispatcher: dispatcher});
      }).to.Throw(/mozLoop/);
    });
  });

  describe("constructed", function() {
    var fakeMozLoop, store;

    beforeEach(function() {
      fakeMozLoop = {
        rooms: {
          create: function() {},
          getAll: function() {},
          on: sandbox.stub()
        }
      };
      store = new loop.store.RoomListStore({
        dispatcher: dispatcher,
        mozLoop: fakeMozLoop
      });
      store.setStoreState({
        error: undefined,
        pendingCreation: false,
        pendingInitialRetrieval: false,
        rooms: []
      });
    });

    describe("MozLoop rooms event listeners", function() {
      beforeEach(function() {
        _.extend(fakeMozLoop.rooms, Backbone.Events);

        fakeMozLoop.rooms.getAll = function(version, cb) {
          cb(null, fakeRoomList);
        };

        store.getAllRooms(); 
      });

      describe("add", function() {
        it("should add the room entry to the list", function() {
          fakeMozLoop.rooms.trigger("add", "add", {
            roomToken: "newToken",
            roomUrl: "http://sample/newToken",
            roomName: "New room",
            maxSize: 2,
            participants: [],
            ctime: 1405517546
          });

          expect(store.getStoreState().rooms).to.have.length.of(4);
        });
      });

      describe("update", function() {
        it("should update a room entry", function() {
          fakeMozLoop.rooms.trigger("update", "update", {
            roomToken: "_nxD4V4FflQ",
            roomUrl: "http://sample/_nxD4V4FflQ",
            roomName: "Changed First Room Name",
            maxSize: 2,
            participants: [],
            ctime: 1405517546
          });

          expect(store.getStoreState().rooms).to.have.length.of(3);
          expect(store.getStoreState().rooms.some(function(room) {
            return room.roomName === "Changed First Room Name";
          })).eql(true);
        });
      });

      describe("remove", function() {
        it("should remove a room from the list", function() {
          fakeMozLoop.rooms.trigger("remove", "remove", {
            roomToken: "_nxD4V4FflQ"
          });

          expect(store.getStoreState().rooms).to.have.length.of(2);
          expect(store.getStoreState().rooms.some(function(room) {
            return room.roomToken === "_nxD4V4FflQ";
          })).eql(false);
        });
      });
    });

    describe("#findNextAvailableRoomNumber", function() {
      var fakeNameTemplate = "RoomWord {{conversationLabel}}";

      it("should find next available room number from an empty room list",
        function() {
          store.setStoreState({rooms: []});

          expect(store.findNextAvailableRoomNumber(fakeNameTemplate)).eql(1);
        });

      it("should find next available room number from a non empty room list",
        function() {
          store.setStoreState({
            rooms: [{roomName: "RoomWord 1"}]
          });

          expect(store.findNextAvailableRoomNumber(fakeNameTemplate)).eql(2);
        });

      it("should not be sensitive to initial list order", function() {
        store.setStoreState({
          rooms: [{roomName: "RoomWord 99"}, {roomName: "RoomWord 98"}]
        });

        expect(store.findNextAvailableRoomNumber(fakeNameTemplate)).eql(100);
      });
    });

    describe("#createRoom", function() {
      var fakeNameTemplate = "Conversation {{conversationLabel}}";
      var fakeLocalRoomId = "777";
      var fakeOwner = "fake@invalid";
      var fakeRoomCreationData = {
        nameTemplate: fakeNameTemplate,
        roomOwner: fakeOwner
      };

      var fakeCreatedRoom = {
        roomName: "Conversation 1",
        roomToken: "fake",
        roomUrl: "http://invalid",
        maxSize: 42,
        participants: [],
        ctime: 1234567890
      };

      beforeEach(function() {
        store.setStoreState({pendingCreation: false, rooms: []});
      });

      it("should request creation of a new room", function() {
        sandbox.stub(fakeMozLoop.rooms, "create");

        store.createRoom(new sharedActions.CreateRoom(fakeRoomCreationData));

        sinon.assert.calledWith(fakeMozLoop.rooms.create, {
          roomName: "Conversation 1",
          roomOwner: fakeOwner,
          maxSize: store.maxRoomCreationSize,
          expiresIn: store.defaultExpiresIn
        });
      });

      it("should store any creation encountered error", function() {
        var err = new Error("fake");
        sandbox.stub(fakeMozLoop.rooms, "create", function(data, cb) {
          cb(err);
        });

        store.createRoom(new sharedActions.CreateRoom(fakeRoomCreationData));

        expect(store.getStoreState().error).eql(err);
      });

      it("should switch the pendingCreation state flag to true", function() {
        sandbox.stub(fakeMozLoop.rooms, "create");

        store.createRoom(new sharedActions.CreateRoom(fakeRoomCreationData));

        expect(store.getStoreState().pendingCreation).eql(true);
      });

      it("should switch the pendingCreation state flag to false once the " +
         "operation is done", function() {
        sandbox.stub(fakeMozLoop.rooms, "create", function(data, cb) {
          cb();
        });

        store.createRoom(new sharedActions.CreateRoom(fakeRoomCreationData));

        expect(store.getStoreState().pendingCreation).eql(false);
      });
    });

    describe("#setStoreState", function() {
      it("should update store state data", function() {
        store.setStoreState({pendingCreation: true});

        expect(store.getStoreState().pendingCreation).eql(true);
      });

      it("should trigger a `change` event", function(done) {
        store.once("change", function() {
          done();
        });

        store.setStoreState({pendingCreation: true});
      });
    });

    describe("#getAllRooms", function() {
      it("should fetch the room list from the MozLoop API", function() {
        fakeMozLoop.rooms.getAll = function(version, cb) {
          cb(null, fakeRoomList);
        };

        store.getAllRooms(new sharedActions.GetAllRooms());

        expect(store.getStoreState().error).to.be.a.undefined;
        expect(store.getStoreState().rooms).to.have.length.of(3);
      });

      it("should order the room list using ctime desc", function() {
        fakeMozLoop.rooms.getAll = function(version, cb) {
          cb(null, fakeRoomList);
        };

        store.getAllRooms(new sharedActions.GetAllRooms());

        var storeState = store.getStoreState();
        expect(storeState.error).to.be.a.undefined;
        expect(storeState.rooms[0].ctime).eql(1405518241);
        expect(storeState.rooms[1].ctime).eql(1405517546);
        expect(storeState.rooms[2].ctime).eql(1405517418);
      });

      it("should report an error", function() {
        var err = new Error("fake");
        fakeMozLoop.rooms.getAll = function(version, cb) {
          cb(err);
        };

        dispatcher.dispatch(new sharedActions.GetAllRooms());

        expect(store.getStoreState().error).eql(err);
      });

      it("should register event listeners after the list is retrieved",
        function() {
          sandbox.stub(store, "startListeningToRoomEvents");
          fakeMozLoop.rooms.getAll = function(version, cb) {
            cb(null, fakeRoomList);
          };

          store.getAllRooms();

          sinon.assert.calledOnce(store.startListeningToRoomEvents);
        });

      it("should set the pendingInitialRetrieval flag to true", function() {
        store.getAllRooms();

        expect(store.getStoreState().pendingInitialRetrieval).eql(true);
      });

      it("should set pendingInitialRetrieval to false once the action is " +
         "performed", function() {
        fakeMozLoop.rooms.getAll = function(version, cb) {
          cb(null, fakeRoomList);
        };

        store.getAllRooms();

        expect(store.getStoreState().pendingInitialRetrieval).eql(false);
      });
    });
  });

  describe("#openRoom", function() {
    var store, fakeMozLoop;

    beforeEach(function() {
      fakeMozLoop = {
        rooms: {
          open: sinon.spy()
        }
      };
      store = new loop.store.RoomListStore({
        dispatcher: dispatcher,
        mozLoop: fakeMozLoop
      });
    });

    it("should open the room via mozLoop", function() {
      dispatcher.dispatch(new sharedActions.OpenRoom({roomToken: "42abc"}));

      sinon.assert.calledOnce(fakeMozLoop.rooms.open);
      sinon.assert.calledWithExactly(fakeMozLoop.rooms.open, "42abc");
    });
  });
});
