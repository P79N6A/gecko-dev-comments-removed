


'use strict';

module.metadata = {
  "stability": "unstable"
};

const { Cc, Ci } = require('chrome'),
      { setTimeout } = require('../timers'),
      { Trait } = require('../deprecated/traits'),
      { openDialog } = require('../window/utils'),

      ON_LOAD = 'load',
      ON_UNLOAD = 'unload',
      STATE_LOADED = 'complete';






const WindowLoader = Trait.compose({
  






  _onLoad: Trait.required,
  _tabOptions: Trait.required,
  





  _onUnload: Trait.required,
  _load: function _load() {
    if (this.__window)
      return;

    this._window = openDialog({
      private: this._isPrivate,
      args: this._tabOptions.map(function(options) options.url).join("|")
    });
  },
  




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
  __unloadListener: null
});
exports.WindowLoader = WindowLoader;
