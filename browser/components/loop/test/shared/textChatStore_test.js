


describe("loop.store.TextChatStore", function () {
  "use strict";

  var expect = chai.expect;
  var sharedActions = loop.shared.actions;
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

    sandbox.stub(window, "dispatchEvent");
    sandbox.stub(window, "CustomEvent", function(name) {
      this.name = name;
    });
  });

  afterEach(function() {
    sandbox.restore();
  });

  describe("#dataChannelsAvailable", function() {
    it("should set textChatEnabled to the supplied state", function() {
      store.dataChannelsAvailable(new sharedActions.DataChannelsAvailable({
        available: true
      }));

      expect(store.getStoreState("textChatEnabled")).eql(true);
    });

    it("should dispatch a LoopChatEnabled event", function() {
      store.dataChannelsAvailable(new sharedActions.DataChannelsAvailable({
        available: true
      }));

      sinon.assert.calledOnce(window.dispatchEvent);
      sinon.assert.calledWithExactly(window.dispatchEvent,
        new CustomEvent("LoopChatEnabled"));
    });

    it("should not dispatch a LoopChatEnabled event if available is false", function() {
      store.dataChannelsAvailable(new sharedActions.DataChannelsAvailable({
        available: false
      }));

      sinon.assert.notCalled(window.dispatchEvent);
    });
  });

  describe("#receivedTextChatMessage", function() {
    it("should add the message to the list", function() {
      var message = "Hello!";

      store.receivedTextChatMessage({
        contentType: CHAT_CONTENT_TYPES.TEXT,
        message: message
      });

      expect(store.getStoreState("messageList")).eql([{
        type: CHAT_MESSAGE_TYPES.RECEIVED,
        contentType: CHAT_CONTENT_TYPES.TEXT,
        message: message,
        extraData: undefined
      }]);
    });

    it("should not add messages for unknown content types", function() {
      store.receivedTextChatMessage({
        contentType: "invalid type",
        message: "Hi"
      });

      expect(store.getStoreState("messageList").length).eql(0);
    });

    it("should dispatch a LoopChatMessageAppended event", function() {
      store.receivedTextChatMessage({
        contentType: CHAT_CONTENT_TYPES.TEXT,
        message: "Hello!"
      });

      sinon.assert.calledOnce(window.dispatchEvent);
      sinon.assert.calledWithExactly(window.dispatchEvent,
        new CustomEvent("LoopChatMessageAppended"));
    });
  });

  describe("#sendTextChatMessage", function() {
    it("should send the message", function() {
      var messageData = {
        contentType: CHAT_CONTENT_TYPES.TEXT,
        message: "Yes, that's what this is called."
      };

      store.sendTextChatMessage(messageData);

      sinon.assert.calledOnce(fakeSdkDriver.sendTextChatMessage);
      sinon.assert.calledWithExactly(fakeSdkDriver.sendTextChatMessage, messageData);
    });

    it("should add the message to the list", function() {
      var messageData = {
        contentType: CHAT_CONTENT_TYPES.TEXT,
        message: "It's awesome!"
      };

      store.sendTextChatMessage(messageData);

      expect(store.getStoreState("messageList")).eql([{
        type: CHAT_MESSAGE_TYPES.SENT,
        contentType: messageData.contentType,
        message: messageData.message,
        extraData: undefined
      }]);
    });

    it("should dipatch a LoopChatMessageAppended event", function() {
      store.sendTextChatMessage({
        contentType: CHAT_CONTENT_TYPES.TEXT,
        message: "Hello!"
      });

      sinon.assert.calledOnce(window.dispatchEvent);
      sinon.assert.calledWithExactly(window.dispatchEvent,
        new CustomEvent("LoopChatMessageAppended"));
    });
  });

  describe("#updateRoomInfo", function() {
    it("should add the room name to the list", function() {
      store.updateRoomInfo(new sharedActions.UpdateRoomInfo({
        roomName: "Let's share!",
        roomOwner: "Mark",
        roomUrl: "fake"
      }));

      expect(store.getStoreState("messageList")).eql([{
        type: CHAT_MESSAGE_TYPES.SPECIAL,
        contentType: CHAT_CONTENT_TYPES.ROOM_NAME,
        message: "Let's share!",
        extraData: undefined
      }]);
    });

    it("should add the context to the list", function() {
      store.updateRoomInfo(new sharedActions.UpdateRoomInfo({
        roomName: "Let's share!",
        roomOwner: "Mark",
        roomUrl: "fake",
        urls: [{
          description: "A wonderful event",
          location: "http://wonderful.invalid",
          thumbnail: "fake"
        }]
      }));

      expect(store.getStoreState("messageList")).eql([
        {
          type: CHAT_MESSAGE_TYPES.SPECIAL,
          contentType: CHAT_CONTENT_TYPES.ROOM_NAME,
          message: "Let's share!",
          extraData: undefined
        }, {
          type: CHAT_MESSAGE_TYPES.SPECIAL,
          contentType: CHAT_CONTENT_TYPES.CONTEXT,
          message: "A wonderful event",
          extraData: {
            location: "http://wonderful.invalid",
            thumbnail: "fake"
          }
        }
      ]);
    });

    it("should not dispatch a LoopChatMessageAppended event", function() {
      store.updateRoomInfo(new sharedActions.UpdateRoomInfo({
        roomName: "Let's share!",
        roomOwner: "Mark",
        roomUrl: "fake"
      }));

      sinon.assert.notCalled(window.dispatchEvent);
    });
  });
});
