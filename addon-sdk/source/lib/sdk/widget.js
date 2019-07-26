





"use strict";



module.metadata = {
  "stability": "stable",
  "engines": {
    "Firefox": "*"
  }
};


const CONTENT_TYPE_URI    = 1;
const CONTENT_TYPE_HTML   = 2;
const CONTENT_TYPE_IMAGE  = 3;

const ERR_CONTENT = "No content or contentURL property found. Widgets must "
                         + "have one or the other.",
      ERR_LABEL = "The widget must have a non-empty label property.",
      ERR_ID = "You have to specify a unique value for the id property of " +
               "your widget in order for the application to remember its " +
               "position.",
      ERR_DESTROYED = "The widget has been destroyed and can no longer be used.";


const EVENTS = {
  "click": "click",
  "mouseover": "mouseover",
  "mouseout": "mouseout",
};

const { validateOptions } = require("./deprecated/api-utils");
const panels = require("./panel");
const { EventEmitter, EventEmitterTrait } = require("./deprecated/events");
const { Trait } = require("./deprecated/traits");
const LightTrait = require('./deprecated/light-traits').Trait;
const { Loader, Symbiont } = require("./content/content");
const { Cortex } = require('./deprecated/cortex');
const windowsAPI = require("./windows");
const { WindowTracker } = require("./deprecated/window-utils");
const { isBrowser } = require("./window/utils");
const { setTimeout } = require("./timers");
const unload = require("./system/unload");
const { uuid } = require("./util/uuid");
const { getNodeView } = require("./view/core");


const valid = {
  number: { is: ["null", "undefined", "number"] },
  string: { is: ["null", "undefined", "string"] },
  id: {
    is: ["string"],
    ok: function (v) v.length > 0,
    msg: ERR_ID,
    readonly: true
  },
  label: {
    is: ["string"],
    ok: function (v) v.length > 0,
    msg: ERR_LABEL
  },
  panel: {
    is: ["null", "undefined", "object"],
    ok: function(v) !v || v instanceof panels.Panel
  },
  width: {
    is: ["null", "undefined", "number"],
    map: function (v) {
      if (null === v || undefined === v) v = 16;
      return v;
    },
    defaultValue: 16
  },
  allow: {
    is: ["null", "undefined", "object"],
    map: function (v) {
      if (!v) v = { script: true };
      return v;
    },
    get defaultValue() ({ script: true })
  },
};


let widgetAttributes = {
  label: valid.label,
  id: valid.id,
  tooltip: valid.string,
  width: valid.width,
  content: valid.string,
  panel: valid.panel,
  allow: valid.allow
};



let loaderAttributes = require("./content/loader").validationAttributes;
for (let i in loaderAttributes)
  widgetAttributes[i] = loaderAttributes[i];

widgetAttributes.contentURL.optional = true;


const WIDGET_EVENTS = [
  "click",
  "mouseover",
  "mouseout",
  "error",
  "message",
  "attach"
];


let model = {

  
  _validate: function _validate(name, suspect, validation) {
    let $1 = {};
    $1[name] = suspect;
    let $2 = {};
    $2[name] = validation;
    return validateOptions($1, $2)[name];
  },

  














  setAttributes: function setAttributes(object, attrs, values) {
    let properties = {};
    for (let name in attrs) {
      let value = values[name];
      let req = attrs[name];

      
      if ((typeof value == "undefined" || value == null) && req.defaultValue)
        value = req.defaultValue;

      
      if (!req.optional || typeof value != "undefined")
        value = model._validate(name, value, req);

      
      let property = null;
      if (req.readonly) {
        property = {
          value: value,
          writable: false,
          enumerable: true,
          configurable: false
        };
      }
      else {
        property = model._createWritableProperty(name, value);
      }

      properties[name] = property;
    }
    Object.defineProperties(object, properties);
  },

  
  _createWritableProperty: function _createWritableProperty(name, value) {
    return {
      get: function () {
        return value;
      },
      set: function (newValue) {
        value = newValue;
        
        
        this._emit("change", name, value);
      },
      enumerable: true,
      configurable: false
    };
  },

  











  setEvents: function setEvents(object, events, listeners) {
    for (let i = 0, l = events.length; i < l; i++) {
      let name = events[i];
      let onName = "on" + name[0].toUpperCase() + name.substr(1);
      if (!listeners[onName])
        continue;
      object.on(name, listeners[onName].bind(object));
    }
  }

};









const WidgetTrait = LightTrait.compose(EventEmitterTrait, LightTrait({

  _initWidget: function _initWidget(options) {
    model.setAttributes(this, widgetAttributes, options);

    browserManager.validate(this);

    
    if (!(this.content || this.contentURL))
      throw new Error(ERR_CONTENT);

    this._views = [];

    
    if (!this.tooltip)
      this.tooltip = this.label;

    model.setEvents(this, WIDGET_EVENTS, options);

    this.on('change', this._onChange.bind(this));

    let self = this;
    this._port = EventEmitterTrait.create({
      emit: function () {
        let args = arguments;
        self._views.forEach(function(v) v.port.emit.apply(v.port, args));
      }
    });
    
    this._port._public = Cortex(this._port);

    
    
    browserManager.addItem(this);
  },

  _onChange: function _onChange(name, value) {
    
    if (name == 'tooltip' && !value) {
      
      
      this.tooltip = this.label;
      return;
    }

    
    if (['width', 'tooltip', 'content', 'contentURL'].indexOf(name) != -1) {
      this._views.forEach(function(v) v[name] = value);
    }
  },

  _onEvent: function _onEvent(type, eventData) {
    this._emit(type, eventData);
  },

  _createView: function _createView() {
    
    let view = WidgetView(this);

    
    this._views.push(view);

    return view;
  },

  
  _onViewDestroyed: function _onViewDestroyed(view) {
    let idx = this._views.indexOf(view);
    this._views.splice(idx, 1);
  },

  




  _onWindowClosed: function _onWindowClosed(window) {
    for each (let view in this._views) {
      if (view._isInChromeWindow(window)) {
        view.destroy();
        break;
      }
    }
  },

  




  getView: function getView(window) {
    for each (let view in this._views) {
      if (view._isInWindow(window)) {
        return view._public;
      }
    }
    return null;
  },

  get port() this._port._public,
  set port(v) {}, 
                  
  _port: null,

  postMessage: function postMessage(message) {
    this._views.forEach(function(v) v.postMessage(message));
  },

  destroy: function destroy() {
    if (this.panel)
      this.panel.destroy();

    
    
    
    for (let i = this._views.length - 1; i >= 0; i--)
      this._views[i].destroy();

    
    
    browserManager.removeItem(this);
  }

}));


const Widget = function Widget(options) {
  let w = WidgetTrait.create(Widget.prototype);
  w._initWidget(options);

  
  let _public = Cortex(w);
  unload.ensure(_public, "destroy");
  return _public;
}
exports.Widget = Widget;








const WidgetViewTrait = LightTrait.compose(EventEmitterTrait, LightTrait({

  
  
  _chrome: null,

  
  
  _public: null,

  _initWidgetView: function WidgetView__initWidgetView(baseWidget) {
    this._baseWidget = baseWidget;

    model.setAttributes(this, widgetAttributes, baseWidget);

    this.on('change', this._onChange.bind(this));

    let self = this;
    this._port = EventEmitterTrait.create({
      emit: function () {
        if (!self._chrome)
          throw new Error(ERR_DESTROYED);
        self._chrome.update(self._baseWidget, "emit", arguments);
      }
    });
    
    this._port._public = Cortex(this._port);

    this._public = Cortex(this);
  },

  
  
  _onWorkerReady: function () {
    
    this._baseWidget._emit("attach", this._public);
  },

  _onChange: function WidgetView__onChange(name, value) {
    if (name == 'tooltip' && !value) {
      this.tooltip = this.label;
      return;
    }

    
    if (['width', 'tooltip', 'content', 'contentURL'].indexOf(name) != -1) {
      this._chrome.update(this._baseWidget, name, value);
    }
  },

  _onEvent: function WidgetView__onEvent(type, eventData, domNode) {
    
    this._emit(type, eventData);

    
    if ("click" == type || type.indexOf("mouse") == 0)
      this._baseWidget._onEvent(type, this._public);
    else
      this._baseWidget._onEvent(type, eventData);

    
    
    if ("click" == type && !this._listeners("click").length && this.panel)
      
      
      
      this.panel.show(getNodeView.implement({}, function() domNode));
  },

  _isInWindow: function WidgetView__isInWindow(window) {
    return windowsAPI.BrowserWindow({
      window: this._chrome.window
    }) == window;
  },

  _isInChromeWindow: function WidgetView__isInChromeWindow(window) {
    return this._chrome.window == window;
  },

  _onPortEvent: function WidgetView__onPortEvent(args) {
    let port = this._port;
    port._emit.apply(port, args);
    let basePort = this._baseWidget._port;
    basePort._emit.apply(basePort, args);
  },

  get port() this._port._public,
  set port(v) {}, 
                  
  _port: null,

  postMessage: function WidgetView_postMessage(message) {
    if (!this._chrome)
      throw new Error(ERR_DESTROYED);
    this._chrome.update(this._baseWidget, "postMessage", message);
  },

  destroy: function WidgetView_destroy() {
    this._chrome.destroy();
    delete this._chrome;
    this._baseWidget._onViewDestroyed(this);
    this._emit("detach");
  }

}));


const WidgetView = function WidgetView(baseWidget) {
  let w = WidgetViewTrait.create(WidgetView.prototype);
  w._initWidgetView(baseWidget);
  return w;
}








let browserManager = {
  items: [],
  windows: [],

  
  
  
  init: function () {
    let windowTracker = new WindowTracker(this);
    unload.ensure(windowTracker);
  },

  
  onTrack: function browserManager_onTrack(window) {
    if (isBrowser(window)) {
      let win = new BrowserWindow(window);
      win.addItems(this.items);
      this.windows.push(win);
    }
  },

  
  
  
  
  
  
  onUntrack: function browserManager_onUntrack(window) {
    if (isBrowser(window)) {
      this.items.forEach(function(i) i._onWindowClosed(window));
      for (let i = 0; i < this.windows.length; i++) {
        if (this.windows[i].window == window) {
          this.windows.splice(i, 1)[0];
          return;
        }
      }

    }
  },

  
  
  validate : function (item) {
    let idx = this.items.indexOf(item);
    if (idx > -1)
      throw new Error("The widget " + item + " has already been added.");
    if (item.id) {
      let sameId = this.items.filter(function(i) i.id == item.id);
      if (sameId.length > 0)
        throw new Error("This widget ID is already used: " + item.id);
    } else {
      item.id = this.items.length;
    }
  },

  
  
  addItem: function browserManager_addItem(item) {
    this.items.push(item);
    this.windows.forEach(function (w) w.addItems([item]));
  },

  
  
  removeItem: function browserManager_removeItem(item) {
    let idx = this.items.indexOf(item);
    if (idx > -1)
      this.items.splice(idx, 1);
  }
};








function BrowserWindow(window) {
  this.window = window;
  this.doc = window.document;
}

BrowserWindow.prototype = {
  
  addItems: function BW_addItems(items) {
    items.forEach(this._addItemToWindow, this);
  },

  _addItemToWindow: function BW__addItemToWindow(baseWidget) {
    
    let widget = baseWidget._createView();

    
    let item = new WidgetChrome({
      widget: widget,
      doc: this.doc,
      window: this.window
    });

    widget._chrome = item;

    this._insertNodeInToolbar(item.node);

    
    
    item.fill();
  },

  _insertNodeInToolbar: function BW__insertNodeInToolbar(node) {
    
    let toolbox = this.doc.getElementById("navigator-toolbox");
    let palette = toolbox.palette;
    palette.appendChild(node);

    
    let container = null;
    let toolbars = this.doc.getElementsByTagName("toolbar");
    let id = node.getAttribute("id");
    for (let i = 0, l = toolbars.length; i < l; i++) {
      let toolbar = toolbars[i];
      if (toolbar.getAttribute("currentset").indexOf(id) == -1)
        continue;
      container = toolbar;
    }

    
    
    
    if (!container) {
      container = this.doc.getElementById("addon-bar");
      
      
      
      
      if (container.collapsed)
        this.window.toggleAddonBar();
    }

    
    
    let nextNode = null;
    let currentSet = container.getAttribute("currentset");
    let ids = (currentSet == "__empty") ? [] : currentSet.split(",");
    let idx = ids.indexOf(id);
    if (idx != -1) {
      for (let i = idx; i < ids.length; i++) {
        nextNode = this.doc.getElementById(ids[i]);
        if (nextNode)
          break;
      }
    }

    
    container.insertItem(id, nextNode, null, false);

    
    
    
    
    if (ids.indexOf(id) == -1) {
      container.setAttribute("currentset", container.currentSet);
      
      this.window.document.persist(container.id, "currentset");
    }
  }
}








function WidgetChrome(options) {
  this.window = options.window;
  this._doc = options.doc;
  this._widget = options.widget;
  this._symbiont = null; 
  this.node = null; 

  this._createNode();
}


WidgetChrome.prototype.update = function WC_update(updatedItem, property, value) {
  switch(property) {
    case "contentURL":
    case "content":
      this.setContent();
      break;
    case "width":
      this.node.style.minWidth = value + "px";
      this.node.querySelector("iframe").style.width = value + "px";
      break;
    case "tooltip":
      this.node.setAttribute("tooltiptext", value);
      break;
    case "postMessage":
      this._symbiont.postMessage(value);
      break;
    case "emit":
      let port = this._symbiont.port;
      port.emit.apply(port, value);
      break;
  }
}


WidgetChrome.prototype._createNode = function WC__createNode() {
  
  let node = this._doc.createElement("toolbaritem");
  let guid = String(uuid());

  
  let jetpackID = "testID";
  try {
    jetpackID = require("./self").id;
  } catch(e) {}

  
  let id = "widget:" + jetpackID + "-" + this._widget.id;
  node.setAttribute("id", id);
  node.setAttribute("label", this._widget.label);
  node.setAttribute("tooltiptext", this._widget.tooltip);
  node.setAttribute("align", "center");
  
  node.setAttribute("context", "");

  
  
  
  node.setAttribute("style", [
      "overflow: hidden; margin: 1px 2px 1px 2px; padding: 0px;",
      "min-height: 16px;",
  ].join(""));

  node.style.minWidth = this._widget.width + "px";

  this.node = node;
}


WidgetChrome.prototype.fill = function WC_fill() {
  
  var iframe = this._doc.createElement("iframe");
  iframe.setAttribute("type", "content");
  iframe.setAttribute("transparent", "transparent");
  iframe.style.overflow = "hidden";
  iframe.style.height = "16px";
  iframe.style.maxHeight = "16px";
  iframe.style.width = this._widget.width + "px";
  iframe.setAttribute("flex", "1");
  iframe.style.border = "none";
  iframe.style.padding = "0px";

  
  
  this.node.appendChild(iframe);

  
  this.addEventHandlers();

  
  this.setContent();
}


WidgetChrome.prototype.getContentType = function WC_getContentType() {
  if (this._widget.content)
    return CONTENT_TYPE_HTML;
  return (this._widget.contentURL && /\.(jpg|gif|png|ico|svg)$/i.test(this._widget.contentURL))
    ? CONTENT_TYPE_IMAGE : CONTENT_TYPE_URI;
}


WidgetChrome.prototype.setContent = function WC_setContent() {
  let type = this.getContentType();
  let contentURL = null;

  switch (type) {
    case CONTENT_TYPE_HTML:
      contentURL = "data:text/html;charset=utf-8," + encodeURIComponent(this._widget.content);
      break;
    case CONTENT_TYPE_URI:
      contentURL = this._widget.contentURL;
      break;
    case CONTENT_TYPE_IMAGE:
      let imageURL = this._widget.contentURL;
      contentURL = "data:text/html;charset=utf-8,<html><body><img src='" +
                   encodeURI(imageURL) + "'></body></html>";
      break;
    default:
      throw new Error("The widget's type cannot be determined.");
  }

  let iframe = this.node.firstElementChild;

  let self = this;
  
  if (this._symbiont)
    this._symbiont.destroy();

  this._symbiont = Trait.compose(Symbiont.resolve({
    _onContentScriptEvent: "_onContentScriptEvent-not-used",
    _onInit: "_initSymbiont"
  }), {
    
    
    _onInit: function () {
      this._initSymbiont();
      self._widget._onWorkerReady();
    },
    _onContentScriptEvent: function () {
      
      self._widget._onPortEvent(arguments);
    }
  })({
    frame: iframe,
    contentURL: contentURL,
    contentScriptFile: this._widget.contentScriptFile,
    contentScript: this._widget.contentScript,
    contentScriptWhen: this._widget.contentScriptWhen,
    contentScriptOptions: this._widget.contentScriptOptions,
    allow: this._widget.allow,
    onMessage: function(message) {
      setTimeout(function() {
        self._widget._onEvent("message", message);
      }, 0);
    }
  });
}


WidgetChrome._isImageDoc = function WC__isImageDoc(doc) {
  return  doc.body.childNodes.length == 1 &&
         doc.body.firstElementChild &&
         doc.body.firstElementChild.tagName == "IMG";
}


WidgetChrome.prototype.addEventHandlers = function WC_addEventHandlers() {
  let contentType = this.getContentType();

  let self = this;
  let listener = function(e) {
    
    if (e.target == self.node.firstElementChild)
      return;

    
    
    if (e.type == "click" && e.button !== 0)
      return;

    
    setTimeout(function() {
      self._widget._onEvent(EVENTS[e.type], null, self.node);
    }, 0);
  };

  this.eventListeners = {};
  let iframe = this.node.firstElementChild;
  for (let type in EVENTS) {
    iframe.addEventListener(type, listener, true, true);

    
    this.eventListeners[type] = listener;
  }

  
  
  function loadListener(e) {
    let containerStyle = self.window.getComputedStyle(self.node.parentNode);
    
    if (e.target == iframe)
      return;
    
    if (e.type == "load" && e.target.location == "about:blank")
      return;

    
    if (!self._symbiont)
      self.setContent();

    let doc = e.target;

    if (contentType == CONTENT_TYPE_IMAGE || WidgetChrome._isImageDoc(doc)) {
      
      
      doc.body.firstElementChild.style.width = self._widget.width + "px";
      doc.body.firstElementChild.style.height = "16px";
    }

    
    doc.body.style.color = containerStyle.color;
    doc.body.style.fontFamily = containerStyle.fontFamily;
    doc.body.style.fontSize = containerStyle.fontSize;
    doc.body.style.fontWeight = containerStyle.fontWeight;
    doc.body.style.textShadow = containerStyle.textShadow;
    
    doc.body.style.margin = "0";
  }

  iframe.addEventListener("load", loadListener, true);
  this.eventListeners["load"] = loadListener;

  
  
  function unloadListener(e) {
    if (e.target.location == "about:blank")
      return;
    self._symbiont.destroy();
    self._symbiont = null;
    
    
    try {
      self.setContent();
    } catch(e) {}

  }

  iframe.addEventListener("unload", unloadListener, true);
  this.eventListeners["unload"] = unloadListener;
}


WidgetChrome.prototype.destroy = function WC_destroy(removedItems) {
  
  for (let type in this.eventListeners) {
    let listener = this.eventListeners[type];
    this.node.firstElementChild.removeEventListener(type, listener, true);
  }
  
  this.node.parentNode.removeChild(this.node);
  
  this._symbiont.destroy();
  
  this.eventListeners = null;
  this._widget = null;
  this._symbiont = null;
}




browserManager.init();
