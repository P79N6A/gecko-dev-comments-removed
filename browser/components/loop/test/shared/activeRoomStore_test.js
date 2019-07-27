

var expect = chai.expect;
var sharedActions = loop.shared.actions;

describe("loop.store.ActiveRoomStore", function () {
  "use strict";

  var SERVER_CODES = loop.store.SERVER_CODES;
  var ROOM_STATES = loop.store.ROOM_STATES;
  var FAILURE_REASONS = loop.shared.utils.FAILURE_REASONS;
  var sandbox, dispatcher, store, fakeMozLoop, fakeSdkDriver;
  var fakeMultiplexGum;

  beforeEach(function() {
    sandbox = sinon.sandbox.create();
    sandbox.useFakeTimers();

    dispatcher = new loop.Dispatcher();
    sandbox.stub(dispatcher, "dispatch");

    fakeMozLoop = {
      rooms: {
        get: sandbox.stub(),
        join: sandbox.stub(),
        refreshMembership: sandbox.stub(),
        leave: sandbox.stub()
      }
    };

    fakeSdkDriver = {
      connectSession: sandbox.stub(),
      disconnectSession: sandbox.stub()
    };

    fakeMultiplexGum = {
        reset: sandbox.spy()
    };

    loop.standaloneMedia = {
      multiplexGum: fakeMultiplexGum
    };

    store = new loop.store.ActiveRoomStore({
      dispatcher: dispatcher,
      mozLoop: fakeMozLoop,
      sdkDriver: fakeSdkDriver
    });
  });

  afterEach(function() {
    sandbox.restore();
  });

  describe("#constructor", function() {
    it("should throw an error if the dispatcher is missing", function() {
      expect(function() {
        new loop.store.ActiveRoomStore({mozLoop: {}});
      }).to.Throw(/dispatcher/);
    });

    it("should throw an error if mozLoop is missing", function() {
      expect(function() {
        new loop.store.ActiveRoomStore({dispatcher: dispatcher});
      }).to.Throw(/mozLoop/);
    });

    it("should throw an error if sdkDriver is missing", function() {
      expect(function() {
        new loop.store.ActiveRoomStore({dispatcher: dispatcher, mozLoop: {}});
      }).to.Throw(/sdkDriver/);
    });
  });

  describe("#roomFailure", function() {
    var fakeError;

    beforeEach(function() {
      sandbox.stub(console, "error");

      fakeError = new Error("fake");

      store.setStoreState({
        roomState: ROOM_STATES.READY
      });
    });

    it("should log the error", function() {
      store.roomFailure({error: fakeError});

      sinon.assert.calledOnce(console.error);
      sinon.assert.calledWith(console.error,
        sinon.match(ROOM_STATES.READY), fakeError);
    });

    it("should set the state to `FULL` on server error room full", function() {
      fakeError.errno = SERVER_CODES.ROOM_FULL;

      store.roomFailure({error: fakeError});

      expect(store._storeState.roomState).eql(ROOM_STATES.FULL);
    });

    it("should set the state to `FAILED` on generic error", function() {
      store.roomFailure({error: fakeError});

      expect(store._storeState.roomState).eql(ROOM_STATES.FAILED);
      expect(store._storeState.failureReason).eql(FAILURE_REASONS.UNKNOWN);
    });

    it("should set the failureReason to EXPIRED_OR_INVALID on server error: " +
      "invalid token", function() {
        fakeError.errno = SERVER_CODES.INVALID_TOKEN;

        store.roomFailure({error: fakeError});

        expect(store._storeState.roomState).eql(ROOM_STATES.FAILED);
        expect(store._storeState.failureReason).eql(FAILURE_REASONS.EXPIRED_OR_INVALID);
      });

    it("should set the failureReason to EXPIRED_OR_INVALID on server error: " +
      "expired", function() {
        fakeError.errno = SERVER_CODES.EXPIRED;

        store.roomFailure({error: fakeError});

        expect(store._storeState.roomState).eql(ROOM_STATES.FAILED);
        expect(store._storeState.failureReason).eql(FAILURE_REASONS.EXPIRED_OR_INVALID);
      });
  });

  describe("#setupWindowData", function() {
    var fakeToken, fakeRoomData;

    beforeEach(function() {
      fakeToken = "337-ff-54";
      fakeRoomData = {
        roomName: "Monkeys",
        roomOwner: "Alfred",
        roomUrl: "http://invalid"
      };

      fakeMozLoop.rooms.get.
        withArgs(fakeToken).
        callsArgOnWith(1, 
        store, 
        null, 
        fakeRoomData
      );
    });

    it("should set the state to `GATHER`",
      function() {
        store.setupWindowData(new sharedActions.SetupWindowData({
          windowId: "42",
          type: "room",
          roomToken: fakeToken
        }));

        expect(store.getStoreState()).
          to.have.property('roomState', ROOM_STATES.GATHER);
      });

    it("should dispatch an UpdateRoomInfo action if the get is successful",
      function() {
        store.setupWindowData(new sharedActions.SetupWindowData({
          windowId: "42",
          type: "room",
          roomToken: fakeToken
        }));

        sinon.assert.calledTwice(dispatcher.dispatch);
        sinon.assert.calledWithExactly(dispatcher.dispatch,
          new sharedActions.UpdateRoomInfo(_.extend({
            roomToken: fakeToken
          }, fakeRoomData)));
      });

    it("should dispatch a JoinRoom action if the get is successful",
      function() {
        store.setupWindowData(new sharedActions.SetupWindowData({
          windowId: "42",
          type: "room",
          roomToken: fakeToken
        }));

        sinon.assert.calledTwice(dispatcher.dispatch);
        sinon.assert.calledWithExactly(dispatcher.dispatch,
          new sharedActions.JoinRoom());
      });

    it("should dispatch a RoomFailure action if the get fails",
      function() {

        var fakeError = new Error("fake error");
        fakeMozLoop.rooms.get.
          withArgs(fakeToken).
          callsArgOnWith(1, 
          store, 
          fakeError); 

        store.setupWindowData(new sharedActions.SetupWindowData({
          windowId: "42",
          type: "room",
          roomToken: fakeToken
        }));

        sinon.assert.calledOnce(dispatcher.dispatch);
        sinon.assert.calledWithExactly(dispatcher.dispatch,
          new sharedActions.RoomFailure({
            error: fakeError
          }));
      });
  });

  describe("#fetchServerData", function() {
    it("should save the token", function() {
      store.fetchServerData(new sharedActions.FetchServerData({
        windowType: "room",
        token: "fakeToken"
      }));

      expect(store.getStoreState().roomToken).eql("fakeToken");
    });

    it("should set the state to `READY`", function() {
      store.fetchServerData(new sharedActions.FetchServerData({
        windowType: "room",
        token: "fakeToken"
      }));

      expect(store.getStoreState().roomState).eql(ROOM_STATES.READY);
    });
  });

  describe("#updateRoomInfo", function() {
    var fakeRoomInfo;

    beforeEach(function() {
      fakeRoomInfo = {
        roomName: "Its a room",
        roomOwner: "Me",
        roomToken: "fakeToken",
        roomUrl: "http://invalid"
      };
    });

    it("should set the state to READY", function() {
      store.updateRoomInfo(new sharedActions.UpdateRoomInfo(fakeRoomInfo));

      expect(store._storeState.roomState).eql(ROOM_STATES.READY);
    });

    it("should save the room information", function() {
      store.updateRoomInfo(new sharedActions.UpdateRoomInfo(fakeRoomInfo));

      var state = store.getStoreState();
      expect(state.roomName).eql(fakeRoomInfo.roomName);
      expect(state.roomOwner).eql(fakeRoomInfo.roomOwner);
      expect(state.roomToken).eql(fakeRoomInfo.roomToken);
      expect(state.roomUrl).eql(fakeRoomInfo.roomUrl);
    });
  });

  describe("#joinRoom", function() {
    beforeEach(function() {
      store.setStoreState({roomToken: "tokenFake"});
    });

    it("should reset failureReason", function() {
      store.setStoreState({failureReason: "Test"});

      store.joinRoom();

      expect(store.getStoreState().failureReason).eql(undefined);
    });

    it("should call rooms.join on mozLoop", function() {
      store.joinRoom();

      sinon.assert.calledOnce(fakeMozLoop.rooms.join);
      sinon.assert.calledWith(fakeMozLoop.rooms.join, "tokenFake");
    });

    it("should dispatch `JoinedRoom` on success", function() {
      var responseData = {
        apiKey: "keyFake",
        sessionToken: "14327659860",
        sessionId: "1357924680",
        expires: 8
      };

      fakeMozLoop.rooms.join.callsArgWith(1, null, responseData);

      store.joinRoom();

      sinon.assert.calledOnce(dispatcher.dispatch);
      sinon.assert.calledWith(dispatcher.dispatch,
        new sharedActions.JoinedRoom(responseData));
    });

    it("should dispatch `RoomFailure` on error", function() {
      var fakeError = new Error("fake");

      fakeMozLoop.rooms.join.callsArgWith(1, fakeError);

      store.joinRoom();

      sinon.assert.calledOnce(dispatcher.dispatch);
      sinon.assert.calledWith(dispatcher.dispatch,
        new sharedActions.RoomFailure({error: fakeError}));
    });
  });

  describe("#joinedRoom", function() {
    var fakeJoinedData;

    beforeEach(function() {
      fakeJoinedData = {
        apiKey: "9876543210",
        sessionToken: "12563478",
        sessionId: "15263748",
        expires: 20
      };

      store.setStoreState({
        roomToken: "fakeToken"
      });
    });

    it("should set the state to `JOINED`", function() {
      store.joinedRoom(new sharedActions.JoinedRoom(fakeJoinedData));

      expect(store._storeState.roomState).eql(ROOM_STATES.JOINED);
    });

    it("should store the session and api values", function() {
      store.joinedRoom(new sharedActions.JoinedRoom(fakeJoinedData));

      var state = store.getStoreState();
      expect(state.apiKey).eql(fakeJoinedData.apiKey);
      expect(state.sessionToken).eql(fakeJoinedData.sessionToken);
      expect(state.sessionId).eql(fakeJoinedData.sessionId);
    });

    it("should start the session connection with the sdk", function() {
      var actionData = new sharedActions.JoinedRoom(fakeJoinedData);

      store.joinedRoom(actionData);

      sinon.assert.calledOnce(fakeSdkDriver.connectSession);
      sinon.assert.calledWithExactly(fakeSdkDriver.connectSession,
        actionData);
    });

    it("should call mozLoop.rooms.refreshMembership before the expiresTime",
      function() {
        store.joinedRoom(new sharedActions.JoinedRoom(fakeJoinedData));

        sandbox.clock.tick(fakeJoinedData.expires * 1000);

        sinon.assert.calledOnce(fakeMozLoop.rooms.refreshMembership);
        sinon.assert.calledWith(fakeMozLoop.rooms.refreshMembership,
          "fakeToken", "12563478");
    });

    it("should call mozLoop.rooms.refreshMembership before the next expiresTime",
      function() {
        fakeMozLoop.rooms.refreshMembership.callsArgWith(2,
          null, {expires: 40});

        store.joinedRoom(new sharedActions.JoinedRoom(fakeJoinedData));

        
        
        sandbox.clock.tick(fakeJoinedData.expires * 1000);

        
        sandbox.clock.tick(40000);

        sinon.assert.calledTwice(fakeMozLoop.rooms.refreshMembership);
        sinon.assert.calledWith(fakeMozLoop.rooms.refreshMembership,
          "fakeToken", "12563478");
    });

    it("should dispatch `RoomFailure` if the refreshMembership call failed",
      function() {
        var fakeError = new Error("fake");
        fakeMozLoop.rooms.refreshMembership.callsArgWith(2, fakeError);

        store.joinedRoom(new sharedActions.JoinedRoom(fakeJoinedData));

        
        
        sandbox.clock.tick(fakeJoinedData.expires * 1000);

        sinon.assert.calledOnce(dispatcher.dispatch);
        sinon.assert.calledWith(dispatcher.dispatch,
          new sharedActions.RoomFailure({
            error: fakeError
          }));
    });
  });

  describe("#connectedToSdkServers", function() {
    it("should set the state to `SESSION_CONNECTED`", function() {
      store.connectedToSdkServers(new sharedActions.ConnectedToSdkServers());

      expect(store.getStoreState().roomState).eql(ROOM_STATES.SESSION_CONNECTED);
    });
  });

  describe("#connectionFailure", function() {
    var connectionFailureAction;

    beforeEach(function() {
      store.setStoreState({
        roomState: ROOM_STATES.JOINED,
        roomToken: "fakeToken",
        sessionToken: "1627384950"
      });

      connectionFailureAction = new sharedActions.ConnectionFailure({
        reason: "FAIL"
      });
    });

    it("should store the failure reason", function() {
      store.connectionFailure(connectionFailureAction);

      expect(store.getStoreState().failureReason).eql("FAIL");
    });

    it("should reset the multiplexGum", function() {
      store.connectionFailure(connectionFailureAction);

      sinon.assert.calledOnce(fakeMultiplexGum.reset);
    });

    it("should disconnect from the servers via the sdk", function() {
      store.connectionFailure(connectionFailureAction);

      sinon.assert.calledOnce(fakeSdkDriver.disconnectSession);
    });

    it("should clear any existing timeout", function() {
      sandbox.stub(window, "clearTimeout");
      store._timeout = {};

      store.connectionFailure(connectionFailureAction);

      sinon.assert.calledOnce(clearTimeout);
    });

    it("should call mozLoop.rooms.leave", function() {
      store.connectionFailure(connectionFailureAction);

      sinon.assert.calledOnce(fakeMozLoop.rooms.leave);
      sinon.assert.calledWithExactly(fakeMozLoop.rooms.leave,
        "fakeToken", "1627384950");
    });

    it("should set the state to `FAILED`", function() {
      store.connectionFailure(connectionFailureAction);

      expect(store.getStoreState().roomState).eql(ROOM_STATES.FAILED);
    });
  });

  describe("#setMute", function() {
    it("should save the mute state for the audio stream", function() {
      store.setStoreState({audioMuted: false});

      store.setMute(new sharedActions.SetMute({
        type: "audio",
        enabled: true
      }));

      expect(store.getStoreState().audioMuted).eql(false);
    });

    it("should save the mute state for the video stream", function() {
      store.setStoreState({videoMuted: true});

      store.setMute(new sharedActions.SetMute({
        type: "video",
        enabled: false
      }));

      expect(store.getStoreState().videoMuted).eql(true);
    });
  });

  describe("#remotePeerConnected", function() {
    it("should set the state to `HAS_PARTICIPANTS`", function() {
      store.remotePeerConnected();

      expect(store.getStoreState().roomState).eql(ROOM_STATES.HAS_PARTICIPANTS);
    });
  });

  describe("#remotePeerDisconnected", function() {
    it("should set the state to `SESSION_CONNECTED`", function() {
      store.remotePeerDisconnected();

      expect(store.getStoreState().roomState).eql(ROOM_STATES.SESSION_CONNECTED);
    });
  });

  describe("#windowUnload", function() {
    beforeEach(function() {
      store.setStoreState({
        roomState: ROOM_STATES.JOINED,
        roomToken: "fakeToken",
        sessionToken: "1627384950"
      });
    });

    it("should reset the multiplexGum", function() {
      store.leaveRoom();

      sinon.assert.calledOnce(fakeMultiplexGum.reset);
    });

    it("should disconnect from the servers via the sdk", function() {
      store.windowUnload();

      sinon.assert.calledOnce(fakeSdkDriver.disconnectSession);
    });

    it("should clear any existing timeout", function() {
      sandbox.stub(window, "clearTimeout");
      store._timeout = {};

      store.windowUnload();

      sinon.assert.calledOnce(clearTimeout);
    });

    it("should call mozLoop.rooms.leave", function() {
      store.windowUnload();

      sinon.assert.calledOnce(fakeMozLoop.rooms.leave);
      sinon.assert.calledWithExactly(fakeMozLoop.rooms.leave,
        "fakeToken", "1627384950");
    });

    it("should set the state to ready", function() {
      store.windowUnload();

      expect(store._storeState.roomState).eql(ROOM_STATES.READY);
    });
  });

  describe("#leaveRoom", function() {
    beforeEach(function() {
      store.setStoreState({
        roomState: ROOM_STATES.JOINED,
        roomToken: "fakeToken",
        sessionToken: "1627384950"
      });
    });

    it("should reset the multiplexGum", function() {
      store.leaveRoom();

      sinon.assert.calledOnce(fakeMultiplexGum.reset);
    });

    it("should disconnect from the servers via the sdk", function() {
      store.leaveRoom();

      sinon.assert.calledOnce(fakeSdkDriver.disconnectSession);
    });

    it("should clear any existing timeout", function() {
      sandbox.stub(window, "clearTimeout");
      store._timeout = {};

      store.leaveRoom();

      sinon.assert.calledOnce(clearTimeout);
    });

    it("should call mozLoop.rooms.leave", function() {
      store.leaveRoom();

      sinon.assert.calledOnce(fakeMozLoop.rooms.leave);
      sinon.assert.calledWithExactly(fakeMozLoop.rooms.leave,
        "fakeToken", "1627384950");
    });

    it("should set the state to ready", function() {
      store.leaveRoom();

      expect(store._storeState.roomState).eql(ROOM_STATES.READY);
    });
  });
});
