var expect = chai.expect;



describe("loop.roomViews", function () {
  "use strict";

  var ROOM_STATES = loop.store.ROOM_STATES;

  var sandbox, dispatcher, roomStore, activeRoomStore, fakeWindow;

  beforeEach(function() {
    sandbox = sinon.sandbox.create();

    dispatcher = new loop.Dispatcher();

    fakeWindow = {
      document: {},
      navigator: {
        mozLoop: {
          getAudioBlob: sinon.stub()
        }
      }
    };
    loop.shared.mixins.setRootObject(fakeWindow);

    
    
    sandbox.stub(document.mozL10n, "get", function(x) {
      return x;
    });

    activeRoomStore = new loop.store.ActiveRoomStore({
      dispatcher: dispatcher,
      mozLoop: {}
    });
    roomStore = new loop.store.RoomStore({
      dispatcher: dispatcher,
      mozLoop: {},
      activeRoomStore: activeRoomStore
    });
  });

  afterEach(function() {
    sandbox.restore();
    loop.shared.mixins.setRootObject(window);
  });

  describe("ActiveRoomStoreMixin", function() {
    it("should merge initial state", function() {
      var TestView = React.createClass({
        mixins: [loop.roomViews.ActiveRoomStoreMixin],
        getInitialState: function() {
          return {foo: "bar"};
        },
        render: function() { return React.DOM.div(); }
      });

      var testView = TestUtils.renderIntoDocument(TestView({
        roomStore: activeRoomStore
      }));

      expect(testView.state).eql({
        roomState: ROOM_STATES.INIT,
        foo: "bar"
      });
    });

    it("should listen to store changes", function() {
      var TestView = React.createClass({
        mixins: [loop.roomViews.ActiveRoomStoreMixin],
        render: function() { return React.DOM.div(); }
      });
      var testView = TestUtils.renderIntoDocument(TestView({
        roomStore: activeRoomStore
      }));

      activeRoomStore.setStoreState({roomState: ROOM_STATES.READY});

      expect(testView.state).eql({roomState: ROOM_STATES.READY});
    });
  });

  describe("DesktopRoomControllerView", function() {
    var view;

    function mountTestComponent() {
      return TestUtils.renderIntoDocument(
        new loop.roomViews.DesktopRoomControllerView({
          mozLoop: {},
          roomStore: roomStore
        }));
    }

    describe("#render", function() {
      it("should set document.title to store.serverData.roomName", function() {
        mountTestComponent();

        activeRoomStore.setStoreState({roomName: "fakeName"});

        expect(fakeWindow.document.title).to.equal("fakeName");
      });

      it("should render the GenericFailureView if the roomState is `FAILED`",
        function() {
          activeRoomStore.setStoreState({roomState: ROOM_STATES.FAILED});

          view = mountTestComponent();

          TestUtils.findRenderedComponentWithType(view,
            loop.conversation.GenericFailureView);
        });

      it("should render the DesktopRoomInvitationView if roomState is `JOINED`",
        function() {
          activeRoomStore.setStoreState({roomState: ROOM_STATES.JOINED});

          view = mountTestComponent();

          TestUtils.findRenderedComponentWithType(view,
            loop.roomViews.DesktopRoomInvitationView);
        });

      it("should render the DesktopRoomConversationView if roomState is `HAS_PARTICIPANTS`",
        function() {
          activeRoomStore.setStoreState({roomState: ROOM_STATES.HAS_PARTICIPANTS});

          view = mountTestComponent();

          TestUtils.findRenderedComponentWithType(view,
            loop.roomViews.DesktopRoomConversationView);
        });
    });
  });
});
