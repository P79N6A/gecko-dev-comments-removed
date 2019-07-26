





var expect = chai.expect;

describe("loop.conversation", function() {
  "use strict";

  var ConversationRouter = loop.conversation.ConversationRouter,
      sandbox,
      notifier;

  beforeEach(function() {
    sandbox = sinon.sandbox.create();
    notifier = {
      notify: sandbox.spy(),
      warn: sandbox.spy(),
      error: sandbox.spy()
    };
  });

  afterEach(function() {
    sandbox.restore();
  });

  describe("ConversationRouter", function() {
    var conversation;

    beforeEach(function() {
      conversation = new loop.shared.models.ConversationModel();
    });

    describe("#constructor", function() {
      it("should require a ConversationModel instance", function() {
        expect(function() {
          new ConversationRouter();
        }).to.Throw(Error, /missing required conversation/);
      });

      it("should require a notifier", function() {
        expect(function() {
          new ConversationRouter({conversation: conversation});
        }).to.Throw(Error, /missing required notifier/);
      });
    });

    describe("Routes", function() {
      var router;

      beforeEach(function() {
        router = new ConversationRouter({
          conversation: conversation,
          notifier: notifier
        });
        sandbox.stub(router, "loadView");
      });

      describe("#start", function() {
        it("should set the loopVersion on the conversation model", function() {
          router.start("fakeVersion");

          expect(conversation.get("loopVersion")).to.equal("fakeVersion");
        });

        it("should initiate the conversation", function() {
          sandbox.stub(conversation, "initiate");

          router.start("fakeVersion");

          sinon.assert.calledOnce(conversation.initiate);
          sinon.assert.calledWithExactly(conversation.initiate, {
            baseServerUrl: "http://example.com",
            outgoing: false
          });
        });
      });

      describe("#conversation", function() {
        it("should load the ConversationView if session is set", function() {
          sandbox.stub(loop.shared.views.ConversationView.prototype,
            "initialize");
          conversation.set("sessionId", "fakeSessionId");

          router.conversation();

          sinon.assert.calledOnce(router.loadView);
          sinon.assert.calledWithMatch(router.loadView, {
            $el: {selector: "#conversation"}
          });
        });

        it("should not load the ConversationView if session is not set",
          function() {
            router.conversation();

            sinon.assert.notCalled(router.loadView);
        });

        it("should notify the user when session is not set",
          function() {
            router.conversation();

            sinon.assert.calledOnce(router._notifier.error);
        });
      });

      describe("#ended", function() {
        
        
        it("should close the window", function() {
          sandbox.stub(window, "close");

          router.ended();

          sinon.assert.calledOnce(window.close);
        });
      });
    });

    describe("Events", function() {
      var router, fakeSessionData;

      beforeEach(function() {
        fakeSessionData = {
          sessionId:    "sessionId",
          sessionToken: "sessionToken",
          apiKey:       "apiKey"
        };
        sandbox.stub(loop.conversation.ConversationRouter.prototype,
                     "navigate");
        conversation.set("loopToken", "fakeToken");
        router = new loop.conversation.ConversationRouter({
          conversation: conversation,
          notifier: notifier
        });
      });

      it("should navigate to call/ongoing once the call session is ready",
        function() {
          conversation.setReady(fakeSessionData);

          sinon.assert.calledOnce(router.navigate);
          sinon.assert.calledWith(router.navigate, "call/ongoing");
        });

      it("should navigate to call/ended when the call session ends",
        function() {
          conversation.trigger("session:ended");

          sinon.assert.calledOnce(router.navigate);
          sinon.assert.calledWith(router.navigate, "call/ended");
        });

      it("should warn the user when peer hangs up", function() {
        conversation.trigger("session:peer-hung");

        sinon.assert.calledOnce(notifier.warn);
      });

      it("should navigate to call/ended when peer hangs up", function() {
        conversation.trigger("session:peer-hung");

        sinon.assert.calledOnce(router.navigate);
        sinon.assert.calledWith(router.navigate, "call/ended");
      });

      it("should warn the user when network disconnects", function() {
        conversation.trigger("session:network-disconnected");

        sinon.assert.calledOnce(notifier.warn);
      });

      it("should navigate to call/{token} when network disconnects",
        function() {
          conversation.trigger("session:network-disconnected");

          sinon.assert.calledOnce(router.navigate);
          sinon.assert.calledWith(router.navigate, "call/ended");
        });
    });

  });
});
