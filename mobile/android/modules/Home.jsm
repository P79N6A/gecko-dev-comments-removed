




"use strict";

this.EXPORTED_SYMBOLS = ["Home"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/SharedPreferences.jsm");


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

  if ("onshown" in options && typeof options.onshown === "function")
    this.onshown = options.onshown;

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

    if (message.onshown)
      message.onshown();
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

function List(options) {
  if ("id" in options)
    this.id = options.id;

  if ("title" in options)
    this.title = options.title;
}

function HomeLists() {
  this.PREF_KEY = "home_lists";

  this._sharedPrefs = new SharedPreferences();
  this._lists = {};

  let prefValue = this._sharedPrefs.getCharPref(this.PREF_KEY);
  if (!prefValue) {
    return;
  }

  JSON.parse(prefValue).forEach(data => {
    let list = new List(data);
    this._lists[list.id] = list;
  });
}

HomeLists.prototype = {
  add: function(options) {
    let list = new List(options);
    if (!list.id || !list.title) {
      throw "Can't create a home list without an id and title!";
    }

    
    if (list.id in this._lists) {
      throw "List already exists: " + list.id;
    }

    this._lists[list.id] = list;
    this._updateSharedPref();

    
    sendMessageToJava({
      type: "HomeLists:Added",
      id: list.id,
      title: list.title
    });
  },

  remove: function(id) {
    delete this._lists[id];
    this._updateSharedPref();
  },

  
  _updateSharedPref: function() {
    let lists = [];
    for (let id in this._lists) {
      let list = this._lists[id];
      lists.push({ id: list.id, title: list.title});
    }
    this._sharedPrefs.setCharPref(this.PREF_KEY, JSON.stringify(lists));
  }

};


this.Home = {
  banner: HomeBanner,
  lists: new HomeLists()
}
