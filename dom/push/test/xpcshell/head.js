


'use strict';

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://gre/modules/Timer.jsm');
Cu.import('resource://gre/modules/Promise.jsm');
Cu.import('resource://gre/modules/Preferences.jsm');

const serviceExports = Cu.import('resource://gre/modules/PushService.jsm', {});
const servicePrefs = new Preferences('dom.push.');

XPCOMUtils.defineLazyServiceGetter(
  this,
  "PushNotificationService",
  "@mozilla.org/push/NotificationService;1",
  "nsIPushNotificationService"
);

const WEBSOCKET_CLOSE_GOING_AWAY = 1001;


Services.obs.addObserver(function observe(subject, topic, data) {
  Services.obs.removeObserver(observe, topic, false);
  serviceExports.PushService.uninit();
  
  
  
  
  let done = false;
  setTimeout(() => done = true, 1000);
  let thread = Services.tm.mainThread;
  while (!done) {
    try {
      thread.processNextEvent(true);
    } catch (e) {
      Cu.reportError(e);
    }
  }
}, 'profile-change-net-teardown', false);









function after(times, func) {
  return function afterFunc() {
    if (--times <= 0) {
      return func.apply(this, arguments);
    }
  };
}









function promisifyDatabase(db) {
  return new Proxy(db, {
    get(target, property) {
      let method = target[property];
      if (typeof method != 'function') {
        return method;
      }
      return function(...params) {
        return new Promise((resolve, reject) => {
          method.call(target, ...params, resolve, reject);
        });
      };
    }
  });
}







function cleanupDatabase(db) {
  return new Promise(resolve => {
    function close() {
      db.close();
      resolve();
    }
    db.drop(close, close);
  });
}







function nextTick(...callbacks) {
  callbacks.reduce((promise, callback) => promise.then(() => {
    callback();
  }), Promise.resolve()).catch(Cu.reportError);
}







function promiseObserverNotification(topic, matchFunc) {
  return new Promise((resolve, reject) => {
    Services.obs.addObserver(function observe(subject, topic, data) {
      let matches = typeof matchFunc != 'function' || matchFunc(subject, data);
      if (!matches) {
        return;
      }
      Services.obs.removeObserver(observe, topic, false);
      resolve({subject, data});
    }, topic, false);
  });
}











function waitForPromise(promise, delay, message = 'Timed out waiting on promise') {
  let timeoutDefer = Promise.defer();
  let id = setTimeout(() => timeoutDefer.reject(new Error(message)), delay);
  return Promise.race([
    promise.then(value => {
      clearTimeout(id);
      return value;
    }, error => {
      clearTimeout(id);
      throw error;
    }),
    timeoutDefer.promise
  ]);
}











function makeStub(target, stubs) {
  return new Proxy(target, {
    get(target, property) {
      if (!stubs || typeof stubs != 'object' || !(property in stubs)) {
        return target[property];
      }
      let stub = stubs[property];
      if (typeof stub != 'function') {
        return stub;
      }
      let original = target[property];
      if (typeof original != 'function') {
        return stub.call(this, original);
      }
      return function callStub(...params) {
        return stub.call(this, original, ...params);
      };
    }
  });
}








function disableServiceWorkerEvents(...scopes) {
  for (let scope of scopes) {
    Services.perms.add(
      Services.io.newURI(scope, null, null),
      'push',
      Ci.nsIPermissionManager.DENY_ACTION
    );
  }
}







function setPrefs(prefs = {}) {
  let defaultPrefs = Object.assign({
    debug: true,
    serverURL: 'wss://push.example.org',
    'connection.enabled': true,
    userAgentID: '',
    enabled: true,
    
    
    'adaptive.enabled': false,
    'udp.wakeupEnabled': false,
    
    requestTimeout: 10000,
    retryBaseInterval: 5000,
    pingInterval: 30 * 60 * 1000,
    'pingInterval.default': 3 * 60 * 1000,
    'pingInterval.mobile': 3 * 60 * 1000,
    'pingInterval.wifi': 3 * 60 * 1000,
    'adaptive.lastGoodPingInterval': 3 * 60 * 1000,
    'adaptive.lastGoodPingInterval.mobile': 3 * 60 * 1000,
    'adaptive.lastGoodPingInterval.wifi': 3 * 60 * 1000,
    'adaptive.gap': 60000,
    'adaptive.upperLimit': 29 * 60 * 1000,
    
    'adaptive.mobile': ''
  }, prefs);
  for (let pref in defaultPrefs) {
    servicePrefs.set(pref, defaultPrefs[pref]);
  }
}

function compareAscending(a, b) {
  return a > b ? 1 : a < b ? -1 : 0;
}




















function MockWebSocket(originalURI, handlers = {}) {
  this._originalURI = originalURI;
  this._onHello = handlers.onHello;
  this._onRegister = handlers.onRegister;
  this._onUnregister = handlers.onUnregister;
  this._onACK = handlers.onACK;
  this._onPing = handlers.onPing;
}

MockWebSocket.prototype = {
  _originalURI: null,
  _onHello: null,
  _onRegister: null,
  _onUnregister: null,
  _onACK: null,
  _onPing: null,

  _listener: null,
  _context: null,

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsISupports,
    Ci.nsIWebSocketChannel
  ]),

  get originalURI() {
    return this._originalURI;
  },

  asyncOpen(uri, origin, listener, context) {
    this._listener = listener;
    this._context = context;
    nextTick(() => this._listener.onStart(this._context));
  },

  _handleMessage(msg) {
    let messageType, request;
    if (msg == '{}') {
      request = {};
      messageType = 'ping';
    } else {
      request = JSON.parse(msg);
      messageType = request.messageType;
    }
    switch (messageType) {
    case 'hello':
      if (typeof this._onHello != 'function') {
        throw new Error('Unexpected handshake request');
      }
      this._onHello(request);
      break;

    case 'register':
      if (typeof this._onRegister != 'function') {
        throw new Error('Unexpected register request');
      }
      this._onRegister(request);
      break;

    case 'unregister':
      if (typeof this._onUnregister != 'function') {
        throw new Error('Unexpected unregister request');
      }
      this._onUnregister(request);
      break;

    case 'ack':
      if (typeof this._onACK != 'function') {
        throw new Error('Unexpected acknowledgement');
      }
      this._onACK(request);
      break;

    case 'ping':
      if (typeof this._onPing == 'function') {
        this._onPing(request);
      } else {
        
        this.serverSendMsg('{}');
      }
      break;

    default:
      throw new Error('Unexpected message: ' + messageType);
    }
  },

  sendMsg(msg) {
    this._handleMessage(msg);
  },

  close(code, reason) {
    nextTick(() => this._listener.onStop(this._context, Cr.NS_OK));
  },

  






  serverSendMsg(msg) {
    if (typeof msg != 'string') {
      throw new Error('Invalid response message');
    }
    nextTick(
      () => this._listener.onMessageAvailable(this._context, msg),
      () => this._listener.onAcknowledge(this._context, 0)
    );
  },

  







  serverClose(statusCode, reason = '') {
    if (!isFinite(statusCode)) {
      statusCode = WEBSOCKET_CLOSE_GOING_AWAY;
    }
    nextTick(
      () => this._listener.onServerClose(this._context, statusCode, reason),
      () => this._listener.onStop(this._context, Cr.NS_BASE_STREAM_CLOSED)
    );
  }
};






function MockDesktopNetworkInfo() {}

MockDesktopNetworkInfo.prototype = {
  getNetworkInformation() {
    return {mcc: '', mnc: '', ip: ''};
  },

  getNetworkState(callback) {
    callback({mcc: '', mnc: '', ip: '', netid: ''});
  },

  getNetworkStateChangeEventName() {
    return 'network:offline-status-changed';
  }
};










function MockMobileNetworkInfo(info = {}) {
  this._info = info;
}

MockMobileNetworkInfo.prototype = {
  _info: null,

  getNetworkInformation() {
    let {mcc, mnc, ip} = this._info;
    return {mcc, mnc, ip};
  },

  getNetworkState(callback) {
    let {mcc, mnc, ip, netid} = this._info;
    callback({mcc, mnc, ip, netid});
  },

  getNetworkStateChangeEventName() {
    return 'network-active-changed';
  }
};
