var expect = chai.expect;

describe("loop.roomViews", function () {
  "use strict";

  var ROOM_STATES = loop.store.ROOM_STATES;

  var sandbox, dispatcher, roomStore, activeRoomStore, fakeWindow;

  beforeEach(function() {
    sandbox = sinon.sandbox.create();

    dispatcher = new loop.Dispatcher();

    fakeWindow = { document: {} };
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

  describe("DesktopRoomView", function() {
    var view;

    function mountTestComponent() {
      return TestUtils.renderIntoDocument(
        new loop.roomViews.DesktopRoomView({
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

      it("should render the GenericFailureView if the roomState is `FAILED`", function() {
        activeRoomStore.setStoreState({roomState: ROOM_STATES.FAILED});

        view = mountTestComponent();

        TestUtils.findRenderedComponentWithType(view,
          loop.conversation.GenericFailureView);
      });

      
      it("should display the main view");
    });
  });
});
