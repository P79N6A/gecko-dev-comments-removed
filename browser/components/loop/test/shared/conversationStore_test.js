


var expect = chai.expect;

describe("loop.ConversationStore", function () {
  "use strict";

  var CALL_STATES = loop.store.CALL_STATES;
  var WS_STATES = loop.store.WS_STATES;
  var sharedActions = loop.shared.actions;
  var sharedUtils = loop.shared.utils;
  var sandbox, dispatcher, client, store, fakeSessionData;
  var connectPromise, resolveConnectPromise, rejectConnectPromise;

  function checkFailures(done, f) {
    try {
      f();
      done();
    } catch (err) {
      done(err);
    }
  }

  beforeEach(function() {
    sandbox = sinon.sandbox.create();

    dispatcher = new loop.Dispatcher();
    client = {
      setupOutgoingCall: sinon.stub()
    };
    store = new loop.store.ConversationStore({}, {
      client: client,
      dispatcher: dispatcher
    });
    fakeSessionData = {
      apiKey: "fakeKey",
      callId: "142536",
      sessionId: "321456",
      sessionToken: "341256",
      websocketToken: "543216",
      progressURL: "fakeURL"
    };

    var dummySocket = {
      close: sinon.spy(),
      send: sinon.spy()
    };

    connectPromise = new Promise(function(resolve, reject) {
      resolveConnectPromise = resolve;
      rejectConnectPromise = reject;
    });

    sandbox.stub(loop.CallConnectionWebSocket.prototype,
      "promiseConnect").returns(connectPromise);
  });

  afterEach(function() {
    sandbox.restore();
  });

  describe("#initialize", function() {
    it("should throw an error if the dispatcher is missing", function() {
      expect(function() {
        new loop.store.ConversationStore({}, {client: client});
      }).to.Throw(/dispatcher/);
    });

    it("should throw an error if the client is missing", function() {
      expect(function() {
        new loop.store.ConversationStore({}, {dispatcher: dispatcher});
      }).to.Throw(/client/);
    });
  });

  describe("#connectionFailure", function() {
    it("should set the state to 'terminated'", function() {
      store.set({callState: CALL_STATES.ALERTING});

      dispatcher.dispatch(
        new sharedActions.ConnectionFailure({reason: "fake"}));

      expect(store.get("callState")).eql(CALL_STATES.TERMINATED);
      expect(store.get("callStateReason")).eql("fake");
    });
  });

  describe("#connectionProgress", function() {
    describe("progress: init", function() {
      it("should change the state from 'gather' to 'connecting'", function() {
        store.set({callState: CALL_STATES.GATHER});

        dispatcher.dispatch(
          new sharedActions.ConnectionProgress({wsState: WS_STATES.INIT}));

        expect(store.get("callState")).eql(CALL_STATES.CONNECTING);
      });
    });

    describe("progress: alerting", function() {
      it("should change the state from 'gather' to 'alerting'", function() {
        store.set({callState: CALL_STATES.GATHER});

        dispatcher.dispatch(
          new sharedActions.ConnectionProgress({wsState: WS_STATES.ALERTING}));

        expect(store.get("callState")).eql(CALL_STATES.ALERTING);
      });

      it("should change the state from 'init' to 'alerting'", function() {
        store.set({callState: CALL_STATES.INIT});

        dispatcher.dispatch(
          new sharedActions.ConnectionProgress({wsState: WS_STATES.ALERTING}));

        expect(store.get("callState")).eql(CALL_STATES.ALERTING);
      });
    });

    describe("progress: connecting", function() {
      it("should change the state to 'ongoing'", function() {
        store.set({callState: CALL_STATES.ALERTING});

        dispatcher.dispatch(
          new sharedActions.ConnectionProgress({wsState: WS_STATES.CONNECTING}));

        expect(store.get("callState")).eql(CALL_STATES.ONGOING);
      });
    });

    describe("progress: half-connected", function() {
      it("should change the state to 'ongoing'", function() {
        store.set({callState: CALL_STATES.ALERTING});

        dispatcher.dispatch(
          new sharedActions.ConnectionProgress({wsState: WS_STATES.HALF_CONNECTED}));

        expect(store.get("callState")).eql(CALL_STATES.ONGOING);
      });
    });

    describe("progress: connecting", function() {
      it("should change the state to 'ongoing'", function() {
        store.set({callState: CALL_STATES.ALERTING});

        dispatcher.dispatch(
          new sharedActions.ConnectionProgress({wsState: WS_STATES.CONNECTED}));

        expect(store.get("callState")).eql(CALL_STATES.ONGOING);
      });
    });
  });

  describe("#gatherCallData", function() {
    beforeEach(function() {
      store.set({callState: CALL_STATES.INIT});
    });

    it("should set the state to 'gather'", function() {
      dispatcher.dispatch(
        new sharedActions.GatherCallData({
          calleeId: "",
          callId: "76543218"
        }));

      expect(store.get("callState")).eql(CALL_STATES.GATHER);
    });

    it("should save the basic call information", function() {
      dispatcher.dispatch(
        new sharedActions.GatherCallData({
          calleeId: "fake",
          callId: "123456"
        }));

      expect(store.get("calleeId")).eql("fake");
      expect(store.get("callId")).eql("123456");
      expect(store.get("outgoing")).eql(true);
    });

    describe("outgoing calls", function() {
      var outgoingCallData;

      beforeEach(function() {
        outgoingCallData = {
          calleeId: "fake",
          callId: "135246"
        };
      });

      it("should request the outgoing call data", function() {
        dispatcher.dispatch(
          new sharedActions.GatherCallData(outgoingCallData));

        sinon.assert.calledOnce(client.setupOutgoingCall);
        sinon.assert.calledWith(client.setupOutgoingCall,
          ["fake"], sharedUtils.CALL_TYPES.AUDIO_VIDEO);
      });

      describe("server response handling", function() {
        beforeEach(function() {
          sandbox.stub(dispatcher, "dispatch");
        });

        it("should dispatch a connect call action on success", function() {
          var callData = {
            apiKey: "fakeKey"
          };

          client.setupOutgoingCall.callsArgWith(2, null, callData);

          store.gatherCallData(
            new sharedActions.GatherCallData(outgoingCallData));

          sinon.assert.calledOnce(dispatcher.dispatch);
          
          sinon.assert.calledWithMatch(dispatcher.dispatch,
            sinon.match.hasOwn("name", "connectCall"));
          sinon.assert.calledWithMatch(dispatcher.dispatch,
            sinon.match.hasOwn("sessionData", callData));
        });

        it("should dispatch a connection failure action on failure", function() {
          client.setupOutgoingCall.callsArgWith(2, {});

          store.gatherCallData(
            new sharedActions.GatherCallData(outgoingCallData));

          sinon.assert.calledOnce(dispatcher.dispatch);
          
          sinon.assert.calledWithMatch(dispatcher.dispatch,
            sinon.match.hasOwn("name", "connectionFailure"));
          sinon.assert.calledWithMatch(dispatcher.dispatch,
            sinon.match.hasOwn("reason", "setup"));
        });
      });
    });
  });

  describe("#connectCall", function() {
    it("should save the call session data", function() {
      dispatcher.dispatch(
        new sharedActions.ConnectCall({sessionData: fakeSessionData}));

      expect(store.get("apiKey")).eql("fakeKey");
      expect(store.get("callId")).eql("142536");
      expect(store.get("sessionId")).eql("321456");
      expect(store.get("sessionToken")).eql("341256");
      expect(store.get("websocketToken")).eql("543216");
      expect(store.get("progressURL")).eql("fakeURL");
    });

    it("should initialize the websocket", function() {
      sandbox.stub(loop, "CallConnectionWebSocket").returns({
        promiseConnect: function() { return connectPromise; },
        on: sinon.spy()
      });

      dispatcher.dispatch(
        new sharedActions.ConnectCall({sessionData: fakeSessionData}));

      sinon.assert.calledOnce(loop.CallConnectionWebSocket);
      sinon.assert.calledWithExactly(loop.CallConnectionWebSocket, {
        url: "fakeURL",
        callId: "142536",
        websocketToken: "543216"
      });
    });

    it("should connect the websocket to the server", function() {
      dispatcher.dispatch(
        new sharedActions.ConnectCall({sessionData: fakeSessionData}));

      sinon.assert.calledOnce(store._websocket.promiseConnect);
    });

    describe("WebSocket connection result", function() {
      beforeEach(function() {
        dispatcher.dispatch(
          new sharedActions.ConnectCall({sessionData: fakeSessionData}));

        sandbox.stub(dispatcher, "dispatch");
      });

      it("should dispatch a connection progress action on success", function(done) {
        resolveConnectPromise(WS_STATES.INIT);

        connectPromise.then(function() {
          checkFailures(done, function() {
            sinon.assert.calledOnce(dispatcher.dispatch);
            
            sinon.assert.calledWithMatch(dispatcher.dispatch,
              sinon.match.hasOwn("name", "connectionProgress"));
            sinon.assert.calledWithMatch(dispatcher.dispatch,
              sinon.match.hasOwn("wsState", WS_STATES.INIT));
          });
        }, function() {
          done(new Error("Promise should have been resolve, not rejected"));
        });
      });

      it("should dispatch a connection failure action on failure", function(done) {
        rejectConnectPromise();

        connectPromise.then(function() {
          done(new Error("Promise should have been rejected, not resolved"));
        }, function() {
          checkFailures(done, function() {
            sinon.assert.calledOnce(dispatcher.dispatch);
            
            sinon.assert.calledWithMatch(dispatcher.dispatch,
              sinon.match.hasOwn("name", "connectionFailure"));
            sinon.assert.calledWithMatch(dispatcher.dispatch,
              sinon.match.hasOwn("reason", "websocket-setup"));
           });
        });
      });
    });
  });

  describe("#hangupCall", function() {
    var wsMediaFailSpy, wsCloseSpy;
    beforeEach(function() {
      wsMediaFailSpy = sinon.spy();
      wsCloseSpy = sinon.spy();

      store._websocket = {
        mediaFail: wsMediaFailSpy,
        close: wsCloseSpy
      };
      store.set({callState: CALL_STATES.ONGOING});
    });

    it("should send a media-fail message to the websocket if it is open", function() {
      dispatcher.dispatch(new sharedActions.HangupCall());

      sinon.assert.calledOnce(wsMediaFailSpy);
    });

    it("should ensure the websocket is closed", function() {
      dispatcher.dispatch(new sharedActions.HangupCall());

      sinon.assert.calledOnce(wsCloseSpy);
    });

    it("should set the callState to finished", function() {
      dispatcher.dispatch(new sharedActions.HangupCall());

      expect(store.get("callState")).eql(CALL_STATES.FINISHED);
    });
  });

  describe("#cancelCall", function() {
    it("should set the state to close if the call has terminated already", function() {
      store.set({callState: CALL_STATES.TERMINATED});

      dispatcher.dispatch(new sharedActions.CancelCall());

      expect(store.get("callState")).eql(CALL_STATES.CLOSE);
    });

    describe("whilst connecting", function() {
      var wsCancelSpy, wsCloseSpy;
      beforeEach(function() {
        wsCancelSpy = sinon.spy();
        wsCloseSpy = sinon.spy();

        store._websocket = {
          cancel: wsCancelSpy,
          close: wsCloseSpy
        };
        store.set({callState: CALL_STATES.CONNECTING});
      });

      it("should send a cancel message to the websocket if it is open", function() {
        dispatcher.dispatch(new sharedActions.CancelCall());

        sinon.assert.calledOnce(wsCancelSpy);
      });

      it("should ensure the websocket is closed", function() {
        dispatcher.dispatch(new sharedActions.CancelCall());

        sinon.assert.calledOnce(wsCloseSpy);
      });

      it("should set the state to close if the call is connecting", function() {
        dispatcher.dispatch(new sharedActions.CancelCall());

        expect(store.get("callState")).eql(CALL_STATES.CLOSE);
      });
    });
  });

  describe("#retryCall", function() {
    it("should set the state to gather", function() {
      store.set({callState: CALL_STATES.TERMINATED});

      dispatcher.dispatch(new sharedActions.RetryCall());

      expect(store.get("callState")).eql(CALL_STATES.GATHER);
    });

    it("should request the outgoing call data", function() {
      store.set({
        callState: CALL_STATES.TERMINATED,
        outgoing: true,
        callType: sharedUtils.CALL_TYPES.AUDIO_VIDEO,
        calleeId: "fake"
      });

      dispatcher.dispatch(new sharedActions.RetryCall());

      sinon.assert.calledOnce(client.setupOutgoingCall);
      sinon.assert.calledWith(client.setupOutgoingCall,
        ["fake"], sharedUtils.CALL_TYPES.AUDIO_VIDEO);
    });
  });

  describe("Events", function() {
    describe("Websocket progress", function() {
      beforeEach(function() {
        dispatcher.dispatch(
          new sharedActions.ConnectCall({sessionData: fakeSessionData}));

        sandbox.stub(dispatcher, "dispatch");
      });

      it("should dispatch a connection failure action on 'terminate'", function() {
        store._websocket.trigger("progress", {
          state: WS_STATES.TERMINATED,
          reason: "reject"
        });

        sinon.assert.calledOnce(dispatcher.dispatch);
        
        sinon.assert.calledWithMatch(dispatcher.dispatch,
          sinon.match.hasOwn("name", "connectionFailure"));
        sinon.assert.calledWithMatch(dispatcher.dispatch,
          sinon.match.hasOwn("reason", "reject"));
      });

      it("should dispatch a connection progress action on 'alerting'", function() {
        store._websocket.trigger("progress", {state: WS_STATES.ALERTING});

        sinon.assert.calledOnce(dispatcher.dispatch);
        
        sinon.assert.calledWithMatch(dispatcher.dispatch,
          sinon.match.hasOwn("name", "connectionProgress"));
        sinon.assert.calledWithMatch(dispatcher.dispatch,
          sinon.match.hasOwn("wsState", WS_STATES.ALERTING));
      });
    });
  });
});
