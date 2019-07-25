





































var EXPORTED_SYMBOLS = ['addListener', 'addObject',
                        'removeListener',
                        'sendMessage', 'log', 'pass', 'fail'];

var listeners = {};


function addListener(msgType, listener) {

  if (listeners[msgType] === undefined) {
    listeners[msgType] = [];
  }
  listeners[msgType].push(listener);
}


function addObject(object) {
  for (var msgType in object) {
    addListener(msgType, object[msgType]);
  }
}


function removeListener(listener) {
  for (var msgType in listeners) {
    for (let i = 0; i < listeners.length; ++i) {
      if (listeners[msgType][i] == listener) {
        listeners[msgType].splice(i, 1); 
      }
    }
  }
}

function sendMessage(msgType, obj) {
  if (listeners[msgType] === undefined) {
    return;
  }
  for (let i = 0; i < listeners[msgType].length; ++i) {
    listeners[msgType][i](obj);
  }
}

function log(obj) {
  sendMessage('log', obj);
}

function pass(obj) {
  sendMessage('pass', obj);
}

function fail(obj) {
  sendMessage('fail', obj);
}
