



"use strict";

module.metadata = {
  "stability": "unstable"
};

const { Cc, Ci } = require("chrome");
const runtime = require("../system/runtime");
const { isString } = require("../lang/type");
const array = require("../util/array");


const SWP = "{{SEPARATOR}}";
const SEPARATOR = "-"
const INVALID_COMBINATION = "Hotkey key combination must contain one or more " +
                            "modifiers and only one key";


const MODIFIERS = exports.MODIFIERS = {
  'accel': runtime.OS === "Darwin" ? 'meta' : 'control',
  'meta': 'meta',
  'control': 'control',
  'ctrl': 'control',
  'option': 'alt',
  'command': 'meta',
  'alt': 'alt',
  'shift': 'shift'
};




const CODES = exports.CODES = new function Codes() {
  let nsIDOMKeyEvent = Ci.nsIDOMKeyEvent;
  
  let aliases = {
    'subtract':     '-',
    'add':          '+',
    'equals':       '=',
    'slash':        '/',
    'backslash':    '\\',
    'openbracket':  '[',
    'closebracket': ']',
    'quote':        '\'',
    'backquote':    '`',
    'period':       '.',
    'semicolon':    ';',
    'comma':        ','
  };

  
  Object.keys(nsIDOMKeyEvent).filter(function(key) {
    
    return key.indexOf('DOM_VK') === 0;
  }).map(function(key) {
    
    return [ key, nsIDOMKeyEvent[key] ];
  }).map(function([key, value]) {
    return [ key.replace('DOM_VK_', '').replace('_', '').toLowerCase(), value ];
  }).forEach(function ([ key, value ]) {
    this[aliases[key] || key] = value;
  }, this);
};


const KEYS = exports.KEYS = new function Keys() {
  Object.keys(CODES).forEach(function(key) {
    this[CODES[key]] = key;
  }, this)
}

exports.getKeyForCode = function getKeyForCode(code) {
  return (code in KEYS) && KEYS[code];
};
exports.getCodeForKey = function getCodeForKey(key) {
  return (key in CODES) && CODES[key];
};

















var normalize = exports.normalize = function normalize(hotkey, separator) {
  if (!isString(hotkey))
    hotkey = toString(hotkey, separator);
  return toString(toJSON(hotkey, separator), separator);
};


















var toJSON = exports.toJSON = function toJSON(hotkey, separator) {
  separator = separator || SEPARATOR;
  
  
  
  hotkey = hotkey.toLowerCase().replace(separator + separator, separator + SWP);

  let value = {};
  let modifiers = [];
  let keys = hotkey.split(separator);
  keys.forEach(function(name) {
    
    if (name === SWP)
      name = separator;
    if (name in MODIFIERS) {
      array.add(modifiers, MODIFIERS[name]);
    } else {
      if (!value.key)
        value.key = name;
      else
        throw new TypeError(INVALID_COMBINATION);
    }
  });

  if (!value.key)
      throw new TypeError(INVALID_COMBINATION);

  value.modifiers = modifiers.sort();
  return value;
};























var toString = exports.toString = function toString(hotkey, separator) {
  let keys = hotkey.modifiers.slice();
  keys.push(hotkey.key);
  return keys.join(separator || SEPARATOR);
};





var isFunctionKey = exports.isFunctionKey = function isFunctionKey(key) {
  var $
  return key[0].toLowerCase() === 'f' &&
         ($ = parseInt(key.substr(1)), 0 < $ && $ < 25);
};
