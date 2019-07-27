



"use strict";

module.metadata = {
  "stability": "unstable"
};

const { EventEmitter } = require('../deprecated/events');
const { isValidURI, isLocalURL, URL } = require('../url');
const file = require('../io/file');
const { contract } = require('../util/contract');
const { isString, isNil, instanceOf } = require('../lang/type');
const { validateOptions,
  string, array, object, either, required } = require('../deprecated/api-utils');

const isJSONable = (value) => {
  try {
    JSON.parse(JSON.stringify(value));
  } catch (e) {
    return false;
  }
  return true;
};

const isValidScriptFile = (value) =>
  (isString(value) || instanceOf(value, URL)) && isLocalURL(value);


const valid = {
  contentURL: {
    is: either(string, object),
    ok: url => isNil(url) || isLocalURL(url) || isValidURI(url),
    msg: 'The `contentURL` option must be a valid URL.'
  },
  contentScriptFile: {
    is: either(string, object, array),
    ok: value => isNil(value) || [].concat(value).every(isValidScriptFile),
    msg: 'The `contentScriptFile` option must be a local URL or an array of URLs.'
  },
  contentScript: {
    is: either(string, array),
    ok: value => isNil(value) || [].concat(value).every(isString),
    msg: 'The `contentScript` option must be a string or an array of strings.'
  },
  contentScriptWhen: {
    is: required(string),
    map: value => value || 'end',
    ok: value => ~['start', 'ready', 'end'].indexOf(value),
    msg: 'The `contentScriptWhen` option must be either "start", "ready" or "end".'
  },
  contentScriptOptions: {
    ok: value => isNil(value) || isJSONable(value),
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

exports.contract = contract(valid);
