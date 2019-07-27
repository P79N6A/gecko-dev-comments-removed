


describe("loop.shared.views.TextChatView", function () {
  "use strict";

  var expect = chai.expect;
  var sharedActions = loop.shared.actions;
  var sharedViews = loop.shared.views;
  var TestUtils = React.addons.TestUtils;
  var CHAT_MESSAGE_TYPES = loop.store.CHAT_MESSAGE_TYPES;
  var CHAT_CONTENT_TYPES = loop.store.CHAT_CONTENT_TYPES;

  var dispatcher, fakeSdkDriver, sandbox, store;

  beforeEach(function() {
    sandbox = sinon.sandbox.create();
    sandbox.useFakeTimers();

    dispatcher = new loop.Dispatcher();
    sandbox.stub(dispatcher, "dispatch");

    fakeSdkDriver = {
      sendTextChatMessage: sinon.stub()
    };

    store = new loop.store.TextChatStore(dispatcher, {
      sdkDriver: fakeSdkDriver
    });

    loop.store.StoreMixin.register({
      textChatStore: store
    });
  });

  afterEach(function() {
    sandbox.restore();
  });

  describe("TextChatEntriesView", function() {
    var view;

    function mountTestComponent(extraProps) {
      var basicProps = {
        dispatcher: dispatcher,
        messageList: []
      };

      return TestUtils.renderIntoDocument(
        React.createElement(loop.shared.views.chat.TextChatEntriesView,
          _.extend(basicProps, extraProps)));
    }

    it("should render message entries when message were sent/ received", function() {
      view = mountTestComponent({
        messageList: [{
          type: CHAT_MESSAGE_TYPES.RECEIVED,
          contentType: CHAT_CONTENT_TYPES.TEXT,
          message: "Hello!"
        }, {
          type: CHAT_MESSAGE_TYPES.SENT,
          contentType: CHAT_CONTENT_TYPES.TEXT,
          message: "Is it me you're looking for?"
        }]
      });

      var node = view.getDOMNode();
      expect(node).to.not.eql(null);

      var entries = node.querySelectorAll(".text-chat-entry");
      expect(entries.length).to.eql(2);
      expect(entries[0].classList.contains("received")).to.eql(true);
      expect(entries[1].classList.contains("received")).to.not.eql(true);
    });

    it("should play a sound when a message is received", function() {
      view = mountTestComponent();
      sandbox.stub(view, "play");

      view.setProps({
        messageList: [{
          type: CHAT_MESSAGE_TYPES.RECEIVED,
          contentType: CHAT_CONTENT_TYPES.TEXT,
          message: "Hello!"
        }]
      });

      sinon.assert.calledOnce(view.play);
      sinon.assert.calledWithExactly(view.play, "message");
    });

    it("should not play a sound when a special message is displayed", function() {
      view = mountTestComponent();
      sandbox.stub(view, "play");

      view.setProps({
        messageList: [{
          type: CHAT_MESSAGE_TYPES.SPECIAL,
          contentType: CHAT_CONTENT_TYPES.ROOM_NAME,
          message: "Hello!"
        }]
      });

      sinon.assert.notCalled(view.play);
    });

    it("should not play a sound when a message is sent", function() {
      view = mountTestComponent();
      sandbox.stub(view, "play");

      view.setProps({
        messageList: [{
          type: CHAT_MESSAGE_TYPES.SENT,
          contentType: CHAT_CONTENT_TYPES.TEXT,
          message: "Hello!"
        }]
      });

      sinon.assert.notCalled(view.play);
    });
  });

  describe("TextChatView", function() {
    var view;

    function mountTestComponent(extraProps) {
      var props = _.extend({
        dispatcher: dispatcher
      }, extraProps);
      return TestUtils.renderIntoDocument(
        React.createElement(loop.shared.views.chat.TextChatView, props));
    }

    beforeEach(function() {
      store.setStoreState({ textChatEnabled: true });
    });

    it("should not display anything if no messages and text chat not enabled and showAlways is false", function() {
      store.setStoreState({ textChatEnabled: false });

      view = mountTestComponent({
        showAlways: false
      });

      expect(view.getDOMNode()).eql(null);
    });

    it("should display the view if no messages and text chat not enabled and showAlways is true", function() {
      store.setStoreState({ textChatEnabled: false });

      view = mountTestComponent({
        showAlways: true
      });

      expect(view.getDOMNode()).not.eql(null);
    });

    it("should display the view if text chat is enabled", function() {
      view = mountTestComponent({
        showAlways: true
      });

      expect(view.getDOMNode()).not.eql(null);
    });

    it("should display only the text chat box if entry is enabled but there are no messages", function() {
      view = mountTestComponent();

      var node = view.getDOMNode();

      expect(node.querySelector(".text-chat-box")).not.eql(null);
      expect(node.querySelector(".text-chat-entries")).eql(null);
    });

    it("should render a room name special entry", function() {
      view = mountTestComponent({
        showRoomName: true
      });

      store.updateRoomInfo(new sharedActions.UpdateRoomInfo({
        roomName: "A wonderful surprise!",
        roomOwner: "Chris",
        roomUrl: "Fake"
      }));

      var node = view.getDOMNode();
      expect(node.querySelector(".text-chat-entries")).to.not.eql(null);

      var entries = node.querySelectorAll(".text-chat-entry");
      expect(entries.length).eql(1);
      expect(entries[0].classList.contains("special")).eql(true);
      expect(entries[0].classList.contains("room-name")).eql(true);
    });

    it("should render a special entry for the context url", function() {
      view = mountTestComponent({
        showRoomName: true
      });

      store.updateRoomInfo(new sharedActions.UpdateRoomInfo({
        roomName: "A Very Long Conversation Name",
        roomOwner: "fake",
        roomUrl: "http://showcase",
        urls: [{
          description: "A wonderful page!",
          location: "http://wonderful.invalid"
          
        }]
      }));

      var node = view.getDOMNode();
      expect(node.querySelector(".text-chat-entries")).to.not.eql(null);

      expect(node.querySelector(".context-url-view-wrapper")).to.not.eql(null);
    });

    it("should dispatch SendTextChatMessage action when enter is pressed", function() {
      view = mountTestComponent();

      var entryNode = view.getDOMNode().querySelector(".text-chat-box > form > input");

      TestUtils.Simulate.change(entryNode, {
        target: {
          value: "Hello!"
        }
      });
      TestUtils.Simulate.keyDown(entryNode, {
        key: "Enter",
        which: 13
      });

      sinon.assert.calledOnce(dispatcher.dispatch);
      sinon.assert.calledWithExactly(dispatcher.dispatch,
        new sharedActions.SendTextChatMessage({
          contentType: CHAT_CONTENT_TYPES.TEXT,
          message: "Hello!"
        }));
    });

    it("should not dispatch SendTextChatMessage when the message is empty", function() {
      view = mountTestComponent();

      var entryNode = view.getDOMNode().querySelector(".text-chat-box > form > input");

      TestUtils.Simulate.keyDown(entryNode, {
        key: "Enter",
        which: 13
      });

      sinon.assert.notCalled(dispatcher.dispatch);
    });
  });
});
