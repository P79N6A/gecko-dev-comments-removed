



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
    },

    




    receivedTextChatMessage: function(actionData) {
      
      
      if (actionData.contentType != CHAT_CONTENT_TYPES.TEXT) {
        return;
      }
      
      
      var newList = this._storeState.messageList.concat({
        type: CHAT_MESSAGE_TYPES.RECEIVED,
        contentType: actionData.contentType,
        message: actionData.message
      });
      this.setStoreState({ messageList: newList });
    },

    




    sendTextChatMessage: function(actionData) {
      
      
      var newList = this._storeState.messageList.concat({
        type: CHAT_MESSAGE_TYPES.SENT,
        contentType: actionData.contentType,
        message: actionData.message
      });
      this._sdkDriver.sendTextChatMessage(actionData);
      this.setStoreState({ messageList: newList });
    }
  });

  return TextChatStore;
})();
