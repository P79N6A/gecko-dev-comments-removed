




"use strict";

this.EXPORTED_SYMBOLS = ["Home"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");


function resolveGeckoURI(aURI) {
  if (!aURI)
    throw "Can't resolve an empty uri";

  if (aURI.startsWith("chrome://")) {
    let registry = Cc['@mozilla.org/chrome/chrome-registry;1'].getService(Ci["nsIChromeRegistry"]);
    return registry.convertChromeURL(Services.io.newURI(aURI, null, null)).spec;
  } else if (aURI.startsWith("resource://")) {
    let handler = Services.io.getProtocolHandler("resource").QueryInterface(Ci.nsIResProtocolHandler);
    return handler.resolveURI(Services.io.newURI(aURI, null, null));
  }
  return aURI;
}

function sendMessageToJava(message) {
  return Services.androidBridge.handleGeckoMessage(JSON.stringify(message));
}

function BannerMessage(options) {
  let uuidgen = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator);
  this.id = uuidgen.generateUUID().toString();

  if ("text" in options && options.text != null)
    this.text = options.text;

  if ("icon" in options && options.icon != null)
    this.iconURI = resolveGeckoURI(options.icon);

  if ("onclick" in options && typeof options.onclick === "function")
    this.onclick = options.onclick;
}

let HomeBanner = {
  
  _messages: {},

  
  _queue: [],

  observe: function(subject, topic, data) {
    switch(topic) {
      case "HomeBanner:Get":
        this._handleGet();
        break;

      case "HomeBanner:Click":
        this._handleClick(data);
        break;
    }
  },

  _handleGet: function() {
    
    
    let id = this._queue.shift();
    this._queue.push(id);

    let message = this._messages[id];
    sendMessageToJava({
      type: "HomeBanner:Data",
      id: message.id,
      text: message.text,
      iconURI: message.iconURI
    });
  },

  _handleClick: function(id) {
    let message = this._messages[id];
    if (message.onclick)
      message.onclick();
  },

  




  add: function(options) {
    let message = new BannerMessage(options);
    this._messages[message.id] = message;

    
    this._queue.push(message.id);

    
    
    if (Object.keys(this._messages).length == 1) {
      Services.obs.addObserver(this, "HomeBanner:Get", false);
      Services.obs.addObserver(this, "HomeBanner:Click", false);

      
      
      this._handleGet();
    }

    return message.id;
  },

  




  remove: function(id) {
    delete this._messages[id];

    
    let index = this._queue.indexOf(id);
    this._queue.splice(index, 1);

    
    if (Object.keys(this._messages).length == 0) {
      Services.obs.removeObserver(this, "HomeBanner:Get");
      Services.obs.removeObserver(this, "HomeBanner:Click");
    }
  }
};


this.Home = {
  banner: HomeBanner
}
