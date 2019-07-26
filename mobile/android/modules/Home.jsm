




"use strict";

this.EXPORTED_SYMBOLS = ["Home"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/SharedPreferences.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");


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

let HomeBanner = Object.freeze({
  
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
});

function Panel(options) {
  if ("id" in options)
    this.id = options.id;

  if ("title" in options)
    this.title = options.title;

  if ("layout" in options)
    this.layout = options.layout;

  if ("views" in options)
    this.views = options.views;
}

let HomePanels = Object.freeze({
  
  Layout: Object.freeze({
    FRAME: "frame"
  }),

  
  View: Object.freeze({
    LIST: "list",
    GRID: "grid"
  }),

  
  Action: Object.freeze({
    INSTALL: "install",
    REFRESH: "refresh"
  }),

  
  ItemHandler: Object.freeze({
    BROWSER: "browser",
    INTENT: "intent"
  }),

  
  _panels: {},

  _panelToJSON : function(panel) {
    return {
      id: panel.id,
      title: panel.title,
      layout: panel.layout,
      views: panel.views
    };
  },

  _handleGet: function(data) {
    let requestId = data.requestId;
    let ids = data.ids || null;

    let panels = [];
    for (let id in this._panels) {
      let panel = this._panels[id];

      
      if (ids == null || ids.indexOf(panel.id) >= 0) {
        panels.push(this._panelToJSON(panel));
      }
    }

    sendMessageToJava({
      type: "HomePanels:Data",
      panels: panels,
      requestId: requestId
    });
  },

  add: function(options) {
    let panel = new Panel(options);
    if (!panel.id || !panel.title) {
      throw "Home.panels: Can't create a home panel without an id and title!";
    }

    let action = options.action;

    
    
    if (panel.id in this._panels && action != this.Action.REFRESH) {
      throw "Home.panels: Panel already exists: id = " + panel.id;
    }

    if (!this._valueExists(this.Layout, panel.layout)) {
      throw "Home.panels: Invalid layout for panel: panel.id = " + panel.id + ", panel.layout =" + panel.layout;
    }

    for (let view of panel.views) {
      if (!this._valueExists(this.View, view.type)) {
        throw "Home.panels: Invalid view type: panel.id = " + panel.id + ", view.type = " + view.type;
      }

      if (!view.itemHandler) {
        
        view.itemHandler = this.ItemHandler.BROWSER;
      } else if (!this._valueExists(this.ItemHandler, view.itemHandler)) {
        throw "Home.panels: Invalid item handler: panel.id = " + panel.id + ", view.itemHandler = " + view.itemHandler;
      }

      if (!view.dataset) {
        throw "Home.panels: No dataset provided for view: panel.id = " + panel.id + ", view.type = " + view.type;
      }
    }

    this._panels[panel.id] = panel;

    if (action) {
      let messageType;

      switch(action) {
        case this.Action.INSTALL:
          messageType = "HomePanels:Install";
          break;

        case this.Action.REFRESH:
          messageType = "HomePanels:Refresh";
          break;

        default:
          throw "Home.panels: Invalid action for panel: panel.id = " + panel.id + ", action = " + action;
      }

      sendMessageToJava({
        type: messageType,
        panel: this._panelToJSON(panel)
      });
    }
  },

  remove: function(id) {
    if (!(id in this._panels)) {
      throw "Home.panels: Panel doesn't exist: id = " + id;
    }

    let panel = this._panels[id];
    delete this._panels[id];

    sendMessageToJava({
      type: "HomePanels:Remove",
      panel: this._panelToJSON(panel)
    });
  },

  
  _valueExists: function(obj, value) {
    for (let key in obj) {
      if (obj[key] == value) {
        return true;
      }
    }
    return false;
  }
});


this.Home = Object.freeze({
  banner: HomeBanner,
  panels: HomePanels,

  
  observe: function(subject, topic, data) {
    switch(topic) {
      case "HomePanels:Get":
        HomePanels._handleGet(JSON.parse(data));
        break;
    }
  }
});
