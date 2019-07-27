


'use strict';

const { Cc, Ci, Cr } = require('chrome'),
      { Trait } = require('../deprecated/traits'),
      { List } = require('../deprecated/list'),
      { EventEmitter } = require('../deprecated/events'),
      { WindowTabs, WindowTabTracker } = require('./tabs-firefox'),
      { WindowDom } = require('./dom'),
      { isBrowser, getWindowDocShell, isFocused,
        windows: windowIterator, isWindowPrivate } = require('../window/utils'),
      { Options } = require('../tabs/common'),
      apiUtils = require('../deprecated/api-utils'),
      unload = require('../system/unload'),
      windowUtils = require('../deprecated/window-utils'),
      { WindowTrackerTrait } = windowUtils,
      { ns } = require('../core/namespace'),
      { observer: windowObserver } = require('./observer');
const { windowNS } = require('../window/namespace');
const { isPrivateBrowsingSupported } = require('../self');
const { ignoreWindow, isPrivate } = require('sdk/private-browsing/utils');
const { viewFor } = require('../view/core');
const { openDialog } = require('../window/utils');
const ON_LOAD = 'load',
      ON_UNLOAD = 'unload',
      STATE_LOADED = 'complete';




const BrowserWindowTrait = Trait.compose(
  EventEmitter,
  WindowDom.resolve({ close: '_close' }),
  WindowTabs,
  WindowTabTracker,
  
  Trait.compose({
    _emit: Trait.required,
    _close: Trait.required,
    




    get _window() this.__window,
    set _window(window) {
      let _window = this.__window;
      if (!window) window = null;

      if (window !== _window) {
        if (_window) {
          if (this.__unloadListener)
            _window.removeEventListener(ON_UNLOAD, this.__unloadListener, false);

          if (this.__loadListener)
            _window.removeEventListener(ON_LOAD, this.__loadListener, false);
        }

        if (window) {
          window.addEventListener(
            ON_UNLOAD,
            this.__unloadListener ||
              (this.__unloadListener = this._unloadListener.bind(this))
            ,
            false
          );

          this.__window = window;

          
          if (STATE_LOADED != window.document.readyState) {
            window.addEventListener(
              ON_LOAD,
              this.__loadListener ||
                (this.__loadListener = this._loadListener.bind(this))
              ,
              false
            );
          }
          else { 
            this._onLoad(window)
          }
        }
        else {
          this.__window = null;
        }
      }
    },
    __window: null,
    




    _loadListener: function _loadListener(event) {
      let window = this._window;
      if (!event.target || event.target.defaultView != window) return;
      window.removeEventListener(ON_LOAD, this.__loadListener, false);
      this._onLoad(window);
    },
    __loadListener: null,
    




    _unloadListener: function _unloadListener(event) {
      let window = this._window;
      if (!event.target
        || event.target.defaultView != window
        || STATE_LOADED != window.document.readyState
      ) return;
      window.removeEventListener(ON_UNLOAD, this.__unloadListener, false);
      this._onUnload(window);
    },
    __unloadListener: null,
    _load: function _load() {
      if (this.__window)
        return;

      this._window = openDialog({
        private: this._isPrivate,
        args: this._tabOptions.map(function(options) options.url).join("|")
      });
    },
    



    constructor: function BrowserWindow(options) {
      
      
      windows.push(this);

      
      this.on('error', console.exception.bind(console));

      if ('onOpen' in options)
        this.on('open', options.onOpen);
      if ('onClose' in options)
        this.on('close', options.onClose);
      if ('onActivate' in options)
        this.on('activate', options.onActivate);
      if ('onDeactivate' in options)
        this.on('deactivate', options.onDeactivate);
      if ('window' in options)
        this._window = options.window;

      if ('tabs' in options) {
        this._tabOptions = Array.isArray(options.tabs) ?
                           options.tabs.map(Options) :
                           [ Options(options.tabs) ];
      }
      else if ('url' in options) {
        this._tabOptions = [ Options(options.url) ];
      }
      for (let tab of this._tabOptions) {
        tab.inNewWindow = true;
      }

      this._isPrivate = isPrivateBrowsingSupported && !!options.isPrivate;

      this._load();

      windowNS(this._public).window = this._window;
      viewFor.implement(this._public, (w) => windowNS(w).window);

      return this;
    },
    destroy: function () this._onUnload(),
    _tabOptions: [],
    _onLoad: function() {
      try {
        this._initWindowTabTracker();
        this._loaded = true;
      }
      catch(e) {
        this._emit('error', e);
      }

      this._emitOnObject(browserWindows, 'open', this._public);
    },
    _onUnload: function() {
      if (!this._window)
        return;
      if (this._loaded)
        this._destroyWindowTabTracker();

      this._emitOnObject(browserWindows, 'close', this._public);
      this._window = null;
      windowNS(this._public).window = null;
      
      windows.splice(windows.indexOf(this), 1);
      this._removeAllListeners();
    },
    close: function close(callback) {
      
      if (callback) this.on('close', callback);
      return this._close();
    }
  })
);





function getRegisteredWindow(chromeWindow) {
  for (let window of windows) {
    if (chromeWindow === window._window)
      return window;
  }

  return null;
}










function BrowserWindow(options) {
  let window = null;

  if ("window" in options)
    window = getRegisteredWindow(options.window);

  return (window || BrowserWindowTrait(options))._public;
}

BrowserWindow.prototype = BrowserWindowTrait.prototype;
exports.BrowserWindow = BrowserWindow;

const windows = [];

const browser = ns();

function onWindowActivation (chromeWindow, event) {
  if (!isBrowser(chromeWindow)) return; 

  let window = getRegisteredWindow(chromeWindow);

  if (window)
    window._emit(event.type, window._public);
  else
    window = BrowserWindowTrait({ window: chromeWindow });

  browser(browserWindows).internals._emit(event.type, window._public);
}

windowObserver.on("activate", onWindowActivation);
windowObserver.on("deactivate", onWindowActivation);







const browserWindows = Trait.resolve({ toString: null }).compose(
  List.resolve({ constructor: '_initList' }),
  EventEmitter.resolve({ toString: null }),
  WindowTrackerTrait.resolve({ constructor: '_initTracker', toString: null }),
  Trait.compose({
    _emit: Trait.required,
    _add: Trait.required,
    _remove: Trait.required,

    

    



    constructor: function BrowserWindows() {
      browser(this._public).internals = this;

      this._trackedWindows = [];
      this._initList();
      this._initTracker();
      unload.ensure(this, "_destructor");
    },
    _destructor: function _destructor() {
      this._removeAllListeners('open');
      this._removeAllListeners('close');
      this._removeAllListeners('activate');
      this._removeAllListeners('deactivate');
      this._clear();

      delete browser(this._public).internals;
    },
    




    get activeWindow() {
      let window = windowUtils.activeBrowserWindow;
      
      if (ignoreWindow(window))
        window = windowIterator()[0];
      return window ? BrowserWindow({window: window}) : null;
    },
    open: function open(options) {
      if (typeof options === "string") {
        
        options = {
          tabs: [Options(options)],
          isPrivate: isPrivateBrowsingSupported && options.isPrivate
        };
      }
      return BrowserWindow(options);
    },

     




    _onTrack: function _onTrack(chromeWindow) {
      if (!isBrowser(chromeWindow)) return;
      let window = BrowserWindow({ window: chromeWindow });
      this._add(window);
      this._emit('open', window);
    },

    




    _onUntrack: function _onUntrack(chromeWindow) {
      if (!isBrowser(chromeWindow)) return;
      let window = BrowserWindow({ window: chromeWindow });
      this._remove(window);
      this._emit('close', window);

      
      
      
      window.destroy();
    }
  }).resolve({ toString: null })
)();

const isBrowserWindow = (x) => x instanceof BrowserWindow;
isPrivate.when(isBrowserWindow, (w) => isWindowPrivate(viewFor(w)));
isFocused.when(isBrowserWindow, (w) => isFocused(viewFor(w)));

exports.browserWindows = browserWindows;
