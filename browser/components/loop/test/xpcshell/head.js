


const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;


do_get_profile();

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Http.jsm");
Cu.import("resource://testing-common/httpd.js");
Cu.import("resource:///modules/loop/MozLoopService.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource:///modules/loop/LoopCalls.jsm");
Cu.import("resource:///modules/loop/LoopRooms.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
const { MozLoopServiceInternal } = Cu.import("resource:///modules/loop/MozLoopService.jsm", {});
const { LoopRoomsInternal } = Cu.import("resource:///modules/loop/LoopRooms.jsm", {});

XPCOMUtils.defineLazyModuleGetter(this, "MozLoopPushHandler",
                                  "resource:///modules/loop/MozLoopPushHandler.jsm");

const kMockWebSocketChannelName = "Mock WebSocket Channel";
const kWebSocketChannelContractID = "@mozilla.org/network/protocol;1?name=wss";

const kServerPushUrl = "ws://localhost";
const kLoopServerUrl = "http://localhost:3465";
const kEndPointUrl = "http://example.com/fake";
const kUAID = "f47ac11b-58ca-4372-9567-0e02b2c3d479";


var loopServer;


Services.prefs.setBoolPref("loop.enabled", true);


do_register_cleanup(() => {
  Services.prefs.clearUserPref("loop.enabled");
  MozLoopService.errors.clear();
});

function setupFakeLoopServer() {
  loopServer = new HttpServer();
  loopServer.start(-1);

  Services.prefs.setCharPref("loop.server",
    "http://localhost:" + loopServer.identity.primaryPort);

  MozLoopServiceInternal.mocks.pushHandler = mockPushHandler;

  do_register_cleanup(function() {
    loopServer.stop(function() {});
    MozLoopServiceInternal.mocks.pushHandler = undefined;
  });
}




function setupFakeFxAUserProfile() {
  MozLoopServiceInternal.fxAOAuthTokenData = { token_type: "bearer" };
  MozLoopServiceInternal.fxAOAuthProfile = { email: "fake@invalid.com" };

  do_register_cleanup(function() {
    MozLoopServiceInternal.fxAOAuthTokenData = null;
    MozLoopServiceInternal.fxAOAuthProfile = null;
  });
}

function waitForCondition(aConditionFn, aMaxTries=50, aCheckInterval=100) {
  function tryAgain() {
    function tryNow() {
      tries++;
      if (aConditionFn()) {
        deferred.resolve();
      } else if (tries < aMaxTries) {
        tryAgain();
      } else {
        deferred.reject("Condition timed out: " + aConditionFn.toSource());
      }
    }
    do_timeout(aCheckInterval, tryNow);
  }
  let deferred = Promise.defer();
  let tries = 0;
  tryAgain();
  return deferred.promise;
}

function getLoopString(stringID) {
  return MozLoopServiceInternal.localizedStrings.get(stringID);
}






let mockPushHandler = {
  
  
  registrationResult: null,
  registrationPushURL: null,
  notificationCallback: {},
  registeredChannels: {},

  


  initialize: function(options = {}) {
    if ("mockWebSocket" in options) {
      this._mockWebSocket = options.mockWebSocket;
    }
  },

  register: function(channelId, registerCallback, notificationCallback) {
    this.notificationCallback[channelId] = notificationCallback;
    this.registeredChannels[channelId] = this.registrationPushURL;
    registerCallback(this.registrationResult, this.registrationPushURL, channelId);
  },

  unregister: function(channelID) {
    return;
  },

  


  notify: function(version, chanId) {
    this.notificationCallback[chanId](version, chanId);
  }
};






function MockWebSocketChannel() {};

MockWebSocketChannel.prototype = {
  QueryInterface: XPCOMUtils.generateQI(Ci.nsIWebSocketChannel),

  initRegStatus: 0,

  defaultMsgHandler: function(msg) {
    
    this.listener.onMessageAvailable(this.context,
                                     JSON.stringify({}));
    return;
  },

  



  asyncOpen: function(aURI, aOrigin, aListener, aContext) {
    this.uri = aURI;
    this.origin = aOrigin;
    this.listener = aListener;
    this.context = aContext;

    this.listener.onStart(this.context);
  },

  sendMsg: function(aMsg) {
    var message = JSON.parse(aMsg);

    switch(message.messageType) {
      case "hello":
        this.listener.onMessageAvailable(this.context,
          JSON.stringify({messageType: "hello",
                          uaid: kUAID}));
        break;
      case "register":
        this.channelID = message.channelID;
        let statusCode = 200;
        if (this.initRegStatus) {
          statusCode = this.initRegStatus;
          this.initRegStatus = 0;
        }
        this.listener.onMessageAvailable(this.context,
          JSON.stringify({messageType: "register",
                          status: statusCode,
                          channelID: this.channelID,
                          pushEndpoint: kEndPointUrl}));
        break;
      default:
        this.defaultMsgHandler && this.defaultMsgHandler(message);
    }
  },

  close: function(aCode, aReason) {
    this.stop(aCode);
  },

  notify: function(version) {
    this.listener.onMessageAvailable(this.context,
      JSON.stringify({
        messageType: "notification", updates: [{
          channelID: this.channelID,
          version: version
        }]
    }));
  },

  stop: function (err) {
    this.listener.onStop(this.context, err || -1);
  },

  serverClose: function (err) {
    this.listener.onServerClose(this.context, err || -1);
  },
};

const extend = function(target, source) {
  for (let key of Object.getOwnPropertyNames(source)) {
    target[key] = source[key];
  }
  return target;
};
