




"use strict";

this.EXPORTED_SYMBOLS = ["Home"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/SharedPreferences.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");


const PREFS_PANEL_AUTH_PREFIX = "home_panels_auth_";


const DEFAULT_WEIGHT = 100;


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

  let weight = parseInt(options.weight, 10);
  this.weight = weight > 0 ? weight : DEFAULT_WEIGHT;
}



let HomeBannerMessageHandlers;

let HomeBanner = (function () {
  
  let _pendingRequest = false;

  
  HomeBannerMessageHandlers = {
    "HomeBanner:Get": function handleBannerGet(data) {
      if (Object.keys(_messages).length > 0) {
        _sendBannerData();
      } else {
        _pendingRequest = true;
      }
    }
  };

  
  let _messages = {};

  
  
  
  let _sendBannerData = function() {
    let totalWeight = 0;
    for (let key in _messages) {
      let message = _messages[key];
      totalWeight += message.weight;
      message.totalWeight = totalWeight;
    }

    let threshold = Math.random() * totalWeight;
    for (let key in _messages) {
      let message = _messages[key];
      if (threshold < message.totalWeight) {
        Messaging.sendRequest({
          type: "HomeBanner:Data",
          id: message.id,
          text: message.text,
          iconURI: message.iconURI
        });
        return;
      }
    }
  };

  let _handleShown = function(id) {
    let message = _messages[id];
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
        case "HomeBanner:Shown":
          _handleShown(data);
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

      
      
      if (Object.keys(_messages).length == 1) {
        Services.obs.addObserver(this, "HomeBanner:Shown", false);
        Services.obs.addObserver(this, "HomeBanner:Click", false);
        Services.obs.addObserver(this, "HomeBanner:Dismiss", false);

        
        if (_pendingRequest) {
          _pendingRequest = false;
          _sendBannerData();
        }
      }

      return message.id;
    },

    




    remove: function(id) {
      if (!(id in _messages)) {
        throw "Home.banner: Can't remove message that doesn't exist: id = " + id;
      }

      delete _messages[id];

      
      if (Object.keys(_messages).length == 0) {
        Services.obs.removeObserver(this, "HomeBanner:Shown");
        Services.obs.removeObserver(this, "HomeBanner:Click");
        Services.obs.removeObserver(this, "HomeBanner:Dismiss");
      }
    }
  });
})();



let HomePanelsMessageHandlers;

let HomePanels = (function () {
  
  HomePanelsMessageHandlers = {

    "HomePanels:Get": function handlePanelsGet(data) {
      data = JSON.parse(data);

      let requestId = data.requestId;
      let ids = data.ids || null;

      let panels = [];
      for (let id in _registeredPanels) {
        
        if (ids == null || ids.indexOf(id) >= 0) {
          try {
            panels.push(_generatePanel(id));
          } catch(e) {
            Cu.reportError("Home.panels: Invalid options, panel.id = " + id + ": " + e);
          }
        }
      }

      Messaging.sendRequest({
        type: "HomePanels:Data",
        panels: panels,
        requestId: requestId
      });
    },

    "HomePanels:Authenticate": function handlePanelsAuthenticate(id) {
      
      let options = _registeredPanels[id]();
      if (!options.auth) {
        throw "Home.panels: Invalid auth for panel.id = " + id;
      }
      if (!options.auth.authenticate || typeof options.auth.authenticate !== "function") {
        throw "Home.panels: Invalid auth authenticate function: panel.id = " + this.id;
      }
      options.auth.authenticate();
    },

    "HomePanels:RefreshView": function handlePanelsRefreshView(data) {
      data = JSON.parse(data);

      let options = _registeredPanels[data.panelId]();
      let view = options.views[data.viewIndex];

      if (!view) {
        throw "Home.panels: Invalid view for panel.id = " + data.panelId
            + ", view.index = " + data.viewIndex;
      }

      if (!view.onrefresh || typeof view.onrefresh !== "function") {
        throw "Home.panels: Invalid onrefresh for panel.id = " + data.panelId
            + ", view.index = " + data.viewIndex;
      }

      view.onrefresh();
    },

    "HomePanels:Installed": function handlePanelsInstalled(id) {
      _assertPanelExists(id);

      let options = _registeredPanels[id]();
      if (!options.oninstall) {
        return;
      }
      if (typeof options.oninstall !== "function") {
        throw "Home.panels: Invalid oninstall function: panel.id = " + this.id;
      }
      options.oninstall();
    },

    "HomePanels:Uninstalled": function handlePanelsUninstalled(id) {
      _assertPanelExists(id);

      let options = _registeredPanels[id]();
      if (!options.onuninstall) {
        return;
      }
      if (typeof options.onuninstall !== "function") {
        throw "Home.panels: Invalid onuninstall function: panel.id = " + this.id;
      }
      options.onuninstall();
    }
  };

  
  
  
  
  
  
  let _registeredPanels = {};

  
  let Layout = Object.freeze({
    FRAME: "frame"
  });

  
  let View = Object.freeze({
    LIST: "list",
    GRID: "grid"
  });

  
  let Item = Object.freeze({
    ARTICLE: "article",
    IMAGE: "image"
  });

  
  let ItemHandler = Object.freeze({
    BROWSER: "browser",
    INTENT: "intent"
  });

  function Panel(id, options) {
    this.id = id;
    this.title = options.title;
    this.layout = options.layout;
    this.views = options.views;

    if (!this.id || !this.title) {
      throw "Home.panels: Can't create a home panel without an id and title!";
    }

    if (!this.layout) {
      
      this.layout = Layout.FRAME;
    } else if (!_valueExists(Layout, this.layout)) {
      throw "Home.panels: Invalid layout for panel: panel.id = " + this.id + ", panel.layout =" + this.layout;
    }

    for (let view of this.views) {
      if (!_valueExists(View, view.type)) {
        throw "Home.panels: Invalid view type: panel.id = " + this.id + ", view.type = " + view.type;
      }

      if (!view.itemType) {
        if (view.type == View.LIST) {
          
          view.itemType = Item.ARTICLE;
        } else if (view.type == View.GRID) {
          
          view.itemType = Item.IMAGE;
        }
      } else if (!_valueExists(Item, view.itemType)) {
        throw "Home.panels: Invalid item type: panel.id = " + this.id + ", view.itemType = " + view.itemType;
      }

      if (!view.itemHandler) {
        
        view.itemHandler = ItemHandler.BROWSER;
      } else if (!_valueExists(ItemHandler, view.itemHandler)) {
        throw "Home.panels: Invalid item handler: panel.id = " + this.id + ", view.itemHandler = " + view.itemHandler;
      }

      if (!view.dataset) {
        throw "Home.panels: No dataset provided for view: panel.id = " + this.id + ", view.type = " + view.type;
      }

      if (view.onrefresh) {
        view.refreshEnabled = true;
      }
    }

    if (options.auth) {
      if (!options.auth.messageText) {
        throw "Home.panels: Invalid auth messageText: panel.id = " + this.id;
      }
      if (!options.auth.buttonText) {
        throw "Home.panels: Invalid auth buttonText: panel.id = " + this.id;
      }

      this.authConfig = {
        messageText: options.auth.messageText,
        buttonText: options.auth.buttonText
      };

      
      if (options.auth.imageUrl) {
        this.authConfig.imageUrl = options.auth.imageUrl;
      }
    }
  }

  let _generatePanel = function(id) {
    let options = _registeredPanels[id]();
    return new Panel(id, options);
  };

  
  let _valueExists = function(obj, value) {
    for (let key in obj) {
      if (obj[key] == value) {
        return true;
      }
    }
    return false;
  };

  let _assertPanelExists = function(id) {
    if (!(id in _registeredPanels)) {
      throw "Home.panels: Panel doesn't exist: id = " + id;
    }
  };

  return Object.freeze({
    Layout: Layout,
    View: View,
    Item: Item,
    ItemHandler: ItemHandler,

    register: function(id, optionsCallback) {
      
      if (id in _registeredPanels) {
        throw "Home.panels: Panel already exists: id = " + id;
      }

      if (!optionsCallback || typeof optionsCallback !== "function") {
        throw "Home.panels: Panel callback must be a function: id = " + id;
      }

      _registeredPanels[id] = optionsCallback;
    },

    unregister: function(id) {
      _assertPanelExists(id);

      delete _registeredPanels[id];
    },

    install: function(id) {
      _assertPanelExists(id);

      Messaging.sendRequest({
        type: "HomePanels:Install",
        panel: _generatePanel(id)
      });
    },

    uninstall: function(id) {
      _assertPanelExists(id);

      Messaging.sendRequest({
        type: "HomePanels:Uninstall",
        id: id
      });
    },

    update: function(id) {
      _assertPanelExists(id);

      Messaging.sendRequest({
        type: "HomePanels:Update",
        panel: _generatePanel(id)
      });
    },

    setAuthenticated: function(id, isAuthenticated) {
      _assertPanelExists(id);

      let authKey = PREFS_PANEL_AUTH_PREFIX + id;
      let sharedPrefs = SharedPreferences.forProfile();
      sharedPrefs.setBoolPref(authKey, isAuthenticated);
    }
  });
})();


this.Home = Object.freeze({
  banner: HomeBanner,
  panels: HomePanels,

  
  observe: function(subject, topic, data) {
    if (topic in HomeBannerMessageHandlers) {
      HomeBannerMessageHandlers[topic](data);
    } else if (topic in HomePanelsMessageHandlers) {
      HomePanelsMessageHandlers[topic](data);
    } else {
      Cu.reportError("Home.observe: message handler not found for topic: " + topic);
    }
  }
});
