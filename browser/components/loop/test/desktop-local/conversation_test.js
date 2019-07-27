





var expect = chai.expect;

describe("loop.conversation", function() {
  "use strict";

  var sharedModels = loop.shared.models,
      sharedView = loop.shared.views,
      sandbox;

  
  

  function stubComponent(obj, component, mockTagName){
    var reactClass = React.createClass({
      render: function() {
        var mockTagName = mockTagName || "div";
        return React.DOM[mockTagName](null, this.props.children);
      }
    });
    return sandbox.stub(obj, component, reactClass);
  }

  beforeEach(function() {
    sandbox = sinon.sandbox.create();
    sandbox.useFakeTimers();

    navigator.mozLoop = {
      doNotDisturb: true,
      getStrings: function() {
        return JSON.stringify({textContent: "fakeText"});
      },
      get locale() {
        return "en-US";
      },
      setLoopCharPref: sinon.stub(),
      getLoopCharPref: sinon.stub().returns("http://fakeurl"),
      getLoopBoolPref: sinon.stub(),
      calls: {
        clearCallInProgress: sinon.stub()
      },
      LOOP_SESSION_TYPE: {
        GUEST: 1,
        FXA: 2
      },
      startAlerting: sinon.stub(),
      stopAlerting: sinon.stub(),
      ensureRegistered: sinon.stub(),
      get appVersionInfo() {
        return {
          version: "42",
          channel: "test",
          platform: "test"
        };
      },
      getAudioBlob: sinon.spy(function(name, callback) {
        callback(null, new Blob([new ArrayBuffer(10)], {type: 'audio/ogg'}));
      })
    };

    
    
    sandbox.stub(document.mozL10n, "get", function(x) {
      return x;
    });
    document.mozL10n.initialize(navigator.mozLoop);
  });

  afterEach(function() {
    delete navigator.mozLoop;
    sandbox.restore();
  });

  describe("#init", function() {
    beforeEach(function() {
      sandbox.stub(React, "renderComponent");
      sandbox.stub(document.mozL10n, "initialize");

      sandbox.stub(loop.shared.models.ConversationModel.prototype,
        "initialize");

      sandbox.stub(loop.Dispatcher.prototype, "dispatch");

      sandbox.stub(loop.shared.utils.Helper.prototype,
        "locationData").returns({
          hash: "#42",
          pathname: "/"
        });

      window.OT = {
        overrideGuidStorage: sinon.stub()
      };
    });

    afterEach(function() {
      delete window.OT;
    });

    it("should initialize L10n", function() {
      loop.conversation.init();

      sinon.assert.calledOnce(document.mozL10n.initialize);
      sinon.assert.calledWithExactly(document.mozL10n.initialize,
        navigator.mozLoop);
    });

    it("should create the AppControllerView", function() {
      loop.conversation.init();

      sinon.assert.calledOnce(React.renderComponent);
      sinon.assert.calledWith(React.renderComponent,
        sinon.match(function(value) {
          return TestUtils.isDescriptorOfType(value,
            loop.conversation.AppControllerView);
      }));
    });

    it("should trigger a getWindowData action", function() {
      loop.conversation.init();

      sinon.assert.calledOnce(loop.Dispatcher.prototype.dispatch);
      sinon.assert.calledWithExactly(loop.Dispatcher.prototype.dispatch,
        new loop.shared.actions.GetWindowData({
          windowId: "42"
        }));
    });
  });

  describe("AppControllerView", function() {
    var conversationStore, conversation, client, ccView, oldTitle, dispatcher;
    var conversationAppStore, roomStore;

    function mountTestComponent() {
      return TestUtils.renderIntoDocument(
        loop.conversation.AppControllerView({
          client: client,
          conversation: conversation,
          roomStore: roomStore,
          sdk: {},
          conversationStore: conversationStore,
          conversationAppStore: conversationAppStore,
          dispatcher: dispatcher
        }));
    }

    beforeEach(function() {
      oldTitle = document.title;
      client = new loop.Client();
      conversation = new loop.shared.models.ConversationModel({}, {
        sdk: {}
      });
      dispatcher = new loop.Dispatcher();
      conversationStore = new loop.store.ConversationStore({
        contact: {
          name: [ "Mr Smith" ],
          email: [{
            type: "home",
            value: "fakeEmail",
            pref: true
          }]
        }
      }, {
        client: client,
        dispatcher: dispatcher,
        sdkDriver: {}
      });
      roomStore = new loop.store.RoomStore({
        mozLoop: navigator.mozLoop,
        dispatcher: dispatcher
      });
      conversationAppStore = new loop.store.ConversationAppStore({
        dispatcher: dispatcher,
        mozLoop: navigator.mozLoop
      });
    });

    afterEach(function() {
      ccView = undefined;
      document.title = oldTitle;
    });

    it("should display the OutgoingConversationView for outgoing calls", function() {
      conversationAppStore.setStoreState({windowType: "outgoing"});

      ccView = mountTestComponent();

      TestUtils.findRenderedComponentWithType(ccView,
        loop.conversationViews.OutgoingConversationView);
    });

    it("should display the IncomingConversationView for incoming calls", function() {
      sandbox.stub(conversation, "setIncomingSessionData");
      sandbox.stub(loop, "CallConnectionWebSocket").returns({
        promiseConnect: function() {
          return new Promise(function() {});
        },
        on: sandbox.spy()
      });
      conversationAppStore.setStoreState({windowType: "incoming"});

      ccView = mountTestComponent();

      TestUtils.findRenderedComponentWithType(ccView,
        loop.conversation.IncomingConversationView);
    });

    it("should display the RoomView for rooms", function() {
      conversationAppStore.setStoreState({windowType: "room"});

      ccView = mountTestComponent();

      TestUtils.findRenderedComponentWithType(ccView,
        loop.roomViews.DesktopRoomConversationView);
    });

    it("should display the GenericFailureView for failures", function() {
      conversationAppStore.setStoreState({windowType: "failed"});

      ccView = mountTestComponent();

      TestUtils.findRenderedComponentWithType(ccView,
        loop.conversation.GenericFailureView);
    });
  });

  describe("IncomingConversationView", function() {
    var conversationAppStore, conversation, client, icView, oldTitle;

    function mountTestComponent() {
      return TestUtils.renderIntoDocument(
        loop.conversation.IncomingConversationView({
          client: client,
          conversation: conversation,
          sdk: {},
          conversationAppStore: conversationAppStore
        }));
    }

    beforeEach(function() {
      oldTitle = document.title;
      client = new loop.Client();
      conversation = new loop.shared.models.ConversationModel({}, {
        sdk: {}
      });
      conversation.set({windowId: 42});
      var dispatcher = new loop.Dispatcher();
      conversationAppStore = new loop.store.ConversationAppStore({
        dispatcher: dispatcher,
        mozLoop: navigator.mozLoop
      });
      sandbox.stub(conversation, "setOutgoingSessionData");
    });

    afterEach(function() {
      icView = undefined;
      document.title = oldTitle;
    });

    describe("start", function() {
      it("should set the title to incoming_call_title2", function() {
        conversationAppStore.setStoreState({
          windowData: {
            progressURL:    "fake",
            websocketToken: "fake",
            callId: 42
          }
        });

        icView = mountTestComponent();

        expect(document.title).eql("incoming_call_title2");
      });
    });

    describe("componentDidMount", function() {
      var fakeSessionData, promise, resolveWebSocketConnect;
      var rejectWebSocketConnect;

      beforeEach(function() {
        fakeSessionData  = {
          sessionId:      "sessionId",
          sessionToken:   "sessionToken",
          apiKey:         "apiKey",
          callType:       "callType",
          callId:         "Hello",
          progressURL:    "http://progress.example.com",
          websocketToken: "7b"
        };

        conversationAppStore.setStoreState({
          windowData: fakeSessionData
        });

        stubComponent(loop.conversation, "IncomingCallView");
        stubComponent(sharedView, "ConversationView");
      });

      it("should start alerting", function() {
        icView = mountTestComponent();

        sinon.assert.calledOnce(navigator.mozLoop.startAlerting);
      });

      describe("Session Data setup", function() {
        beforeEach(function() {
          sandbox.stub(loop, "CallConnectionWebSocket").returns({
            promiseConnect: function () {
              promise = new Promise(function(resolve, reject) {
                resolveWebSocketConnect = resolve;
                rejectWebSocketConnect = reject;
              });
              return promise;
            },
            on: sinon.stub()
          });
        });

        it("should store the session data", function() {
          sandbox.stub(conversation, "setIncomingSessionData");

          icView = mountTestComponent();

          sinon.assert.calledOnce(conversation.setIncomingSessionData);
          sinon.assert.calledWithExactly(conversation.setIncomingSessionData,
                                         fakeSessionData);
        });

        it("should setup the websocket connection", function() {
          icView = mountTestComponent();

          sinon.assert.calledOnce(loop.CallConnectionWebSocket);
          sinon.assert.calledWithExactly(loop.CallConnectionWebSocket, {
            callId: "Hello",
            url: "http://progress.example.com",
            websocketToken: "7b"
          });
        });
      });

      describe("WebSocket Handling", function() {
        beforeEach(function() {
          promise = new Promise(function(resolve, reject) {
            resolveWebSocketConnect = resolve;
            rejectWebSocketConnect = reject;
          });

          sandbox.stub(loop.CallConnectionWebSocket.prototype, "promiseConnect").returns(promise);
        });

        it("should set the state to incoming on success", function(done) {
          icView = mountTestComponent();
          resolveWebSocketConnect("incoming");

          promise.then(function () {
            expect(icView.state.callStatus).eql("incoming");
            done();
          });
        });

        it("should set the state to close on success if the progress " +
          "state is terminated", function(done) {
            icView = mountTestComponent();
            resolveWebSocketConnect("terminated");

            promise.then(function () {
              expect(icView.state.callStatus).eql("close");
              done();
            });
          });

        
        
        it.skip("should should switch view state to failed", function(done) {
          icView = mountTestComponent();
          rejectWebSocketConnect();

          promise.then(function() {}, function() {
            done();
          });
        });
      });

      describe("WebSocket Events", function() {
        describe("Call cancelled or timed out before acceptance", function() {
          beforeEach(function() {
            
            
            icView = mountTestComponent();
            promise = new Promise(function(resolve, reject) {
              resolve();
            });

            sandbox.stub(loop.CallConnectionWebSocket.prototype, "promiseConnect").returns(promise);
            sandbox.stub(loop.CallConnectionWebSocket.prototype, "close");
            sandbox.stub(window, "close");
          });

          describe("progress - terminated (previousState = alerting)", function() {
            it("should stop alerting", function(done) {
              promise.then(function() {
                icView._websocket.trigger("progress", {
                  state: "terminated",
                  reason: "timeout"
                }, "alerting");

                sinon.assert.calledOnce(navigator.mozLoop.stopAlerting);
                done();
              });
            });

            it("should close the websocket", function(done) {
              promise.then(function() {
                icView._websocket.trigger("progress", {
                  state: "terminated",
                  reason: "closed"
                }, "alerting");

                sinon.assert.calledOnce(icView._websocket.close);
                done();
              });
            });

            it("should close the window", function(done) {
              promise.then(function() {
                icView._websocket.trigger("progress", {
                  state: "terminated",
                  reason: "answered-elsewhere"
                }, "alerting");

                sandbox.clock.tick(1);

                sinon.assert.calledOnce(window.close);
                done();
              });
            });
          });

          describe("progress - terminated (previousState not init" +
                   " nor alerting)",
            function() {
              it("should set the state to end", function(done) {
                promise.then(function() {
                  icView._websocket.trigger("progress", {
                    state: "terminated",
                    reason: "media-fail"
                  }, "connecting");

                  expect(icView.state.callStatus).eql("end");
                  done();
                });
              });

              it("should stop alerting", function(done) {
                promise.then(function() {
                  icView._websocket.trigger("progress", {
                    state: "terminated",
                    reason: "media-fail"
                  }, "connecting");

                  sinon.assert.calledOnce(navigator.mozLoop.stopAlerting);
                  done();
                });
              });
            });
        });
      });

      describe("#accept", function() {
        beforeEach(function() {
          icView = mountTestComponent();
          conversation.setIncomingSessionData({
            sessionId:      "sessionId",
            sessionToken:   "sessionToken",
            apiKey:         "apiKey",
            callType:       "callType",
            callId:         "Hello",
            progressURL:    "http://progress.example.com",
            websocketToken: 123
          });

          sandbox.stub(icView._websocket, "accept");
          sandbox.stub(icView.props.conversation, "accepted");
        });

        it("should initiate the conversation", function() {
          icView.accept();

          sinon.assert.calledOnce(icView.props.conversation.accepted);
        });

        it("should notify the websocket of the user acceptance", function() {
          icView.accept();

          sinon.assert.calledOnce(icView._websocket.accept);
        });

        it("should stop alerting", function() {
          icView.accept();

          sinon.assert.calledOnce(navigator.mozLoop.stopAlerting);
        });
      });

      describe("#decline", function() {
        beforeEach(function() {
          icView = mountTestComponent();

          sandbox.stub(window, "close");
          icView._websocket = {
            decline: sinon.stub(),
            close: sinon.stub()
          };
          conversation.set({
            windowId: "8699"
          });
          conversation.setIncomingSessionData({
            websocketToken: 123
          });
        });

        it("should close the window", function() {
          icView.decline();

          sandbox.clock.tick(1);

          sinon.assert.calledOnce(window.close);
        });

        it("should stop alerting", function() {
          icView.decline();

          sinon.assert.calledOnce(navigator.mozLoop.stopAlerting);
        });

        it("should release callData", function() {
          icView.decline();

          sinon.assert.calledOnce(navigator.mozLoop.calls.clearCallInProgress);
          sinon.assert.calledWithExactly(
            navigator.mozLoop.calls.clearCallInProgress, "8699");
        });
      });

      describe("#blocked", function() {
        var mozLoop;

        beforeEach(function() {
          icView = mountTestComponent();

          icView._websocket = {
            decline: sinon.spy(),
            close: sinon.stub()
          };
          sandbox.stub(window, "close");

          mozLoop = {
            LOOP_SESSION_TYPE: {
              GUEST: 1,
              FXA: 2
            }
          };
        });

        it("should call mozLoop.stopAlerting", function() {
          icView.declineAndBlock();

          sinon.assert.calledOnce(navigator.mozLoop.stopAlerting);
        });

        it("should call delete call", function() {
          sandbox.stub(conversation, "get").withArgs("callToken")
                                           .returns("fakeToken")
                                           .withArgs("sessionType")
                                           .returns(mozLoop.LOOP_SESSION_TYPE.FXA);

          var deleteCallUrl = sandbox.stub(loop.Client.prototype,
                                           "deleteCallUrl");
          icView.declineAndBlock();

          sinon.assert.calledOnce(deleteCallUrl);
          sinon.assert.calledWithExactly(deleteCallUrl,
            "fakeToken", mozLoop.LOOP_SESSION_TYPE.FXA, sinon.match.func);
        });

        it("should get callToken from conversation model", function() {
          sandbox.stub(conversation, "get");
          icView.declineAndBlock();

          sinon.assert.called(conversation.get);
          sinon.assert.calledWithExactly(conversation.get, "callToken");
          sinon.assert.calledWithExactly(conversation.get, "windowId");
        });

        it("should trigger error handling in case of error", function() {
          
          var log = sandbox.stub(console, "log");
          var fakeError = {
            error: true
          };
          sandbox.stub(loop.Client.prototype, "deleteCallUrl", function(_, __, cb) {
            cb(fakeError);
          });
          icView.declineAndBlock();

          sinon.assert.calledOnce(log);
          sinon.assert.calledWithExactly(log, fakeError);
        });

        it("should close the window", function() {
          icView.declineAndBlock();

          sandbox.clock.tick(1);

          sinon.assert.calledOnce(window.close);
        });
      });
    });

    describe("Events", function() {
      var fakeSessionData;

      beforeEach(function() {

        fakeSessionData = {
          sessionId:    "sessionId",
          sessionToken: "sessionToken",
          apiKey:       "apiKey"
        };

        conversationAppStore.setStoreState({
          windowData: fakeSessionData
        });

        sandbox.stub(conversation, "setIncomingSessionData");
        sandbox.stub(loop, "CallConnectionWebSocket").returns({
          promiseConnect: function() {
            return new Promise(function() {});
          },
          on: sandbox.spy()
        });

        icView = mountTestComponent();

        conversation.set("loopToken", "fakeToken");
        navigator.mozLoop.getLoopCharPref.returns("http://fake");
        stubComponent(sharedView, "ConversationView");
      });

      describe("call:accepted", function() {
        it("should display the ConversationView",
          function() {
            conversation.accepted();

            TestUtils.findRenderedComponentWithType(icView,
              sharedView.ConversationView);
          });

        it("should set the title to the call identifier", function() {
          sandbox.stub(conversation, "getCallIdentifier").returns("fakeId");

          conversation.accepted();

          expect(document.title).eql("fakeId");
        });
      });

      describe("session:ended", function() {
        it("should display the feedback view when the call session ends",
          function() {
            conversation.trigger("session:ended");

            TestUtils.findRenderedComponentWithType(icView,
              sharedView.FeedbackView);
          });
      });

      describe("session:peer-hungup", function() {
        it("should display the feedback view when the peer hangs up",
          function() {
            conversation.trigger("session:peer-hungup");

              TestUtils.findRenderedComponentWithType(icView,
                sharedView.FeedbackView);
          });
      });

      describe("session:network-disconnected", function() {
        it("should navigate to call failed when network disconnects",
          function() {
            conversation.trigger("session:network-disconnected");

            TestUtils.findRenderedComponentWithType(icView,
              loop.conversation.GenericFailureView);
          });

        it("should update the conversation window toolbar title",
          function() {
            conversation.trigger("session:network-disconnected");

            expect(document.title).eql("generic_failure_title");
          });
      });

      describe("Published and Subscribed Streams", function() {
        beforeEach(function() {
          icView._websocket = {
            mediaUp: sinon.spy()
          };
        });

        describe("publishStream", function() {
          it("should not notify the websocket if only one stream is up",
            function() {
              conversation.set("publishedStream", true);

              sinon.assert.notCalled(icView._websocket.mediaUp);
            });

          it("should notify the websocket that media is up if both streams" +
             "are connected", function() {
              conversation.set("subscribedStream", true);
              conversation.set("publishedStream", true);

              sinon.assert.calledOnce(icView._websocket.mediaUp);
            });
        });

        describe("subscribedStream", function() {
          it("should not notify the websocket if only one stream is up",
            function() {
              conversation.set("subscribedStream", true);

              sinon.assert.notCalled(icView._websocket.mediaUp);
            });

          it("should notify the websocket that media is up if both streams" +
             "are connected", function() {
              conversation.set("publishedStream", true);
              conversation.set("subscribedStream", true);

              sinon.assert.calledOnce(icView._websocket.mediaUp);
            });
        });
      });
    });
  });

  describe("IncomingCallView", function() {
    var view, model, fakeAudio;

    beforeEach(function() {
      var Model = Backbone.Model.extend({
        getCallIdentifier: function() {return "fakeId";}
      });
      model = new Model();
      sandbox.spy(model, "trigger");
      sandbox.stub(model, "set");

      fakeAudio = {
        play: sinon.spy(),
        pause: sinon.spy(),
        removeAttribute: sinon.spy()
      };
      sandbox.stub(window, "Audio").returns(fakeAudio);

      view = TestUtils.renderIntoDocument(loop.conversation.IncomingCallView({
        model: model,
        video: true
      }));
    });

    describe("default answer mode", function() {
      it("should display video as primary answer mode", function() {
        view = TestUtils.renderIntoDocument(loop.conversation.IncomingCallView({
          model: model,
          video: true
        }));
        var primaryBtn = view.getDOMNode()
                                  .querySelector('.fx-embedded-btn-icon-video');

        expect(primaryBtn).not.to.eql(null);
      });

      it("should display audio as primary answer mode", function() {
        view = TestUtils.renderIntoDocument(loop.conversation.IncomingCallView({
          model: model,
          video: false
        }));
        var primaryBtn = view.getDOMNode()
                                  .querySelector('.fx-embedded-btn-icon-audio');

        expect(primaryBtn).not.to.eql(null);
      });

      it("should accept call with video", function() {
        view = TestUtils.renderIntoDocument(loop.conversation.IncomingCallView({
          model: model,
          video: true
        }));
        var primaryBtn = view.getDOMNode()
                                  .querySelector('.fx-embedded-btn-icon-video');

        React.addons.TestUtils.Simulate.click(primaryBtn);

        sinon.assert.calledOnce(model.set);
        sinon.assert.calledWithExactly(model.set, "selectedCallType", "audio-video");
        sinon.assert.calledOnce(model.trigger);
        sinon.assert.calledWithExactly(model.trigger, "accept");
      });

      it("should accept call with audio", function() {
        view = TestUtils.renderIntoDocument(loop.conversation.IncomingCallView({
          model: model,
          video: false
        }));
        var primaryBtn = view.getDOMNode()
                                  .querySelector('.fx-embedded-btn-icon-audio');

        React.addons.TestUtils.Simulate.click(primaryBtn);

        sinon.assert.calledOnce(model.set);
        sinon.assert.calledWithExactly(model.set, "selectedCallType", "audio");
        sinon.assert.calledOnce(model.trigger);
        sinon.assert.calledWithExactly(model.trigger, "accept");
      });

      it("should accept call with video when clicking on secondary btn",
         function() {
           view = TestUtils.renderIntoDocument(loop.conversation.IncomingCallView({
             model: model,
             video: false
           }));
           var secondaryBtn = view.getDOMNode()
           .querySelector('.fx-embedded-btn-video-small');

           React.addons.TestUtils.Simulate.click(secondaryBtn);

           sinon.assert.calledOnce(model.set);
           sinon.assert.calledWithExactly(model.set, "selectedCallType", "audio-video");
           sinon.assert.calledOnce(model.trigger);
           sinon.assert.calledWithExactly(model.trigger, "accept");
         });

      it("should accept call with audio when clicking on secondary btn",
         function() {
           view = TestUtils.renderIntoDocument(loop.conversation.IncomingCallView({
             model: model,
             video: true
           }));
           var secondaryBtn = view.getDOMNode()
           .querySelector('.fx-embedded-btn-audio-small');

           React.addons.TestUtils.Simulate.click(secondaryBtn);

           sinon.assert.calledOnce(model.set);
           sinon.assert.calledWithExactly(model.set, "selectedCallType", "audio");
           sinon.assert.calledOnce(model.trigger);
           sinon.assert.calledWithExactly(model.trigger, "accept");
         });
    });

    describe("click event on .btn-accept", function() {
      it("should trigger an 'accept' conversation model event", function () {
        var buttonAccept = view.getDOMNode().querySelector(".btn-accept");
        model.trigger.withArgs("accept");
        TestUtils.Simulate.click(buttonAccept);

        
        sinon.assert.calledOnce(model.trigger.withArgs("accept"));
      });

      it("should set selectedCallType to audio-video", function () {
        var buttonAccept = view.getDOMNode().querySelector(".btn-accept");

        TestUtils.Simulate.click(buttonAccept);

        sinon.assert.calledOnce(model.set);
        sinon.assert.calledWithExactly(model.set, "selectedCallType",
          "audio-video");
      });
    });

    describe("click event on .btn-decline", function() {
      it("should trigger an 'decline' conversation model event", function() {
        var buttonDecline = view.getDOMNode().querySelector(".btn-decline");

        TestUtils.Simulate.click(buttonDecline);

        sinon.assert.calledOnce(model.trigger);
        sinon.assert.calledWith(model.trigger, "decline");
        });
    });

    describe("click event on .btn-block", function() {
      it("should trigger a 'block' conversation model event", function() {
        var buttonBlock = view.getDOMNode().querySelector(".btn-block");

        TestUtils.Simulate.click(buttonBlock);

        sinon.assert.calledOnce(model.trigger);
        sinon.assert.calledWith(model.trigger, "declineAndBlock");
      });
    });
  });

  describe("GenericFailureView", function() {
    var view, fakeAudio;

    beforeEach(function() {
      fakeAudio = {
        play: sinon.spy(),
        pause: sinon.spy(),
        removeAttribute: sinon.spy()
      };
      navigator.mozLoop.doNotDisturb = false;
      sandbox.stub(window, "Audio").returns(fakeAudio);

      view = TestUtils.renderIntoDocument(
        loop.conversation.GenericFailureView({
          cancelCall: function() {}
        })
      );
    });

    it("should play a failure sound, once", function() {
      sinon.assert.calledOnce(navigator.mozLoop.getAudioBlob);
      sinon.assert.calledWithExactly(navigator.mozLoop.getAudioBlob,
                                     "failure", sinon.match.func);
      sinon.assert.calledOnce(fakeAudio.play);
      expect(fakeAudio.loop).to.equal(false);
    });

  });
});
