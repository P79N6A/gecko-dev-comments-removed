



var loop = loop || {};
loop.store = loop.store || {};

loop.store.TextChatStore = (function() {
  "use strict";

  var sharedActions = loop.shared.actions;

  var CHAT_MESSAGE_TYPES = loop.store.CHAT_MESSAGE_TYPES = {
    RECEIVED: "recv",
    SENT: "sent",
    SPECIAL: "special"
  };

  var CHAT_CONTENT_TYPES = loop.store.CHAT_CONTENT_TYPES = {
    CONTEXT: "chat-context",
    TEXT: "chat-text",
    ROOM_NAME: "room-name"
  };

  



  var TextChatStore = loop.store.createStore({
    actions: [
      "dataChannelsAvailable",
      "receivedTextChatMessage",
      "sendTextChatMessage",
      "updateRoomInfo"
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

    





    dataChannelsAvailable: function(actionData) {
      this.setStoreState({ textChatEnabled: actionData.available });

      if (actionData.available) {
        window.dispatchEvent(new CustomEvent("LoopChatEnabled"));
      }
    },

    








    _appendTextChatMessage: function(type, messageData) {
      
      
      var message = {
        type: type,
        contentType: messageData.contentType,
        message: messageData.message,
        extraData: messageData.extraData,
        sentTimestamp: messageData.sentTimestamp,
        receivedTimestamp: messageData.receivedTimestamp
      };
      var newList = [].concat(this._storeState.messageList);
      var isContext = message.contentType === CHAT_CONTENT_TYPES.CONTEXT;
      if (isContext) {
        var contextUpdated = false;
        for (var i = 0, l = newList.length; i < l; ++i) {
          
          if (newList[i].contentType === CHAT_CONTENT_TYPES.CONTEXT) {
            newList[i] = message;
            contextUpdated = true;
            break;
          }
        }
        if (!contextUpdated) {
          newList.push(message);
        }
      } else {
        newList.push(message);
      }
      this.setStoreState({ messageList: newList });

      
      
      if (message.contentType != CHAT_CONTENT_TYPES.ROOM_NAME) {
        window.dispatchEvent(new CustomEvent("LoopChatMessageAppended"));
      }
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
    },

    





    updateRoomInfo: function(actionData) {
      
      
      if (actionData.roomName) {
        this._appendTextChatMessage(CHAT_MESSAGE_TYPES.SPECIAL, {
          contentType: CHAT_CONTENT_TYPES.ROOM_NAME,
          message: actionData.roomName
        });
      }

      
      if (("urls" in actionData) && actionData.urls && actionData.urls.length) {
        
        var urlData = actionData.urls[0];

        this._appendTextChatMessage(CHAT_MESSAGE_TYPES.SPECIAL, {
          contentType: CHAT_CONTENT_TYPES.CONTEXT,
          message: urlData.description,
          extraData: {
            location: urlData.location,
            thumbnail: urlData.thumbnail
          }
        });
      }
    }
  });

  return TextChatStore;
})();
