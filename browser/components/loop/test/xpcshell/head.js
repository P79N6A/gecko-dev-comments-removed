


const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Http.jsm");
Cu.import("resource://testing-common/httpd.js");

XPCOMUtils.defineLazyModuleGetter(this, "MozLoopService",
                                  "resource:///modules/loop/MozLoopService.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "MozLoopPushHandler",
                                  "resource:///modules/loop/MozLoopPushHandler.jsm");

const kMockWebSocketChannelName = "Mock WebSocket Channel";
const kWebSocketChannelContractID = "@mozilla.org/network/protocol;1?name=wss";

const kServerPushUrl = "http://localhost:3456";
const kEndPointUrl = "http://example.com/fake";
const kUAID = "f47ac11b-58ca-4372-9567-0e02b2c3d479";


var loopServer;


Services.prefs.setBoolPref("loop.enabled", true);
Services.prefs.setBoolPref("loop.throttled", false);

function setupFakeLoopServer() {
  loopServer = new HttpServer();
  loopServer.start(-1);

  Services.prefs.setCharPref("services.push.serverURL", kServerPushUrl);

  Services.prefs.setCharPref("loop.server",
    "http://localhost:" + loopServer.identity.primaryPort);

  do_register_cleanup(function() {
    loopServer.stop(function() {});
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






let mockPushHandler = {
  
  
  registrationResult: null,
  registrationPushURL: undefined,

  


  initialize: function(registerCallback, notificationCallback) {
    registerCallback(this.registrationResult, this.registrationPushURL);
    this._notificationCallback = notificationCallback;
  },

  


  notify: function(version) {
    this._notificationCallback(version);
  }
};






let MockWebSocketChannel = function(initRegStatus) {
  this.initRegStatus = initRegStatus;
};

MockWebSocketChannel.prototype = {
  QueryInterface: XPCOMUtils.generateQI(Ci.nsIWebSocketChannel),

  



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
    }
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
