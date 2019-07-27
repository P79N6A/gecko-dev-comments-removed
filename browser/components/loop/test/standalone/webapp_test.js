





var expect = chai.expect;
var TestUtils = React.addons.TestUtils;

describe("loop.webapp", function() {
  "use strict";

  var sharedModels = loop.shared.models,
      sharedViews = loop.shared.views,
      sandbox,
      notifications;

  beforeEach(function() {
    sandbox = sinon.sandbox.create();
    notifications = new sharedModels.NotificationCollection();
  });

  afterEach(function() {
    sandbox.restore();
  });

  describe("#init", function() {
    var conversationSetStub;

    beforeEach(function() {
      sandbox.stub(React, "renderComponent");
      sandbox.stub(loop.webapp.WebappHelper.prototype,
                   "locationHash").returns("#call/fake-Token");
      conversationSetStub =
        sandbox.stub(sharedModels.ConversationModel.prototype, "set");
    });

    it("should create the WebappRootView", function() {
      loop.webapp.init();

      sinon.assert.calledOnce(React.renderComponent);
      sinon.assert.calledWith(React.renderComponent,
        sinon.match(function(value) {
          return TestUtils.isDescriptorOfType(value,
            loop.webapp.WebappRootView);
      }));
    });

    it("should set the loopToken on the conversation", function() {
      loop.webapp.init();

       sinon.assert.called(conversationSetStub);
       sinon.assert.calledWithExactly(conversationSetStub, "loopToken", "fake-Token");
    });
  });

  describe("OutgoingConversationView", function() {
    var ocView, conversation, client;

    function mountTestComponent(props) {
      return TestUtils.renderIntoDocument(
        loop.webapp.OutgoingConversationView(props));
    }

    beforeEach(function() {
      client = new loop.StandaloneClient({
        baseServerUrl: "http://fake.example.com"
      });
      sandbox.stub(client, "requestCallInfo");
      sandbox.stub(client, "requestCallUrlInfo");
      conversation = new sharedModels.ConversationModel({}, {
        sdk: {}
      });
      conversation.set("loopToken", "fakeToken");
      ocView = mountTestComponent({
        helper: new loop.webapp.WebappHelper(),
        client: client,
        conversation: conversation,
        notifications: notifications,
        sdk: {}
      });
    });

    describe("start", function() {
      it("should display the StartConversationView", function() {
        TestUtils.findRenderedComponentWithType(ocView,
          loop.webapp.StartConversationView);
      });
    });

    
    
    describe("#_setupWebSocket", function() {
      beforeEach(function() {
        conversation.setOutgoingSessionData({
          sessionId:      "sessionId",
          sessionToken:   "sessionToken",
          apiKey:         "apiKey",
          callId:         "Hello",
          progressURL:    "http://invalid/url",
          websocketToken: 123
        });
      });

      describe("Websocket connection successful", function() {
        var promise;

        beforeEach(function() {
          sandbox.stub(loop, "CallConnectionWebSocket").returns({
            promiseConnect: function() {
              promise = new Promise(function(resolve, reject) {
                resolve();
              });
              return promise;
            },

            on: sandbox.spy()
          });
        });

        it("should create a CallConnectionWebSocket", function(done) {
          ocView._setupWebSocket();

          promise.then(function () {
            sinon.assert.calledOnce(loop.CallConnectionWebSocket);
            sinon.assert.calledWithExactly(loop.CallConnectionWebSocket, {
              callId: "Hello",
              url: "http://invalid/url",
              
              websocketToken: "7b"
            });
            done();
          });
        });
      });

      describe("Websocket connection failed", function() {
        var promise;

        beforeEach(function() {
          sandbox.stub(loop, "CallConnectionWebSocket").returns({
            promiseConnect: function() {
              promise = new Promise(function(resolve, reject) {
                reject();
              });
              return promise;
            },

            on: sandbox.spy()
          });
        });

        it("should display an error", function(done) {
          sandbox.stub(notifications, "errorL10n");
          ocView._setupWebSocket();

          promise.then(function() {
          }, function () {
            sinon.assert.calledOnce(notifications.errorL10n);
            sinon.assert.calledWithExactly(notifications.errorL10n,
              "cannot_start_call_session_not_ready");
            done();
          });
        });
      });

      describe("Websocket Events", function() {
        beforeEach(function() {
          conversation.setOutgoingSessionData({
            sessionId:      "sessionId",
            sessionToken:   "sessionToken",
            apiKey:         "apiKey",
            callId:         "Hello",
            progressURL:    "http://progress.example.com",
            websocketToken: 123
          });

          sandbox.stub(loop.CallConnectionWebSocket.prototype,
                       "promiseConnect").returns({
            then: sandbox.spy()
          });

          ocView._setupWebSocket();
        });

        describe("Progress", function() {
          describe("state: terminate, reason: reject", function() {
            beforeEach(function() {
              sandbox.stub(notifications, "errorL10n");
            });

            it("should display the StartConversationView", function() {
              ocView._websocket.trigger("progress", {
                state: "terminated",
                reason: "reject"
              });

              TestUtils.findRenderedComponentWithType(ocView,
                loop.webapp.StartConversationView);
            });

            it("should display an error message if the reason is not 'cancel'",
              function() {
                ocView._websocket.trigger("progress", {
                  state: "terminated",
                  reason: "reject"
                });

                sinon.assert.calledOnce(notifications.errorL10n);
                sinon.assert.calledWithExactly(notifications.errorL10n,
                  "call_timeout_notification_text");
              });

            it("should not display an error message if the reason is 'cancel'",
              function() {
                ocView._websocket.trigger("progress", {
                  state: "terminated",
                  reason: "cancel"
                });

                sinon.assert.notCalled(notifications.errorL10n);
              });
          });

          describe("state: connecting", function() {
            it("should set display the ConversationView", function() {
              
              
              sandbox.stub(conversation, "startSession");

              conversation.set({"loopToken": "fakeToken"});

              ocView._websocket.trigger("progress", {
                state: "connecting"
              });

              TestUtils.findRenderedComponentWithType(ocView,
                sharedViews.ConversationView);
            });
          });
        });
      });
    });

    describe("Events", function() {
      var fakeSessionData, promiseConnectStub;

      beforeEach(function() {
        fakeSessionData = {
          sessionId:      "sessionId",
          sessionToken:   "sessionToken",
          apiKey:         "apiKey",
          websocketToken: 123,
          progressURL:    "fakeUrl",
          callId:         "fakeCallId"
        };
        conversation.set(fakeSessionData);
        conversation.set("loopToken", "fakeToken");
        sandbox.stub(notifications, "errorL10n");
        sandbox.stub(notifications, "warnL10n");
        promiseConnectStub =
          sandbox.stub(loop.CallConnectionWebSocket.prototype, "promiseConnect");
        promiseConnectStub.returns(new Promise(function(resolve, reject) {}));
      });

      describe("call:outgoing", function() {
        it("should set display the StartConversationView if session token is missing",
          function() {
            conversation.set("loopToken", "");

            ocView.startCall();

            TestUtils.findRenderedComponentWithType(ocView,
              loop.webapp.StartConversationView);
          });

        it("should notify the user if session token is missing", function() {
          conversation.set("loopToken", "");

          ocView.startCall();

          sinon.assert.calledOnce(notifications.errorL10n);
          sinon.assert.calledWithExactly(notifications.errorL10n,
                                         "missing_conversation_info");
        });

        it("should setup the websocket if session token is available",
          function() {
            ocView.startCall();

            sinon.assert.calledOnce(promiseConnectStub);
          });

        it("should show the PendingConversationView if session token is available",
          function() {
            ocView.startCall();

            TestUtils.findRenderedComponentWithType(ocView,
              loop.webapp.PendingConversationView);
          });
      });

      describe("session:ended", function() {
        it("should set display the StartConversationView", function() {
          conversation.trigger("session:ended");

          TestUtils.findRenderedComponentWithType(ocView,
            loop.webapp.StartConversationView);
        });
      });

      describe("session:peer-hungup", function() {
        it("should set display the StartConversationView", function() {
          conversation.trigger("session:peer-hungup");

          TestUtils.findRenderedComponentWithType(ocView,
            loop.webapp.StartConversationView);
        });

        it("should notify the user", function() {
          conversation.trigger("session:peer-hungup");

          sinon.assert.calledOnce(notifications.warnL10n);
          sinon.assert.calledWithExactly(notifications.warnL10n,
                                         "peer_ended_conversation2");
        });

      });

      describe("session:network-disconnected", function() {
        it("should display the StartConversationView",
          function() {
            conversation.trigger("session:network-disconnected");

            TestUtils.findRenderedComponentWithType(ocView,
              loop.webapp.StartConversationView);
          });

        it("should notify the user", function() {
          conversation.trigger("session:network-disconnected");

          sinon.assert.calledOnce(notifications.warnL10n);
          sinon.assert.calledWithExactly(notifications.warnL10n,
                                         "network_disconnected");
        });
      });

      describe("Published and Subscribed Streams", function() {
        beforeEach(function() {
          ocView._websocket = {
            mediaUp: sinon.spy()
          };
        });

        describe("publishStream", function() {
          it("should not notify the websocket if only one stream is up",
            function() {
              conversation.set("publishedStream", true);

              sinon.assert.notCalled(ocView._websocket.mediaUp);
            });

          it("should notify the websocket that media is up if both streams" +
             "are connected", function() {
              conversation.set("subscribedStream", true);
              conversation.set("publishedStream", true);

              sinon.assert.calledOnce(ocView._websocket.mediaUp);
            });
        });

        describe("subscribedStream", function() {
          it("should not notify the websocket if only one stream is up",
            function() {
              conversation.set("subscribedStream", true);

              sinon.assert.notCalled(ocView._websocket.mediaUp);
            });

          it("should notify the websocket that media is up if both streams" +
             "are connected", function() {
              conversation.set("publishedStream", true);
              conversation.set("subscribedStream", true);

              sinon.assert.calledOnce(ocView._websocket.mediaUp);
            });
        });
      });

      describe("#setupOutgoingCall", function() {
        describe("No loop token", function() {
          beforeEach(function() {
            conversation.set("loopToken", "");
          });

          it("should set display the StartConversationView", function() {
            conversation.setupOutgoingCall();

            TestUtils.findRenderedComponentWithType(ocView,
              loop.webapp.StartConversationView);
          });

          it("should display an error", function() {
            conversation.setupOutgoingCall();

            sinon.assert.calledOnce(notifications.errorL10n);
          });
        });

        describe("Has loop token", function() {
          beforeEach(function() {
            conversation.set("selectedCallType", "audio-video");
            sandbox.stub(conversation, "outgoing");
          });

          it("should call requestCallInfo on the client",
            function() {
              conversation.setupOutgoingCall();

              sinon.assert.calledOnce(client.requestCallInfo);
              sinon.assert.calledWith(client.requestCallInfo, "fakeToken",
                                      "audio-video");
            });

          describe("requestCallInfo response handling", function() {
            it("should set display the CallUrlExpiredView if the call has expired",
               function() {
                client.requestCallInfo.callsArgWith(2, {errno: 105});

                conversation.setupOutgoingCall();

                TestUtils.findRenderedComponentWithType(ocView,
                  loop.webapp.CallUrlExpiredView);
              });

            it("should set display the StartConversationView on any other error",
               function() {
                client.requestCallInfo.callsArgWith(2, {errno: 104});

                conversation.setupOutgoingCall();

                TestUtils.findRenderedComponentWithType(ocView,
                  loop.webapp.StartConversationView);
              });

            it("should notify the user on any other error", function() {
              client.requestCallInfo.callsArgWith(2, {errno: 104});

              conversation.setupOutgoingCall();

              sinon.assert.calledOnce(notifications.errorL10n);
            });

            it("should call outgoing on the conversation model when details " +
               "are successfully received", function() {
                client.requestCallInfo.callsArgWith(2, null, fakeSessionData);

                conversation.setupOutgoingCall();

                sinon.assert.calledOnce(conversation.outgoing);
                sinon.assert.calledWithExactly(conversation.outgoing, fakeSessionData);
              });
          });
        });
      });
    });
  });

  describe("WebappRootView", function() {
    var webappHelper, sdk, conversationModel, client, props;

    function mountTestComponent() {
      return TestUtils.renderIntoDocument(
        loop.webapp.WebappRootView({
        client: client,
        helper: webappHelper,
        sdk: sdk,
        conversation: conversationModel
      }));
    }

    beforeEach(function() {
      webappHelper = new loop.webapp.WebappHelper();
      sdk = {
        checkSystemRequirements: function() { return true; }
      };
      conversationModel = new sharedModels.ConversationModel({}, {
        sdk: sdk
      });
      client = new loop.StandaloneClient({
        baseServerUrl: "fakeUrl"
      });
      
      
      sandbox.stub(client, "requestCallUrlInfo");
    });

    it("should mount the unsupportedDevice view if the device is running iOS",
      function() {
        sandbox.stub(webappHelper, "isIOS").returns(true);

        var webappRootView = mountTestComponent();

        TestUtils.findRenderedComponentWithType(webappRootView,
          loop.webapp.UnsupportedDeviceView);
      });

    it("should mount the unsupportedBrowser view if the sdk detects " +
      "the browser is unsupported", function() {
        sdk.checkSystemRequirements = function() {
          return false;
        };

        var webappRootView = mountTestComponent();

        TestUtils.findRenderedComponentWithType(webappRootView,
          loop.webapp.UnsupportedBrowserView);
      });

    it("should mount the OutgoingConversationView view if there is a loopToken",
      function() {
        conversationModel.set("loopToken", "fakeToken");

        var webappRootView = mountTestComponent();

        TestUtils.findRenderedComponentWithType(webappRootView,
          loop.webapp.OutgoingConversationView);
      });

    it("should mount the Home view there is no loopToken", function() {
        var webappRootView = mountTestComponent();

        TestUtils.findRenderedComponentWithType(webappRootView,
          loop.webapp.HomeView);
    });
  });

  describe("PendingConversationView", function() {
    var view, websocket;

    beforeEach(function() {
      websocket = new loop.CallConnectionWebSocket({
        url: "wss://fake/",
        callId: "callId",
        websocketToken: "7b"
      });

      sinon.stub(websocket, "cancel");

      view = React.addons.TestUtils.renderIntoDocument(
        loop.webapp.PendingConversationView({
          websocket: websocket
        })
      );
    });

    describe("#_cancelOutgoingCall", function() {
      it("should inform the websocket to cancel the setup", function() {
        var button = view.getDOMNode().querySelector(".btn-cancel");
        React.addons.TestUtils.Simulate.click(button);

        sinon.assert.calledOnce(websocket.cancel);
      });
    });

    describe("Events", function() {
      describe("progress:alerting", function() {
        it("should update the callstate to ringing", function () {
          websocket.trigger("progress:alerting");

          expect(view.state.callState).to.be.equal("ringing");
        });
      });
    });
  });

  describe("StartConversationView", function() {
    describe("#initiate", function() {
      var conversation, setupOutgoingCall, view, fakeSubmitEvent,
          requestCallUrlInfo;

      beforeEach(function() {
        conversation = new sharedModels.ConversationModel({}, {
          sdk: {}
        });

        fakeSubmitEvent = {preventDefault: sinon.spy()};
        setupOutgoingCall = sinon.stub(conversation, "setupOutgoingCall");

        var standaloneClientStub = {
          requestCallUrlInfo: function(token, cb) {
            cb(null, {urlCreationDate: 0});
          },
          settings: {baseServerUrl: loop.webapp.baseServerUrl}
        };

        view = React.addons.TestUtils.renderIntoDocument(
            loop.webapp.StartConversationView({
              model: conversation,
              notifications: notifications,
              client: standaloneClientStub
            })
        );
      });

      it("should start the audio-video conversation establishment process",
        function() {
          var button = view.getDOMNode().querySelector(".btn-accept");
          React.addons.TestUtils.Simulate.click(button);

          sinon.assert.calledOnce(setupOutgoingCall);
          sinon.assert.calledWithExactly(setupOutgoingCall);
      });

      it("should start the audio-only conversation establishment process",
        function() {
          var button = view.getDOMNode().querySelector(".start-audio-only-call");
          React.addons.TestUtils.Simulate.click(button);

          sinon.assert.calledOnce(setupOutgoingCall);
          sinon.assert.calledWithExactly(setupOutgoingCall);
        });

      it("should disable audio-video button once session is initiated",
         function() {
           conversation.set("loopToken", "fake");

           var button = view.getDOMNode().querySelector(".btn-accept");
           React.addons.TestUtils.Simulate.click(button);

           expect(button.disabled).to.eql(true);
         });

      it("should disable audio-only button once session is initiated",
         function() {
           conversation.set("loopToken", "fake");

           var button = view.getDOMNode().querySelector(".start-audio-only-call");
           React.addons.TestUtils.Simulate.click(button);

           expect(button.disabled).to.eql(true);
         });

         it("should set selectedCallType to audio", function() {
           conversation.set("loopToken", "fake");

           var button = view.getDOMNode().querySelector(".start-audio-only-call");
           React.addons.TestUtils.Simulate.click(button);

           expect(conversation.get("selectedCallType")).to.eql("audio");
         });

         it("should set selectedCallType to audio-video", function() {
           conversation.set("loopToken", "fake");

           var button = view.getDOMNode().querySelector(".standalone-call-btn-video-icon");
           React.addons.TestUtils.Simulate.click(button);

           expect(conversation.get("selectedCallType")).to.eql("audio-video");
         });

      it("should set state.urlCreationDateString to a locale date string",
         function() {
        
        
        var date = new Date(0);
        var options = {year: "numeric", month: "long", day: "numeric"};
        var timestamp = date.toLocaleDateString(navigator.language, options);

        expect(view.state.urlCreationDateString).to.eql(timestamp);
      });

    });

    describe("Events", function() {
      var conversation, view, StandaloneClient, requestCallUrlInfo;

      beforeEach(function() {
        conversation = new sharedModels.ConversationModel({
          loopToken: "fake"
        }, {
          sdk: {}
        });

        sandbox.spy(conversation, "listenTo");
        requestCallUrlInfo = sandbox.stub();

        view = React.addons.TestUtils.renderIntoDocument(
            loop.webapp.StartConversationView({
              model: conversation,
              notifications: notifications,
              client: {requestCallUrlInfo: requestCallUrlInfo}
            })
          );
      });

      it("should call requestCallUrlInfo", function() {
        sinon.assert.calledOnce(requestCallUrlInfo);
        sinon.assert.calledWithExactly(requestCallUrlInfo,
                                       sinon.match.string,
                                       sinon.match.func);
      });

      it("should listen for session:error events", function() {
        sinon.assert.calledOnce(conversation.listenTo);
        sinon.assert.calledWithExactly(conversation.listenTo, conversation,
                                       "session:error", sinon.match.func);
      });

      it("should trigger a notication when a session:error model event is " +
         " received", function() {
        sandbox.stub(notifications, "errorL10n");
        conversation.trigger("session:error", "tech error");

        sinon.assert.calledOnce(notifications.errorL10n);
        sinon.assert.calledWithExactly(notifications.errorL10n,
                                       "unable_retrieve_call_info");
      });
    });

    describe("#render", function() {
      var conversation, view, requestCallUrlInfo, oldLocalStorageValue;

      beforeEach(function() {
        oldLocalStorageValue = localStorage.getItem("has-seen-tos");
        localStorage.removeItem("has-seen-tos");

        conversation = new sharedModels.ConversationModel({
          loopToken: "fake"
        }, {
          sdk: {}
        });

        requestCallUrlInfo = sandbox.stub();
      });

      afterEach(function() {
        if (oldLocalStorageValue !== null)
          localStorage.setItem("has-seen-tos", oldLocalStorageValue);
      });

      it("should show the TOS", function() {
        var tos;

        view = React.addons.TestUtils.renderIntoDocument(
          loop.webapp.StartConversationView({
            model: conversation,
            notifications: notifications,
            client: {requestCallUrlInfo: requestCallUrlInfo}
          })
        );
        tos = view.getDOMNode().querySelector(".terms-service");

        expect(tos.classList.contains("hide")).to.equal(false);
      });

      it("should not show the TOS if it has already been seen", function() {
        var tos;

        localStorage.setItem("has-seen-tos", "true");
        view = React.addons.TestUtils.renderIntoDocument(
          loop.webapp.StartConversationView({
            model: conversation,
            notifications: notifications,
            client: {requestCallUrlInfo: requestCallUrlInfo}
          })
        );
        tos = view.getDOMNode().querySelector(".terms-service");

        expect(tos.classList.contains("hide")).to.equal(true);
      });
    });
  });

  describe("PromoteFirefoxView", function() {
    describe("#render", function() {
      it("should not render when using Firefox", function() {
        var comp = TestUtils.renderIntoDocument(loop.webapp.PromoteFirefoxView({
          helper: {isFirefox: function() { return true; }}
        }));

        expect(comp.getDOMNode().querySelectorAll("h3").length).eql(0);
      });

      it("should render when not using Firefox", function() {
        var comp = TestUtils.renderIntoDocument(loop.webapp.PromoteFirefoxView({
          helper: {isFirefox: function() { return false; }}
        }));

        expect(comp.getDOMNode().querySelectorAll("h3").length).eql(1);
      });
    });
  });

  describe("WebappHelper", function() {
    var helper;

    beforeEach(function() {
      helper = new loop.webapp.WebappHelper();
    });

    describe("#isIOS", function() {
      it("should detect iOS", function() {
        expect(helper.isIOS("iPad")).eql(true);
        expect(helper.isIOS("iPod")).eql(true);
        expect(helper.isIOS("iPhone")).eql(true);
        expect(helper.isIOS("iPhone Simulator")).eql(true);
      });

      it("shouldn't detect iOS with other platforms", function() {
        expect(helper.isIOS("MacIntel")).eql(false);
      });
    });

    describe("#isFirefox", function() {
      it("should detect Firefox", function() {
        expect(helper.isFirefox("Firefox")).eql(true);
        expect(helper.isFirefox("Gecko/Firefox")).eql(true);
        expect(helper.isFirefox("Firefox/Gecko")).eql(true);
        expect(helper.isFirefox("Gecko/Firefox/Chuck Norris")).eql(true);
      });

      it("shouldn't detect Firefox with other platforms", function() {
        expect(helper.isFirefox("Opera")).eql(false);
      });
    });
  });
});
