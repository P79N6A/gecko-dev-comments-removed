





 

"use strict";

var TestEchoReceiver = {
  init: function init() {
    addMessageListener("Test:EchoRequest", this);
  },

  receiveMessage: function receiveMessage(aMessage) {
    let json = aMessage.json;
    switch (aMessage.name) {
      case "Test:EchoRequest":
        sendAsyncMessage("Test:EchoResponse", json);
        break;
    }
  },

};

TestEchoReceiver.init();

