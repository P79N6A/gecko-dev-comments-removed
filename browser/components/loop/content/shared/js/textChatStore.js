



var loop = loop || {};
loop.store = loop.store || {};

loop.store.TextChatStore = (function() {
  "use strict";

  var sharedActions = loop.shared.actions;

  var CHAT_MESSAGE_TYPES = loop.store.CHAT_MESSAGE_TYPES = {
    RECEIVED: "recv",
    SENT: "sent"
  };

  var CHAT_CONTENT_TYPES = loop.store.CHAT_CONTENT_TYPES = {
    TEXT: "chat-text"
  };

  



  var TextChatStore = loop.store.createStore({
    actions: [
      "dataChannelsAvailable",
      "receivedTextChatMessage",
      "sendTextChatMessage"
    ],

    







    initialize: function(options) {
      options = options || {};

      if (!options.sdkDriver) {
        throw new Error("Missing option sdkDriver");
      }

      this._sdkDriver = options.sdkDriver;
    },

    


    getInitialStoreState: function() {
      return {
        textChatEnabled: false,
        
        
        
        messageList: [],
        length: 0
      };
    },

    



    dataChannelsAvailable: function() {
      this.setStoreState({ textChatEnabled: true });
      window.dispatchEvent(new CustomEvent("LoopChatEnabled"));
    },

    





    _appendTextChatMessage: function(type, actionData) {
      
      
      var message = {
        type: type,
        contentType: actionData.contentType,
        message: actionData.message
      };
      var newList = this._storeState.messageList.concat(message);
      this.setStoreState({ messageList: newList });

      window.dispatchEvent(new CustomEvent("LoopChatMessageAppended"));
    },

    




    receivedTextChatMessage: function(actionData) {
      
      
      if (actionData.contentType != CHAT_CONTENT_TYPES.TEXT) {
        return;
      }

      this._appendTextChatMessage(CHAT_MESSAGE_TYPES.RECEIVED, actionData);
    },

    




    sendTextChatMessage: function(actionData) {
      this._appendTextChatMessage(CHAT_MESSAGE_TYPES.SENT, actionData);
      this._sdkDriver.sendTextChatMessage(actionData);
    }
  });

  return TextChatStore;
})();
