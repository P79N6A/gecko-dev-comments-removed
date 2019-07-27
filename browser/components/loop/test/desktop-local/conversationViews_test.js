


var expect = chai.expect;

describe("loop.conversationViews", function () {
  var sandbox, oldTitle, view, dispatcher;

  var CALL_STATES = loop.store.CALL_STATES;

  beforeEach(function() {
    sandbox = sinon.sandbox.create();

    oldTitle = document.title;
    sandbox.stub(document.mozL10n, "get", function(x) {
      return x;
    });

    dispatcher = new loop.Dispatcher();
    sandbox.stub(dispatcher, "dispatch");
  });

  afterEach(function() {
    document.title = oldTitle;
    view = undefined;
    sandbox.restore();
  });

  describe("ConversationDetailView", function() {
    function mountTestComponent(props) {
      return TestUtils.renderIntoDocument(
        loop.conversationViews.ConversationDetailView(props));
    }

    it("should set the document title to the calledId", function() {
      mountTestComponent({calleeId: "mrsmith"});

      expect(document.title).eql("mrsmith");
    });

    it("should set display the calledId", function() {
      view = mountTestComponent({calleeId: "mrsmith"});

      expect(TestUtils.findRenderedDOMComponentWithTag(
        view, "h2").props.children).eql("mrsmith");
    });
  });

  describe("PendingConversationView", function() {
    function mountTestComponent(props) {
      return TestUtils.renderIntoDocument(
        loop.conversationViews.PendingConversationView(props));
    }

    it("should set display connecting string when the state is not alerting",
      function() {
        view = mountTestComponent({
          callState: CALL_STATES.CONNECTING,
          calleeId: "mrsmith",
          dispatcher: dispatcher
        });

        var label = TestUtils.findRenderedDOMComponentWithClass(
          view, "btn-label").props.children;

        expect(label).to.have.string("connecting");
    });

    it("should set display ringing string when the state is alerting",
      function() {
        view = mountTestComponent({
          callState: CALL_STATES.ALERTING,
          calleeId: "mrsmith",
          dispatcher: dispatcher
        });

        var label = TestUtils.findRenderedDOMComponentWithClass(
          view, "btn-label").props.children;

        expect(label).to.have.string("ringing");
    });

    it("should disable the cancel button if enableCancelButton is false",
      function() {
        view = mountTestComponent({
          callState: CALL_STATES.CONNECTING,
          calleeId: "mrsmith",
          dispatcher: dispatcher,
          enableCancelButton: false
        });

        var cancelBtn = view.getDOMNode().querySelector('.btn-cancel');

        expect(cancelBtn.classList.contains("disabled")).eql(true);
      });

    it("should enable the cancel button if enableCancelButton is false",
      function() {
        view = mountTestComponent({
          callState: CALL_STATES.CONNECTING,
          calleeId: "mrsmith",
          dispatcher: dispatcher,
          enableCancelButton: true
        });

        var cancelBtn = view.getDOMNode().querySelector('.btn-cancel');

        expect(cancelBtn.classList.contains("disabled")).eql(false);
      });

    it("should dispatch a cancelCall action when the cancel button is pressed",
      function() {
        view = mountTestComponent({
          callState: CALL_STATES.CONNECTING,
          calleeId: "mrsmith",
          dispatcher: dispatcher
        });

        var cancelBtn = view.getDOMNode().querySelector('.btn-cancel');

        React.addons.TestUtils.Simulate.click(cancelBtn);

        sinon.assert.calledOnce(dispatcher.dispatch);
        sinon.assert.calledWithMatch(dispatcher.dispatch,
          sinon.match.hasOwn("name", "cancelCall"));
      });
  });

  describe("CallFailedView", function() {
    function mountTestComponent(props) {
      return TestUtils.renderIntoDocument(
        loop.conversationViews.CallFailedView({
          dispatcher: dispatcher
        }));
    }

    it("should dispatch a retryCall action when the retry button is pressed",
      function() {
        view = mountTestComponent();

        var retryBtn = view.getDOMNode().querySelector('.btn-retry');

        React.addons.TestUtils.Simulate.click(retryBtn);

        sinon.assert.calledOnce(dispatcher.dispatch);
        sinon.assert.calledWithMatch(dispatcher.dispatch,
          sinon.match.hasOwn("name", "retryCall"));
      });

    it("should dispatch a cancelCall action when the cancel button is pressed",
      function() {
        view = mountTestComponent();

        var cancelBtn = view.getDOMNode().querySelector('.btn-cancel');

        React.addons.TestUtils.Simulate.click(cancelBtn);

        sinon.assert.calledOnce(dispatcher.dispatch);
        sinon.assert.calledWithMatch(dispatcher.dispatch,
          sinon.match.hasOwn("name", "cancelCall"));
      });
  });

  describe("OngoingConversationView", function() {
    function mountTestComponent(props) {
      return TestUtils.renderIntoDocument(
        loop.conversationViews.OngoingConversationView(props));
    }

    it("should dispatch a setupStreamElements action when the view is created",
      function() {
        view = mountTestComponent({
          dispatcher: dispatcher
        });

        sinon.assert.calledOnce(dispatcher.dispatch);
        sinon.assert.calledWithMatch(dispatcher.dispatch,
          sinon.match.hasOwn("name", "setupStreamElements"));
      });

    it("should dispatch a hangupCall action when the hangup button is pressed",
      function() {
        view = mountTestComponent({
          dispatcher: dispatcher
        });

        var hangupBtn = view.getDOMNode().querySelector('.btn-hangup');

        React.addons.TestUtils.Simulate.click(hangupBtn);

        sinon.assert.calledWithMatch(dispatcher.dispatch,
          sinon.match.hasOwn("name", "hangupCall"));
      });

    it("should dispatch a setMute action when the audio mute button is pressed",
      function() {
        view = mountTestComponent({
          dispatcher: dispatcher,
          audio: {enabled: false}
        });

        var muteBtn = view.getDOMNode().querySelector('.btn-mute-audio');

        React.addons.TestUtils.Simulate.click(muteBtn);

        sinon.assert.calledWithMatch(dispatcher.dispatch,
          sinon.match.hasOwn("name", "setMute"));
        sinon.assert.calledWithMatch(dispatcher.dispatch,
          sinon.match.hasOwn("enabled", true));
        sinon.assert.calledWithMatch(dispatcher.dispatch,
          sinon.match.hasOwn("type", "audio"));
      });

    it("should dispatch a setMute action when the video mute button is pressed",
      function() {
        view = mountTestComponent({
          dispatcher: dispatcher,
          video: {enabled: true}
        });

        var muteBtn = view.getDOMNode().querySelector('.btn-mute-video');

        React.addons.TestUtils.Simulate.click(muteBtn);

        sinon.assert.calledWithMatch(dispatcher.dispatch,
          sinon.match.hasOwn("name", "setMute"));
        sinon.assert.calledWithMatch(dispatcher.dispatch,
          sinon.match.hasOwn("enabled", false));
        sinon.assert.calledWithMatch(dispatcher.dispatch,
          sinon.match.hasOwn("type", "video"));
      });

    it("should set the mute button as mute off", function() {
      view = mountTestComponent({
        dispatcher: dispatcher,
        video: {enabled: true}
      });

      var muteBtn = view.getDOMNode().querySelector('.btn-mute-video');

      expect(muteBtn.classList.contains("muted")).eql(false);
    });

    it("should set the mute button as mute on", function() {
      view = mountTestComponent({
        dispatcher: dispatcher,
        audio: {enabled: false}
      });

      var muteBtn = view.getDOMNode().querySelector('.btn-mute-audio');

      expect(muteBtn.classList.contains("muted")).eql(true);
    });
  });

  describe("OutgoingConversationView", function() {
    var store;

    function mountTestComponent() {
      return TestUtils.renderIntoDocument(
        loop.conversationViews.OutgoingConversationView({
          dispatcher: dispatcher,
          store: store
        }));
    }

    beforeEach(function() {
      navigator.mozLoop = {
        getLoopCharPref: function() { return "fake"; },
        appVersionInfo: sinon.spy()
      };

      store = new loop.store.ConversationStore({}, {
        dispatcher: dispatcher,
        client: {},
        sdkDriver: {}
      });
    });

    afterEach(function() {
      delete navigator.mozLoop;
    });

    it("should render the CallFailedView when the call state is 'terminated'",
      function() {
        store.set({callState: CALL_STATES.TERMINATED});

        view = mountTestComponent();

        TestUtils.findRenderedComponentWithType(view,
          loop.conversationViews.CallFailedView);
    });

    it("should render the PendingConversationView when the call state is 'init'",
      function() {
        store.set({callState: CALL_STATES.INIT});

        view = mountTestComponent();

        TestUtils.findRenderedComponentWithType(view,
          loop.conversationViews.PendingConversationView);
    });

    it("should render the OngoingConversationView when the call state is 'ongoing'",
      function() {
        store.set({callState: CALL_STATES.ONGOING});

        view = mountTestComponent();

        TestUtils.findRenderedComponentWithType(view,
          loop.conversationViews.OngoingConversationView);
    });

    it("should render the FeedbackView when the call state is 'finished'",
      function() {
        store.set({callState: CALL_STATES.FINISHED});

        view = mountTestComponent();

        TestUtils.findRenderedComponentWithType(view,
          loop.shared.views.FeedbackView);
    });

    it("should update the rendered views when the state is changed.",
      function() {
        store.set({callState: CALL_STATES.INIT});

        view = mountTestComponent();

        TestUtils.findRenderedComponentWithType(view,
          loop.conversationViews.PendingConversationView);

        store.set({callState: CALL_STATES.TERMINATED});

        TestUtils.findRenderedComponentWithType(view,
          loop.conversationViews.CallFailedView);
    });
  });
});
