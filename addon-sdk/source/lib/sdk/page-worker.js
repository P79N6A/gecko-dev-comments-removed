




"use strict";

module.metadata = {
  "stability": "stable"
};

const { Symbiont } = require("./content/symbiont");
const { Trait } = require("./deprecated/traits");

const Page = Trait.compose(
  Symbiont.resolve({
    constructor: '_initSymbiont'
  }),
  {
    _frame: Trait.required,
    _initFrame: Trait.required,
    postMessage: Symbiont.required,
    on: Symbiont.required,
    destroy: Symbiont.required,

    constructor: function Page(options) {
      options = options || {};

      this.contentURL = 'contentURL' in options ? options.contentURL
        : 'about:blank';
      if ('contentScriptWhen' in options)
        this.contentScriptWhen = options.contentScriptWhen;
      if ('contentScriptFile' in options)
        this.contentScriptFile = options.contentScriptFile;
      if ('contentScriptOptions' in options)
        this.contentScriptOptions = options.contentScriptOptions;
      if ('contentScript' in options)
        this.contentScript = options.contentScript;
      if ('allow' in options)
        this.allow = options.allow;
      if ('onError' in options)
        this.on('error', options.onError);
      if ('onMessage' in options)
        this.on('message', options.onMessage);

      this.on('propertyChange', this._onChange.bind(this));

      this._initSymbiont();
    },
    
    _onChange: function _onChange(e) {
      if ('contentURL' in e && this._frame) {
        
        
        this._workerCleanup();
        this._initFrame(this._frame);
      }
    }
  }
);
exports.Page = function(options) Page(options);
exports.Page.prototype = Page.prototype;
