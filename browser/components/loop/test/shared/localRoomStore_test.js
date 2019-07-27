

var expect = chai.expect;
var sharedActions = loop.shared.actions;

describe("loop.store.LocalRoomStore", function () {
  "use strict";

  var sandbox, dispatcher;

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
        new loop.store.LocalRoomStore({mozLoop: {}});
      }).to.Throw(/dispatcher/);
    });

    it("should throw an error if mozLoop is missing", function() {
      expect(function() {
        new loop.store.LocalRoomStore({dispatcher: dispatcher});
      }).to.Throw(/mozLoop/);
    });
  });

  describe("#setupEmptyRoom", function() {
    var store, fakeMozLoop, fakeRoomId, fakeRoomName;

    beforeEach(function() {
      fakeRoomId = "337-ff-54";
      fakeRoomName = "Monkeys";
      fakeMozLoop = {
        rooms: { getRoomData: sandbox.stub() }
      };

      store = new loop.store.LocalRoomStore(
        {mozLoop: fakeMozLoop, dispatcher: dispatcher});
      fakeMozLoop.rooms.getRoomData.
        withArgs(fakeRoomId).
        callsArgOnWith(1, 
        store, 
        null, 
        {roomName: fakeRoomName}
      );
    });

    it("should trigger a change event", function(done) {
      store.on("change", function() {
        done();
      });

      dispatcher.dispatch(new sharedActions.SetupEmptyRoom(
        {localRoomId: fakeRoomId}));
    });

    it("should set localRoomId on the store from the action data",
      function(done) {

        store.once("change", function () {
          expect(store.getStoreState()).
            to.have.property('localRoomId', fakeRoomId);
          done();
        });

        dispatcher.dispatch(
          new sharedActions.SetupEmptyRoom({localRoomId: fakeRoomId}));
    });

    it("should set serverData.roomName from the getRoomData callback",
      function(done) {

        store.once("change", function () {
          expect(store.getStoreState()).to.have.deep.property(
            'serverData.roomName', fakeRoomName);
          done();
        });

        dispatcher.dispatch(
          new sharedActions.SetupEmptyRoom({localRoomId: fakeRoomId}));
      });

    it("should set error on the store when getRoomData calls back an error",
      function(done) {

        var fakeError = new Error("fake error");
        fakeMozLoop.rooms.getRoomData.
          withArgs(fakeRoomId).
          callsArgOnWith(1, 
          store, 
          fakeError); 

        store.once("change", function() {
          expect(this.getStoreState()).to.have.property('error', fakeError);
          done();
        });

        dispatcher.dispatch(
          new sharedActions.SetupEmptyRoom({localRoomId: fakeRoomId}));
      });

  });
});
