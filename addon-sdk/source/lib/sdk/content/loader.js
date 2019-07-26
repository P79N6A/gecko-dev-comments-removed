





"use strict";

module.metadata = {
  "stability": "unstable"
};

const { EventEmitter } = require('../deprecated/events');
const { validateOptions } = require('../deprecated/api-utils');
const { URL } = require('../url');
const file = require('../io/file');

const LOCAL_URI_SCHEMES = ['resource', 'data'];


function ensureNull(value) {
  return value == null ? null : value;
}


const valid = {
  contentURL: {
    ok: function (value) {
      try {
        URL(value);
      }
      catch(e) {
        return false;
      }
      return true;
    },
    msg: 'The `contentURL` option must be a valid URL.'
  },
  contentScriptFile: {
    is: ['undefined', 'null', 'string', 'array'],
    map: ensureNull,
    ok: function(value) {
      if (value === null)
        return true;

      value = [].concat(value);

      
      return value.every(function (item) {
        try {
          return ~LOCAL_URI_SCHEMES.indexOf(URL(item).scheme);
        }
        catch(e) {
          return false;
        }
      });

    },
    msg: 'The `contentScriptFile` option must be a local URL or an array of URLs.'
  },
  contentScript: {
    is: ['undefined', 'null', 'string', 'array'],
    map: ensureNull,
    ok: function(value) {
      return !Array.isArray(value) || value.every(
        function(item) { return typeof item === 'string' }
      );
    },
    msg: 'The `contentScript` option must be a string or an array of strings.'
  },
  contentScriptWhen: {
    is: ['string'],
    ok: function(value) { return ~['start', 'ready', 'end'].indexOf(value) },
    map: function(value) {
      return value || 'end';
    },
    msg: 'The `contentScriptWhen` option must be either "start", "ready" or "end".'
  },
  contentScriptOptions: {
    ok: function(value) {
      if ( value === undefined ) { return true; }
      try { JSON.parse( JSON.stringify( value ) ); } catch(e) { return false; }
      return true;
    },
    map: function(value) 'undefined' === getTypeOf(value) ? null : value,
    msg: 'The contentScriptOptions should be a jsonable value.'
  }
};
exports.validationAttributes = valid;








function validate(suspect, validation) validateOptions(
  { $: suspect },
  { $: validation }
).$

function Allow(script) ({
  get script() script,
  set script(value) script = !!value
})







const Loader = EventEmitter.compose({
  





  get allow() this._allow || (this._allow = Allow(true)),
  set allow(value) this.allow.script = value && value.script,
  _allow: null,
  



  get contentURL() this._contentURL,
  set contentURL(value) {
    value = validate(value, valid.contentURL);
    if (this._contentURL != value) {
      this._emit('propertyChange', {
        contentURL: this._contentURL = value
      });
    }
  },
  _contentURL: null,
  










  get contentScriptWhen() this._contentScriptWhen,
  set contentScriptWhen(value) {
    value = validate(value, valid.contentScriptWhen);
    if (value !== this._contentScriptWhen) {
      this._emit('propertyChange', {
        contentScriptWhen: this._contentScriptWhen = value
      });
    }
  },
  _contentScriptWhen: 'end',
  








  get contentScriptOptions() this._contentScriptOptions,
  set contentScriptOptions(value) this._contentScriptOptions = value,
  _contentScriptOptions: null,
  





  get contentScriptFile() this._contentScriptFile,
  set contentScriptFile(value) {
    value = validate(value, valid.contentScriptFile);
    if (value != this._contentScriptFile) {
      this._emit('propertyChange', {
        contentScriptFile: this._contentScriptFile = value
      });
    }
  },
  _contentScriptFile: null,
  





  get contentScript() this._contentScript,
  set contentScript(value) {
    value = validate(value, valid.contentScript);
    if (value != this._contentScript) {
      this._emit('propertyChange', {
        contentScript: this._contentScript = value
      });
    }
  },
  _contentScript: null
});
exports.Loader = Loader;
