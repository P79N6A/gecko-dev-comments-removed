




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

  if ("ondismiss" in options && typeof options.ondismiss === "function")
    this.ondismiss = options.ondismiss;
}

let HomeBanner = (function () {
  
  let _messages = {};

  
  let _queue = [];


  let _handleGet = function() {
    
    
    let id = _queue.shift();
    _queue.push(id);

    let message = _messages[id];
    sendMessageToJava({
      type: "HomeBanner:Data",
      id: message.id,
      text: message.text,
      iconURI: message.iconURI
    });

    if (message.onshown)
      message.onshown();
  };

  let _handleClick = function(id) {
    let message = _messages[id];
    if (message.onclick)
      message.onclick();
  };

  let _handleDismiss = function(id) {
    let message = _messages[id];
    if (message.ondismiss)
      message.ondismiss();
  };

  return Object.freeze({
    observe: function(subject, topic, data) {
      switch(topic) {
        case "HomeBanner:Get":
          _handleGet();
          break;

        case "HomeBanner:Click":
          _handleClick(data);
          break;

        case "HomeBanner:Dismiss":
          _handleDismiss(data);
          break; 
      }
    },

    




    add: function(options) {
      let message = new BannerMessage(options);
      _messages[message.id] = message;

      
      _queue.push(message.id);

      
      
      if (Object.keys(_messages).length == 1) {
        Services.obs.addObserver(this, "HomeBanner:Get", false);
        Services.obs.addObserver(this, "HomeBanner:Click", false);
        Services.obs.addObserver(this, "HomeBanner:Dismiss", false);

        
        
        _handleGet();
      }

      return message.id;
    },

    




    remove: function(id) {
      delete _messages[id];

      
      let index = _queue.indexOf(id);
      _queue.splice(index, 1);

      
      if (Object.keys(_messages).length == 0) {
        Services.obs.removeObserver(this, "HomeBanner:Get");
        Services.obs.removeObserver(this, "HomeBanner:Click");
        Services.obs.removeObserver(this, "HomeBanner:Dismiss");
      }
    }
  });
})();

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

let HomePanels = (function () {
  
  let _panels = {};

  let _panelToJSON = function(panel) {
    return {
      id: panel.id,
      title: panel.title,
      layout: panel.layout,
      views: panel.views
    };
  };

  let _handleGet = function(data) {
    let requestId = data.requestId;
    let ids = data.ids || null;

    let panels = [];
    for (let id in _panels) {
      let panel = _panels[id];

      
      if (ids == null || ids.indexOf(panel.id) >= 0) {
        panels.push(_panelToJSON(panel));
      }
    }

    sendMessageToJava({
      type: "HomePanels:Data",
      panels: panels,
      requestId: requestId
    });
  };

  
  let _valueExists = function(obj, value) {
    for (let key in obj) {
      if (obj[key] == value) {
        return true;
      }
    }
    return false;
  };

  return Object.freeze({
    
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

    
    Item: Object.freeze({
      ARTICLE: "article",
      IMAGE: "image"
    }),

    
    ItemHandler: Object.freeze({
      BROWSER: "browser",
      INTENT: "intent"
    }),

    add: function(options) {
      let panel = new Panel(options);
      if (!panel.id || !panel.title) {
        throw "Home.panels: Can't create a home panel without an id and title!";
      }

      let action = options.action;

      
      
      if (panel.id in _panels && action != this.Action.REFRESH) {
        throw "Home.panels: Panel already exists: id = " + panel.id;
      }

      if (!_valueExists(this.Layout, panel.layout)) {
        throw "Home.panels: Invalid layout for panel: panel.id = " + panel.id + ", panel.layout =" + panel.layout;
      }

      for (let view of panel.views) {
        if (!_valueExists(this.View, view.type)) {
          throw "Home.panels: Invalid view type: panel.id = " + panel.id + ", view.type = " + view.type;
        }

        if (!view.itemType) {
          if (view.type == this.View.LIST) {
            
            view.itemType = this.Item.ARTICLE;
          } else if (view.type == this.View.GRID) {
            
            view.itemType = this.Item.IMAGE;
          }
        } else if (!_valueExists(this.Item, view.itemType)) {
          throw "Home.panels: Invalid item type: panel.id = " + panel.id + ", view.itemType = " + view.itemType;
        }

        if (!view.itemHandler) {
          
          view.itemHandler = this.ItemHandler.BROWSER;
        } else if (!_valueExists(this.ItemHandler, view.itemHandler)) {
          throw "Home.panels: Invalid item handler: panel.id = " + panel.id + ", view.itemHandler = " + view.itemHandler;
        }

        if (!view.dataset) {
          throw "Home.panels: No dataset provided for view: panel.id = " + panel.id + ", view.type = " + view.type;
        }
      }

      _panels[panel.id] = panel;

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
          panel: _panelToJSON(panel)
        });
      }
    },

    remove: function(id) {
      if (!(id in _panels)) {
        throw "Home.panels: Panel doesn't exist: id = " + id;
      }

      let panel = _panels[id];
      delete _panels[id];

      sendMessageToJava({
        type: "HomePanels:Uninstall",
        panel: _panelToJSON(panel)
      });
    }
  });
})();


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
