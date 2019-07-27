



'use strict';

this.EXPORTED_SYMBOLS = ['PromiseMessage'];

let msgId = 0;

let PromiseMessage = {
  send(messageManager, name, data = {}) {
    let id = msgId++;

    
    let dataCopy = {};
    for (let prop in data) {
      dataCopy[prop] = data[prop];
    }
    dataCopy.id = id;

    
    messageManager.sendAsyncMessage(name, dataCopy);

    
    return new Promise(resolve => {
      messageManager.addMessageListener(name, function listener(reply) {
        if (reply.data.id !== id) {
          return;
        }
        messageManager.removeMessageListener(name, listener);
        resolve(reply);
      });
    });
  }
};
