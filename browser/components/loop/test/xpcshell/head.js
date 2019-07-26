


const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Http.jsm");
Cu.import("resource://testing-common/httpd.js");

const kMockWebSocketChannelName = "Mock WebSocket Channel";
const kWebSocketChannelContractID = "@mozilla.org/network/protocol;1?name=wss";






let MockWebSocketChannel = function() {
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
          JSON.stringify({messageType: "hello"}));
        break;
      case "register":
        this.listener.onMessageAvailable(this.context,
          JSON.stringify({messageType: "register", pushEndpoint: "http://example.com/fake"}));
        break;
    }
  }
};




let gMockWebSocketChannelFactory = {
  _registered: false,

  
  
  createdInstances: [],

  resetInstances: function() {
    this.createdInstances = [];
  },

  get CID() {
    let registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
    return registrar.contractIDToCID(kWebSocketChannelContractID);
  },

  


  register: function() {
    if (this._registered)
      return;

    let registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);

    this._origFactory = Components.manager
                                  .getClassObject(Cc[kWebSocketChannelContractID],
                                                  Ci.nsIFactory);

    registrar.unregisterFactory(Components.ID(this.CID),
                                this._origFactory);

    registrar.registerFactory(Components.ID(this.CID),
                              kMockWebSocketChannelName,
                              kWebSocketChannelContractID,
                              gMockWebSocketChannelFactory);

    this._registered = true;
  },

  


  unregister: function() {
    if (!this._registered)
      return;

    let registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);

    registrar.unregisterFactory(Components.ID(this.CID),
                                gMockWebSocketChannelFactory);
    registrar.registerFactory(Components.ID(this.CID),
                              kMockWebSocketChannelName,
                              kWebSocketChannelContractID,
                              this._origFactory);

    delete this._origFactory;

    this._registered = false;
  },

  createInstance: function(outer, iid) {
    if (outer != null) {
      throw Cr.NS_ERROR_NO_AGGREGATION;
    }

    let newChannel = new MockWebSocketChannel()

    this.createdInstances.push(newChannel);

    return newChannel;
  }
};
