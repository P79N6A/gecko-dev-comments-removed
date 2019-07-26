



'use strict';

const Cc = Components.classes;
const Ci = Components.interfaces;


let ppmm = Cc['@mozilla.org/parentprocessmessagemanager;1']
             .getService(Ci.nsIMessageListenerManager);

let pickResult = null;

function processPickMessage(message) {
  let sender = message.target.QueryInterface(Ci.nsIMessageSender);
  
  sender.sendAsyncMessage('file-picked', pickResult);
  
  sendAsyncMessage('file-picked-posted', { type: 'file-picked-posted' });
}

function updatePickResult(result) {
  pickResult = result;
  sendAsyncMessage('pick-result-updated', { type: 'pick-result-updated' });
}

ppmm.addMessageListener('file-picker', processPickMessage);

addMessageListener('update-pick-result', updatePickResult);
