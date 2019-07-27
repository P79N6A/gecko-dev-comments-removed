





















'use strict';
this.EXPORTED_SYMBOLS = ['ManifestObtainer'];

const MSG_KEY = 'DOM:ManifestObtainer:Obtain';
let messageCounter = 0;





const browsersMap = new WeakMap();

function ManifestObtainer() {}

ManifestObtainer.prototype = {
  obtainManifest(aBrowserWindow) {
    if (!aBrowserWindow) {
      const err = new TypeError('Invalid input. Expected xul browser.');
      return Promise.reject(err);
    }
    const mm = aBrowserWindow.messageManager;
    const onMessage = function(aMsg) {
      const msgId = aMsg.data.msgId;
      const {
        resolve, reject
      } = browsersMap.get(aBrowserWindow).get(msgId);
      browsersMap.get(aBrowserWindow).delete(msgId);
      
      
      if (!browsersMap.get(aBrowserWindow).size) {
        browsersMap.delete(aBrowserWindow);
        mm.removeMessageListener(MSG_KEY, onMessage);
      }
      if (aMsg.data.success) {
        return resolve(aMsg.data.result);
      }
      reject(toError(aMsg.data.result));
    };
    
    
    if (!browsersMap.has(aBrowserWindow)) {
      browsersMap.set(aBrowserWindow, new Map());
      mm.addMessageListener(MSG_KEY, onMessage);
    }
    return new Promise((resolve, reject) => {
      const msgId = messageCounter++;
      browsersMap.get(aBrowserWindow).set(msgId, {
        resolve: resolve,
        reject: reject
      });
      mm.sendAsyncMessage(MSG_KEY, {
        msgId: msgId
      });
    });

    function toError(aErrorClone) {
      const error = new Error();
      Object.getOwnPropertyNames(aErrorClone)
        .forEach(name => error[name] = aErrorClone[name]);
      return error;
    }
  }
};
