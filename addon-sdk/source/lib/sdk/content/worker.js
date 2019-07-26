




"use strict";

module.metadata = {
  "stability": "unstable"
};

const { Trait } = require('../deprecated/traits');
const { EventEmitter, EventEmitterTrait } = require('../deprecated/events');
const { Ci, Cu, Cc } = require('chrome');
const timer = require('../timers');
const { URL } = require('../url');
const unload = require('../system/unload');
const observers = require('../deprecated/observer-service');
const { Cortex } = require('../deprecated/cortex');
const { sandbox, evaluate, load } = require("../loader/sandbox");
const { merge } = require('../util/object');
const xulApp = require("../system/xul-app");
const { getInnerId } = require("../window/utils")
const USE_JS_PROXIES = !xulApp.versionInRange(xulApp.platformVersion,
                                              "17.0a2", "*");
const { getTabForWindow } = require('../tabs/helpers');
const { getTabForContentWindow } = require('../tabs/utils');






let prefix = module.uri.split('worker.js')[0];
const CONTENT_PROXY_URL = prefix + 'content-proxy.js';
const CONTENT_WORKER_URL = prefix + 'content-worker.js';

const JS_VERSION = '1.8';

const ERR_DESTROYED =
  "Couldn't find the worker to receive this message. " +
  "The script may not be initialized yet, or may already have been unloaded.";

const ERR_FROZEN = "The page is currently hidden and can no longer be used " +
                   "until it is visible again.";








const PRIVATE_KEY = {};


const WorkerSandbox = EventEmitter.compose({

  


  emit: function emit() {
    
    
    let array = Array.slice(arguments);
    
    
    function replacer(k, v) {
      return typeof v === "function" ? undefined : v;
    }
    
    let self = this;
    timer.setTimeout(function () {
      self._emitToContent(JSON.stringify(array, replacer));
    }, 0);
  },

  





  emitSync: function emitSync() {
    let args = Array.slice(arguments);
    
    
    
    if ("_wrap" in this)
      args = args.map(this._wrap);
    return this._emitToContent(args);
  },

  




  hasListenerFor: function hasListenerFor(name) {
    return this._hasListenerFor(name);
  },

  


  _onContentEvent: function onContentEvent(args) {
    
    let self = this;
    timer.setTimeout(function () {
      
      self._emit.apply(self, JSON.parse(args));
    }, 0);
  },

  




  constructor: function WorkerSandbox(worker) {
    this._addonWorker = worker;

    
    this.emit = this.emit.bind(this);
    this.emitSync = this.emitSync.bind(this);

    
    let window = worker._window;
    let proto = window;

    
    
    let apiSandbox = sandbox(window, { wantXrays: true });
    apiSandbox.console = console;

    
    
    if (USE_JS_PROXIES && XPCNativeWrapper.unwrap(window) !== window) {
      
      load(apiSandbox, CONTENT_PROXY_URL);
      
      proto = apiSandbox.create(window);
      
      this._wrap = apiSandbox.wrap;
    }

    
    
    let content = this._sandbox = sandbox(window, {
      sandboxPrototype: proto,
      wantXrays: true
    });
    
    
    
    let top = window.top === window ? content : content.top;
    let parent = window.parent === window ? content : content.parent;
    merge(content, {
      
      get window() content,
      get top() top,
      get parent() parent,
      
      
      
      
      
      get unsafeWindow() window.wrappedJSObject
    });

    
    
    
    load(apiSandbox, CONTENT_WORKER_URL);

    
    let options = 'contentScriptOptions' in worker ?
      JSON.stringify( worker.contentScriptOptions ) :
      undefined;

    
    
    
    
    
    
    
    
    let chromeAPI = {
      timers: {
        setTimeout: timer.setTimeout,
        setInterval: timer.setInterval,
        clearTimeout: timer.clearTimeout,
        clearInterval: timer.clearInterval,
        __exposedProps__: {
          setTimeout: 'r',
          setInterval: 'r',
          clearTimeout: 'r',
          clearInterval: 'r'
        }
      },
      __exposedProps__: {
        timers: 'r'
      }
    };
    let onEvent = this._onContentEvent.bind(this);
    
    let result = apiSandbox.ContentWorker.inject(content, chromeAPI, onEvent, options);
    this._emitToContent = result.emitToContent;
    this._hasListenerFor = result.hasListenerFor;

    
    let self = this;
    
    this.on("console", function consoleListener(kind) {
      console[kind].apply(console, Array.slice(arguments, 1));
    });

    
    this.on("message", function postMessage(data) {
      
      if (self._addonWorker)
        self._addonWorker._emit('message', data);
    });

    
    this.on("event", function portEmit(name, args) {
      
      if (self._addonWorker)
        self._addonWorker._onContentScriptEvent.apply(self._addonWorker, arguments);
    });

    
    
    
    if (apiSandbox && worker._expose_key)
      content.UNWRAP_ACCESS_KEY = apiSandbox.UNWRAP_ACCESS_KEY;

    
    
    if (worker._injectInDocument) {
      let win = window.wrappedJSObject ? window.wrappedJSObject : window;
      Object.defineProperty(win, "addon", {
          value: content.self
        }
      );
    }

    
    
    
    if (!getTabForContentWindow(window)) {
      let win = window.wrappedJSObject ? window.wrappedJSObject : window;

      
      
      
      
      
      
      let con = Cu.createObjectIn(win);

      let genPropDesc = function genPropDesc(fun) {
        return { enumerable: true, configurable: true, writable: true,
          value: console[fun] };
      }

      const properties = {
        log: genPropDesc('log'),
        info: genPropDesc('info'),
        warn: genPropDesc('warn'),
        error: genPropDesc('error'),
        debug: genPropDesc('debug'),
        trace: genPropDesc('trace'),
        dir: genPropDesc('dir'),
        group: genPropDesc('group'),
        groupCollapsed: genPropDesc('groupCollapsed'),
        groupEnd: genPropDesc('groupEnd'),
        time: genPropDesc('time'),
        timeEnd: genPropDesc('timeEnd'),
        profile: genPropDesc('profile'),
        profileEnd: genPropDesc('profileEnd'),
       __noSuchMethod__: { enumerable: true, configurable: true, writable: true,
                            value: function() {} }
      };

      Object.defineProperties(con, properties);
      Cu.makeObjectPropsNormal(con);

      win.console = con;
    };

    
    
    
    let contentScriptFile = ('contentScriptFile' in worker) ? worker.contentScriptFile
          : null,
        contentScript = ('contentScript' in worker) ? worker.contentScript : null;

    if (contentScriptFile) {
      if (Array.isArray(contentScriptFile))
        this._importScripts.apply(this, contentScriptFile);
      else
        this._importScripts(contentScriptFile);
    }
    if (contentScript) {
      this._evaluate(
        Array.isArray(contentScript) ? contentScript.join(';\n') : contentScript
      );
    }
  },
  destroy: function destroy() {
    this.emitSync("detach");
    this._sandbox = null;
    this._addonWorker = null;
    this._wrap = null;
  },

  



  _sandbox: null,

  



  _addonWorker: null,

  






  _evaluate: function(code, filename) {
    try {
      evaluate(this._sandbox, code, filename || 'javascript:' + code);
    }
    catch(e) {
      this._addonWorker._emit('error', e);
    }
  },
  








  _importScripts: function _importScripts(url) {
    let urls = Array.slice(arguments, 0);
    for each (let contentScriptFile in urls) {
      try {
        let uri = URL(contentScriptFile);
        if (uri.scheme === 'resource')
          load(this._sandbox, String(uri));
        else
          throw Error("Unsupported `contentScriptFile` url: " + String(uri));
      }
      catch(e) {
        this._addonWorker._emit('error', e);
      }
    }
  }
});






const Worker = EventEmitter.compose({
  on: Trait.required,
  _removeAllListeners: Trait.required,

  
  get _earlyEvents() {
    delete this._earlyEvents;
    this._earlyEvents = [];
    return this._earlyEvents;
  },

  











  postMessage: function (data) {
    let args = ['message'].concat(Array.slice(arguments));
    if (!this._inited) {
      this._earlyEvents.push(args);
      return;
    }
    processMessage.apply(this, args);
  },

  





  get port() {
    
    

    
    this._port = EventEmitterTrait.create({
      emit: this._emitEventToContent.bind(this)
    });

    
    
    
    
    delete this._public.port;
    this._public.port = Cortex(this._port);
    
    delete this.port;
    this.port = this._public.port;

    return this._port;
  },

  



  _port: null,

  



  _emitEventToContent: function () {
    let args = ['event'].concat(Array.slice(arguments));
    if (!this._inited) {
      this._earlyEvents.push(args);
      return;
    }
    processMessage.apply(this, args);
  },

  
  _inited: false,

  
  
  _frozen: true,

  constructor: function Worker(options) {
    options = options || {};

    if ('window' in options)
      this._window = options.window;
    if ('contentScriptFile' in options)
      this.contentScriptFile = options.contentScriptFile;
    if ('contentScriptOptions' in options)
      this.contentScriptOptions = options.contentScriptOptions;
    if ('contentScript' in options)
      this.contentScript = options.contentScript;
    if ('onError' in options)
      this.on('error', options.onError);
    if ('onMessage' in options)
      this.on('message', options.onMessage);
    if ('onDetach' in options)
      this.on('detach', options.onDetach);

    
    
    if ('exposeUnlockKey' in options && options.exposeUnlockKey === PRIVATE_KEY)
      this._expose_key = true;

    
    
    
    
    this._windowID = getInnerId(this._window);
    observers.add("inner-window-destroyed",
                  this._documentUnload = this._documentUnload.bind(this));

    
    
    this._window.addEventListener("pageshow",
                                  this._pageShow = this._pageShow.bind(this),
                                  true);
    this._window.addEventListener("pagehide",
                                  this._pageHide = this._pageHide.bind(this),
                                  true);

    unload.ensure(this._public, "destroy");

    
    
    this.port;

    
    this._contentWorker = WorkerSandbox(this);

    
    this._inited = true;
    this._frozen = false;

    
    
    this._earlyEvents.forEach((function (args) {
      processMessage.apply(this, args);
    }).bind(this));
  },

  _documentUnload: function _documentUnload(subject, topic, data) {
    let innerWinID = subject.QueryInterface(Ci.nsISupportsPRUint64).data;
    if (innerWinID != this._windowID) return false;
    this._workerCleanup();
    return true;
  },

  _pageShow: function _pageShow() {
    this._contentWorker.emitSync("pageshow");
    this._emit("pageshow");
    this._frozen = false;
  },

  _pageHide: function _pageHide() {
    this._contentWorker.emitSync("pagehide");
    this._emit("pagehide");
    this._frozen = true;
  },

  get url() {
    
    return this._window ? this._window.document.location.href : null;
  },

  get tab() {
    
    if (this._window)
      return getTabForWindow(this._window);
    return null;
  },

  



  destroy: function destroy() {
    this._workerCleanup();
    this._removeAllListeners();
  },

  



  _workerCleanup: function _workerCleanup() {
    
    
    if (this._contentWorker)
      this._contentWorker.destroy();
    this._contentWorker = null;
    if (this._window) {
      this._window.removeEventListener("pageshow", this._pageShow, true);
      this._window.removeEventListener("pagehide", this._pageHide, true);
    }
    this._window = null;
    
    
    if (this._windowID) {
      this._windowID = null;
      observers.remove("inner-window-destroyed", this._documentUnload);
      this._earlyEvents.length = 0;
      this._emit("detach");
    }
  },

  



  _onContentScriptEvent: function _onContentScriptEvent() {
    this._port._emit.apply(this._port, arguments);
  },

  



  _contentWorker: null,

  




  _window: null,

  



  _injectInDocument: false
});







function processMessage () {
  if (!this._contentWorker)
    throw new Error(ERR_DESTROYED);
  if (this._frozen)
    throw new Error(ERR_FROZEN);

  this._contentWorker.emit.apply(null, Array.slice(arguments));
}

exports.Worker = Worker;
