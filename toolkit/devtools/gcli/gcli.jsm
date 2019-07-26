















"use strict";









this.EXPORTED_SYMBOLS = [ "gcli" ];

var define = Components.utils.import("resource://gre/modules/devtools/Require.jsm", {}).define;
var require = Components.utils.import("resource://gre/modules/devtools/Require.jsm", {}).require;
var console = Components.utils.import("resource://gre/modules/devtools/Console.jsm", {}).console;
var setTimeout = Components.utils.import("resource://gre/modules/Timer.jsm", {}).setTimeout;
var clearTimeout = Components.utils.import("resource://gre/modules/Timer.jsm", {}).clearTimeout;
var Node = Components.interfaces.nsIDOMNode;
var HTMLElement = Components.interfaces.nsIDOMHTMLElement;
var Event = Components.interfaces.nsIDOMEvent;

















define('gcli/index', ['require', 'exports', 'module' , 'gcli/settings', 'gcli/api', 'gcli/types/selection', 'gcli/types/delegate', 'gcli/types/array', 'gcli/types/boolean', 'gcli/types/command', 'gcli/types/date', 'gcli/types/file', 'gcli/types/javascript', 'gcli/types/node', 'gcli/types/number', 'gcli/types/resource', 'gcli/types/setting', 'gcli/types/string', 'gcli/converters', 'gcli/converters/basic', 'gcli/converters/terminal', 'gcli/ui/intro', 'gcli/ui/focus', 'gcli/ui/fields/basic', 'gcli/ui/fields/javascript', 'gcli/ui/fields/selection', 'gcli/commands/connect', 'gcli/commands/context', 'gcli/commands/help', 'gcli/commands/pref', 'gcli/ui/ffdisplay'], function(require, exports, module) {

'use strict';

require('gcli/settings').startup();

var api = require('gcli/api');
api.populateApi(exports);

exports.addItems(require('gcli/types/selection').items);
exports.addItems(require('gcli/types/delegate').items);

exports.addItems(require('gcli/types/array').items);
exports.addItems(require('gcli/types/boolean').items);
exports.addItems(require('gcli/types/command').items);
exports.addItems(require('gcli/types/date').items);
exports.addItems(require('gcli/types/file').items);
exports.addItems(require('gcli/types/javascript').items);
exports.addItems(require('gcli/types/node').items);
exports.addItems(require('gcli/types/number').items);
exports.addItems(require('gcli/types/resource').items);
exports.addItems(require('gcli/types/setting').items);
exports.addItems(require('gcli/types/string').items);

exports.addItems(require('gcli/converters').items);
exports.addItems(require('gcli/converters/basic').items);


exports.addItems(require('gcli/converters/terminal').items);

exports.addItems(require('gcli/ui/intro').items);
exports.addItems(require('gcli/ui/focus').items);

exports.addItems(require('gcli/ui/fields/basic').items);
exports.addItems(require('gcli/ui/fields/javascript').items);
exports.addItems(require('gcli/ui/fields/selection').items);




exports.addItems(require('gcli/commands/connect').items);
exports.addItems(require('gcli/commands/context').items);
exports.addItems(require('gcli/commands/help').items);
exports.addItems(require('gcli/commands/pref').items);















exports.createDisplay = function(opts) {
  var FFDisplay = require('gcli/ui/ffdisplay').FFDisplay;
  return new FFDisplay(opts);
};

var prefSvc = Components.classes['@mozilla.org/preferences-service;1']
                        .getService(Components.interfaces.nsIPrefService);
var prefBranch = prefSvc.getBranch(null)
                        .QueryInterface(Components.interfaces.nsIPrefBranch2);

exports.hiddenByChromePref = function() {
  return !prefBranch.prefHasUserValue('devtools.chrome.enabled');
};


try {
  var Services = Components.utils.import('resource://gre/modules/Services.jsm', {}).Services;
  var stringBundle = Services.strings.createBundle(
          'chrome://browser/locale/devtools/gclicommands.properties');

  


  exports.lookup = function(name) {
    try {
      return stringBundle.GetStringFromName(name);
    }
    catch (ex) {
      throw new Error('Failure in lookup(\'' + name + '\')');
    }
  };

  


  exports.lookupFormat = function(name, swaps) {
    try {
      return stringBundle.formatStringFromName(name, swaps, swaps.length);
    }
    catch (ex) {
      throw new Error('Failure in lookupFormat(\'' + name + '\')');
    }
  };
}
catch (ex) {
  console.error('Using string fallbacks', ex);

  exports.lookup = function(name) {
    return name;
  };
  exports.lookupFormat = function(name, swaps) {
    return name;
  };
}


});
















define('gcli/settings', ['require', 'exports', 'module' , 'util/util', 'gcli/types'], function(require, exports, module) {

'use strict';

var imports = {};

var XPCOMUtils = Components.utils.import('resource://gre/modules/XPCOMUtils.jsm', {}).XPCOMUtils;
var Services = Components.utils.import('resource://gre/modules/Services.jsm', {}).Services;

XPCOMUtils.defineLazyGetter(imports, 'prefBranch', function() {
  var prefService = Components.classes['@mozilla.org/preferences-service;1']
          .getService(Components.interfaces.nsIPrefService);
  return prefService.getBranch(null)
          .QueryInterface(Components.interfaces.nsIPrefBranch2);
});

XPCOMUtils.defineLazyGetter(imports, 'supportsString', function() {
  return Components.classes['@mozilla.org/supports-string;1']
          .createInstance(Components.interfaces.nsISupportsString);
});


var util = require('util/util');
var types = require('gcli/types');




var DEVTOOLS_PREFIX = 'devtools.gcli.';





function Setting(prefSpec) {
  if (typeof prefSpec === 'string') {
    
    this.name = prefSpec;
    this.description = '';
  }
  else {
    
    this.name = DEVTOOLS_PREFIX + prefSpec.name;

    if (prefSpec.ignoreTypeDifference !== true && prefSpec.type) {
      if (this.type.name !== prefSpec.type) {
        throw new Error('Locally declared type (' + prefSpec.type + ') != ' +
            'Mozilla declared type (' + this.type.name + ') for ' + this.name);
      }
    }

    this.description = prefSpec.description;
  }

  this.onChange = util.createEvent('Setting.onChange');
}




Object.defineProperty(Setting.prototype, 'type', {
  get: function() {
    switch (imports.prefBranch.getPrefType(this.name)) {
      case imports.prefBranch.PREF_BOOL:
        return types.createType('boolean');

      case imports.prefBranch.PREF_INT:
        return types.createType('number');

      case imports.prefBranch.PREF_STRING:
        return types.createType('string');

      default:
        throw new Error('Unknown type for ' + this.name);
    }
  },
  enumerable: true
});




Object.defineProperty(Setting.prototype, 'value', {
  get: function() {
    switch (imports.prefBranch.getPrefType(this.name)) {
      case imports.prefBranch.PREF_BOOL:
        return imports.prefBranch.getBoolPref(this.name);

      case imports.prefBranch.PREF_INT:
        return imports.prefBranch.getIntPref(this.name);

      case imports.prefBranch.PREF_STRING:
        var value = imports.prefBranch.getComplexValue(this.name,
                Components.interfaces.nsISupportsString).data;
        
        if (/^chrome:\/\/.+\/locale\/.+\.properties/.test(value)) {
          value = imports.prefBranch.getComplexValue(this.name,
                  Components.interfaces.nsIPrefLocalizedString).data;
        }
        return value;

      default:
        throw new Error('Invalid value for ' + this.name);
    }
  },

  set: function(value) {
    if (imports.prefBranch.prefIsLocked(this.name)) {
      throw new Error('Locked preference ' + this.name);
    }

    switch (imports.prefBranch.getPrefType(this.name)) {
      case imports.prefBranch.PREF_BOOL:
        imports.prefBranch.setBoolPref(this.name, value);
        break;

      case imports.prefBranch.PREF_INT:
        imports.prefBranch.setIntPref(this.name, value);
        break;

      case imports.prefBranch.PREF_STRING:
        imports.supportsString.data = value;
        imports.prefBranch.setComplexValue(this.name,
                Components.interfaces.nsISupportsString,
                imports.supportsString);
        break;

      default:
        throw new Error('Invalid value for ' + this.name);
    }

    Services.prefs.savePrefFile(null);
  },

  enumerable: true
});




Setting.prototype.setDefault = function() {
  imports.prefBranch.clearUserPref(this.name);
  Services.prefs.savePrefFile(null);
};





var settingsAll = [];




var settingsMap = new Map();




var hasReadSystem = false;




function reset() {
  settingsMap = new Map();
  settingsAll = [];
  hasReadSystem = false;
}




exports.startup = function() {
  reset();
};

exports.shutdown = function() {
  reset();
};





function readSystem() {
  if (hasReadSystem) {
    return;
  }

  imports.prefBranch.getChildList('').forEach(function(name) {
    var setting = new Setting(name);
    settingsAll.push(setting);
    settingsMap.set(name, setting);
  });

  settingsAll.sort(function(s1, s2) {
    return s1.name.localeCompare(s2.name);
  });

  hasReadSystem = true;
}





exports.getAll = function(filter) {
  readSystem();

  if (filter == null) {
    return settingsAll;
  }

  return settingsAll.filter(function(setting) {
    return setting.name.indexOf(filter) !== -1;
  });
};




exports.addSetting = function(prefSpec) {
  var setting = new Setting(prefSpec);

  if (settingsMap.has(setting.name)) {
    
    for (var i = 0; i < settingsAll.length; i++) {
      if (settingsAll[i].name === setting.name) {
        settingsAll[i] = setting;
      }
    }
  }

  settingsMap.set(setting.name, setting);
  exports.onChange({ added: setting.name });

  return setting;
};










exports.getSetting = function(name) {
  
  
  var found = settingsMap.get(name);
  if (!found) {
    found = settingsMap.get(DEVTOOLS_PREFIX + name);
  }

  if (found) {
    return found;
  }

  if (hasReadSystem) {
    return undefined;
  }
  else {
    readSystem();
    found = settingsMap.get(name);
    if (!found) {
      found = settingsMap.get(DEVTOOLS_PREFIX + name);
    }
    return found;
  }
};




exports.onChange = util.createEvent('Settings.onChange');




exports.removeSetting = function() { };


});
















define('util/util', ['require', 'exports', 'module' , 'util/promise'], function(require, exports, module) {

'use strict';







var eventDebug = false;




if (eventDebug) {
  if (console.group == null) {
    console.group = function() { console.log(arguments); };
  }
  if (console.groupEnd == null) {
    console.groupEnd = function() { console.log(arguments); };
  }
}




function nameFunction(handler) {
  var scope = handler.scope ? handler.scope.constructor.name + '.' : '';
  var name = handler.func.name;
  if (name) {
    return scope + name;
  }
  for (var prop in handler.scope) {
    if (handler.scope[prop] === handler.func) {
      return scope + prop;
    }
  }
  return scope + handler.func;
}





















exports.createEvent = function(name) {
  var handlers = [];
  var fireHoldCount = 0;
  var heldEvents = [];
  var eventCombiner;

  



  var event = function(ev) {
    if (fireHoldCount > 0) {
      heldEvents.push(ev);
      if (eventDebug) {
        console.log('Held fire: ' + name, ev);
      }
      return;
    }

    if (eventDebug) {
      console.group('Fire: ' + name + ' to ' + handlers.length + ' listeners', ev);
    }

    
    
    for (var i = 0; i < handlers.length; i++) {
      var handler = handlers[i];
      if (eventDebug) {
        console.log(nameFunction(handler));
      }
      handler.func.call(handler.scope, ev);
    }

    if (eventDebug) {
      console.groupEnd();
    }
  };

  




  event.add = function(func, scope) {
    if (eventDebug) {
      console.log('Adding listener to ' + name);
    }

    handlers.push({ func: func, scope: scope });
  };

  





  event.remove = function(func, scope) {
    if (eventDebug) {
      console.log('Removing listener from ' + name);
    }

    var found = false;
    handlers = handlers.filter(function(test) {
      var match = (test.func === func && test.scope === scope);
      if (match) {
        found = true;
      }
      return !match;
    });
    if (!found) {
      console.warn('Handler not found. Attached to ' + name);
    }
  };

  



  event.removeAll = function() {
    handlers = [];
  };

  



  event.holdFire = function() {
    if (eventDebug) {
      console.group('Holding fire: ' + name);
    }

    fireHoldCount++;
  };

  






  event.resumeFire = function() {
    if (eventDebug) {
      console.groupEnd('Resume fire: ' + name);
    }

    if (fireHoldCount === 0) {
      throw new Error('fireHoldCount === 0 during resumeFire on ' + name);
    }

    fireHoldCount--;
    if (heldEvents.length === 0) {
      return;
    }

    if (heldEvents.length === 1) {
      event(heldEvents[0]);
    }
    else {
      var first = heldEvents[0];
      var last = heldEvents[heldEvents.length - 1];
      if (eventCombiner) {
        event(eventCombiner(first, last, heldEvents));
      }
      else {
        event(last);
      }
    }

    heldEvents = [];
  };

  









  Object.defineProperty(event, 'eventCombiner', {
    set: function(newEventCombiner) {
      if (typeof newEventCombiner !== 'function') {
        throw new Error('eventCombiner is not a function');
      }
      eventCombiner = newEventCombiner;
    },

    enumerable: true
  });

  return event;
};



var promise = require('util/promise');






exports.synchronize = function(p) {
  if (p == null || typeof p.then !== 'function') {
    return p;
  }
  var failure;
  var reply;
  var onDone = function(value) {
    failure = false;
    reply = value;
  };
  var onError = function (value) {
    failure = true;
    reply = value;
  };
  p.then(onDone, onError);
  if (failure === undefined) {
    throw new Error('non synchronizable promise');
  }
  if (failure) {
    throw reply;
  }
  return reply;
};













exports.promiseEach = function(array, action, scope) {
  if (array.length === 0) {
    return promise.resolve([]);
  }

  var deferred = promise.defer();
  var replies = [];

  var callNext = function(index) {
    var onSuccess = function(reply) {
      replies[index] = reply;

      if (index + 1 >= array.length) {
        deferred.resolve(replies);
      }
      else {
        callNext(index + 1);
      }
    };

    var onFailure = function(ex) {
      deferred.reject(ex);
    };

    var reply = action.call(scope, array[index], index, array);
    promise.resolve(reply).then(onSuccess).then(null, onFailure);
  };

  callNext(0);
  return deferred.promise;
};








exports.errorHandler = function(ex) {
  if (ex instanceof Error) {
    
    if (ex.stack.indexOf(ex.message) !== -1) {
      console.error(ex.stack);
    }
    else {
      console.error('' + ex);
      console.error(ex.stack);
    }
  }
  else {
    console.error(ex);
  }
};







exports.NS_XHTML = 'http://www.w3.org/1999/xhtml';




exports.NS_XUL = 'http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul';












exports.createElement = function(doc, tag) {
  if (exports.isXmlDocument(doc)) {
    return doc.createElementNS(exports.NS_XHTML, tag);
  }
  else {
    return doc.createElement(tag);
  }
};





exports.clearElement = function(elem) {
  while (elem.hasChildNodes()) {
    elem.removeChild(elem.firstChild);
  }
};

var isAllWhitespace = /^\s*$/;










exports.removeWhitespace = function(elem, deep) {
  var i = 0;
  while (i < elem.childNodes.length) {
    var child = elem.childNodes.item(i);
    if (child.nodeType === 3  &&
        isAllWhitespace.test(child.textContent)) {
      elem.removeChild(child);
    }
    else {
      if (deep && child.nodeType === 1 ) {
        exports.removeWhitespace(child, deep);
      }
      i++;
    }
  }
};









exports.importCss = function(cssText, doc, id) {
  if (!cssText) {
    return undefined;
  }

  doc = doc || document;

  if (!id) {
    id = 'hash-' + hash(cssText);
  }

  var found = doc.getElementById(id);
  if (found) {
    if (found.tagName.toLowerCase() !== 'style') {
      console.error('Warning: importCss passed id=' + id +
              ', but that pre-exists (and isn\'t a style tag)');
    }
    return found;
  }

  var style = exports.createElement(doc, 'style');
  style.id = id;
  style.appendChild(doc.createTextNode(cssText));

  var head = doc.getElementsByTagName('head')[0] || doc.documentElement;
  head.appendChild(style);

  return style;
};







function hash(str) {
  var h = 0;
  if (str.length === 0) {
    return h;
  }
  for (var i = 0; i < str.length; i++) {
    var character = str.charCodeAt(i);
    h = ((h << 5) - h) + character;
    h = h & h; 
  }
  return h;
}





exports.setTextContent = function(elem, text) {
  exports.clearElement(elem);
  var child = elem.ownerDocument.createTextNode(text);
  elem.appendChild(child);
};





exports.setContents = function(elem, contents) {
  if (typeof HTMLElement !== 'undefined' && contents instanceof HTMLElement) {
    exports.clearElement(elem);
    elem.appendChild(contents);
    return;
  }

  if ('innerHTML' in elem) {
    elem.innerHTML = contents;
  }
  else {
    try {
      var ns = elem.ownerDocument.documentElement.namespaceURI;
      if (!ns) {
        ns = exports.NS_XHTML;
      }
      exports.clearElement(elem);
      contents = '<div xmlns="' + ns + '">' + contents + '</div>';
      var range = elem.ownerDocument.createRange();
      var child = range.createContextualFragment(contents).firstChild;
      while (child.hasChildNodes()) {
        elem.appendChild(child.firstChild);
      }
    }
    catch (ex) {
      console.error('Bad XHTML', ex);
      console.trace();
      throw ex;
    }
  }
};





exports.linksToNewTab = function(element) {
  var links = element.ownerDocument.querySelectorAll('*[href]');
  for (var i = 0; i < links.length; i++) {
    links[i].setAttribute('target', '_blank');
  }
  return element;
};





exports.toDom = function(document, html) {
  var div = exports.createElement(document, 'div');
  exports.setContents(div, html);
  return div.children[0];
};








exports.isXmlDocument = function(doc) {
  doc = doc || document;
  
  if (doc.contentType && doc.contentType != 'text/html') {
    return true;
  }
  
  if (doc.xmlVersion != null) {
    return true;
  }
  return false;
};





function positionInNodeList(element, nodeList) {
  for (var i = 0; i < nodeList.length; i++) {
    if (element === nodeList[i]) {
      return i;
    }
  }
  return -1;
}




exports.createEmptyNodeList = function(doc) {
  if (doc.createDocumentFragment) {
    return doc.createDocumentFragment().childNodes;
  }
  return doc.querySelectorAll('x>:root');
};






exports.findCssSelector = function(ele) {
  var document = ele.ownerDocument;
  if (ele.id && document.getElementById(ele.id) === ele) {
    return '#' + ele.id;
  }

  
  var tagName = ele.tagName.toLowerCase();
  if (tagName === 'html') {
    return 'html';
  }
  if (tagName === 'head') {
    return 'head';
  }
  if (tagName === 'body') {
    return 'body';
  }

  if (ele.parentNode == null) {
    console.log('danger: ' + tagName);
  }

  
  var selector, index, matches;
  if (ele.classList.length > 0) {
    for (var i = 0; i < ele.classList.length; i++) {
      
      selector = '.' + ele.classList.item(i);
      matches = document.querySelectorAll(selector);
      if (matches.length === 1) {
        return selector;
      }
      
      selector = tagName + selector;
      matches = document.querySelectorAll(selector);
      if (matches.length === 1) {
        return selector;
      }
      
      index = positionInNodeList(ele, ele.parentNode.children) + 1;
      selector = selector + ':nth-child(' + index + ')';
      matches = document.querySelectorAll(selector);
      if (matches.length === 1) {
        return selector;
      }
    }
  }

  
  index = positionInNodeList(ele, ele.parentNode.children) + 1;
  selector = exports.findCssSelector(ele.parentNode) + ' > ' +
          tagName + ':nth-child(' + index + ')';

  return selector;
};




exports.createUrlLookup = function(callingModule) {
  return function imageUrl(path) {
    try {
      return require('text!gcli/ui/' + path);
    }
    catch (ex) {
      
      
      
      if (callingModule.filename) {
        return callingModule.filename + path;
      }

      var filename = callingModule.id.split('/').pop() + '.js';

      if (callingModule.uri.substr(-filename.length) !== filename) {
        console.error('Can\'t work out path from module.uri/module.id');
        return path;
      }

      if (callingModule.uri) {
        var end = callingModule.uri.length - filename.length - 1;
        return callingModule.uri.substr(0, end) + '/' + path;
      }

      return filename + '/' + path;
    }
  };
};
















if (typeof 'KeyEvent' === 'undefined') {
  exports.KeyEvent = this.KeyEvent;
}
else {
  exports.KeyEvent = {
    DOM_VK_CANCEL: 3,
    DOM_VK_HELP: 6,
    DOM_VK_BACK_SPACE: 8,
    DOM_VK_TAB: 9,
    DOM_VK_CLEAR: 12,
    DOM_VK_RETURN: 13,
    DOM_VK_ENTER: 14,
    DOM_VK_SHIFT: 16,
    DOM_VK_CONTROL: 17,
    DOM_VK_ALT: 18,
    DOM_VK_PAUSE: 19,
    DOM_VK_CAPS_LOCK: 20,
    DOM_VK_ESCAPE: 27,
    DOM_VK_SPACE: 32,
    DOM_VK_PAGE_UP: 33,
    DOM_VK_PAGE_DOWN: 34,
    DOM_VK_END: 35,
    DOM_VK_HOME: 36,
    DOM_VK_LEFT: 37,
    DOM_VK_UP: 38,
    DOM_VK_RIGHT: 39,
    DOM_VK_DOWN: 40,
    DOM_VK_PRINTSCREEN: 44,
    DOM_VK_INSERT: 45,
    DOM_VK_DELETE: 46,
    DOM_VK_0: 48,
    DOM_VK_1: 49,
    DOM_VK_2: 50,
    DOM_VK_3: 51,
    DOM_VK_4: 52,
    DOM_VK_5: 53,
    DOM_VK_6: 54,
    DOM_VK_7: 55,
    DOM_VK_8: 56,
    DOM_VK_9: 57,
    DOM_VK_SEMICOLON: 59,
    DOM_VK_EQUALS: 61,
    DOM_VK_A: 65,
    DOM_VK_B: 66,
    DOM_VK_C: 67,
    DOM_VK_D: 68,
    DOM_VK_E: 69,
    DOM_VK_F: 70,
    DOM_VK_G: 71,
    DOM_VK_H: 72,
    DOM_VK_I: 73,
    DOM_VK_J: 74,
    DOM_VK_K: 75,
    DOM_VK_L: 76,
    DOM_VK_M: 77,
    DOM_VK_N: 78,
    DOM_VK_O: 79,
    DOM_VK_P: 80,
    DOM_VK_Q: 81,
    DOM_VK_R: 82,
    DOM_VK_S: 83,
    DOM_VK_T: 84,
    DOM_VK_U: 85,
    DOM_VK_V: 86,
    DOM_VK_W: 87,
    DOM_VK_X: 88,
    DOM_VK_Y: 89,
    DOM_VK_Z: 90,
    DOM_VK_CONTEXT_MENU: 93,
    DOM_VK_NUMPAD0: 96,
    DOM_VK_NUMPAD1: 97,
    DOM_VK_NUMPAD2: 98,
    DOM_VK_NUMPAD3: 99,
    DOM_VK_NUMPAD4: 100,
    DOM_VK_NUMPAD5: 101,
    DOM_VK_NUMPAD6: 102,
    DOM_VK_NUMPAD7: 103,
    DOM_VK_NUMPAD8: 104,
    DOM_VK_NUMPAD9: 105,
    DOM_VK_MULTIPLY: 106,
    DOM_VK_ADD: 107,
    DOM_VK_SEPARATOR: 108,
    DOM_VK_SUBTRACT: 109,
    DOM_VK_DECIMAL: 110,
    DOM_VK_DIVIDE: 111,
    DOM_VK_F1: 112,
    DOM_VK_F2: 113,
    DOM_VK_F3: 114,
    DOM_VK_F4: 115,
    DOM_VK_F5: 116,
    DOM_VK_F6: 117,
    DOM_VK_F7: 118,
    DOM_VK_F8: 119,
    DOM_VK_F9: 120,
    DOM_VK_F10: 121,
    DOM_VK_F11: 122,
    DOM_VK_F12: 123,
    DOM_VK_F13: 124,
    DOM_VK_F14: 125,
    DOM_VK_F15: 126,
    DOM_VK_F16: 127,
    DOM_VK_F17: 128,
    DOM_VK_F18: 129,
    DOM_VK_F19: 130,
    DOM_VK_F20: 131,
    DOM_VK_F21: 132,
    DOM_VK_F22: 133,
    DOM_VK_F23: 134,
    DOM_VK_F24: 135,
    DOM_VK_NUM_LOCK: 144,
    DOM_VK_SCROLL_LOCK: 145,
    DOM_VK_COMMA: 188,
    DOM_VK_PERIOD: 190,
    DOM_VK_SLASH: 191,
    DOM_VK_BACK_QUOTE: 192,
    DOM_VK_OPEN_BRACKET: 219,
    DOM_VK_BACK_SLASH: 220,
    DOM_VK_CLOSE_BRACKET: 221,
    DOM_VK_QUOTE: 222,
    DOM_VK_META: 224
  };
}


});
















define('util/promise', ['require', 'exports', 'module' ], function(require, exports, module) {

'use strict';

var imported = {};
Components.utils.import('resource://gre/modules/commonjs/sdk/core/promise.js',
                        imported);

exports.defer = imported.Promise.defer;
exports.resolve = imported.Promise.resolve;
exports.reject = imported.Promise.reject;
exports.promised = imported.Promise.promised;
exports.all = imported.Promise.all;

});
















define('gcli/types', ['require', 'exports', 'module' , 'util/util', 'util/promise', 'gcli/argument'], function(require, exports, module) {

'use strict';

var util = require('util/util');
var promise = require('util/promise');
var Argument = require('gcli/argument').Argument;
var BlankArgument = require('gcli/argument').BlankArgument;







var Status = {
  




  VALID: {
    toString: function() { return 'VALID'; },
    valueOf: function() { return 0; }
  },

  





  INCOMPLETE: {
    toString: function() { return 'INCOMPLETE'; },
    valueOf: function() { return 1; }
  },

  





  ERROR: {
    toString: function() { return 'ERROR'; },
    valueOf: function() { return 2; }
  },

  



  combine: function() {
    var combined = Status.VALID;
    for (var i = 0; i < arguments.length; i++) {
      var status = arguments[i];
      if (Array.isArray(status)) {
        status = Status.combine.apply(null, status);
      }
      if (status > combined) {
        combined = status;
      }
    }
    return combined;
  },

  fromString: function(str) {
    switch (str) {
      case Status.VALID.toString():
        return Status.VALID;
      case Status.INCOMPLETE.toString():
        return Status.INCOMPLETE;
      case Status.ERROR.toString():
        return Status.ERROR;
      default:
        throw new Error('\'' + str + '\' is not a status');
    }
  }
};

exports.Status = Status;


































function Conversion(value, arg, status, message, predictions) {
  
  this.value = value;

  
  this.arg = arg;
  if (arg == null) {
    throw new Error('Missing arg');
  }

  if (predictions != null) {
    var toCheck = typeof predictions === 'function' ? predictions() : predictions;
    if (typeof toCheck.then !== 'function') {
      throw new Error('predictions is not a promise');
    }
    toCheck.then(function(value) {
      if (!Array.isArray(value)) {
        throw new Error('prediction resolves to non array');
      }
    }, util.errorHandler);
  }

  this._status = status || Status.VALID;
  this.message = message;
  this.predictions = predictions;
}







Object.defineProperty(Conversion.prototype, 'assignment', {
  get: function() { return this.arg.assignment; },
  set: function(assignment) { this.arg.assignment = assignment; },
  enumerable: true
});




Conversion.prototype.isDataProvided = function() {
  return this.arg.type !== 'BlankArgument';
};






Conversion.prototype.equals = function(that) {
  if (this === that) {
    return true;
  }
  if (that == null) {
    return false;
  }
  return this.valueEquals(that) && this.argEquals(that);
};






Conversion.prototype.valueEquals = function(that) {
  return that != null && this.value === that.value;
};






Conversion.prototype.argEquals = function(that) {
  return that == null ? false : this.arg.equals(that.arg);
};




Conversion.prototype.getStatus = function(arg) {
  return this._status;
};




Conversion.prototype.toString = function() {
  return this.arg.toString();
};















Conversion.prototype.getPredictions = function() {
  if (typeof this.predictions === 'function') {
    return this.predictions();
  }
  return promise.resolve(this.predictions || []);
};





Conversion.prototype.constrainPredictionIndex = function(index) {
  if (index == null) {
    return promise.resolve();
  }

  return this.getPredictions().then(function(value) {
    if (value.length === 0) {
      return undefined;
    }

    index = index % value.length;
    if (index < 0) {
      index = value.length + index;
    }
    return index;
  }.bind(this));
};





Conversion.maxPredictions = 11;

exports.Conversion = Conversion;









function ArrayConversion(conversions, arg) {
  this.arg = arg;
  this.conversions = conversions;
  this.value = conversions.map(function(conversion) {
    return conversion.value;
  }, this);

  this._status = Status.combine(conversions.map(function(conversion) {
    return conversion.getStatus();
  }));

  
  
  this.message = '';

  
  this.predictions = [];
}

ArrayConversion.prototype = Object.create(Conversion.prototype);

Object.defineProperty(ArrayConversion.prototype, 'assignment', {
  get: function() { return this._assignment; },
  set: function(assignment) {
    this._assignment = assignment;

    this.conversions.forEach(function(conversion) {
      conversion.assignment = assignment;
    }, this);
  },
  enumerable: true
});

ArrayConversion.prototype.getStatus = function(arg) {
  if (arg && arg.conversion) {
    return arg.conversion.getStatus();
  }
  return this._status;
};

ArrayConversion.prototype.isDataProvided = function() {
  return this.conversions.length > 0;
};

ArrayConversion.prototype.valueEquals = function(that) {
  if (that == null) {
    return false;
  }

  if (!(that instanceof ArrayConversion)) {
    throw new Error('Can\'t compare values with non ArrayConversion');
  }

  if (this.value === that.value) {
    return true;
  }

  if (this.value.length !== that.value.length) {
    return false;
  }

  for (var i = 0; i < this.conversions.length; i++) {
    if (!this.conversions[i].valueEquals(that.conversions[i])) {
      return false;
    }
  }

  return true;
};

ArrayConversion.prototype.toString = function() {
  return '[ ' + this.conversions.map(function(conversion) {
    return conversion.toString();
  }, this).join(', ') + ' ]';
};

exports.ArrayConversion = ArrayConversion;








function Type() {
}








Type.prototype.stringify = function(value, context) {
  throw new Error('Not implemented');
};









Type.prototype.parse = function(arg, context) {
  throw new Error('Not implemented');
};






Type.prototype.parseString = function(str, context) {
  return this.parse(new Argument(str), context);
};







Type.prototype.name = undefined;





Type.prototype.increment = function(value, context) {
  return undefined;
};





Type.prototype.decrement = function(value, context) {
  return undefined;
};







Type.prototype.getBlank = function(context) {
  return new Conversion(undefined, new BlankArgument(), Status.INCOMPLETE, '');
};







Type.prototype.getType = function(context) {
  return this;
};





Type.prototype.item = 'type';

exports.Type = Type;





var registeredTypes = {};

exports.getTypeNames = function() {
  return Object.keys(registeredTypes);
};










exports.addType = function(type) {
  if (typeof type === 'object') {
    if (!type.name) {
      throw new Error('All registered types must have a name');
    }

    if (type instanceof Type) {
      registeredTypes[type.name] = type;
    }
    else {
      var name = type.name;
      var parent = type.parent;
      type.name = parent;
      delete type.parent;

      registeredTypes[name] = exports.createType(type);

      type.name = name;
      type.parent = parent;
    }
  }
  else if (typeof type === 'function') {
    if (!type.prototype.name) {
      throw new Error('All registered types must have a name');
    }
    registeredTypes[type.prototype.name] = type;
  }
  else {
    throw new Error('Unknown type: ' + type);
  }
};




exports.removeType = function(type) {
  delete registeredTypes[type.name];
};




exports.createType = function(typeSpec) {
  if (typeof typeSpec === 'string') {
    typeSpec = { name: typeSpec };
  }

  if (typeof typeSpec !== 'object') {
    throw new Error('Can\'t extract type from ' + typeSpec);
  }

  var type, newType;
  if (typeSpec.name == null || typeSpec.name == 'type') {
    type = Type;
  }
  else {
    type = registeredTypes[typeSpec.name];
  }

  if (!type) {
    console.error('Known types: ' + Object.keys(registeredTypes).join(', '));
    throw new Error('Unknown type: \'' + typeSpec.name + '\'');
  }

  if (typeof type === 'function') {
    newType = new type(typeSpec);
  }
  else {
    
    newType = {};
    copyProperties(type, newType);
  }

  
  copyProperties(typeSpec, newType);

  if (typeof type !== 'function') {
    if (typeof newType.constructor === 'function') {
      newType.constructor();
    }
  }

  return newType;
};

function copyProperties(src, dest) {
  for (var key in src) {
    var descriptor;
    var obj = src;
    while (true) {
      descriptor = Object.getOwnPropertyDescriptor(obj, key);
      if (descriptor != null) {
        break;
      }
      obj = Object.getPrototypeOf(obj);
      if (obj == null) {
        throw new Error('Can\'t find descriptor of ' + key);
      }
    }

    if ('value' in descriptor) {
      dest[key] = src[key];
    }
    else if ('get' in descriptor) {
      Object.defineProperty(dest, key, {
        get: descriptor.get,
        set: descriptor.set,
        enumerable: descriptor.enumerable
      });
    }
    else {
      throw new Error('Don\'t know how to copy ' + key + ' property.');
    }
  }
}


});
















define('gcli/argument', ['require', 'exports', 'module' ], function(require, exports, module) {

'use strict';
























function Argument(text, prefix, suffix) {
  if (text === undefined) {
    this.text = '';
    this.prefix = '';
    this.suffix = '';
  }
  else {
    this.text = text;
    this.prefix = prefix !== undefined ? prefix : '';
    this.suffix = suffix !== undefined ? suffix : '';
  }
}

Argument.prototype.type = 'Argument';





Argument.prototype.merge = function(following) {
  
  
  return new Argument(
    this.text + this.suffix + following.prefix + following.text,
    this.prefix, following.suffix);
};
















Argument.prototype.beget = function(options) {
  var text = this.text;
  var prefix = this.prefix;
  var suffix = this.suffix;

  if (options.text != null) {
    text = options.text;

    
    if (!options.dontQuote) {
      var needsQuote = text.indexOf(' ') >= 0 || text.length === 0;
      var hasQuote = /['"]$/.test(prefix);
      if (needsQuote && !hasQuote) {
        prefix = prefix + '\'';
        suffix = '\'' + suffix;
      }
    }
  }

  if (options.prefixSpace && prefix.charAt(0) !== ' ') {
    prefix = ' ' + prefix;
  }

  if (options.prefixPostSpace && prefix.charAt(prefix.length - 1) !== ' ') {
    prefix = prefix + ' ';
  }

  if (options.suffixSpace && suffix.charAt(suffix.length - 1) !== ' ') {
    suffix = suffix + ' ';
  }

  if (text === this.text && suffix === this.suffix && prefix === this.prefix) {
    return this;
  }

  var type = options.type || Argument;
  return new type(text, prefix, suffix);
};




Object.defineProperty(Argument.prototype, 'assignment', {
  get: function() { return this._assignment; },
  set: function(assignment) { this._assignment = assignment; },
  enumerable: true
});







Argument.prototype.getArgs = function() {
  return [ this ];
};







Argument.prototype.equals = function(that) {
  if (this === that) {
    return true;
  }
  if (that == null || !(that instanceof Argument)) {
    return false;
  }

  return this.text === that.text &&
       this.prefix === that.prefix && this.suffix === that.suffix;
};




Argument.prototype.toString = function() {
  
  
  return this.prefix + this.text + this.suffix;
};





Argument.merge = function(argArray, start, end) {
  start = (start === undefined) ? 0 : start;
  end = (end === undefined) ? argArray.length : end;

  var joined;
  for (var i = start; i < end; i++) {
    var arg = argArray[i];
    if (!joined) {
      joined = arg;
    }
    else {
      joined = joined.merge(arg);
    }
  }
  return joined;
};






Object.defineProperty(Argument.prototype, '_summaryJson', {
  get: function() {
    var assignStatus = this.assignment == null ?
            'null' :
            this.assignment.param.name;
    return '<' + this.prefix + ':' + this.text + ':' + this.suffix + '>' +
        ' (a=' + assignStatus + ',' + ' t=' + this.type + ')';
  },
  enumerable: true
});

exports.Argument = Argument;






function BlankArgument() {
  this.text = '';
  this.prefix = '';
  this.suffix = '';
}

BlankArgument.prototype = Object.create(Argument.prototype);

BlankArgument.prototype.type = 'BlankArgument';

exports.BlankArgument = BlankArgument;









function ScriptArgument(text, prefix, suffix) {
  this.text = text !== undefined ? text : '';
  this.prefix = prefix !== undefined ? prefix : '';
  this.suffix = suffix !== undefined ? suffix : '';

  ScriptArgument._moveSpaces(this);
}

ScriptArgument.prototype = Object.create(Argument.prototype);

ScriptArgument.prototype.type = 'ScriptArgument';








ScriptArgument._moveSpaces = function(arg) {
  while (arg.text.charAt(0) === ' ') {
    arg.prefix = arg.prefix + ' ';
    arg.text = arg.text.substring(1);
  }

  while (arg.text.charAt(arg.text.length - 1) === ' ') {
    arg.suffix = ' ' + arg.suffix;
    arg.text = arg.text.slice(0, -1);
  }
};




ScriptArgument.prototype.beget = function(options) {
  options.type = ScriptArgument;
  var begotten = Argument.prototype.beget.call(this, options);
  ScriptArgument._moveSpaces(begotten);
  return begotten;
};

exports.ScriptArgument = ScriptArgument;







function MergedArgument(args, start, end) {
  if (!Array.isArray(args)) {
    throw new Error('args is not an array of Arguments');
  }

  if (start === undefined) {
    this.args = args;
  }
  else {
    this.args = args.slice(start, end);
  }

  var arg = Argument.merge(this.args);
  this.text = arg.text;
  this.prefix = arg.prefix;
  this.suffix = arg.suffix;
}

MergedArgument.prototype = Object.create(Argument.prototype);

MergedArgument.prototype.type = 'MergedArgument';





Object.defineProperty(MergedArgument.prototype, 'assignment', {
  get: function() { return this._assignment; },
  set: function(assignment) {
    this._assignment = assignment;

    this.args.forEach(function(arg) {
      arg.assignment = assignment;
    }, this);
  },
  enumerable: true
});

MergedArgument.prototype.getArgs = function() {
  return this.args;
};

MergedArgument.prototype.equals = function(that) {
  if (this === that) {
    return true;
  }
  if (that == null || !(that instanceof MergedArgument)) {
    return false;
  }

  

  return this.text === that.text &&
       this.prefix === that.prefix && this.suffix === that.suffix;
};

exports.MergedArgument = MergedArgument;






function TrueNamedArgument(arg) {
  this.arg = arg;
  this.text = arg.text;
  this.prefix = arg.prefix;
  this.suffix = arg.suffix;
}

TrueNamedArgument.prototype = Object.create(Argument.prototype);

TrueNamedArgument.prototype.type = 'TrueNamedArgument';

Object.defineProperty(TrueNamedArgument.prototype, 'assignment', {
  get: function() { return this._assignment; },
  set: function(assignment) {
    this._assignment = assignment;

    if (this.arg) {
      this.arg.assignment = assignment;
    }
  },
  enumerable: true
});

TrueNamedArgument.prototype.getArgs = function() {
  return [ this.arg ];
};

TrueNamedArgument.prototype.equals = function(that) {
  if (this === that) {
    return true;
  }
  if (that == null || !(that instanceof TrueNamedArgument)) {
    return false;
  }

  return this.text === that.text &&
       this.prefix === that.prefix && this.suffix === that.suffix;
};




TrueNamedArgument.prototype.beget = function(options) {
  if (options.text) {
    console.error('Can\'t change text of a TrueNamedArgument', this, options);
  }

  options.type = TrueNamedArgument;
  var begotten = Argument.prototype.beget.call(this, options);
  begotten.arg = new Argument(begotten.text, begotten.prefix, begotten.suffix);
  return begotten;
};

exports.TrueNamedArgument = TrueNamedArgument;






function FalseNamedArgument() {
  this.text = '';
  this.prefix = '';
  this.suffix = '';
}

FalseNamedArgument.prototype = Object.create(Argument.prototype);

FalseNamedArgument.prototype.type = 'FalseNamedArgument';

FalseNamedArgument.prototype.getArgs = function() {
  return [ ];
};

FalseNamedArgument.prototype.equals = function(that) {
  if (this === that) {
    return true;
  }
  if (that == null || !(that instanceof FalseNamedArgument)) {
    return false;
  }

  return this.text === that.text &&
       this.prefix === that.prefix && this.suffix === that.suffix;
};

exports.FalseNamedArgument = FalseNamedArgument;


















function NamedArgument() {
  if (typeof arguments[0] === 'string') {
    this.nameArg = null;
    this.valueArg = null;
    this.text = arguments[0];
    this.prefix = arguments[1];
    this.suffix = arguments[2];
  }
  else if (arguments[1] == null) {
    this.nameArg = arguments[0];
    this.valueArg = null;
    this.text = '';
    this.prefix = this.nameArg.toString();
    this.suffix = '';
  }
  else {
    this.nameArg = arguments[0];
    this.valueArg = arguments[1];
    this.text = this.valueArg.text;
    this.prefix = this.nameArg.toString() + this.valueArg.prefix;
    this.suffix = this.valueArg.suffix;
  }
}

NamedArgument.prototype = Object.create(Argument.prototype);

NamedArgument.prototype.type = 'NamedArgument';

Object.defineProperty(NamedArgument.prototype, 'assignment', {
  get: function() { return this._assignment; },
  set: function(assignment) {
    this._assignment = assignment;

    this.nameArg.assignment = assignment;
    if (this.valueArg != null) {
      this.valueArg.assignment = assignment;
    }
  },
  enumerable: true
});

NamedArgument.prototype.getArgs = function() {
  return this.valueArg ? [ this.nameArg, this.valueArg ] : [ this.nameArg ];
};

NamedArgument.prototype.equals = function(that) {
  if (this === that) {
    return true;
  }
  if (that == null) {
    return false;
  }

  if (!(that instanceof NamedArgument)) {
    return false;
  }

  

  return this.text === that.text &&
       this.prefix === that.prefix && this.suffix === that.suffix;
};




NamedArgument.prototype.beget = function(options) {
  options.type = NamedArgument;
  var begotten = Argument.prototype.beget.call(this, options);

  
  
  var matches = /^([\s]*)([^\s]*)([\s]*['"]?)$/.exec(begotten.prefix);

  if (this.valueArg == null && begotten.text === '') {
    begotten.nameArg = new Argument(matches[2], matches[1], matches[3]);
    begotten.valueArg = null;
  }
  else {
    begotten.nameArg = new Argument(matches[2], matches[1], '');
    begotten.valueArg = new Argument(begotten.text, matches[3], begotten.suffix);
  }

  return begotten;
};

exports.NamedArgument = NamedArgument;






function ArrayArgument() {
  this.args = [];
}

ArrayArgument.prototype = Object.create(Argument.prototype);

ArrayArgument.prototype.type = 'ArrayArgument';

ArrayArgument.prototype.addArgument = function(arg) {
  this.args.push(arg);
};

ArrayArgument.prototype.addArguments = function(args) {
  Array.prototype.push.apply(this.args, args);
};

ArrayArgument.prototype.getArguments = function() {
  return this.args;
};

Object.defineProperty(ArrayArgument.prototype, 'assignment', {
  get: function() { return this._assignment; },
  set: function(assignment) {
    this._assignment = assignment;

    this.args.forEach(function(arg) {
      arg.assignment = assignment;
    }, this);
  },
  enumerable: true
});

ArrayArgument.prototype.getArgs = function() {
  return this.args;
};

ArrayArgument.prototype.equals = function(that) {
  if (this === that) {
    return true;
  }
  if (that == null) {
    return false;
  }

  if (that.type !== 'ArrayArgument') {
    return false;
  }

  if (this.args.length !== that.args.length) {
    return false;
  }

  for (var i = 0; i < this.args.length; i++) {
    if (!this.args[i].equals(that.args[i])) {
      return false;
    }
  }

  return true;
};




ArrayArgument.prototype.toString = function() {
  return '{' + this.args.map(function(arg) {
    return arg.toString();
  }, this).join(',') + '}';
};

exports.ArrayArgument = ArrayArgument;


});
















define('gcli/api', ['require', 'exports', 'module' , 'gcli/canon', 'gcli/converters', 'gcli/types', 'gcli/settings', 'gcli/ui/fields'], function(require, exports, module) {

'use strict';

var canon = require('gcli/canon');
var converters = require('gcli/converters');
var types = require('gcli/types');
var settings = require('gcli/settings');
var fields = require('gcli/ui/fields');




exports.getApi = function() {
  return {
    addCommand: canon.addCommand,
    removeCommand: canon.removeCommand,
    addConverter: converters.addConverter,
    removeConverter: converters.removeConverter,
    addType: types.addType,
    removeType: types.removeType,

    addItems: function(items) {
      items.forEach(function(item) {
        
        
        var type = item.item;
        if (type == null && item.prototype) {
            type = item.prototype.item;
        }
        if (type === 'command') {
          canon.addCommand(item);
        }
        else if (type === 'type') {
          types.addType(item);
        }
        else if (type === 'converter') {
          converters.addConverter(item);
        }
        else if (type === 'setting') {
          settings.addSetting(item);
        }
        else if (type === 'field') {
          fields.addField(item);
        }
        else {
          console.error('Error for: ', item);
          throw new Error('item property not found');
        }
      });
    },

    removeItems: function(items) {
      items.forEach(function(item) {
        if (item.item === 'command') {
          canon.removeCommand(item);
        }
        else if (item.item === 'type') {
          types.removeType(item);
        }
        else if (item.item === 'converter') {
          converters.removeConverter(item);
        }
        else if (item.item === 'settings') {
          settings.removeSetting(item);
        }
        else if (item.item === 'field') {
          fields.removeField(item);
        }
        else {
          throw new Error('item property not found');
        }
      });
    }
  };
};





exports.populateApi = function(obj) {
  var exportable = exports.getApi();
  Object.keys(exportable).forEach(function(key) {
    obj[key] = exportable[key];
  });
};

});
















define('gcli/canon', ['require', 'exports', 'module' , 'util/util', 'util/l10n', 'gcli/types'], function(require, exports, module) {

'use strict';

var util = require('util/util');
var l10n = require('util/l10n');

var types = require('gcli/types');
var Status = require('gcli/types').Status;








function lookup(data, onUndefined) {
  if (data == null) {
    if (onUndefined) {
      return l10n.lookup(onUndefined);
    }

    return data;
  }

  if (typeof data === 'string') {
    return data;
  }

  if (typeof data === 'object') {
    if (data.key) {
      return l10n.lookup(data.key);
    }

    var locales = l10n.getPreferredLocales();
    var translated;
    locales.some(function(locale) {
      translated = data[locale];
      return translated != null;
    });
    if (translated != null) {
      return translated;
    }

    console.error('Can\'t find locale in descriptions: ' +
            'locales=' + JSON.stringify(locales) + ', ' +
            'description=' + JSON.stringify(data));
    return '(No description)';
  }

  return l10n.lookup(onUndefined);
}






function Command(commandSpec) {
  Object.keys(commandSpec).forEach(function(key) {
    this[key] = commandSpec[key];
  }, this);

  if (!this.name) {
    throw new Error('All registered commands must have a name');
  }

  if (this.params == null) {
    this.params = [];
  }
  if (!Array.isArray(this.params)) {
    throw new Error('command.params must be an array in ' + this.name);
  }

  this.hasNamedParameters = false;
  this.description = 'description' in this ? this.description : undefined;
  this.description = lookup(this.description, 'canonDescNone');
  this.manual = 'manual' in this ? this.manual : undefined;
  this.manual = lookup(this.manual);

  
  
  var paramSpecs = this.params;
  this.params = [];
  this.paramGroups = {};
  this._shortParams = {};

  var addParam = function(param) {
    var groupName = param.groupName || Parameter.DEFAULT_GROUP_NAME;
    this.params.push(param);
    if (!this.paramGroups.hasOwnProperty(groupName)) {
      this.paramGroups[groupName] = [];
    }
    this.paramGroups[groupName].push(param);
  }.bind(this);

  
  
  
  
  
  var usingGroups = false;

  
  
  
  
  paramSpecs.forEach(function(spec) {
    if (!spec.group) {
      var param = new Parameter(spec, this, null);
      addParam(param);

      if (!param.isPositionalAllowed) {
        this.hasNamedParameters = true;
      }

      if (usingGroups && param.groupName == null) {
        throw new Error('Parameters can\'t come after param groups.' +
                        ' Ignoring ' + this.name + '/' + spec.name);
      }

      if (param.groupName != null) {
        usingGroups = true;
      }
    }
    else {
      spec.params.forEach(function(ispec) {
        var param = new Parameter(ispec, this, spec.group);
        addParam(param);

        if (!param.isPositionalAllowed) {
          this.hasNamedParameters = true;
        }
      }, this);

      usingGroups = true;
    }
  }, this);

  this.params.forEach(function(param) {
    if (param.short != null) {
      if (this._shortParams[param.short] != null) {
        throw new Error('Multiple params using short name ' + param.short);
      }
      this._shortParams[param.short] = param;
    }
  }, this);
}




Object.defineProperty(Command.prototype, 'json', {
  get: function() {
    return {
      name: this.name,
      description: this.description,
      manual: this.manual,
      params: this.params.map(function(param) { return param.json; }),
      returnType: this.returnType,
      isParent: (this.exec == null)
    };
  },
  enumerable: true
});




Command.prototype.getParameterByShortName = function(short) {
  return this._shortParams[short];
};

exports.Command = Command;






function Parameter(paramSpec, command, groupName) {
  this.command = command || { name: 'unnamed' };
  this.paramSpec = paramSpec;
  this.name = this.paramSpec.name;
  this.type = this.paramSpec.type;
  this.short = this.paramSpec.short;

  if (this.short != null && !/[0-9A-Za-z]/.test(this.short)) {
    throw new Error('\'short\' value must be a single alphanumeric digit.');
  }

  this.groupName = groupName;
  if (this.groupName != null) {
    if (this.paramSpec.option != null) {
      throw new Error('Can\'t have a "option" property in a nested parameter');
    }
  }
  else {
    if (this.paramSpec.option != null) {
      this.groupName = this.paramSpec.option === true ?
              Parameter.DEFAULT_GROUP_NAME :
              '' + this.paramSpec.option;
    }
  }

  if (!this.name) {
    throw new Error('In ' + this.command.name +
                    ': all params must have a name');
  }

  var typeSpec = this.type;
  this.type = types.createType(typeSpec);
  if (this.type == null) {
    console.error('Known types: ' + types.getTypeNames().join(', '));
    throw new Error('In ' + this.command.name + '/' + this.name +
                    ': can\'t find type for: ' + JSON.stringify(typeSpec));
  }

  
  
  if (this.type.name === 'boolean' &&
      this.paramSpec.defaultValue !== undefined) {
    throw new Error('In ' + this.command.name + '/' + this.name +
                    ': boolean parameters can not have a defaultValue.' +
                    ' Ignoring');
  }

  
  
  
  
  if (this._defaultValue != null) {
    try {
      
      
      var context = null;
      var defaultText = this.type.stringify(this.paramSpec.defaultValue, context);
      var parsed = this.type.parseString(defaultText, context);
      parsed.then(function(defaultConversion) {
        if (defaultConversion.getStatus() !== Status.VALID) {
          console.error('In ' + this.command.name + '/' + this.name +
                        ': Error round tripping defaultValue. status = ' +
                        defaultConversion.getStatus());
        }
      }.bind(this), util.errorHandler);
    }
    catch (ex) {
      throw new Error('In ' + this.command.name + '/' + this.name + ': ' + ex);
    }
  }

  
  
  if (!this.isPositionalAllowed && this.paramSpec.defaultValue === undefined &&
      this.type.getBlank == null && this.type.name !== 'boolean') {
    throw new Error('In ' + this.command.name + '/' + this.name +
                    ': Missing defaultValue for optional parameter.');
  }
}




Parameter.DEFAULT_GROUP_NAME = l10n.lookup('canonDefaultGroupName');




Object.defineProperty(Parameter.prototype, 'defaultValue', {
  get: function() {
    if (!('_defaultValue' in this)) {
      this._defaultValue = (this.paramSpec.defaultValue !== undefined) ?
          this.paramSpec.defaultValue :
          this.type.getBlank().value;
    }

    return this._defaultValue;
  },
  enumerable : true
});






Parameter.prototype.isKnownAs = function(name) {
  return (name === '--' + this.name) || (name === '-' + this.short);
};





Object.defineProperty(Parameter.prototype, 'manual', {
  get: function() {
    return lookup(this.paramSpec.manual || undefined);
  },
  enumerable: true
});





Object.defineProperty(Parameter.prototype, 'description', {
  get: function() {
    return lookup(this.paramSpec.description || undefined, 'canonDescNone');
  },
  enumerable: true
});





Object.defineProperty(Parameter.prototype, 'isDataRequired', {
  get: function() {
    return this.defaultValue === undefined;
  },
  enumerable: true
});




Object.defineProperty(Parameter.prototype, 'hidden', {
  get: function() {
    return this.paramSpec.hidden;
  },
  enumerable: true
});





Object.defineProperty(Parameter.prototype, 'isPositionalAllowed', {
  get: function() {
    return this.groupName == null;
  },
  enumerable: true
});




Object.defineProperty(Parameter.prototype, 'json', {
  get: function() {
    var json = {
      name: this.name,
      type: this.paramSpec.type,
      description: this.description
    };
    if (this.defaultValue !== undefined && json.type !== 'boolean') {
      json.defaultValue = this.defaultValue;
    }
    if (this.option !== undefined) {
      json.option = this.option;
    }
    if (this.short !== undefined) {
      json.short = this.short;
    }
    return json;
  },
  enumerable: true
});

exports.Parameter = Parameter;





function Canon() {
  
  this._commands = {};
  
  this._commandNames = [];
  
  this._commandSpecs = {};

  
  this.onCanonChange = util.createEvent('canon.onCanonChange');
}








Canon.prototype.addCommand = function(commandSpec) {
  if (this._commands[commandSpec.name] != null) {
    
    delete this._commands[commandSpec.name];
    this._commandNames = this._commandNames.filter(function(test) {
      return test !== commandSpec.name;
    });
  }

  var command = new Command(commandSpec);
  this._commands[commandSpec.name] = command;
  this._commandNames.push(commandSpec.name);
  this._commandNames.sort();

  this._commandSpecs[commandSpec.name] = commandSpec;

  this.onCanonChange();
  return command;
};







Canon.prototype.removeCommand = function(commandOrName) {
  var name = typeof commandOrName === 'string' ?
          commandOrName :
          commandOrName.name;

  if (!this._commands[name]) {
    return false;
  }

  
  delete this._commands[name];
  delete this._commandSpecs[name];
  this._commandNames = this._commandNames.filter(function(test) {
    return test !== name;
  });

  this.onCanonChange();
  return true;
};





Canon.prototype.getCommand = function(name) {
  
  return this._commands[name] || undefined;
};




Canon.prototype.getCommands = function() {
  return Object.keys(this._commands).map(function(name) {
    return this._commands[name];
  }, this);
};




Canon.prototype.getCommandNames = function() {
  return this._commandNames.slice(0);
};





Canon.prototype.getCommandSpecs = function() {
  var specs = {};

  Object.keys(this._commands).forEach(function(name) {
    var command = this._commands[name];
    if (!command.noRemote) {
      specs[name] = command.json;
    }
  }.bind(this));

  return specs;
};












Canon.prototype.addProxyCommands = function(prefix, commandSpecs, remoter, to) {
  var names = Object.keys(commandSpecs);

  if (this._commands[prefix] != null) {
    throw new Error(l10n.lookupFormat('canonProxyExists', [ prefix ]));
  }

  
  
  this.addCommand({
    name: prefix,
    isProxy: true,
    description: l10n.lookupFormat('canonProxyDesc', [ to ]),
    manual: l10n.lookupFormat('canonProxyManual', [ to ])
  });

  names.forEach(function(name) {
    var commandSpec = commandSpecs[name];

    if (commandSpec.noRemote) {
      return;
    }

    if (!commandSpec.isParent) {
      commandSpec.exec = function(args, context) {
        context.commandName = name;
        return remoter(args, context);
      }.bind(this);
    }

    commandSpec.name = prefix + ' ' + commandSpec.name;
    commandSpec.isProxy = true;
    this.addCommand(commandSpec);
  }.bind(this));
};












Canon.prototype.removeProxyCommands = function(prefix) {
  var toRemove = [];
  Object.keys(this._commandSpecs).forEach(function(name) {
    if (name.indexOf(prefix) === 0) {
      toRemove.push(name);
    }
  }.bind(this));

  var removed = [];
  toRemove.forEach(function(name) {
    var command = this.getCommand(name);
    if (command.isProxy) {
      this.removeCommand(name);
      removed.push(name);
    }
    else {
      console.error('Skipping removal of \'' + name +
                    '\' because it is not a proxy command.');
    }
  }.bind(this));

  return removed;
};

var canon = new Canon();

exports.Canon = Canon;
exports.addCommand = canon.addCommand.bind(canon);
exports.removeCommand = canon.removeCommand.bind(canon);
exports.onCanonChange = canon.onCanonChange;
exports.getCommands = canon.getCommands.bind(canon);
exports.getCommand = canon.getCommand.bind(canon);
exports.getCommandNames = canon.getCommandNames.bind(canon);
exports.getCommandSpecs = canon.getCommandSpecs.bind(canon);
exports.addProxyCommands = canon.addProxyCommands.bind(canon);
exports.removeProxyCommands = canon.removeProxyCommands.bind(canon);










function CommandOutputManager() {
  this.onOutput = util.createEvent('CommandOutputManager.onOutput');
}

exports.CommandOutputManager = CommandOutputManager;


});
















define('util/l10n', ['require', 'exports', 'module' ], function(require, exports, module) {

'use strict';

var XPCOMUtils = Components.utils.import('resource://gre/modules/XPCOMUtils.jsm', {}).XPCOMUtils;
var Services = Components.utils.import('resource://gre/modules/Services.jsm', {}).Services;

var imports = {};
XPCOMUtils.defineLazyGetter(imports, 'stringBundle', function () {
  return Services.strings.createBundle('chrome://browser/locale/devtools/gcli.properties');
});





exports.registerStringsSource = function(modulePath) {
  throw new Error('registerStringsSource is not available in mozilla');
};

exports.unregisterStringsSource = function(modulePath) {
  throw new Error('unregisterStringsSource is not available in mozilla');
};

exports.lookupSwap = function(key, swaps) {
  throw new Error('lookupSwap is not available in mozilla');
};

exports.lookupPlural = function(key, ord, swaps) {
  throw new Error('lookupPlural is not available in mozilla');
};

exports.getPreferredLocales = function() {
  return [ 'root' ];
};


exports.lookup = function(key) {
  try {
    
    
    
    





    return imports.stringBundle.GetStringFromName(key);
  }
  catch (ex) {
    console.error('Failed to lookup ', key, ex);
    return key;
  }
};


exports.propertyLookup = Proxy.create({
  get: function(rcvr, name) {
    return exports.lookup(name);
  }
});


exports.lookupFormat = function(key, swaps) {
  try {
    return imports.stringBundle.formatStringFromName(key, swaps, swaps.length);
  }
  catch (ex) {
    console.error('Failed to format ', key, ex);
    return key;
  }
};


});
















define('gcli/converters', ['require', 'exports', 'module' , 'util/promise'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');






var fallbackDomConverter = {
  from: '*',
  to: 'dom',
  exec: function(data, conversionContext) {
    return conversionContext.document.createTextNode(data || '');
  }
};




var fallbackStringConverter = {
  from: '*',
  to: 'string',
  exec: function(data, conversionContext) {
    return data == null ? '' : data.toString();
  }
};




var viewDomConverter = {
  item: 'converter',
  from: 'view',
  to: 'dom',
  exec: function(view, conversionContext) {
    return view.toDom(conversionContext.document);
  }
};




var viewStringConverter = {
  item: 'converter',
  from: 'view',
  to: 'string',
  exec: function(view, conversionContext) {
    return view.toDom(conversionContext.document).textContent;
  }
};




function getChainConverter(first, second) {
  if (first.to !== second.from) {
    throw new Error('Chain convert impossible: ' + first.to + '!=' + second.from);
  }
  return {
    from: first.from,
    to: second.to,
    exec: function(data, conversionContext) {
      var intermediate = first.exec(data, conversionContext);
      return second.exec(intermediate, conversionContext);
    }
  };
}




var converters = {
  from: {}
};




exports.addConverter = function(converter) {
  var fromMatch = converters.from[converter.from];
  if (fromMatch == null) {
    fromMatch = {};
    converters.from[converter.from] = fromMatch;
  }

  fromMatch[converter.to] = converter;
};




exports.removeConverter = function(converter) {
  var fromMatch = converters.from[converter.from];
  if (fromMatch == null) {
    return;
  }

  if (fromMatch[converter.to] === converter) {
    fromMatch[converter.to] = null;
  }
};




function getConverter(from, to) {
  var fromMatch = converters.from[from];
  if (fromMatch == null) {
    return getFallbackConverter(from, to);
  }

  var converter = fromMatch[to];
  if (converter == null) {
    
    
    
    
    if (to === 'dom') {
      converter = fromMatch.view;
      if (converter != null) {
        return getChainConverter(converter, viewDomConverter);
      }
    }
    if (to === 'string') {
      converter = fromMatch.view;
      if (converter != null) {
        return getChainConverter(converter, viewStringConverter);
      }
    }
    return getFallbackConverter(from, to);
  }
  return converter;
}




function getFallbackConverter(from, to) {
  console.error('No converter from ' + from + ' to ' + to + '. Using fallback');

  if (to === 'dom') {
    return fallbackDomConverter;
  }

  if (to === 'string') {
    return fallbackStringConverter;
  }

  throw new Error('No conversion possible from ' + from + ' to ' + to + '.');
}









exports.convert = function(data, from, to, conversionContext) {
  if (from === to) {
    return promise.resolve(data);
  }
  return promise.resolve(getConverter(from, to).exec(data, conversionContext));
};




exports.items = [ viewDomConverter, viewStringConverter ];


});
















define('gcli/ui/fields', ['require', 'exports', 'module' , 'util/promise', 'util/util', 'gcli/argument'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var util = require('util/util');
var KeyEvent = require('util/util').KeyEvent;
var Argument = require('gcli/argument').Argument;

















function Field(type, options) {
  this.type = type;
  this.document = options.document;
  this.requisition = options.requisition;
}




Field.prototype.item = 'field';






Field.prototype.element = undefined;




Field.prototype.destroy = function() {
  delete this.messageElement;
};








Field.prototype.setConversion = function(conversion) {
  throw new Error('Field should not be used directly');
};





Field.prototype.getConversion = function() {
  throw new Error('Field should not be used directly');
};





Field.prototype.setMessageElement = function(element) {
  this.messageElement = element;
};




Field.prototype.setMessage = function(message) {
  if (this.messageElement) {
    util.setTextContent(this.messageElement, message || '');
  }
};





Field.prototype.onInputChange = function(ev) {
  promise.resolve(this.getConversion()).then(function(conversion) {
    this.onFieldChange({ conversion: conversion });
    this.setMessage(conversion.message);

    if (ev.keyCode === KeyEvent.DOM_VK_RETURN) {
      this.requisition.exec();
    }
  }.bind(this), util.errorHandler);
};





Field.prototype.isImportant = false;






Field.claim = function(type, context) {
  throw new Error('Field should not be used directly');
};









Field.TOOLTIP_MATCH = 5;   
Field.TOOLTIP_DEFAULT = 4; 
Field.MATCH = 3;           
Field.DEFAULT = 2;         
Field.BASIC = 1;           
Field.NO_MATCH = 0;        

exports.Field = Field;





var fieldCtors = [];





exports.addField = function(fieldCtor) {
  if (typeof fieldCtor !== 'function') {
    console.error('addField erroring on ', fieldCtor);
    throw new Error('addField requires a Field constructor');
  }
  fieldCtors.push(fieldCtor);
};






exports.removeField = function(field) {
  if (typeof field !== 'string') {
    fieldCtors = fieldCtors.filter(function(test) {
      return test !== field;
    });
  }
  else if (field instanceof Field) {
    exports.removeField(field.name);
  }
  else {
    console.error('removeField erroring on ', field);
    throw new Error('removeField requires an instance of Field');
  }
};














exports.getField = function(type, options) {
  var ctor;
  var highestClaim = -1;
  fieldCtors.forEach(function(fieldCtor) {
    var claim = fieldCtor.claim(type, options.requisition.executionContext);
    if (claim > highestClaim) {
      highestClaim = claim;
      ctor = fieldCtor;
    }
  });

  if (!ctor) {
    console.error('Unknown field type ', type, ' in ', fieldCtors);
    throw new Error('Can\'t find field for ' + type);
  }

  if (options.tooltip && highestClaim < Field.TOOLTIP_DEFAULT) {
    return new BlankField(type, options);
  }

  return new ctor(type, options);
};






function BlankField(type, options) {
  Field.call(this, type, options);

  this.element = util.createElement(this.document, 'div');

  this.onFieldChange = util.createEvent('BlankField.onFieldChange');
}

BlankField.prototype = Object.create(Field.prototype);

BlankField.claim = function(type, context) {
  return type.name === 'blank' ? Field.MATCH : Field.NO_MATCH;
};

BlankField.prototype.setConversion = function(conversion) {
  this.setMessage(conversion.message);
};

BlankField.prototype.getConversion = function() {
  return this.type.parse(new Argument(), this.requisition.executionContext);
};

exports.addField(BlankField);


});
















define('gcli/types/selection', ['require', 'exports', 'module' , 'util/promise', 'util/util', 'util/l10n', 'util/spell', 'gcli/types', 'gcli/argument'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var util = require('util/util');
var l10n = require('util/l10n');
var spell = require('util/spell');
var Type = require('gcli/types').Type;
var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;
var BlankArgument = require('gcli/argument').BlankArgument;

























function SelectionType(typeSpec) {
  if (typeSpec) {
    Object.keys(typeSpec).forEach(function(key) {
      this[key] = typeSpec[key];
    }, this);
  }
}

SelectionType.prototype = Object.create(Type.prototype);

SelectionType.prototype.stringify = function(value, context) {
  if (value == null) {
    return '';
  }
  if (this.stringifyProperty != null) {
    return value[this.stringifyProperty];
  }

  try {
    var name = null;
    var lookup = util.synchronize(this.getLookup(context));
    lookup.some(function(item) {
      if (item.value === value) {
        name = item.name;
        return true;
      }
      return false;
    }, this);
    return name;
  }
  catch (ex) {
    
    
    
    return value.toString();
  }
};





SelectionType.prototype.clearCache = function() {
  delete this._cachedLookup;
};






SelectionType.prototype.getLookup = function(context) {
  if (this._cachedLookup != null) {
    return this._cachedLookup;
  }

  var reply;
  if (this.lookup == null) {
    reply = resolve(this.data, context, this.neverForceAsync).then(dataToLookup);
  }
  else {
    var lookup = (typeof this.lookup === 'function') ?
            this.lookup.bind(this) :
            this.lookup;

    reply = resolve(lookup, context, this.neverForceAsync);
  }

  if (this.cacheable && !forceAsync) {
    this._cachedLookup = reply;
  }

  return reply;
};

var forceAsync = false;







function resolve(thing, context, neverForceAsync) {
  if (forceAsync && !neverForceAsync) {
    var deferred = promise.defer();
    setTimeout(function() {
      promise.resolve(thing).then(function(resolved) {
        if (typeof resolved === 'function') {
          resolved = resolve(resolved(), neverForceAsync);
        }

        deferred.resolve(resolved);
      });
    }, 500);
    return deferred.promise;
  }

  return promise.resolve(thing).then(function(resolved) {
    if (typeof resolved === 'function') {
      return resolve(resolved(context), context, neverForceAsync);
    }
    return resolved;
  });
}






function dataToLookup(data) {
  if (!Array.isArray(data)) {
    throw new Error('SelectionType has no lookup or data');
  }

  return data.map(function(option) {
    return { name: option, value: option };
  });
}






SelectionType.prototype._findPredictions = function(arg, context) {
  return promise.resolve(this.getLookup(context)).then(function(lookup) {
    var predictions = [];
    var i, option;
    var maxPredictions = Conversion.maxPredictions;
    var match = arg.text.toLowerCase();

    
    
    if (arg.suffix.length > 0) {
      for (i = 0; i < lookup.length && predictions.length < maxPredictions; i++) {
        option = lookup[i];
        if (option.name === arg.text) {
          predictions.push(option);
        }
      }

      return predictions;
    }

    
    for (i = 0; i < lookup.length; i++) {
      option = lookup[i];
      if (option._gcliLowerName == null) {
        option._gcliLowerName = option.name.toLowerCase();
      }
    }

    
    
    for (i = 0; i < lookup.length && predictions.length < maxPredictions; i++) {
      option = lookup[i];
      if (option.name === arg.text) {
        predictions.push(option);
      }
    }

    
    for (i = 0; i < lookup.length && predictions.length < maxPredictions; i++) {
      option = lookup[i];
      if (option._gcliLowerName.indexOf(match) === 0 && !option.value.hidden) {
        if (predictions.indexOf(option) === -1) {
          predictions.push(option);
        }
      }
    }

    
    if (predictions.length < (maxPredictions / 2)) {
      for (i = 0; i < lookup.length && predictions.length < maxPredictions; i++) {
        option = lookup[i];
        if (option._gcliLowerName.indexOf(match) !== -1 && !option.value.hidden) {
          if (predictions.indexOf(option) === -1) {
            predictions.push(option);
          }
        }
      }
    }

    
    if (predictions.length === 0) {
      var names = [];
      lookup.forEach(function(opt) {
        if (!opt.value.hidden) {
          names.push(opt.name);
        }
      });
      var corrected = spell.correct(match, names);
      if (corrected) {
        lookup.forEach(function(opt) {
          if (opt.name === corrected) {
            predictions.push(opt);
          }
        }, this);
      }
    }

    return predictions;
  }.bind(this));
};

SelectionType.prototype.parse = function(arg, context) {
  return this._findPredictions(arg, context).then(function(predictions) {
    if (predictions.length === 0) {
      var msg = l10n.lookupFormat('typesSelectionNomatch', [ arg.text ]);
      return new Conversion(undefined, arg, Status.ERROR, msg,
                            promise.resolve(predictions));
    }

    if (predictions[0].name === arg.text) {
      var value = predictions[0].value;
      return new Conversion(value, arg, Status.VALID, '',
                            promise.resolve(predictions));
    }

    return new Conversion(undefined, arg, Status.INCOMPLETE, '',
                          promise.resolve(predictions));
  }.bind(this));
};

SelectionType.prototype.getBlank = function(context) {
  var predictFunc = function() {
    return promise.resolve(this.getLookup(context)).then(function(lookup) {
      return lookup.filter(function(option) {
        return !option.value.hidden;
      }).slice(0, Conversion.maxPredictions - 1);
    });
  }.bind(this);

  return new Conversion(undefined, new BlankArgument(), Status.INCOMPLETE, '',
                        predictFunc);
};








SelectionType.prototype.decrement = function(value, context) {
  var lookup = util.synchronize(this.getLookup(context));
  var index = this._findValue(lookup, value);
  if (index === -1) {
    index = 0;
  }
  index++;
  if (index >= lookup.length) {
    index = 0;
  }
  return lookup[index].value;
};




SelectionType.prototype.increment = function(value, context) {
  var lookup = util.synchronize(this.getLookup(context));
  var index = this._findValue(lookup, value);
  if (index === -1) {
    
    
    
    index = 1;
  }
  index--;
  if (index < 0) {
    index = lookup.length - 1;
  }
  return lookup[index].value;
};









SelectionType.prototype._findValue = function(lookup, value) {
  var index = -1;
  for (var i = 0; i < lookup.length; i++) {
    var pair = lookup[i];
    if (pair.value === value) {
      index = i;
      break;
    }
  }
  return index;
};





SelectionType.prototype.isSelection = true;

SelectionType.prototype.name = 'selection';

exports.SelectionType = SelectionType;
exports.items = [ SelectionType ];


});
















define('util/spell', ['require', 'exports', 'module' ], function(require, exports, module) {

'use strict';





var CASE_CHANGE_COST = 1;
var INSERTION_COST = 10;
var DELETION_COST = 10;
var SWAP_COST = 10;
var SUBSTITUTION_COST = 20;
var MAX_EDIT_DISTANCE = 40;






var distance = exports.distance = function(wordi, wordj) {
  var wordiLen = wordi.length;
  var wordjLen = wordj.length;

  
  
  var row0 = new Array(wordiLen+1);
  var row1 = new Array(wordiLen+1);
  var row2 = new Array(wordiLen+1);
  var tmp;

  var i, j;

  
  
  for (i = 0; i <= wordiLen; i++) {
    row1[i] = i * INSERTION_COST;
  }

  
  
  for (j = 1; j <= wordjLen; j++) {
    
    
    row0[0] = j * INSERTION_COST;

    for (i = 1; i <= wordiLen; i++) {
      
      
      
      var dc = row0[i - 1] + DELETION_COST;
      var ic = row1[i] + INSERTION_COST;
      var sc0;
      if (wordi[i-1] === wordj[j-1]) {
        sc0 = 0;
      }
      else {
        if (wordi[i-1].toLowerCase() === wordj[j-1].toLowerCase()) {
          sc0 = CASE_CHANGE_COST;
        }
        else {
          sc0 = SUBSTITUTION_COST;
        }
      }
      var sc = row1[i-1] + sc0;

      row0[i] = Math.min(dc, ic, sc);

      
      
      if (i > 1 && j > 1 && wordi[i-1] === wordj[j-2] && wordj[j-1] === wordi[i-2]) {
        row0[i] = Math.min(row0[i], row2[i-2] + SWAP_COST);
      }
    }

    tmp = row2;
    row2 = row1;
    row1 = row0;
    row0 = tmp;
  }

  return row1[wordiLen];
};






var distancePrefix = exports.distancePrefix = function(word, name) {
  var dist = 0;

  for (var i = 0; i < word.length; i++) {
    if (name[i] !== word[i]) {
      if (name[i].toLowerCase() === word[i].toLowerCase()) {
        dist++;
      }
      else {
        
        
        return exports.distance(word, name);
      }
    }
  }

  return dist;
};




exports.correct = function(word, names) {
  if (names.length === 0) {
    return undefined;
  }

  var distances = {};
  var sortedCandidates;

  names.forEach(function(candidate) {
    distances[candidate] = exports.distance(word, candidate);
  });

  sortedCandidates = names.sort(function(worda, wordb) {
    if (distances[worda] !== distances[wordb]) {
      return distances[worda] - distances[wordb];
    }
    else {
      
      
      return worda < wordb;
    }
  });

  if (distances[sortedCandidates[0]] <= MAX_EDIT_DISTANCE) {
    return sortedCandidates[0];
  }
  else {
    return undefined;
  }
};





















exports.rank = function(word, names, options) {
  options = options || {};

  var reply = names.map(function(name) {
    
    
    var algo = options.prefixZero ? distancePrefix : distance;
    return {
      name: name,
      dist: algo(word, name)
    };
  });

  if (!options.noSort) {
    reply = reply.sort(function(d1, d2) {
      return d1.dist - d2.dist;
    });
  }

  return reply;
};


});
















define('gcli/types/delegate', ['require', 'exports', 'module' , 'util/promise', 'gcli/types'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var Conversion = require('gcli/types').Conversion;




var delegate = {
  item: 'type',
  name: 'delegate',

  constructor: function() {
    if (typeof this.delegateType !== 'function') {
      throw new Error('Instances of DelegateType need typeSpec.delegateType' +
                      ' to be a function that returns a type');
    }
  },

  
  
  
  delegateType: function(context) {
    throw new Error('Not implemented');
  },

  stringify: function(value, context) {
    return this.delegateType(context).stringify(value, context);
  },

  parse: function(arg, context) {
    return this.delegateType(context).parse(arg, context);
  },

  decrement: function(value, context) {
    var delegated = this.delegateType(context);
    return (delegated.decrement ? delegated.decrement(value, context) : undefined);
  },

  increment: function(value, context) {
    var delegated = this.delegateType(context);
    return (delegated.increment ? delegated.increment(value, context) : undefined);
  },

  getType: function(context) {
    return this.delegateType(context);
  },

  
  
  isDelegate: true,
};

Object.defineProperty(delegate, 'isImportant', {
  get: function() {
    return this.delegateType().isImportant;
  },
  enumerable: true
});





var blank = {
  item: 'type',
  name: 'blank',

  stringify: function(value, context) {
    return '';
  },

  parse: function(arg, context) {
    return promise.resolve(new Conversion(undefined, arg));
  }
};




exports.items = [ delegate, blank ];


});
















define('gcli/types/array', ['require', 'exports', 'module' , 'util/promise', 'gcli/types', 'gcli/argument'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var types = require('gcli/types');
var ArrayConversion = require('gcli/types').ArrayConversion;
var ArrayArgument = require('gcli/argument').ArrayArgument;

exports.items = [
  {
    
    item: 'type',
    name: 'array',
    subtype: undefined,

    constructor: function() {
      if (!this.subtype) {
        console.error('Array.typeSpec is missing subtype. Assuming string.' +
            this.name);
        this.subtype = 'string';
      }
      this.subtype = types.createType(this.subtype);
    },

    stringify: function(values, context) {
      if (values == null) {
        return '';
      }
      
      return values.join(' ');
    },

    parse: function(arg, context) {
      if (arg.type !== 'ArrayArgument') {
        console.error('non ArrayArgument to ArrayType.parse', arg);
        throw new Error('non ArrayArgument to ArrayType.parse');
      }

      
      
      
      
      var subArgParse = function(subArg) {
        return this.subtype.parse(subArg, context).then(function(conversion) {
          subArg.conversion = conversion;
          return conversion;
        }.bind(this));
      }.bind(this);

      var conversionPromises = arg.getArguments().map(subArgParse);
      return promise.all(conversionPromises).then(function(conversions) {
        return new ArrayConversion(conversions, arg);
      });
    },

    getBlank: function() {
      return new ArrayConversion([], new ArrayArgument());
    }
  },
];


});
















define('gcli/types/boolean', ['require', 'exports', 'module' , 'util/promise', 'gcli/types', 'gcli/types/selection', 'gcli/argument'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;
var SelectionType = require('gcli/types/selection').SelectionType;

var BlankArgument = require('gcli/argument').BlankArgument;

exports.items = [
  {
    
    item: 'type',
    name: 'boolean',
    parent: 'selection',

    lookup: [
      { name: 'false', value: false },
      { name: 'true', value: true }
    ],

    parse: function(arg, context) {
      if (arg.type === 'TrueNamedArgument') {
        return promise.resolve(new Conversion(true, arg));
      }
      if (arg.type === 'FalseNamedArgument') {
        return promise.resolve(new Conversion(false, arg));
      }
      return SelectionType.prototype.parse.call(this, arg, context);
    },

    stringify: function(value, context) {
      if (value == null) {
        return '';
      }
      return '' + value;
    },

    getBlank: function(context) {
      return new Conversion(false, new BlankArgument(), Status.VALID, '',
                            promise.resolve(this.lookup));
    }
  }
];


});
















define('gcli/types/command', ['require', 'exports', 'module' , 'util/promise', 'util/l10n', 'util/spell', 'gcli/canon', 'gcli/types/selection', 'gcli/types'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var l10n = require('util/l10n');
var spell = require('util/spell');
var canon = require('gcli/canon');
var SelectionType = require('gcli/types/selection').SelectionType;
var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;

exports.items = [
  {
    
    item: 'type',
    name: 'param',
    parent: 'selection',
    stringifyProperty: 'name',
    neverForceAsync: true,
    requisition: undefined,
    isIncompleteName: undefined,

    lookup: function() {
      var displayedParams = [];
      var command = this.requisition.commandAssignment.value;
      if (command != null) {
        command.params.forEach(function(param) {
          var arg = this.requisition.getAssignment(param.name).arg;
          if (!param.isPositionalAllowed && arg.type === 'BlankArgument') {
            displayedParams.push({ name: '--' + param.name, value: param });
          }
        }, this);
      }
      return displayedParams;
    },

    parse: function(arg, context) {
      if (this.isIncompleteName) {
        return SelectionType.prototype.parse.call(this, arg, context);
      }
      else {
        var message = l10n.lookup('cliUnusedArg');
        return promise.resolve(new Conversion(undefined, arg, Status.ERROR, message));
      }
    }
  },
  {
    
    
    
    
    
    item: 'type',
    name: 'command',
    parent: 'selection',
    stringifyProperty: 'name',
    neverForceAsync: true,
    allowNonExec: true,

    lookup: function() {
      var commands = canon.getCommands();
      commands.sort(function(c1, c2) {
        return c1.name.localeCompare(c2.name);
      });
      return commands.map(function(command) {
        return { name: command.name, value: command };
      }, this);
    },

    parse: function(arg, context) {
      
      
      
      var execWhereNeeded = function(command) {
        return this.allowNonExec || typeof command.exec === 'function';
      }.bind(this);

      var command = canon.getCommand(arg.text);

      
      
      var predictFunc = function() {
        return this._findPredictions(arg).then(function(predictions) {
          
          
          if (command && command.name === arg.text &&
              execWhereNeeded(command) && predictions.length === 1) {
            return [];
          }

          return predictions;
        }.bind(this));
      }.bind(this);

      if (command) {
        var status = execWhereNeeded(command) ? Status.VALID : Status.INCOMPLETE;
        var conversion = new Conversion(command, arg, status, '', predictFunc);
        return promise.resolve(conversion);
      }

      return this._findPredictions(arg).then(function(predictions) {
        if (predictions.length === 0) {
          var msg = l10n.lookupFormat('typesSelectionNomatch', [ arg.text ]);
          return new Conversion(undefined, arg, Status.ERROR, msg, predictFunc);
        }

        command = predictions[0].value;

        if (predictions.length === 1) {
          
          
          if (command.name === arg.text && execWhereNeeded(command)) {
            return new Conversion(command, arg, Status.VALID, '');
          }

          return new Conversion(undefined, arg, Status.INCOMPLETE, '', predictFunc);
        }

        
        if (predictions[0].name === arg.text) {
          return new Conversion(command, arg, Status.VALID, '', predictFunc);
        }

        return new Conversion(undefined, arg, Status.INCOMPLETE, '', predictFunc);
      }.bind(this));
    },

    _findPredictions: function(arg, context) {
      return promise.resolve(this.getLookup(context)).then(function(lookup) {
        var predictions = [];
        var i, option;
        var maxPredictions = Conversion.maxPredictions;
        var match = arg.text.toLowerCase();

        
        var addToPredictions = function(option) {
          if (arg.text.length === 0) {
            
            
            
            if (option.name.indexOf(' ') === -1) {
              predictions.push(option);
            }
          }
          else {
            
            
            
            if (option.value.exec != null) {
              predictions.push(option);
            }
          }
        };

        
        
        if (arg.suffix.match(/ +/)) {
          for (i = 0; i < lookup.length && predictions.length < maxPredictions; i++) {
            option = lookup[i];
            if (option.name === arg.text ||
                option.name.indexOf(arg.text + ' ') === 0) {
              addToPredictions(option);
            }
          }

          return predictions;
        }

        
        for (i = 0; i < lookup.length; i++) {
          option = lookup[i];
          if (option._gcliLowerName == null) {
            option._gcliLowerName = option.name.toLowerCase();
          }
        }

        
        
        for (i = 0; i < lookup.length && predictions.length < maxPredictions; i++) {
          option = lookup[i];
          if (option.name === arg.text) {
            addToPredictions(option);
          }
        }

        
        for (i = 0; i < lookup.length && predictions.length < maxPredictions; i++) {
          option = lookup[i];
          if (option._gcliLowerName.indexOf(match) === 0 && !option.value.hidden) {
            if (predictions.indexOf(option) === -1) {
              addToPredictions(option);
            }
          }
        }

        
        if (predictions.length < (maxPredictions / 2)) {
          for (i = 0; i < lookup.length && predictions.length < maxPredictions; i++) {
            option = lookup[i];
            if (option._gcliLowerName.indexOf(match) !== -1 && !option.value.hidden) {
              if (predictions.indexOf(option) === -1) {
                addToPredictions(option);
              }
            }
          }
        }

        
        if (predictions.length === 0) {
          var names = [];
          lookup.forEach(function(opt) {
            if (!opt.value.hidden) {
              names.push(opt.name);
            }
          });
          var corrected = spell.correct(match, names);
          if (corrected) {
            lookup.forEach(function(opt) {
              if (opt.name === corrected) {
                predictions.push(opt);
              }
            }, this);
          }
        }

        return predictions;
      }.bind(this));
    }
  }
];

});
















define('gcli/types/date', ['require', 'exports', 'module' , 'util/promise', 'util/l10n', 'gcli/types'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var l10n = require('util/l10n');
var Type = require('gcli/types').Type;
var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;


function DateType(typeSpec) {
  
  
  typeSpec = typeSpec || {};

  this._step = typeSpec.step || 1;
  this._min = new Date(-8640000000000000);
  this._max = new Date(8640000000000000);

  if (typeSpec.min != null) {
    if (typeof typeSpec.min === 'string') {
      this._min = toDate(typeSpec.min);
    }
    else if (isDate(typeSpec.min) || typeof typeSpec.min === 'function') {
      this._min = typeSpec.min;
    }
    else {
      throw new Error('date min value must be a string a date or a function');
    }
  }

  if (typeSpec.max != null) {
    if (typeof typeSpec.max === 'string') {
      this._max = toDate(typeSpec.max);
    }
    else if (isDate(typeSpec.max) || typeof typeSpec.max === 'function') {
      this._max = typeSpec.max;
    }
    else {
      throw new Error('date max value must be a string a date or a function');
    }
  }
}

DateType.prototype = Object.create(Type.prototype);





function pad(number) {
  var r = String(number);
  return r.length === 1 ? '0' + r : r;
}

DateType.prototype.stringify = function(value) {
  if (!isDate(value)) {
    return '';
  }

  var str = pad(value.getFullYear()) + '-' +
            pad(value.getMonth() + 1) + '-' +
            pad(value.getDate());

  
  if (value.getHours() !== 0 || value.getMinutes() !== 0 ||
      value.getSeconds() !== 0 || value.getMilliseconds() !== 0) {

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    str += ' ' + pad(value.getHours());
    str += ':' + pad(value.getMinutes());

    
    if (value.getSeconds() !== 0 || value.getMilliseconds() !== 0) {
      str += ':' + pad(value.getSeconds());
      if (value.getMilliseconds() !== 0) {
        str += '.' + String((value.getUTCMilliseconds()/1000).toFixed(3)).slice(2, 5);
      }
    }
  }

  return str;
};

DateType.prototype.getMin = function(context) {
  if (typeof this._min === 'function') {
    return this._min(context);
  }
  if (isDate(this._min)) {
    return this._min;
  }
  return undefined;
};

DateType.prototype.getMax = function(context) {
  if (typeof this._max === 'function') {
    return this._max(context);
  }
  if (isDate(this._max)) {
    return this._max;
  }
  return undefined;
};

DateType.prototype.parse = function(arg, context) {
  var value;

  if (arg.text.replace(/\s/g, '').length === 0) {
    return promise.resolve(new Conversion(undefined, arg, Status.INCOMPLETE, ''));
  }

  
  
  if (arg.text.toLowerCase() === 'now' || arg.text.toLowerCase() === 'today') {
    value = new Date();
  }
  else if (arg.text.toLowerCase() === 'yesterday') {
    value = new Date();
    value.setDate(value.getDate() - 1);
  }
  else if (arg.text.toLowerCase() === 'tomorrow') {
    value = new Date();
    value.setDate(value.getDate() + 1);
  }
  else {
    
    
    
    
    
    
    

    
    
    
    
    

    
    
    
    if (arg.text.indexOf('Z') !== -1) {
      value = new Date(arg.text);
    }
    else {
      
      
      value = new Date(arg.text.replace(/-/g, '/'));
    }

    if (isNaN(value.getTime())) {
      var msg = l10n.lookupFormat('typesDateNan', [ arg.text ]);
      return promise.resolve(new Conversion(undefined, arg, Status.ERROR, msg));
    }
  }

  return promise.resolve(new Conversion(value, arg));
};

DateType.prototype.decrement = function(value, context) {
  if (!isDate(value)) {
    return new Date();
  }

  var newValue = new Date(value);
  newValue.setDate(value.getDate() - this._step);

  if (newValue >= this.getMin(context)) {
    return newValue;
  }
  else {
    return this.getMin(context);
  }
};

DateType.prototype.increment = function(value, context) {
  if (!isDate(value)) {
    return new Date();
  }

  var newValue = new Date(value);
  newValue.setDate(value.getDate() + this._step);

  if (newValue <= this.getMax(context)) {
    return newValue;
  }
  else {
    return this.getMax();
  }
};

DateType.prototype.name = 'date';

exports.items = [ DateType ];






function toDate(str) {
  var millis = Date.parse(str);
  if (isNaN(millis)) {
    throw new Error(l10n.lookupFormat('typesDateNan', [ str ]));
  }
  return new Date(millis);
}





function isDate(thing) {
  return Object.prototype.toString.call(thing) === '[object Date]'
          && !isNaN(thing.getTime());
}


});
















define('gcli/types/file', ['require', 'exports', 'module' , 'gcli/types/fileparser', 'gcli/types'], function(require, exports, module) {

'use strict';






















var fileparser = require('gcli/types/fileparser');
var Conversion = require('gcli/types').Conversion;

exports.items = [
  {
    item: 'type',
    name: 'file',

    filetype: 'any',    
    existing: 'maybe',  
    matches: undefined, 

    isSelection: true,  

    constructor: function() {
      if (this.filetype !== 'any' && this.filetype !== 'file' &&
          this.filetype !== 'directory') {
        throw new Error('filetype must be one of [any|file|directory]');
      }

      if (this.existing !== 'yes' && this.existing !== 'no' &&
          this.existing !== 'maybe') {
        throw new Error('existing must be one of [yes|no|maybe]');
      }
    },

    stringify: function(file) {
      if (file == null) {
        return '';
      }

      return file.toString();
    },

    parse: function(arg, context) {
      var options = {
        filetype: this.filetype,
        existing: this.existing,
        matches: this.matches
      };
      var promise = fileparser.parse(arg.text, options);

      return promise.then(function(reply) {
        return new Conversion(reply.value, arg, reply.status,
                              reply.message, reply.predictor);
      });
    }
  }
];

});
















define('gcli/types/fileparser', ['require', 'exports', 'module' , 'util/fileparser'], function(require, exports, module) {

'use strict';

var fileparser = require('util/fileparser');

fileparser.supportsPredictions = false;
exports.parse = fileparser.parse;

});
















define('util/fileparser', ['require', 'exports', 'module' , 'util/util', 'util/l10n', 'util/spell', 'util/filesystem', 'gcli/types'], function(require, exports, module) {

'use strict';

var util = require('util/util');
var l10n = require('util/l10n');
var spell = require('util/spell');
var filesystem = require('util/filesystem');
var Status = require('gcli/types').Status;











exports.parse = function(typed, options) {
  return filesystem.stat(typed).then(function(stats) {
    
    if (options.existing === 'no' && stats.exists) {
      return {
        value: undefined,
        status: Status.INCOMPLETE,
        message: l10n.lookupFormat('fileErrExists', [ typed ]),
        predictor: undefined 
      };
    }

    if (stats.exists) {
      
      if (options.filetype === 'file' && !stats.isFile) {
        return {
          value: undefined,
          status: Status.INCOMPLETE,
          message: l10n.lookupFormat('fileErrIsNotFile', [ typed ]),
          predictor: getPredictor(typed, options)
        };
      }

      if (options.filetype === 'directory' && !stats.isDir) {
        return {
          value: undefined,
          status: Status.INCOMPLETE,
          message: l10n.lookupFormat('fileErrIsNotDirectory', [ typed ]),
          predictor: getPredictor(typed, options)
        };
      }

      
      if (options.matches != null && !options.matches.test(typed)) {
        return {
          value: undefined,
          status: Status.INCOMPLETE,
          message: l10n.lookupFormat('fileErrDoesntMatch',
                                     [ typed, options.source ]),
          predictor: getPredictor(typed, options)
        };
      }
    }
    else {
      if (options.existing === 'yes') {
        
        
        var parentName = filesystem.dirname(typed);
        return filesystem.stat(parentName).then(function(stats) {
          return {
            value: undefined,
            status: stats.isDir ? Status.INCOMPLETE : Status.ERROR,
            message: l10n.lookupFormat('fileErrNotExists', [ typed ]),
            predictor: getPredictor(typed, options)
          };
        });
      }
    }

    
    return {
      value: typed,
      status: Status.VALID,
      message: undefined,
      predictor: getPredictor(typed, options)
    };
  });
};

var RANK_OPTIONS = { noSort: true, prefixZero: true };




exports.supportsPredictions = true;





function getPredictor(typed, options) {
  if (!exports.supportsPredictions) {
    return undefined;
  }

  return function() {
    var allowFile = (options.filetype !== 'directory');
    var parts = filesystem.split(typed);

    var absolute = (typed.indexOf('/') === 0);
    var roots;
    if (absolute) {
      roots = [ { name: '/', dist: 0, original: '/' } ];
    }
    else {
      roots = history.getCommonDirectories().map(function(root) {
        return { name: root, dist: 0, original: root };
      });
    }

    
    
    
    var partsAdded = util.promiseEach(parts, function(part, index) {

      var partsSoFar = filesystem.join.apply(filesystem, parts.slice(0, index + 1));

      
      
      var allowFileForPart = (allowFile && index >= parts.length - 1);

      var rootsPromise = util.promiseEach(roots, function(root) {

        
        var matchFile = allowFileForPart ? options.matches : null;
        var promise = filesystem.ls(root.name, matchFile);

        var onSuccess = function(entries) {
          
          if (!allowFileForPart) {
            entries = entries.filter(function(entry) {
              return entry.isDir;
            });
          }
          var entryMap = {};
          entries.forEach(function(entry) {
            entryMap[entry.pathname] = entry;
          });
          return entryMap;
        };

        var onError = function(err) {
          
          
          
          var noComplainCodes = [ 'ENOTDIR', 'EACCES', 'EBADF', 'ENOENT' ];
          if (noComplainCodes.indexOf(err.code) === -1) {
            console.error('Error looing up', root.name, err);
          }
          return {};
        };

        promise = promise.then(onSuccess, onError);

        
        
        var compare = filesystem.join(root.original, partsSoFar);

        return promise.then(function(entryMap) {

          var ranks = spell.rank(compare, Object.keys(entryMap), RANK_OPTIONS);
          
          ranks.forEach(function(rank) {
            rank.original = root.original;
            rank.stats = entryMap[rank.name];
          });
          return ranks;
        });
      });

      return rootsPromise.then(function(data) {
        
        data = data.reduce(function(prev, curr) {
          return prev.concat(curr);
        }, []);

        data.sort(function(r1, r2) {
          return r1.dist - r2.dist;
        });

        
        
        
        
        
        
        
        var isLast = index >= parts.length - 1;
        var start = isLast ? 1 : 5;
        var end = isLast ? 7 : 10;

        var maxDeltaAt = start;
        var maxDelta = data[start].dist - data[start - 1].dist;

        for (var i = start + 1; i < end; i++) {
          var delta = data[i].dist - data[i - 1].dist;
          if (delta >= maxDelta) {
            maxDelta = delta;
            maxDeltaAt = i;
          }
        }

        
        roots = data.slice(0, maxDeltaAt);
      });
    });

    return partsAdded.then(function() {
      var predictions = roots.map(function(root) {
        var isFile = root.stats && root.stats.isFile;
        var isDir = root.stats && root.stats.isDir;

        var name = root.name;
        if (isDir && name.charAt(name.length) !== filesystem.sep) {
          name += filesystem.sep;
        }

        return {
          name: name,
          incomplete: !(allowFile && isFile),
          isFile: isFile,  
          dist: root.dist, 
        };
      });

      return util.promiseEach(predictions, function(prediction) {
        if (!prediction.isFile) {
          prediction.description = '(' + prediction.dist + ')';
          prediction.dist = undefined;
          prediction.isFile = undefined;
          return prediction;
        }

        return filesystem.describe(prediction.name).then(function(description) {
          prediction.description = description;
          prediction.dist = undefined;
          prediction.isFile = undefined;
          return prediction;
        });
      });
    });
  };
}









var history = {
  getCommonDirectories: function() {
    return [
      filesystem.sep,  
      filesystem.home  
    ];
  },
  addCommonDirectory: function(ignore) {
    
  }
};


});
















define('util/filesystem', ['require', 'exports', 'module' , 'util/promise'], function(require, exports, module) {

'use strict';

var OS = Components.utils.import('resource://gre/modules/osfile.jsm', {}).OS;
var promise = require('util/promise');







exports.join = OS.Path.join;
exports.sep = OS.Path.sep;
exports.dirname = OS.Path.dirname;

var dirService = Components.classes['@mozilla.org/file/directory_service;1']
                           .getService(Components.interfaces.nsIProperties);
exports.home = dirService.get('Home', Components.interfaces.nsIFile).path;

if ('winGetDrive' in OS.Path) {
  exports.sep = '\\';
}
else {
  exports.sep = '/';
}






exports.split = function(pathname) {
  return OS.Path.split(pathname).components;
};











exports.ls = function(pathname, matches) {
  var iterator = new OS.File.DirectoryIterator(pathname);
  var entries = [];

  var iteratePromise = iterator.forEach(function(entry) {
    entries.push({
      exists: true,
      isDir: entry.isDir,
      isFile: !entry.isFile,
      filename: entry.name,
      pathname: entry.path
    });
  });

  return iteratePromise.then(function onSuccess() {
      iterator.close();
      return entries;
    },
    function onFailure(reason) {
      iterator.close();
      throw reason;
    }
  );
};







exports.stat = function(pathname) {
  var onResolve = function(stats) {
    return {
      exists: true,
      isDir: stats.isDir,
      isFile: !stats.isFile
    };
  };

  var onReject = function(err) {
    if (err instanceof OS.File.Error && err.becauseNoSuchFile) {
      return {
        exists: false,
        isDir: false,
        isFile: false
      };
    }
    throw err;
  };

  return OS.File.stat(pathname).then(onResolve, onReject);
};





exports.describe = function(pathname) {
  return promise.resolve('');
};


});
















define('gcli/types/javascript', ['require', 'exports', 'module' , 'util/promise', 'util/l10n', 'gcli/types'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var l10n = require('util/l10n');
var types = require('gcli/types');

var Conversion = types.Conversion;
var Type = types.Type;
var Status = types.Status;





var globalObject;
if (typeof window !== 'undefined') {
  globalObject = window;
}




exports.setGlobalObject = function(obj) {
  globalObject = obj;
};





exports.getGlobalObject = function() {
  return globalObject;
};




exports.unsetGlobalObject = function() {
  globalObject = undefined;
};





function JavascriptType(typeSpec) {
}

JavascriptType.prototype = Object.create(Type.prototype);

JavascriptType.prototype.stringify = function(value, context) {
  if (value == null) {
    return '';
  }
  return value;
};





JavascriptType.MAX_COMPLETION_MATCHES = 10;

JavascriptType.prototype.parse = function(arg, context) {
  var typed = arg.text;
  var scope = globalObject;

  
  if (typed === '') {
    return promise.resolve(new Conversion(undefined, arg, Status.INCOMPLETE));
  }
  
  if (!isNaN(parseFloat(typed)) && isFinite(typed)) {
    return promise.resolve(new Conversion(typed, arg));
  }
  
  if (typed.trim().match(/(null|undefined|NaN|Infinity|true|false)/)) {
    return promise.resolve(new Conversion(typed, arg));
  }

  
  
  var beginning = this._findCompletionBeginning(typed);

  
  if (beginning.err) {
    return promise.resolve(new Conversion(typed, arg, Status.ERROR, beginning.err));
  }

  
  
  if (beginning.state === ParseState.COMPLEX) {
    return promise.resolve(new Conversion(typed, arg));
  }

  
  
  if (beginning.state !== ParseState.NORMAL) {
    return promise.resolve(new Conversion(typed, arg, Status.INCOMPLETE, ''));
  }

  var completionPart = typed.substring(beginning.startPos);
  var properties = completionPart.split('.');
  var matchProp;
  var prop;

  if (properties.length > 1) {
    matchProp = properties.pop().trimLeft();
    for (var i = 0; i < properties.length; i++) {
      prop = properties[i].trim();

      
      if (scope == null) {
        return promise.resolve(new Conversion(typed, arg, Status.ERROR,
                                        l10n.lookup('jstypeParseScope')));
      }

      if (prop === '') {
        return promise.resolve(new Conversion(typed, arg, Status.INCOMPLETE, ''));
      }

      
      
      if (this._isSafeProperty(scope, prop)) {
        return promise.resolve(new Conversion(typed, arg));
      }

      try {
        scope = scope[prop];
      }
      catch (ex) {
        
        
        
        return promise.resolve(new Conversion(typed, arg, Status.VALID, ''));
      }
    }
  }
  else {
    matchProp = properties[0].trimLeft();
  }

  
  
  if (prop && !prop.match(/^[0-9A-Za-z]*$/)) {
    return promise.resolve(new Conversion(typed, arg));
  }

  
  if (scope == null) {
    var msg = l10n.lookupFormat('jstypeParseMissing', [ prop ]);
    return promise.resolve(new Conversion(typed, arg, Status.ERROR, msg));
  }

  
  
  if (!matchProp.match(/^[0-9A-Za-z]*$/)) {
    return promise.resolve(new Conversion(typed, arg));
  }

  
  if (this._isIteratorOrGenerator(scope)) {
    return promise.resolve(new Conversion(typed, arg));
  }

  var matchLen = matchProp.length;
  var prefix = matchLen === 0 ? typed : typed.slice(0, -matchLen);
  var status = Status.INCOMPLETE;
  var message = '';

  
  
  var matches = {};

  
  
  
  
  var distUpPrototypeChain = 0;
  var root = scope;
  try {
    while (root != null &&
        Object.keys(matches).length < JavascriptType.MAX_COMPLETION_MATCHES) {

      Object.keys(root).forEach(function(property) {
        
        
        
        if (property.indexOf(matchProp) === 0 && !(property in matches)) {
          matches[property] = {
            prop: property,
            distUpPrototypeChain: distUpPrototypeChain
          };
        }
      });

      distUpPrototypeChain++;
      root = Object.getPrototypeOf(root);
    }
  }
  catch (ex) {
    return promise.resolve(new Conversion(typed, arg, Status.INCOMPLETE, ''));
  }

  
  
  matches = Object.keys(matches).map(function(property) {
    if (property === matchProp) {
      status = Status.VALID;
    }
    return matches[property];
  });

  
  
  
  
  matches.sort(function(m1, m2) {
    if (m1.distUpPrototypeChain !== m2.distUpPrototypeChain) {
      return m1.distUpPrototypeChain - m2.distUpPrototypeChain;
    }
    
    return isVendorPrefixed(m1.prop) ?
      (isVendorPrefixed(m2.prop) ? m1.prop.localeCompare(m2.prop) : 1) :
      (isVendorPrefixed(m2.prop) ? -1 : m1.prop.localeCompare(m2.prop));
  });

  
  
  
  
  if (matches.length > JavascriptType.MAX_COMPLETION_MATCHES) {
    matches = matches.slice(0, JavascriptType.MAX_COMPLETION_MATCHES - 1);
  }

  
  
  
  
  
  var predictions = matches.map(function(match) {
    var description;
    var incomplete = true;

    if (this._isSafeProperty(scope, match.prop)) {
      description = '(property getter)';
    }
    else {
      try {
        var value = scope[match.prop];

        if (typeof value === 'function') {
          description = '(function)';
        }
        else if (typeof value === 'boolean' || typeof value === 'number') {
          description = '= ' + value;
          incomplete = false;
        }
        else if (typeof value === 'string') {
          if (value.length > 40) {
            value = value.substring(0, 37) + 'â€¦';
          }
          description = '= \'' + value + '\'';
          incomplete = false;
        }
        else {
          description = '(' + typeof value + ')';
        }
      }
      catch (ex) {
        description = '(' + l10n.lookup('jstypeParseError') + ')';
      }
    }

    return {
      name: prefix + match.prop,
      value: {
        name: prefix + match.prop,
        description: description
      },
      description: description,
      incomplete: incomplete
    };
  }, this);

  if (predictions.length === 0) {
    status = Status.ERROR;
    message = l10n.lookupFormat('jstypeParseMissing', [ matchProp ]);
  }

  
  if (predictions.length === 1 && status === Status.VALID) {
    predictions = [];
  }

  return promise.resolve(new Conversion(typed, arg, status, message,
                                  promise.resolve(predictions)));
};





function isVendorPrefixed(name) {
  return name.indexOf('moz') === 0 ||
         name.indexOf('webkit') === 0 ||
         name.indexOf('ms') === 0;
}




var ParseState = {
  



  NORMAL: 0,

  



  COMPLEX: 1,

  


  QUOTE: 2,

  


  DQUOTE: 3
};

var OPEN_BODY = '{[('.split('');
var CLOSE_BODY = '}])'.split('');
var OPEN_CLOSE_BODY = {
  '{': '}',
  '[': ']',
  '(': ')'
};





var simpleChars = /[a-zA-Z0-9.]/;













JavascriptType.prototype._findCompletionBeginning = function(text) {
  var bodyStack = [];

  var state = ParseState.NORMAL;
  var start = 0;
  var c;
  var complex = false;

  for (var i = 0; i < text.length; i++) {
    c = text[i];
    if (!simpleChars.test(c)) {
      complex = true;
    }

    switch (state) {
      
      case ParseState.NORMAL:
        if (c === '"') {
          state = ParseState.DQUOTE;
        }
        else if (c === '\'') {
          state = ParseState.QUOTE;
        }
        else if (c === ';') {
          start = i + 1;
        }
        else if (c === ' ') {
          start = i + 1;
        }
        else if (OPEN_BODY.indexOf(c) != -1) {
          bodyStack.push({
            token: c,
            start: start
          });
          start = i + 1;
        }
        else if (CLOSE_BODY.indexOf(c) != -1) {
          var last = bodyStack.pop();
          if (!last || OPEN_CLOSE_BODY[last.token] != c) {
            return { err: l10n.lookup('jstypeBeginSyntax') };
          }
          if (c === '}') {
            start = i + 1;
          }
          else {
            start = last.start;
          }
        }
        break;

      
      case ParseState.DQUOTE:
        if (c === '\\') {
          i ++;
        }
        else if (c === '\n') {
          return { err: l10n.lookup('jstypeBeginUnterm') };
        }
        else if (c === '"') {
          state = ParseState.NORMAL;
        }
        break;

      
      case ParseState.QUOTE:
        if (c === '\\') {
          i ++;
        }
        else if (c === '\n') {
          return { err: l10n.lookup('jstypeBeginUnterm') };
        }
        else if (c === '\'') {
          state = ParseState.NORMAL;
        }
        break;
    }
  }

  if (state === ParseState.NORMAL && complex) {
    state = ParseState.COMPLEX;
  }

  return {
    state: state,
    startPos: start
  };
};






JavascriptType.prototype._isIteratorOrGenerator = function(obj) {
  if (obj === null) {
    return false;
  }

  if (typeof aObject === 'object') {
    if (typeof obj.__iterator__ === 'function' ||
        obj.constructor && obj.constructor.name === 'Iterator') {
      return true;
    }

    try {
      var str = obj.toString();
      if (typeof obj.next === 'function' &&
          str.indexOf('[object Generator') === 0) {
        return true;
      }
    }
    catch (ex) {
      
      return false;
    }
  }

  return false;
};








JavascriptType.prototype._isSafeProperty = function(scope, prop) {
  if (typeof scope !== 'object') {
    return false;
  }

  
  
  var propDesc;
  while (scope) {
    try {
      propDesc = Object.getOwnPropertyDescriptor(scope, prop);
      if (propDesc) {
        break;
      }
    }
    catch (ex) {
      
      if (ex.name === 'NS_ERROR_XPC_BAD_CONVERT_JS' ||
          ex.name === 'NS_ERROR_XPC_BAD_OP_ON_WN_PROTO') {
        return false;
      }
      return true;
    }
    scope = Object.getPrototypeOf(scope);
  }

  if (!propDesc) {
    return false;
  }

  if (!propDesc.get) {
    return false;
  }

  
  
  return typeof propDesc.get !== 'function' || 'prototype' in propDesc.get;
};

JavascriptType.prototype.name = 'javascript';

exports.items = [ JavascriptType ];


});
















define('gcli/types/node', ['require', 'exports', 'module' , 'util/promise', 'util/host', 'util/l10n', 'util/util', 'gcli/types', 'gcli/argument'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var Highlighter = require('util/host').Highlighter;
var l10n = require('util/l10n');
var util = require('util/util');
var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;
var BlankArgument = require('gcli/argument').BlankArgument;






var doc;
if (typeof document !== 'undefined') {
  doc = document;
}







exports._empty = [];




exports.setDocument = function(document) {
  doc = document;
  if (doc != null) {
    exports._empty = util.createEmptyNodeList(doc);
  }
};




exports.unsetDocument = function() {
  doc = undefined;
  exports._empty = undefined;
};





exports.getDocument = function() {
  return doc;
};





function onEnter(assignment) {
  assignment.highlighter = new Highlighter(doc);
  assignment.highlighter.nodelist = assignment.conversion.matches;
}


function onLeave(assignment) {
  if (!assignment.highlighter) {
    return;
  }

  assignment.highlighter.destroy();
  delete assignment.highlighter;
}

function onChange(assignment) {
  if (assignment.conversion.matches == null) {
    return;
  }
  if (!assignment.highlighter) {
    return;
  }

  assignment.highlighter.nodelist = assignment.conversion.matches;
}




exports.items = [
  {
    
    item: 'type',
    name: 'node',

    stringify: function(value, context) {
      if (value == null) {
        return '';
      }
      return value.__gcliQuery || 'Error';
    },

    parse: function(arg, context) {
      var reply;

      if (arg.text === '') {
        reply = new Conversion(undefined, arg, Status.INCOMPLETE);
        reply.matches = util.createEmptyNodeList(doc);
      }
      else {
        var nodes;
        try {
          nodes = doc.querySelectorAll(arg.text);
          if (nodes.length === 0) {
            reply = new Conversion(undefined, arg, Status.INCOMPLETE,
                                   l10n.lookup('nodeParseNone'));
          }
          else if (nodes.length === 1) {
            var node = nodes.item(0);
            node.__gcliQuery = arg.text;

            reply = new Conversion(node, arg, Status.VALID, '');
          }
          else {
            var msg = l10n.lookupFormat('nodeParseMultiple', [ nodes.length ]);
            reply = new Conversion(undefined, arg, Status.ERROR, msg);
          }

          reply.matches = nodes;
        }
        catch (ex) {
          reply = new Conversion(undefined, arg, Status.ERROR,
                                 l10n.lookup('nodeParseSyntax'));
        }
      }

      return promise.resolve(reply);
    },

    onEnter: onEnter,
    onLeave: onLeave,
    onChange: onChange
  },
  {
    
    item: 'type',
    name: 'nodelist',

    
    
    
    
    
    
    
    
    allowEmpty: false,

    constructor: function() {
      if (typeof this.allowEmpty !== 'boolean') {
        throw new Error('Legal values for allowEmpty are [true|false]');
      }
    },

    getBlank: function(context) {
      return new Conversion(exports._empty, new BlankArgument(), Status.VALID);
    },

    stringify: function(value, context) {
      if (value == null) {
        return '';
      }
      return value.__gcliQuery || 'Error';
    },

    parse: function(arg, context) {
      var reply;
      try {
        if (arg.text === '') {
          reply = new Conversion(undefined, arg, Status.INCOMPLETE);
          reply.matches = util.createEmptyNodeList(doc);
        }
        else {
          var nodes = doc.querySelectorAll(arg.text);

          if (nodes.length === 0 && !this.allowEmpty) {
            reply = new Conversion(undefined, arg, Status.INCOMPLETE,
                                   l10n.lookup('nodeParseNone'));
          }
          else {
            reply = new Conversion(nodes, arg, Status.VALID, '');
          }

          reply.matches = nodes;
        }
      }
      catch (ex) {
        reply = new Conversion(undefined, arg, Status.ERROR,
                               l10n.lookup('nodeParseSyntax'));
        reply.matches = util.createEmptyNodeList(doc);
      }

      return promise.resolve(reply);
    },

    onEnter: onEnter,
    onLeave: onLeave,
    onChange: onChange
  }
];

});
















define('util/host', ['require', 'exports', 'module' , 'util/util'], function(require, exports, module) {

'use strict';

var util = require('util/util');





exports.chromeWindow = undefined;

function Highlighter(document) {
  this._document = document;
  this._nodes = util.createEmptyNodeList(this._document);
}

Object.defineProperty(Highlighter.prototype, 'nodelist', {
  set: function(nodes) {
    Array.prototype.forEach.call(this._nodes, this._unhighlightNode, this);
    this._nodes = (nodes == null) ?
        util.createEmptyNodeList(this._document) :
        nodes;
    Array.prototype.forEach.call(this._nodes, this._highlightNode, this);
  },
  get: function() {
    return this._nodes;
  },
  enumerable: true
});

Highlighter.prototype.destroy = function() {
  this.nodelist = null;
};

Highlighter.prototype._highlightNode = function(node) {
  
};

Highlighter.prototype._unhighlightNode = function(node) {
  
};

exports.Highlighter = Highlighter;





exports.exec = function(execSpec) {
  throw new Error('Not supported');
};


});
















define('gcli/types/number', ['require', 'exports', 'module' , 'util/promise', 'util/l10n', 'gcli/types'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var l10n = require('util/l10n');
var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;

exports.items = [
  {
    
    
    
    item: 'type',
    name: 'number',
    allowFloat: false,
    max: undefined,
    min: undefined,
    step: 1,

    constructor: function() {
      if (!this.allowFloat &&
          (this._isFloat(this.min) ||
           this._isFloat(this.max) ||
           this._isFloat(this.step))) {
        throw new Error('allowFloat is false, but non-integer values given in type spec');
      }
    },

    stringify: function(value, context) {
      if (value == null) {
        return '';
      }
      return '' + value;
    },

    getMin: function(context) {
      if (this.min) {
        if (typeof this.min === 'function') {
          return this.min(context);
        }
        if (typeof this.min === 'number') {
          return this.min;
        }
      }
      return undefined;
    },

    getMax: function(context) {
      if (this.max) {
        if (typeof this.max === 'function') {
          return this.max(context);
        }
        if (typeof this.max === 'number') {
          return this.max;
        }
      }
      return undefined;
    },

    parse: function(arg, context) {
      var msg;
      if (arg.text.replace(/^\s*-?/, '').length === 0) {
        return promise.resolve(new Conversion(undefined, arg, Status.INCOMPLETE, ''));
      }

      if (!this.allowFloat && (arg.text.indexOf('.') !== -1)) {
        msg = l10n.lookupFormat('typesNumberNotInt2', [ arg.text ]);
        return promise.resolve(new Conversion(undefined, arg, Status.ERROR, msg));
      }

      var value;
      if (this.allowFloat) {
        value = parseFloat(arg.text);
      }
      else {
        value = parseInt(arg.text, 10);
      }

      if (isNaN(value)) {
        msg = l10n.lookupFormat('typesNumberNan', [ arg.text ]);
        return promise.resolve(new Conversion(undefined, arg, Status.ERROR, msg));
      }

      var max = this.getMax(context);
      if (max != null && value > max) {
        msg = l10n.lookupFormat('typesNumberMax', [ value, max ]);
        return promise.resolve(new Conversion(undefined, arg, Status.ERROR, msg));
      }

      var min = this.getMin(context);
      if (min != null && value < min) {
        msg = l10n.lookupFormat('typesNumberMin', [ value, min ]);
        return promise.resolve(new Conversion(undefined, arg, Status.ERROR, msg));
      }

      return promise.resolve(new Conversion(value, arg));
    },

    decrement: function(value, context) {
      if (typeof value !== 'number' || isNaN(value)) {
        return this.getMax(context) || 1;
      }
      var newValue = value - this.step;
      
      newValue = Math.ceil(newValue / this.step) * this.step;
      return this._boundsCheck(newValue, context);
    },

    increment: function(value, context) {
      if (typeof value !== 'number' || isNaN(value)) {
        var min = this.getMin(context);
        return min != null ? min : 0;
      }
      var newValue = value + this.step;
      
      newValue = Math.floor(newValue / this.step) * this.step;
      if (this.getMax(context) == null) {
        return newValue;
      }
      return this._boundsCheck(newValue, context);
    },

    
    
    
    _boundsCheck: function(value, context) {
      var min = this.getMin(context);
      if (min != null && value < min) {
        return min;
      }
      var max = this.getMax(context);
      if (max != null && value > max) {
        return max;
      }
      return value;
    },

    
    
    _isFloat: function(value) {
      return ((typeof value === 'number') && isFinite(value) && (value % 1 !== 0));
    }
  }
];


});
















define('gcli/types/resource', ['require', 'exports', 'module' , 'util/promise', 'gcli/types/selection'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var SelectionType = require('gcli/types/selection').SelectionType;


exports.clearResourceCache = function() {
  ResourceCache.clear();
};





var doc;
if (typeof document !== 'undefined') {
  doc = document;
}




exports.setDocument = function(document) {
  doc = document;
};




exports.unsetDocument = function() {
  ResourceCache.clear();
  doc = undefined;
};





exports.getDocument = function() {
  return doc;
};







function Resource(name, type, inline, element) {
  this.name = name;
  this.type = type;
  this.inline = inline;
  this.element = element;
}





Resource.prototype.getContents = function() {
  throw new Error('not implemented');
};

Resource.TYPE_SCRIPT = 'text/javascript';
Resource.TYPE_CSS = 'text/css';





function CssResource(domSheet) {
  this.name = domSheet.href;
  if (!this.name) {
    this.name = domSheet.ownerNode.id ?
            'css#' + domSheet.ownerNode.id :
            'inline-css';
  }

  this.inline = (domSheet.href == null);
  this.type = Resource.TYPE_CSS;
  this.element = domSheet;
}

CssResource.prototype = Object.create(Resource.prototype);

CssResource.prototype.loadContents = function(callback) {
  callback(this.element.ownerNode.innerHTML);
};

CssResource._getAllStyles = function() {
  var resources = [];
  if (doc == null) {
    return resources;
  }

  Array.prototype.forEach.call(doc.styleSheets, function(domSheet) {
    CssResource._getStyle(domSheet, resources);
  });

  dedupe(resources, function(clones) {
    for (var i = 0; i < clones.length; i++) {
      clones[i].name = clones[i].name + '-' + i;
    }
  });

  return resources;
};

CssResource._getStyle = function(domSheet, resources) {
  var resource = ResourceCache.get(domSheet);
  if (!resource) {
    resource = new CssResource(domSheet);
    ResourceCache.add(domSheet, resource);
  }
  resources.push(resource);

  
  try {
    Array.prototype.forEach.call(domSheet.cssRules, function(domRule) {
      if (domRule.type == CSSRule.IMPORT_RULE && domRule.styleSheet) {
        CssResource._getStyle(domRule.styleSheet, resources);
      }
    }, this);
  }
  catch (ex) {
    
  }
};





function ScriptResource(scriptNode) {
  this.name = scriptNode.src;
  if (!this.name) {
    this.name = scriptNode.id ?
            'script#' + scriptNode.id :
            'inline-script';
  }

  this.inline = (scriptNode.src === '' || scriptNode.src == null);
  this.type = Resource.TYPE_SCRIPT;
  this.element = scriptNode;
}

ScriptResource.prototype = Object.create(Resource.prototype);

ScriptResource.prototype.loadContents = function(callback) {
  if (this.inline) {
    callback(this.element.innerHTML);
  }
  else {
    
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
      if (xhr.readyState !== xhr.DONE) {
        return;
      }
      callback(xhr.responseText);
    };
    xhr.open('GET', this.element.src, true);
    xhr.send();
  }
};

ScriptResource._getAllScripts = function() {
  if (doc == null) {
    return [];
  }

  var scriptNodes = doc.querySelectorAll('script');
  var resources = Array.prototype.map.call(scriptNodes, function(scriptNode) {
    var resource = ResourceCache.get(scriptNode);
    if (!resource) {
      resource = new ScriptResource(scriptNode);
      ResourceCache.add(scriptNode, resource);
    }
    return resource;
  });

  dedupe(resources, function(clones) {
    for (var i = 0; i < clones.length; i++) {
      clones[i].name = clones[i].name + '-' + i;
    }
  });

  return resources;
};




function dedupe(resources, onDupe) {
  
  var names = {};
  resources.forEach(function(scriptResource) {
    if (names[scriptResource.name] == null) {
      names[scriptResource.name] = [];
    }
    names[scriptResource.name].push(scriptResource);
  });

  
  Object.keys(names).forEach(function(name) {
    var clones = names[name];
    if (clones.length > 1) {
      onDupe(clones);
    }
  });
}







var ResourceCache = {
  _cached: [],

  


  get: function(node) {
    for (var i = 0; i < ResourceCache._cached.length; i++) {
      if (ResourceCache._cached[i].node === node) {
        return ResourceCache._cached[i].resource;
      }
    }
    return null;
  },

  


  add: function(node, resource) {
    ResourceCache._cached.push({ node: node, resource: resource });
  },

  


  clear: function() {
    ResourceCache._cached = [];
  }
};




exports.items = [
  {
    item: 'type',
    constructor: function() {
      if (this.include !== Resource.TYPE_SCRIPT &&
          this.include !== Resource.TYPE_CSS &&
          this.include != null) {
        throw new Error('invalid include property: ' + this.include);
      }
    },
    name: 'resource',
    parent: 'selection',
    include: null,
    cacheable: false,
    lookup: function() {
      var resources = [];
      if (this.include !== Resource.TYPE_SCRIPT) {
        Array.prototype.push.apply(resources, CssResource._getAllStyles());
      }
      if (this.include !== Resource.TYPE_CSS) {
        Array.prototype.push.apply(resources, ScriptResource._getAllScripts());
      }

      return promise.resolve(resources.map(function(resource) {
        return { name: resource.name, value: resource };
      }));
    }
  }
];

});
















define('gcli/types/setting', ['require', 'exports', 'module' , 'gcli/settings', 'gcli/types'], function(require, exports, module) {

'use strict';

var settings = require('gcli/settings');
var types = require('gcli/types');

exports.items = [
  {
    
    item: 'type',
    name: 'setting',
    parent: 'selection',
    cacheable: true,
    constructor: function() {
      settings.onChange.add(function(ev) {
        this.clearCache();
      }, this);
    },
    lookup: function() {
      return settings.getAll().map(function(setting) {
        return { name: setting.name, value: setting };
      });
    }
  },
  {
    
    
    
    
    item: 'type',
    name: 'settingValue',
    parent: 'delegate',
    settingParamName: 'setting',
    delegateType: function(context) {
      if (context != null) {
        var setting = context.getArgsObject()[this.settingParamName];
        if (setting != null) {
          return setting.type;
        }
      }

      return types.createType('blank');
    }
  }
];

});
















define('gcli/types/string', ['require', 'exports', 'module' , 'util/promise', 'gcli/types'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;

exports.items = [
  {
    
    
    
    
    
    
    
    
    
    item: 'type',
    name: 'string',
    allowBlank: false,

    stringify: function(value, context) {
      if (value == null) {
        return '';
      }

      return value
           .replace(/\\/g, '\\\\')
           .replace(/\f/g, '\\f')
           .replace(/\n/g, '\\n')
           .replace(/\r/g, '\\r')
           .replace(/\t/g, '\\t')
           .replace(/\v/g, '\\v')
           .replace(/\n/g, '\\n')
           .replace(/\r/g, '\\r')
           .replace(/ /g, '\\ ')
           .replace(/'/g, '\\\'')
           .replace(/"/g, '\\"')
           .replace(/{/g, '\\{')
           .replace(/}/g, '\\}');
    },

    parse:function(arg, context) {
      if (!this.allowBlank && (arg.text == null || arg.text === '')) {
        return promise.resolve(new Conversion(undefined, arg, Status.INCOMPLETE, ''));
      }

      
      
      
      
      var value = arg.text
           .replace(/\\\\/g, '\uF000')
           .replace(/\\f/g, '\f')
           .replace(/\\n/g, '\n')
           .replace(/\\r/g, '\r')
           .replace(/\\t/g, '\t')
           .replace(/\\v/g, '\v')
           .replace(/\\n/g, '\n')
           .replace(/\\r/g, '\r')
           .replace(/\\ /g, ' ')
           .replace(/\\'/g, '\'')
           .replace(/\\"/g, '"')
           .replace(/\\{/g, '{')
           .replace(/\\}/g, '}')
           .replace(/\uF000/g, '\\');

      return promise.resolve(new Conversion(value, arg));
    }
  }
];


});
















define('gcli/converters/basic', ['require', 'exports', 'module' , 'util/util'], function(require, exports, module) {

'use strict';

var util = require('util/util');




function nodeFromDataToString(data, conversionContext) {
  var node = util.createElement(conversionContext.document, 'p');
  node.textContent = data.toString();
  return node;
}

exports.items = [
  {
    item: 'converter',
    from: 'string',
    to: 'dom',
    exec: nodeFromDataToString
  },
  {
    item: 'converter',
    from: 'number',
    to: 'dom',
    exec: nodeFromDataToString
  },
  {
    item: 'converter',
    from: 'boolean',
    to: 'dom',
    exec: nodeFromDataToString
  },
  {
    item: 'converter',
    from: 'undefined',
    to: 'dom',
    exec: function(data, conversionContext) {
      return util.createElement(conversionContext.document, 'span');
    }
  },
  {
    item: 'converter',
    from: 'error',
    to: 'dom',
    exec: function(ex, conversionContext) {
      var node = util.createElement(conversionContext.document, 'p');
      node.className = 'gcli-error';
      node.textContent = ex;
      return node;
    }
  },
  {
    item: 'converter',
    from: 'error',
    to: 'string',
    exec: function(ex, conversionContext) {
      return '' + ex;
    }
  }
];

});
















define('gcli/converters/terminal', ['require', 'exports', 'module' , 'util/util'], function(require, exports, module) {

'use strict';

var util = require('util/util');





exports.items = [
  {
    item: 'converter',
    from: 'terminal',
    to: 'dom',
    createTextArea: function(text, conversionContext) {
      var node = util.createElement(conversionContext.document, 'textarea');
      node.classList.add('gcli-row-subterminal');
      node.readOnly = true;
      node.textContent = text;
      return node;
    },
    exec: function(data, conversionContext) {
      if (Array.isArray(data)) {
        var node = util.createElement(conversionContext.document, 'div');
        data.forEach(function(member) {
          node.appendChild(this.createTextArea(member, conversionContext));
        });
        return node;
      }
      return this.createTextArea(data);
    }
  },
  {
    item: 'converter',
    from: 'terminal',
    to: 'string',
    exec: function(data, conversionContext) {
      return Array.isArray(data) ? data.join('') : '' + data;
    }
  }
];


});
















define('gcli/ui/intro', ['require', 'exports', 'module' , 'util/l10n', 'gcli/settings', 'gcli/ui/view', 'gcli/cli', 'text!gcli/ui/intro.html'], function(require, exports, module) {

'use strict';

var l10n = require('util/l10n');
var settings = require('gcli/settings');
var view = require('gcli/ui/view');
var Output = require('gcli/cli').Output;




exports.items = [
  {
    item: 'setting',
    name: 'hideIntro',
    type: 'boolean',
    description: l10n.lookup('hideIntroDesc'),
    defaultValue: false
  }
];




exports.maybeShowIntro = function(commandOutputManager, conversionContext) {
  var hideIntro = settings.getSetting('hideIntro');
  if (hideIntro.value) {
    return;
  }

  var output = new Output();
  output.type = 'view';
  commandOutputManager.onOutput({ output: output });

  var viewData = this.createView(null, conversionContext, output);

  output.complete({ isTypedData: true, type: 'view', data: viewData });
};




exports.createView = function(ignore, conversionContext, output) {
  return view.createView({
    html: require('text!gcli/ui/intro.html'),
    options: { stack: 'intro.html' },
    data: {
      l10n: l10n.propertyLookup,
      onclick: conversionContext.update,
      ondblclick: conversionContext.updateExec,
      showHideButton: (output != null),
      onGotIt: function(ev) {
        var hideIntro = settings.getSetting('hideIntro');
        hideIntro.value = true;
        output.onClose();
      }
    }
  });
};

});
















define('gcli/ui/view', ['require', 'exports', 'module' , 'util/util', 'util/domtemplate'], function(require, exports, module) {

'use strict';

var util = require('util/util');
var domtemplate = require('util/domtemplate');























exports.createView = function(options) {
  if (options.html == null) {
    throw new Error('options.html is missing');
  }

  return {
    


    isView: true,

    




    appendTo: function(element, clear) {
      
      
      
      if (clear === true) {
        util.clearElement(element);
      }

      element.appendChild(this.toDom(element.ownerDocument));
    },

    




    toDom: function(document) {
      if (options.css) {
        util.importCss(options.css, document, options.cssId);
      }

      var child = util.toDom(document, options.html);
      domtemplate.template(child, options.data || {}, options.options || {});
      return child;
    }
  };
};


});
















define('util/domtemplate', ['require', 'exports', 'module' ], function(require, exports, module) {

  'use strict';

  var obj = {};
  Components.utils.import('resource://gre/modules/devtools/Templater.jsm', obj);
  exports.template = obj.template;

});
















define('gcli/cli', ['require', 'exports', 'module' , 'util/promise', 'util/util', 'util/l10n', 'gcli/ui/view', 'gcli/converters', 'gcli/canon', 'gcli/types', 'gcli/argument'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var util = require('util/util');
var l10n = require('util/l10n');

var view = require('gcli/ui/view');
var converters = require('gcli/converters');
var canon = require('gcli/canon');
var CommandOutputManager = require('gcli/canon').CommandOutputManager;

var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;

var Argument = require('gcli/argument').Argument;
var ArrayArgument = require('gcli/argument').ArrayArgument;
var NamedArgument = require('gcli/argument').NamedArgument;
var TrueNamedArgument = require('gcli/argument').TrueNamedArgument;
var MergedArgument = require('gcli/argument').MergedArgument;
var ScriptArgument = require('gcli/argument').ScriptArgument;




function getEvalCommand() {
  if (getEvalCommand._cmd == null) {
    getEvalCommand._cmd = canon.getCommand(evalCmd.name);
  }
  return getEvalCommand._cmd;
}













function Assignment(param, paramIndex) {
  
  this.param = param;

  this.conversion = undefined;

  
  
  
  this.paramIndex = paramIndex;
}






Object.defineProperty(Assignment.prototype, 'arg', {
  get: function() {
    return this.conversion == null ? undefined : this.conversion.arg;
  },
  enumerable: true
});






Object.defineProperty(Assignment.prototype, 'value', {
  get: function() {
    return this.conversion == null ? undefined : this.conversion.value;
  },
  enumerable: true
});




Object.defineProperty(Assignment.prototype, 'message', {
  get: function() {
    return this.conversion == null || !this.conversion.message ?
        '' : this.conversion.message;
  },
  enumerable: true
});






Assignment.prototype.getPredictions = function() {
  return this.conversion == null ? [] : this.conversion.getPredictions();
};









Assignment.prototype.getPredictionAt = function(index) {
  if (index == null) {
    index = 0;
  }

  if (this.isInName()) {
    return promise.resolve(undefined);
  }

  return this.getPredictions().then(function(predictions) {
    if (predictions.length === 0) {
      return undefined;
    }

    index = index % predictions.length;
    if (index < 0) {
      index = predictions.length + index;
    }
    return predictions[index];
  }.bind(this));
};








Assignment.prototype.isInName = function() {
  return this.conversion.arg.type === 'NamedArgument' &&
         this.conversion.arg.prefix.slice(-1) !== ' ';
};








Assignment.prototype.getStatus = function(arg) {
  if (this.param.isDataRequired && !this.conversion.isDataProvided()) {
    return Status.INCOMPLETE;
  }

  
  
  
  if (!this.param.isDataRequired && this.arg.type === 'BlankArgument') {
    return Status.VALID;
  }

  return this.conversion.getStatus(arg);
};




Assignment.prototype.toString = function() {
  return this.conversion.toString();
};






Object.defineProperty(Assignment.prototype, '_summaryJson', {
  get: function() {
    var predictionCount = '<async>';
    this.getPredictions().then(function(predictions) {
      predictionCount = predictions.length;
    }, console.log);
    return {
      param: this.param.name + '/' + this.param.type.name,
      defaultValue: this.param.defaultValue,
      arg: this.conversion.arg._summaryJson,
      value: this.value,
      message: this.message,
      status: this.getStatus().toString(),
      predictionCount: predictionCount
    };
  },
  enumerable: true
});

exports.Assignment = Assignment;





var customEval = eval;





exports.setEvalFunction = function(newCustomEval) {
  customEval = newCustomEval;
};











exports.unsetEvalFunction = function() {
  customEval = undefined;
};




var evalCmd = {
  item: 'command',
  name: '{',
  params: [
    {
      name: 'javascript',
      type: 'javascript',
      description: ''
    }
  ],
  hidden: true,
  returnType: 'object',
  description: { key: 'cliEvalJavascript' },
  exec: function(args, context) {
    return customEval(args.javascript);
  },
  isCommandRegexp: /^\s*{\s*/
};

exports.items = [ evalCmd ];




function CommandAssignment() {
  var commandParamMetadata = {
    name: '__command',
    type: { name: 'command', allowNonExec: false }
  };
  
  
  
  var self = this;
  Object.defineProperty(commandParamMetadata, 'description', {
    get: function() {
      var value = self.value;
      return value && value.description ?
          value.description :
          'The command to execute';
    },
    enumerable: true
  });
  this.param = new canon.Parameter(commandParamMetadata);
  this.paramIndex = -1;
}

CommandAssignment.prototype = Object.create(Assignment.prototype);

CommandAssignment.prototype.getStatus = function(arg) {
  return Status.combine(
    Assignment.prototype.getStatus.call(this, arg),
    this.conversion.value && this.conversion.value.exec ?
            Status.VALID : Status.INCOMPLETE
  );
};

exports.CommandAssignment = CommandAssignment;





function UnassignedAssignment(requisition, arg) {
  this.param = new canon.Parameter({
    name: '__unassigned',
    description: l10n.lookup('cliOptions'),
    type: {
      name: 'param',
      requisition: requisition,
      isIncompleteName: (arg.text.charAt(0) === '-')
    }
  });
  this.paramIndex = -1;

  
  var parsed = this.param.type.parse(arg, requisition.executionContext);
  this.conversion = util.synchronize(parsed);
  this.conversion.assignment = this;
}

UnassignedAssignment.prototype = Object.create(Assignment.prototype);

UnassignedAssignment.prototype.getStatus = function(arg) {
  return this.conversion.getStatus();
};

exports.logErrors = true;



























function Requisition(environment, doc, commandOutputManager) {
  this.environment = environment;
  this.document = doc;
  if (this.document == null) {
    try {
      this.document = document;
    }
    catch (ex) {
      
    }
  }

  this.commandOutputManager = commandOutputManager || new CommandOutputManager();
  this.shell = {
    cwd: '/', 
    env: {}   
  };

  this.onTextChange = util.createEvent('Requisition.onTextChange');

  
  
  this.commandAssignment = new CommandAssignment();
  var assignPromise = this.setAssignment(this.commandAssignment, null,
                                   { internal: true });
  util.synchronize(assignPromise);

  
  
  
  
  
  
  this._assignments = {};

  
  this.assignmentCount = 0;

  
  this._args = [];

  
  this._unassigned = [];

  
  
  this._nextUpdateId = 0;

  
  
  this.prefix = '';
}




Requisition.prototype.destroy = function() {
  delete this.document;
  delete this.environment;
};








Requisition.prototype._beginChange = function() {
  this.onTextChange.holdFire();

  var updateId = this._nextUpdateId;
  this._nextUpdateId++;
  return updateId;
};








Requisition.prototype._isChangeCurrent = function(updateId) {
  return updateId + 1 === this._nextUpdateId;
};




Requisition.prototype._endChangeCheckOrder = function(updateId) {
  this.onTextChange.resumeFire();

  if (updateId + 1 !== this._nextUpdateId) {
    
    
    return false;
  }

  return true;
};

var legacy = false;




Object.defineProperty(Requisition.prototype, 'executionContext', {
  get: function() {
    if (this._executionContext == null) {
      this._executionContext = {
        defer: function() {
          return promise.defer();
        },
        typedData: function(type, data) {
          return {
            isTypedData: true,
            data: data,
            type: type
          };
        },
        getArgsObject: this.getArgsObject.bind(this)
      };

      
      var requisition = this;
      Object.defineProperty(this._executionContext, 'typed', {
        get: function() { return requisition.toString(); },
        enumerable: true
      });
      Object.defineProperty(this._executionContext, 'environment', {
        get: function() { return requisition.environment; },
        enumerable: true
      });
      Object.defineProperty(this._executionContext, 'shell', {
        get: function() { return requisition.shell; },
        enumerable : true
      });

      



      Object.defineProperty(this._executionContext, '__dlhjshfw', {
        get: function() { return requisition; },
        enumerable: false
      });

      if (legacy) {
        this._executionContext.createView = view.createView;
        this._executionContext.exec = this.exec.bind(this);
        this._executionContext.update = this.update.bind(this);
        this._executionContext.updateExec = this.updateExec.bind(this);

        Object.defineProperty(this._executionContext, 'document', {
          get: function() { return requisition.document; },
          enumerable: true
        });
      }
    }

    return this._executionContext;
  },
  enumerable: true
});




Object.defineProperty(Requisition.prototype, 'conversionContext', {
  get: function() {
    if (this._conversionContext == null) {
      this._conversionContext = {
        defer: function() {
          return promise.defer();
        },

        createView: view.createView,
        exec: this.exec.bind(this),
        update: this.update.bind(this),
        updateExec: this.updateExec.bind(this)
      };

      
      var requisition = this;

      Object.defineProperty(this._conversionContext, 'document', {
        get: function() { return requisition.document; },
        enumerable: true
      });
      Object.defineProperty(this._conversionContext, 'environment', {
        get: function() { return requisition.environment; },
        enumerable: true
      });

      



      Object.defineProperty(this._conversionContext, '__dlhjshfw', {
        get: function() { return requisition; },
        enumerable: false
      });
    }

    return this._conversionContext;
  },
  enumerable: true
});






Requisition.prototype.getAssignment = function(nameOrNumber) {
  var name = (typeof nameOrNumber === 'string') ?
    nameOrNumber :
    Object.keys(this._assignments)[nameOrNumber];
  return this._assignments[name] || undefined;
};









Requisition.prototype._getFirstBlankPositionalAssignment = function() {
  var reply = null;
  Object.keys(this._assignments).some(function(name) {
    var assignment = this.getAssignment(name);
    if (assignment.arg.type === 'BlankArgument' &&
            assignment.param.isPositionalAllowed) {
      reply = assignment;
      return true; 
    }
    return false;
  }, this);
  return reply;
};




Requisition.prototype.getParameterNames = function() {
  return Object.keys(this._assignments);
};






Requisition.prototype.cloneAssignments = function() {
  return Object.keys(this._assignments).map(function(name) {
    return this._assignments[name];
  }, this);
};








Object.defineProperty(Requisition.prototype, 'status', {
  get : function() {
    var status = Status.VALID;
    if (this._unassigned.length !== 0) {
      var isAllIncomplete = true;
      this._unassigned.forEach(function(assignment) {
        if (!assignment.param.type.isIncompleteName) {
          isAllIncomplete = false;
        }
      });
      status = isAllIncomplete ? Status.INCOMPLETE : Status.ERROR;
    }

    this.getAssignments(true).forEach(function(assignment) {
      var assignStatus = assignment.getStatus();
      if (assignStatus > status) {
        status = assignStatus;
      }
    }, this);
    if (status === Status.INCOMPLETE) {
      status = Status.ERROR;
    }
    return status;
  },
  enumerable : true
});








Requisition.prototype.getStatusMessage = function() {
  if (this.commandAssignment.getStatus() !== Status.VALID) {
    return l10n.lookup('cliUnknownCommand');
  }

  var assignments = this.getAssignments();
  for (var i = 0; i < assignments.length; i++) {
    if (assignments[i].getStatus() !== Status.VALID) {
      return assignments[i].message;
    }
  }

  if (this._unassigned.length !== 0) {
    return l10n.lookup('cliUnusedArg');
  }

  return null;
};





Requisition.prototype.getArgsObject = function() {
  var args = {};
  this.getAssignments().forEach(function(assignment) {
    args[assignment.param.name] = assignment.conversion.isDataProvided() ?
            assignment.value :
            assignment.param.defaultValue;
  }, this);
  return args;
};







Requisition.prototype.getAssignments = function(includeCommand) {
  var assignments = [];
  if (includeCommand === true) {
    assignments.push(this.commandAssignment);
  }
  Object.keys(this._assignments).forEach(function(name) {
    assignments.push(this.getAssignment(name));
  }, this);
  return assignments;
};





Requisition.prototype._setAssignmentInternal = function(assignment, conversion) {
  var oldConversion = assignment.conversion;

  assignment.conversion = conversion;
  assignment.conversion.assignment = assignment;

  
  if (assignment.conversion.equals(oldConversion)) {
    if (assignment === this.commandAssignment) {
      this.setBlankArguments();
    }
    return;
  }

  
  if (assignment === this.commandAssignment) {
    this._assignments = {};

    var command = this.commandAssignment.value;
    if (command) {
      for (var i = 0; i < command.params.length; i++) {
        var param = command.params[i];
        var newAssignment = new Assignment(param, i);
        var assignPromise = this.setAssignment(newAssignment, null, { internal: true });
        util.synchronize(assignPromise);

        this._assignments[param.name] = newAssignment;
      }
    }
    this.assignmentCount = Object.keys(this._assignments).length;
  }

  
  if (!assignment.conversion.argEquals(oldConversion)) {
    this.onTextChange();
  }
};


















Requisition.prototype.setAssignment = function(assignment, arg, options) {
  options = options || {};
  if (!options.internal) {
    var originalArgs = assignment.arg.getArgs();

    
    var replacementArgs = arg.getArgs();
    var maxLen = Math.max(originalArgs.length, replacementArgs.length);
    for (var i = 0; i < maxLen; i++) {
      
      
      if (i >= originalArgs.length || originalArgs[i].type === 'BlankArgument') {
        this._args.push(replacementArgs[i]);
        continue;
      }

      var index = this._args.indexOf(originalArgs[i]);
      if (index === -1) {
        console.error('Couldn\'t find ', originalArgs[i], ' in ', this._args);
        throw new Error('Couldn\'t find ' + originalArgs[i]);
      }

      
      
      if (i >= replacementArgs.length) {
        this._args.splice(index, 1);
      }
      else {
        if (options.matchPadding) {
          if (replacementArgs[i].prefix.length === 0 &&
              this._args[index].prefix.length !== 0) {
            replacementArgs[i].prefix = this._args[index].prefix;
          }
          if (replacementArgs[i].suffix.length === 0 &&
              this._args[index].suffix.length !== 0) {
            replacementArgs[i].suffix = this._args[index].suffix;
          }
        }
        this._args[index] = replacementArgs[i];
      }
    }
  }

  var updateId = options.internal ? null : this._beginChange();

  var setAssignmentInternal = function(conversion) {
    if (options.internal || this._endChangeCheckOrder(updateId)) {
      this._setAssignmentInternal(assignment, conversion);
    }

    return promise.resolve(undefined);
  }.bind(this);

  if (arg == null) {
    var blank = assignment.param.type.getBlank(this.executionContext);
    return setAssignmentInternal(blank);
  }

  if (typeof arg.getStatus === 'function') {
    
    return setAssignmentInternal(arg);
  }

  var parsed = assignment.param.type.parse(arg, this.executionContext);
  return parsed.then(setAssignmentInternal);
};




Requisition.prototype.setBlankArguments = function() {
  this.getAssignments().forEach(function(assignment) {
    var assignPromise = this.setAssignment(assignment, null, { internal: true });
    util.synchronize(assignPromise);
  }, this);
};



















Requisition.prototype.complete = function(cursor, predictionChoice) {
  var assignment = this.getAssignmentAt(cursor.start);

  var predictionPromise = assignment.getPredictionAt(predictionChoice);
  return predictionPromise.then(function(prediction) {
    var outstanding = [];
    this.onTextChange.holdFire();

    
    
    

    if (prediction == null) {
      
      
      
      
      
      
      if (assignment.arg.suffix.slice(-1) !== ' ' &&
              assignment.getStatus() === Status.VALID) {
        outstanding.push(this._addSpace(assignment));
      }

      
      
      
      if (assignment.isInName()) {
        var newArg = assignment.arg.beget({ prefixPostSpace: true });
        outstanding.push(this.setAssignment(assignment, newArg));
      }
    }
    else {
      
      var arg = assignment.arg.beget({
        text: prediction.name,
        dontQuote: (assignment === this.commandAssignment)
      });
      var assignPromise = this.setAssignment(assignment, arg);

      if (!prediction.incomplete) {
        assignPromise = assignPromise.then(function() {
          
          return this._addSpace(assignment).then(function() {
            
            if (assignment instanceof UnassignedAssignment) {
              return this.update(this.toString());
            }
          }.bind(this));
        }.bind(this));
      }

      outstanding.push(assignPromise);
    }

    return promise.all(outstanding).then(function() {
      this.onTextChange();
      this.onTextChange.resumeFire();
      return true;
    }.bind(this));
  }.bind(this));
};




Requisition.prototype._assertArgsAssigned = function() {
  this._args.forEach(function(arg) {
    if (arg.assignment == null) {
      console.log('No assignment for ' + arg);
    }
  }, this);
};






Requisition.prototype._addSpace = function(assignment) {
  var arg = assignment.arg.beget({ suffixSpace: true });
  if (arg !== assignment.arg) {
    return this.setAssignment(assignment, arg);
  }
  else {
    return promise.resolve(undefined);
  }
};




Requisition.prototype.decrement = function(assignment) {
  var replacement = assignment.param.type.decrement(assignment.value,
                                                    this.executionContext);
  if (replacement != null) {
    var str = assignment.param.type.stringify(replacement,
                                              this.executionContext);
    var arg = assignment.arg.beget({ text: str });
    var assignPromise = this.setAssignment(assignment, arg);
    util.synchronize(assignPromise);
  }
};




Requisition.prototype.increment = function(assignment) {
  var replacement = assignment.param.type.increment(assignment.value,
                                                    this.executionContext);
  if (replacement != null) {
    var str = assignment.param.type.stringify(replacement,
                                              this.executionContext);
    var arg = assignment.arg.beget({ text: str });
    var assignPromise = this.setAssignment(assignment, arg);
    util.synchronize(assignPromise);
  }
};




Requisition.prototype.toCanonicalString = function() {
  var line = [];

  var cmd = this.commandAssignment.value ?
      this.commandAssignment.value.name :
      this.commandAssignment.arg.text;
  line.push(cmd);

  Object.keys(this._assignments).forEach(function(name) {
    var assignment = this._assignments[name];
    var type = assignment.param.type;
    
    
    
    if (assignment.value !== assignment.param.defaultValue) {
      line.push(' ');
      line.push(type.stringify(assignment.value, this.executionContext));
    }
  }, this);

  
  if (cmd === '{') {
    if (this.getAssignment(0).arg.suffix.indexOf('}') === -1) {
      line.push(' }');
    }
  }

  return line.join('');
};





















Requisition.prototype.createInputArgTrace = function() {
  if (!this._args) {
    throw new Error('createInputMap requires a command line. See source.');
    
    
  }

  var args = [];
  var i;
  this._args.forEach(function(arg) {
    for (i = 0; i < arg.prefix.length; i++) {
      args.push({ arg: arg, character: arg.prefix[i], part: 'prefix' });
    }
    for (i = 0; i < arg.text.length; i++) {
      args.push({ arg: arg, character: arg.text[i], part: 'text' });
    }
    for (i = 0; i < arg.suffix.length; i++) {
      args.push({ arg: arg, character: arg.suffix[i], part: 'suffix' });
    }
  });

  return args;
};




Requisition.prototype.toString = function() {
  if (this._args) {
    return this._args.map(function(arg) {
      return arg.toString();
    }).join('');
  }

  return this.toCanonicalString();
};











Requisition.prototype.typedEndsWithSeparator = function() {
  
  
  if (this._args) {
    var lastArg = this._args.slice(-1)[0];
    if (lastArg.suffix.slice(-1) === ' ') {
      return true;
    }
    return lastArg.text === '' && lastArg.suffix === ''
        && lastArg.prefix.slice(-1) === ' ';
  }

  return this.toCanonicalString().slice(-1) === ' ';
};










Requisition.prototype.getInputStatusMarkup = function(cursor) {
  var argTraces = this.createInputArgTrace();
  
  
  cursor = cursor === 0 ? 0 : cursor - 1;
  var cTrace = argTraces[cursor];

  var markup = [];
  for (var i = 0; i < argTraces.length; i++) {
    var argTrace = argTraces[i];
    var arg = argTrace.arg;
    var status = Status.VALID;
    if (argTrace.part === 'text') {
      status = arg.assignment.getStatus(arg);
      
      if (status === Status.INCOMPLETE) {
        
        
        
        
        
        
        var isNamed = (cTrace.arg.assignment.arg.type === 'NamedArgument');
        var isInside = cTrace.part === 'text' ||
                        (isNamed && cTrace.part === 'suffix');
        if (arg.assignment !== cTrace.arg.assignment || !isInside) {
          
          if (!(arg.assignment instanceof CommandAssignment)) {
            status = Status.ERROR;
          }
        }
      }
    }

    markup.push({ status: status, string: argTrace.character });
  }

  
  i = 0;
  while (i < markup.length - 1) {
    if (markup[i].status === markup[i + 1].status) {
      markup[i].string += markup[i + 1].string;
      markup.splice(i + 1, 1);
    }
    else {
      i++;
    }
  }

  return markup;
};






Requisition.prototype.getAssignmentAt = function(cursor) {
  if (!this._args) {
    console.trace();
    throw new Error('Missing args');
  }

  
  
  if (cursor === 0) {
    return this.commandAssignment;
  }

  var assignForPos = [];
  var i, j;
  for (i = 0; i < this._args.length; i++) {
    var arg = this._args[i];
    var assignment = arg.assignment;

    
    for (j = 0; j < arg.prefix.length; j++) {
      assignForPos.push(assignment);
    }
    for (j = 0; j < arg.text.length; j++) {
      assignForPos.push(assignment);
    }

    
    
    if (arg.assignment.arg.type === 'NamedArgument') {
      
    }
    else if (this._args.length > i + 1) {
      
      assignment = this._args[i + 1].assignment;
    }
    else {
      
      var nextAssignment = this._getFirstBlankPositionalAssignment();
      if (nextAssignment != null) {
        assignment = nextAssignment;
      }
    }

    for (j = 0; j < arg.suffix.length; j++) {
      assignForPos.push(assignment);
    }
  }

  
  

  var reply = assignForPos[cursor - 1];

  if (!reply) {
    throw new Error('Missing assignment.' +
        ' cursor=' + cursor + ' text=' + this.toString());
  }

  return reply;
};











Requisition.prototype.exec = function(options) {
  var command = null;
  var args = null;
  var hidden = false;

  if (options) {
    if (options.hidden) {
      hidden = true;
    }

    if (options.command != null) {
      
      
      command = canon.getCommand(options.command);
      if (!command) {
        console.error('Command not found: ' + options.command);
      }
      args = options.args;
    }
  }

  if (!command) {
    command = this.commandAssignment.value;
    args = this.getArgsObject();
  }

  
  var typed = this.toString();
  if (evalCmd.isCommandRegexp.test(typed)) {
    typed = typed.replace(evalCmd.isCommandRegexp, '');
    
    typed = typed.replace(/\s*}\s*$/, '');
  }

  var output = new Output({
    command: command,
    args: args,
    typed: typed,
    canonical: this.toCanonicalString(),
    hidden: hidden
  });

  this.commandOutputManager.onOutput({ output: output });

  var onDone = function(data) {
    output.complete(data, false);
    return output;
  };

  var onError = function(ex) {
    if (exports.logErrors) {
      util.errorHandler(ex);
    }

    var data = ex.isTypedData ? ex : {
      isTypedData: true,
      data: ex,
      type: 'error'
    };
    output.complete(data, true);
    return output;
  };

  if (this.status !== Status.VALID) {
    var ex = new Error(this.getStatusMessage());
    return promise.resolve(onError(ex)).then(function(output) {
      this.clear();
      return output;
    });
  }
  else {
    try {
      var reply = command.exec(args, this.executionContext);
      return promise.resolve(reply).then(onDone, onError);
    }
    catch (ex) {
      return promise.resolve(onError(ex));
    }
    finally {
      this.clear();
    }
  }
};







Requisition.prototype.updateExec = function(input, options) {
  return this.update(input).then(function() {
    return this.exec(options);
  }.bind(this));
};




Requisition.prototype.clear = function() {
  this.onTextChange.holdFire();

  var arg = new Argument('', '', '');
  this._args = [ arg ];

  var commandType = this.commandAssignment.param.type;
  var parsePromise = commandType.parse(arg, this.executionContext);
  this.setAssignment(this.commandAssignment,
                     util.synchronize(parsePromise),
                     { internal: true });

  this.onTextChange.resumeFire();
  this.onTextChange();
};




function getDataCommandAttribute(element) {
  var command = element.getAttribute('data-command');
  if (!command) {
    command = element.querySelector('*[data-command]')
                     .getAttribute('data-command');
  }
  return command;
}





Requisition.prototype.update = function(typed) {
  if (typeof HTMLElement !== 'undefined' && typed instanceof HTMLElement) {
    typed = getDataCommandAttribute(typed);
  }
  if (typeof Event !== 'undefined' && typed instanceof Event) {
    typed = getDataCommandAttribute(typed.currentTarget);
  }

  var updateId = this._beginChange();

  this._args = exports.tokenize(typed);
  var args = this._args.slice(0); 

  return this._split(args).then(function() {
    if (!this._isChangeCurrent(updateId)) {
      return false;
    }

    return this._assign(args).then(function() {
      if (this._endChangeCheckOrder(updateId)) {
        this.onTextChange();
        return true;
      }

      return false;
    }.bind(this));
  }.bind(this));
};






Object.defineProperty(Requisition.prototype, '_summaryJson', {
  get: function() {
    var summary = {
      $args: this._args.map(function(arg) {
        return arg._summaryJson;
      }),
      _command: this.commandAssignment._summaryJson,
      _unassigned: this._unassigned.forEach(function(assignment) {
        return assignment._summaryJson;
      })
    };

    Object.keys(this._assignments).forEach(function(name) {
      summary[name] = this.getAssignment(name)._summaryJson;
    }.bind(this));

    return summary;
  },
  enumerable: true
});




var In = {
  





  WHITESPACE: 1,

  




  SIMPLE: 2,

  



  SINGLE_Q: 3,

  



  DOUBLE_Q: 4,

  












  SCRIPT: 5
};








exports.tokenize = function(typed) {
  
  if (typed == null || typed.length === 0) {
    return [ new Argument('', '', '') ];
  }

  if (isSimple(typed)) {
    return [ new Argument(typed, '', '') ];
  }

  var mode = In.WHITESPACE;

  
  
  
  typed = typed
      .replace(/\\\\/g, '\uF000')
      .replace(/\\ /g, '\uF001')
      .replace(/\\'/g, '\uF002')
      .replace(/\\"/g, '\uF003')
      .replace(/\\{/g, '\uF004')
      .replace(/\\}/g, '\uF005');

  function unescape2(escaped) {
    return escaped
        .replace(/\uF000/g, '\\\\')
        .replace(/\uF001/g, '\\ ')
        .replace(/\uF002/g, '\\\'')
        .replace(/\uF003/g, '\\\"')
        .replace(/\uF004/g, '\\{')
        .replace(/\uF005/g, '\\}');
  }

  var i = 0;          
  var start = 0;      
  var prefix = '';    
  var args = [];      
  var blockDepth = 0; 

  
  
  

  while (true) {
    var c = typed[i];
    var str;
    switch (mode) {
      case In.WHITESPACE:
        if (c === '\'') {
          prefix = typed.substring(start, i + 1);
          mode = In.SINGLE_Q;
          start = i + 1;
        }
        else if (c === '"') {
          prefix = typed.substring(start, i + 1);
          mode = In.DOUBLE_Q;
          start = i + 1;
        }
        else if (c === '{') {
          prefix = typed.substring(start, i + 1);
          mode = In.SCRIPT;
          blockDepth++;
          start = i + 1;
        }
        else if (/ /.test(c)) {
          
        }
        else {
          prefix = typed.substring(start, i);
          mode = In.SIMPLE;
          start = i;
        }
        break;

      case In.SIMPLE:
        
        
        if (c === ' ') {
          str = unescape2(typed.substring(start, i));
          args.push(new Argument(str, prefix, ''));
          mode = In.WHITESPACE;
          start = i;
          prefix = '';
        }
        break;

      case In.SINGLE_Q:
        if (c === '\'') {
          str = unescape2(typed.substring(start, i));
          args.push(new Argument(str, prefix, c));
          mode = In.WHITESPACE;
          start = i + 1;
          prefix = '';
        }
        break;

      case In.DOUBLE_Q:
        if (c === '"') {
          str = unescape2(typed.substring(start, i));
          args.push(new Argument(str, prefix, c));
          mode = In.WHITESPACE;
          start = i + 1;
          prefix = '';
        }
        break;

      case In.SCRIPT:
        if (c === '{') {
          blockDepth++;
        }
        else if (c === '}') {
          blockDepth--;
          if (blockDepth === 0) {
            str = unescape2(typed.substring(start, i));
            args.push(new ScriptArgument(str, prefix, c));
            mode = In.WHITESPACE;
            start = i + 1;
            prefix = '';
          }
        }
        break;
    }

    i++;

    if (i >= typed.length) {
      
      if (mode === In.WHITESPACE) {
        if (i !== start) {
          
          
          var extra = typed.substring(start, i);
          var lastArg = args[args.length - 1];
          if (!lastArg) {
            args.push(new Argument('', extra, ''));
          }
          else {
            lastArg.suffix += extra;
          }
        }
      }
      else if (mode === In.SCRIPT) {
        str = unescape2(typed.substring(start, i + 1));
        args.push(new ScriptArgument(str, prefix, ''));
      }
      else {
        str = unescape2(typed.substring(start, i + 1));
        args.push(new Argument(str, prefix, ''));
      }
      break;
    }
  }

  return args;
};





function isSimple(typed) {
   for (var i = 0; i < typed.length; i++) {
     var c = typed.charAt(i);
     if (c === ' ' || c === '"' || c === '\'' ||
         c === '{' || c === '}' || c === '\\') {
       return false;
     }
   }
   return true;
}





Requisition.prototype._split = function(args) {
  
  
  var noArgUp = { internal: true };

  
  
  
  var conversion;
  if (args[0].type === 'ScriptArgument') {
    
    
    conversion = new Conversion(getEvalCommand(), new ScriptArgument());
    return this.setAssignment(this.commandAssignment, conversion, noArgUp);
  }

  var argsUsed = 1;

  var parsePromise;
  var commandType = this.commandAssignment.param.type;
  while (argsUsed <= args.length) {
    var arg = (argsUsed === 1) ?
              args[0] :
              new MergedArgument(args, 0, argsUsed);

    
    

    if (this.prefix != null && this.prefix !== '') {
      var prefixArg = new Argument(this.prefix, '', ' ');
      var prefixedArg = new MergedArgument([ prefixArg, arg ]);

      parsePromise = commandType.parse(prefixedArg, this.executionContext);
      conversion = util.synchronize(parsePromise);

      if (conversion.value == null) {
        parsePromise = commandType.parse(arg, this.executionContext);
        conversion = util.synchronize(parsePromise);
      }
    }
    else {
      parsePromise = commandType.parse(arg, this.executionContext);
      conversion = util.synchronize(parsePromise);
    }

    
    
    
    if (!conversion.value || conversion.value.exec) {
      break;
    }

    
    
    
    

    argsUsed++;
  }

  
  for (var i = 0; i < argsUsed; i++) {
    args.shift();
  }

  
  
  

  return this.setAssignment(this.commandAssignment, conversion, noArgUp);
};




Requisition.prototype._addUnassignedArgs = function(args) {
  args.forEach(function(arg) {
    this._unassigned.push(new UnassignedAssignment(this, arg));
  }.bind(this));
};




Requisition.prototype._assign = function(args) {
  
  var noArgUp = { internal: true };

  this._unassigned = [];
  var outstanding = [];

  if (!this.commandAssignment.value) {
    this._addUnassignedArgs(args);
    return promise.all(outstanding);
  }

  if (args.length === 0) {
    this.setBlankArguments();
    return promise.all(outstanding);
  }

  
  
  if (this.assignmentCount === 0) {
    this._addUnassignedArgs(args);
    return promise.all(outstanding);
  }

  
  
  if (this.assignmentCount === 1) {
    var assignment = this.getAssignment(0);
    if (assignment.param.type.name === 'string') {
      var arg = (args.length === 1) ? args[0] : new MergedArgument(args);
      outstanding.push(this.setAssignment(assignment, arg, noArgUp));
      return promise.all(outstanding);
    }
  }

  
  
  var unassignedParams = this.getParameterNames();

  
  var arrayArgs = {};

  
  this.getAssignments(false).forEach(function(assignment) {
    
    
    var i = 0;
    while (i < args.length) {
      if (assignment.param.isKnownAs(args[i].text)) {
        var arg = args.splice(i, 1)[0];
        unassignedParams = unassignedParams.filter(function(test) {
          return test !== assignment.param.name;
        });

        
        if (assignment.param.type.name === 'boolean') {
          arg = new TrueNamedArgument(arg);
        }
        else {
          var valueArg = null;
          if (i + 1 <= args.length) {
            valueArg = args.splice(i, 1)[0];
          }
          arg = new NamedArgument(arg, valueArg);
        }

        if (assignment.param.type.name === 'array') {
          var arrayArg = arrayArgs[assignment.param.name];
          if (!arrayArg) {
            arrayArg = new ArrayArgument();
            arrayArgs[assignment.param.name] = arrayArg;
          }
          arrayArg.addArgument(arg);
        }
        else {
          if (assignment.arg.type === 'BlankArgument') {
            outstanding.push(this.setAssignment(assignment, arg, noArgUp));
          }
          else {
            this._addUnassignedArgs(arg.getArgs());
          }
        }
      }
      else {
        
        i++;
      }
    }
  }, this);

  
  unassignedParams.forEach(function(name) {
    var assignment = this.getAssignment(name);

    
    
    if (!assignment.param.isPositionalAllowed) {
      outstanding.push(this.setAssignment(assignment, null, noArgUp));
      return;
    }

    
    
    if (assignment.param.type.name === 'array') {
      var arrayArg = arrayArgs[assignment.param.name];
      if (!arrayArg) {
        arrayArg = new ArrayArgument();
        arrayArgs[assignment.param.name] = arrayArg;
      }
      arrayArg.addArguments(args);
      args = [];
      
      return;
    }

    
    if (args.length === 0) {
      outstanding.push(this.setAssignment(assignment, null, noArgUp));
      return;
    }

    var arg = args.splice(0, 1)[0];
    
    
    var isIncompleteName = assignment.param.type.name === 'number' ?
        /-[-a-zA-Z_]/.test(arg.text) :
        arg.text.charAt(0) === '-';

    if (isIncompleteName) {
      this._unassigned.push(new UnassignedAssignment(this, arg));
    }
    else {
      outstanding.push(this.setAssignment(assignment, arg, noArgUp));
    }
  }, this);

  
  Object.keys(arrayArgs).forEach(function(name) {
    var assignment = this.getAssignment(name);
    outstanding.push(this.setAssignment(assignment, arrayArgs[name], noArgUp));
  }, this);

  
  this._addUnassignedArgs(args);

  return promise.all(outstanding);
};

exports.Requisition = Requisition;




function Output(options) {
  options = options || {};
  this.command = options.command || '';
  this.args = options.args || {};
  this.typed = options.typed || '';
  this.canonical = options.canonical || '';
  this.hidden = options.hidden === true ? true : false;

  this.type = undefined;
  this.data = undefined;
  this.completed = false;
  this.error = false;
  this.start = new Date();

  this._deferred = promise.defer();
  this.promise = this._deferred.promise;

  this.onClose = util.createEvent('Output.onClose');
}





Output.prototype.complete = function(data, error) {
  this.end = new Date();
  this.duration = this.end.getTime() - this.start.getTime();
  this.completed = true;
  this.error = error;

  if (data != null && data.isTypedData) {
    this.data = data.data;
    this.type = data.type;
  }
  else {
    this.data = data;
    this.type = this.command.returnType;
    if (this.type == null) {
      this.type = (this.data == null) ? 'undefined' : typeof this.data;
    }
  }

  if (this.type === 'object') {
    throw new Error('No type from output of ' + this.typed);
  }

  this._deferred.resolve();
};




Output.prototype.convert = function(type, conversionContext) {
  return converters.convert(this.data, this.type, type, conversionContext);
};

exports.Output = Output;


});
define("text!gcli/ui/intro.html", [], "\n" +
  "<div>\n" +
  "  <p>${l10n.introTextOpening2}</p>\n" +
  "\n" +
  "  <p>\n" +
  "    ${l10n.introTextCommands}\n" +
  "    <span class=\"gcli-out-shortcut\" onclick=\"${onclick}\"\n" +
  "        ondblclick=\"${ondblclick}\" data-command=\"help\">help</span>${l10n.introTextKeys2}\n" +
  "    <code>${l10n.introTextF1Escape}</code>.\n" +
  "  </p>\n" +
  "\n" +
  "  <button onclick=\"${onGotIt}\" if=\"${showHideButton}\">${l10n.introTextGo}</button>\n" +
  "</div>\n" +
  "");

















define('gcli/ui/focus', ['require', 'exports', 'module' , 'util/util', 'util/l10n', 'gcli/settings'], function(require, exports, module) {

'use strict';

var util = require('util/util');
var l10n = require('util/l10n');
var settings = require('gcli/settings');




var Eagerness = {
  NEVER: 1,
  SOMETIMES: 2,
  ALWAYS: 3
};




exports.items = [
  {
    item: 'setting',
    name: 'eagerHelper',
    type: {
      name: 'selection',
      lookup: [
        { name: 'never', value: Eagerness.NEVER },
        { name: 'sometimes', value: Eagerness.SOMETIMES },
        { name: 'always', value: Eagerness.ALWAYS }
      ]
    },
    defaultValue: Eagerness.SOMETIMES,
    description: l10n.lookup('eagerHelperDesc'),
    ignoreTypeDifference: true
  }
];
















function FocusManager(options, components) {
  options = options || {};

  this._document = components.document || document;
  this._requisition = components.requisition;

  this._debug = options.debug || false;
  this._blurDelay = options.blurDelay || 150;
  this._window = this._document.defaultView;

  this._requisition.commandOutputManager.onOutput.add(this._outputted, this);

  this._blurDelayTimeout = null; 
  this._monitoredElements = [];  

  this._isError = false;
  this._hasFocus = false;
  this._helpRequested = false;
  this._recentOutput = false;

  this.onVisibilityChange = util.createEvent('FocusManager.onVisibilityChange');

  this._focused = this._focused.bind(this);
  this._document.addEventListener('focus', this._focused, true);

  var eagerHelper = settings.getSetting('eagerHelper');
  eagerHelper.onChange.add(this._eagerHelperChanged, this);

  this.isTooltipVisible = undefined;
  this.isOutputVisible = undefined;
  this._checkShow();
}




FocusManager.prototype.destroy = function() {
  var eagerHelper = settings.getSetting('eagerHelper');
  eagerHelper.onChange.remove(this._eagerHelperChanged, this);

  this._document.removeEventListener('focus', this._focused, true);
  this._requisition.commandOutputManager.onOutput.remove(this._outputted, this);

  for (var i = 0; i < this._monitoredElements.length; i++) {
    var monitor = this._monitoredElements[i];
    console.error('Hanging monitored element: ', monitor.element);

    monitor.element.removeEventListener('focus', monitor.onFocus, true);
    monitor.element.removeEventListener('blur', monitor.onBlur, true);
  }

  if (this._blurDelayTimeout) {
    this._window.clearTimeout(this._blurDelayTimeout);
    this._blurDelayTimeout = null;
  }

  delete this._focused;
  delete this._document;
  delete this._window;
  delete this._requisition;
};








FocusManager.prototype.addMonitoredElement = function(element, where) {
  if (this._debug) {
    console.log('FocusManager.addMonitoredElement(' + (where || 'unknown') + ')');
  }

  var monitor = {
    element: element,
    where: where,
    onFocus: function() { this._reportFocus(where); }.bind(this),
    onBlur: function() { this._reportBlur(where); }.bind(this)
  };

  element.addEventListener('focus', monitor.onFocus, true);
  element.addEventListener('blur', monitor.onBlur, true);

  if (this._document.activeElement === element) {
    this._reportFocus(where);
  }

  this._monitoredElements.push(monitor);
};






FocusManager.prototype.removeMonitoredElement = function(element, where) {
  if (this._debug) {
    console.log('FocusManager.removeMonitoredElement(' + (where || 'unknown') + ')');
  }

  var newMonitoredElements = this._monitoredElements.filter(function(monitor) {
    if (monitor.element === element) {
      element.removeEventListener('focus', monitor.onFocus, true);
      element.removeEventListener('blur', monitor.onBlur, true);
      return false;
    }
    return true;
  });

  this._monitoredElements = newMonitoredElements;
};




FocusManager.prototype.updatePosition = function(dimensions) {
  var ev = {
    tooltipVisible: this.isTooltipVisible,
    outputVisible: this.isOutputVisible,
    dimensions: dimensions
  };
  this.onVisibilityChange(ev);
};




FocusManager.prototype._outputted = function(ev) {
  this._recentOutput = true;
  this._helpRequested = false;
  this._checkShow();
};





FocusManager.prototype._focused = function() {
  this._reportBlur('document');
};






FocusManager.prototype._reportFocus = function(where) {
  if (this._debug) {
    console.log('FocusManager._reportFocus(' + (where || 'unknown') + ')');
  }

  if (this._blurDelayTimeout) {
    if (this._debug) {
      console.log('FocusManager.cancelBlur');
    }
    this._window.clearTimeout(this._blurDelayTimeout);
    this._blurDelayTimeout = null;
  }

  if (!this._hasFocus) {
    this._hasFocus = true;
  }
  this._checkShow();
};







FocusManager.prototype._reportBlur = function(where) {
  if (this._debug) {
    console.log('FocusManager._reportBlur(' + where + ')');
  }

  if (this._hasFocus) {
    if (this._blurDelayTimeout) {
      if (this._debug) {
        console.log('FocusManager.blurPending');
      }
      return;
    }

    this._blurDelayTimeout = this._window.setTimeout(function() {
      if (this._debug) {
        console.log('FocusManager.blur');
      }
      this._hasFocus = false;
      this._checkShow();
      this._blurDelayTimeout = null;
    }.bind(this), this._blurDelay);
  }
};




FocusManager.prototype._eagerHelperChanged = function() {
  this._checkShow();
};





FocusManager.prototype.onInputChange = function() {
  this._recentOutput = false;
  this._checkShow();
};





FocusManager.prototype.helpRequest = function() {
  if (this._debug) {
    console.log('FocusManager.helpRequest');
  }

  this._helpRequested = true;
  this._recentOutput = false;
  this._checkShow();
};





FocusManager.prototype.removeHelp = function() {
  if (this._debug) {
    console.log('FocusManager.removeHelp');
  }

  this._importantFieldFlag = false;
  this._isError = false;
  this._helpRequested = false;
  this._recentOutput = false;
  this._checkShow();
};




FocusManager.prototype.setImportantFieldFlag = function(flag) {
  if (this._debug) {
    console.log('FocusManager.setImportantFieldFlag', flag);
  }
  this._importantFieldFlag = flag;
  this._checkShow();
};




FocusManager.prototype.setError = function(isError) {
  if (this._debug) {
    console.log('FocusManager._isError', isError);
  }
  this._isError = isError;
  this._checkShow();
};





FocusManager.prototype._checkShow = function() {
  var fire = false;
  var ev = {
    tooltipVisible: this.isTooltipVisible,
    outputVisible: this.isOutputVisible
  };

  var showTooltip = this._shouldShowTooltip();
  if (this.isTooltipVisible !== showTooltip.visible) {
    ev.tooltipVisible = this.isTooltipVisible = showTooltip.visible;
    fire = true;
  }

  var showOutput = this._shouldShowOutput();
  if (this.isOutputVisible !== showOutput.visible) {
    ev.outputVisible = this.isOutputVisible = showOutput.visible;
    fire = true;
  }

  if (fire) {
    if (this._debug) {
      console.log('FocusManager.onVisibilityChange', ev);
    }
    this.onVisibilityChange(ev);
  }
};





FocusManager.prototype._shouldShowTooltip = function() {
  if (!this._hasFocus) {
    return { visible: false, reason: 'notHasFocus' };
  }

  var eagerHelper = settings.getSetting('eagerHelper');
  if (eagerHelper.value === Eagerness.NEVER) {
    return { visible: false, reason: 'eagerHelperNever' };
  }

  if (eagerHelper.value === Eagerness.ALWAYS) {
    return { visible: true, reason: 'eagerHelperAlways' };
  }

  if (this._isError) {
    return { visible: true, reason: 'isError' };
  }

  if (this._helpRequested) {
    return { visible: true, reason: 'helpRequested' };
  }

  if (this._importantFieldFlag) {
    return { visible: true, reason: 'importantFieldFlag' };
  }

  return { visible: false, reason: 'default' };
};





FocusManager.prototype._shouldShowOutput = function() {
  if (!this._hasFocus) {
    return { visible: false, reason: 'notHasFocus' };
  }

  if (this._recentOutput) {
    return { visible: true, reason: 'recentOutput' };
  }

  return { visible: false, reason: 'default' };
};

exports.FocusManager = FocusManager;


});
















define('gcli/ui/fields/basic', ['require', 'exports', 'module' , 'util/util', 'util/promise', 'util/l10n', 'gcli/argument', 'gcli/types', 'gcli/ui/fields'], function(require, exports, module) {

'use strict';

var util = require('util/util');
var promise = require('util/promise');
var l10n = require('util/l10n');

var Argument = require('gcli/argument').Argument;
var TrueNamedArgument = require('gcli/argument').TrueNamedArgument;
var FalseNamedArgument = require('gcli/argument').FalseNamedArgument;
var ArrayArgument = require('gcli/argument').ArrayArgument;
var ArrayConversion = require('gcli/types').ArrayConversion;

var Field = require('gcli/ui/fields').Field;
var fields = require('gcli/ui/fields');





function StringField(type, options) {
  Field.call(this, type, options);
  this.arg = new Argument();

  this.element = util.createElement(this.document, 'input');
  this.element.type = 'text';
  this.element.classList.add('gcli-field');

  this.onInputChange = this.onInputChange.bind(this);
  this.element.addEventListener('keyup', this.onInputChange, false);

  this.onFieldChange = util.createEvent('StringField.onFieldChange');
}

StringField.prototype = Object.create(Field.prototype);

StringField.prototype.destroy = function() {
  Field.prototype.destroy.call(this);
  this.element.removeEventListener('keyup', this.onInputChange, false);
  delete this.element;
  delete this.document;
  delete this.onInputChange;
};

StringField.prototype.setConversion = function(conversion) {
  this.arg = conversion.arg;
  this.element.value = conversion.arg.text;
  this.setMessage(conversion.message);
};

StringField.prototype.getConversion = function() {
  
  this.arg = this.arg.beget({ text: this.element.value, prefixSpace: true });
  return this.type.parse(this.arg, this.requisition.executionContext);
};

StringField.claim = function(type, context) {
  return type.name === 'string' ? Field.MATCH : Field.BASIC;
};





function NumberField(type, options) {
  Field.call(this, type, options);
  this.arg = new Argument();

  this.element = util.createElement(this.document, 'input');
  this.element.type = 'number';
  if (this.type.max) {
    this.element.max = this.type.max;
  }
  if (this.type.min) {
    this.element.min = this.type.min;
  }
  if (this.type.step) {
    this.element.step = this.type.step;
  }

  this.onInputChange = this.onInputChange.bind(this);
  this.element.addEventListener('keyup', this.onInputChange, false);

  this.onFieldChange = util.createEvent('NumberField.onFieldChange');
}

NumberField.prototype = Object.create(Field.prototype);

NumberField.claim = function(type, context) {
  return type.name === 'number' ? Field.MATCH : Field.NO_MATCH;
};

NumberField.prototype.destroy = function() {
  Field.prototype.destroy.call(this);
  this.element.removeEventListener('keyup', this.onInputChange, false);
  delete this.element;
  delete this.document;
  delete this.onInputChange;
};

NumberField.prototype.setConversion = function(conversion) {
  this.arg = conversion.arg;
  this.element.value = conversion.arg.text;
  this.setMessage(conversion.message);
};

NumberField.prototype.getConversion = function() {
  this.arg = this.arg.beget({ text: this.element.value, prefixSpace: true });
  return this.type.parse(this.arg, this.requisition.executionContext);
};





function BooleanField(type, options) {
  Field.call(this, type, options);

  this.name = options.name;
  this.named = options.named;

  this.element = util.createElement(this.document, 'input');
  this.element.type = 'checkbox';
  this.element.id = 'gcliForm' + this.name;

  this.onInputChange = this.onInputChange.bind(this);
  this.element.addEventListener('change', this.onInputChange, false);

  this.onFieldChange = util.createEvent('BooleanField.onFieldChange');
}

BooleanField.prototype = Object.create(Field.prototype);

BooleanField.claim = function(type, context) {
  return type.name === 'boolean' ? Field.MATCH : Field.NO_MATCH;
};

BooleanField.prototype.destroy = function() {
  Field.prototype.destroy.call(this);
  this.element.removeEventListener('change', this.onInputChange, false);
  delete this.element;
  delete this.document;
  delete this.onInputChange;
};

BooleanField.prototype.setConversion = function(conversion) {
  this.element.checked = conversion.value;
  this.setMessage(conversion.message);
};

BooleanField.prototype.getConversion = function() {
  var arg;
  if (this.named) {
    arg = this.element.checked ?
            new TrueNamedArgument(new Argument(' --' + this.name)) :
            new FalseNamedArgument();
  }
  else {
    arg = new Argument(' ' + this.element.checked);
  }
  return this.type.parse(arg, this.requisition.executionContext);
};






function DelegateField(type, options) {
  Field.call(this, type, options);
  this.options = options;
  this.requisition.onTextChange.add(this.update, this);

  this.element = util.createElement(this.document, 'div');
  this.update();

  this.onFieldChange = util.createEvent('DelegateField.onFieldChange');
}

DelegateField.prototype = Object.create(Field.prototype);

DelegateField.prototype.update = function() {
  var subtype = this.type.delegateType();
  if (subtype === this.subtype) {
    return;
  }

  if (this.field) {
    this.field.onFieldChange.remove(this.fieldChanged, this);
    this.field.destroy();
  }

  this.subtype = subtype;
  this.field = fields.getField(subtype, this.options);
  this.field.onFieldChange.add(this.fieldChanged, this);

  util.clearElement(this.element);
  this.element.appendChild(this.field.element);
};

DelegateField.claim = function(type, context) {
  return type.isDelegate ? Field.MATCH : Field.NO_MATCH;
};

DelegateField.prototype.destroy = function() {
  Field.prototype.destroy.call(this);
  this.requisition.onTextChange.remove(this.update, this);
  delete this.element;
  delete this.document;
  delete this.onInputChange;
};

DelegateField.prototype.setConversion = function(conversion) {
  this.field.setConversion(conversion);
};

DelegateField.prototype.getConversion = function() {
  return this.field.getConversion();
};

Object.defineProperty(DelegateField.prototype, 'isImportant', {
  get: function() {
    return this.field.isImportant;
  },
  enumerable: true
});






function ArrayField(type, options) {
  Field.call(this, type, options);
  this.options = options;

  this._onAdd = this._onAdd.bind(this);
  this.members = [];

  
  this.element = util.createElement(this.document, 'div');
  this.element.classList.add('gcli-array-parent');

  
  this.addButton = util.createElement(this.document, 'button');
  this.addButton.classList.add('gcli-array-member-add');
  this.addButton.addEventListener('click', this._onAdd, false);
  this.addButton.textContent = l10n.lookup('fieldArrayAdd');
  this.element.appendChild(this.addButton);

  
  this.container = util.createElement(this.document, 'div');
  this.container.classList.add('gcli-array-members');
  this.element.appendChild(this.container);

  this.onInputChange = this.onInputChange.bind(this);

  this.onFieldChange = util.createEvent('ArrayField.onFieldChange');
}

ArrayField.prototype = Object.create(Field.prototype);

ArrayField.claim = function(type, context) {
  return type.name === 'array' ? Field.MATCH : Field.NO_MATCH;
};

ArrayField.prototype.destroy = function() {
  Field.prototype.destroy.call(this);
  this.addButton.removeEventListener('click', this._onAdd, false);
};

ArrayField.prototype.setConversion = function(conversion) {
  
  util.clearElement(this.container);
  this.members = [];

  conversion.conversions.forEach(function(subConversion) {
    this._onAdd(null, subConversion);
  }, this);
};

ArrayField.prototype.getConversion = function() {
  var conversions = [];
  var arrayArg = new ArrayArgument();

  var addConversion = function(conversion) {
    conversions.push(conversion);
    arrayArg.addArgument(conversion.arg);
  }.bind(this);

  for (var i = 0; i < this.members.length; i++) {
    var reply = this.members[i].field.getConversion();
    promise.resolve(reply).then(addConversion, util.errorHandler);
  }

  return new ArrayConversion(conversions, arrayArg);
};

ArrayField.prototype._onAdd = function(ev, subConversion) {
  
  var element = util.createElement(this.document, 'div');
  element.classList.add('gcli-array-member');
  this.container.appendChild(element);

  
  var field = fields.getField(this.type.subtype, this.options);
  field.onFieldChange.add(function() {
    promise.resolve(this.getConversion()).then(function(conversion) {
      this.onFieldChange({ conversion: conversion });
      this.setMessage(conversion.message);
    }.bind(this), util.errorHandler);
  }, this);

  if (subConversion) {
    field.setConversion(subConversion);
  }
  element.appendChild(field.element);

  
  var delButton = util.createElement(this.document, 'button');
  delButton.classList.add('gcli-array-member-del');
  delButton.addEventListener('click', this._onDel, false);
  delButton.textContent = l10n.lookup('fieldArrayDel');
  element.appendChild(delButton);

  var member = {
    element: element,
    field: field,
    parent: this
  };
  member.onDelete = function() {
    this.parent.container.removeChild(this.element);
    this.parent.members = this.parent.members.filter(function(test) {
      return test !== this;
    });
    this.parent.onInputChange();
  }.bind(member);
  delButton.addEventListener('click', member.onDelete, false);

  this.members.push(member);
};




exports.items = [
  StringField, NumberField, BooleanField, DelegateField, ArrayField
];


});
















define('gcli/ui/fields/javascript', ['require', 'exports', 'module' , 'util/util', 'util/promise', 'gcli/types', 'gcli/argument', 'gcli/ui/fields/menu', 'gcli/ui/fields'], function(require, exports, module) {

'use strict';

var util = require('util/util');
var promise = require('util/promise');

var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;
var ScriptArgument = require('gcli/argument').ScriptArgument;

var Menu = require('gcli/ui/fields/menu').Menu;
var Field = require('gcli/ui/fields').Field;




function JavascriptField(type, options) {
  Field.call(this, type, options);

  this.onInputChange = this.onInputChange.bind(this);
  this.arg = new ScriptArgument('', '{ ', ' }');

  this.element = util.createElement(this.document, 'div');

  this.input = util.createElement(this.document, 'input');
  this.input.type = 'text';
  this.input.addEventListener('keyup', this.onInputChange, false);
  this.input.classList.add('gcli-field');
  this.input.classList.add('gcli-field-javascript');
  this.element.appendChild(this.input);

  this.menu = new Menu({
    document: this.document,
    field: true,
    type: type
  });
  this.element.appendChild(this.menu.element);

  var initial = new Conversion(undefined, new ScriptArgument(''),
                               Status.INCOMPLETE, '');
  this.setConversion(initial);

  this.onFieldChange = util.createEvent('JavascriptField.onFieldChange');

  
  this.menu.onItemClick.add(this.itemClicked, this);
}

JavascriptField.prototype = Object.create(Field.prototype);

JavascriptField.claim = function(type, context) {
  return type.name === 'javascript' ? Field.TOOLTIP_MATCH : Field.NO_MATCH;
};

JavascriptField.prototype.destroy = function() {
  Field.prototype.destroy.call(this);
  this.input.removeEventListener('keyup', this.onInputChange, false);
  this.menu.onItemClick.remove(this.itemClicked, this);
  this.menu.destroy();
  delete this.element;
  delete this.input;
  delete this.menu;
  delete this.document;
  delete this.onInputChange;
};

JavascriptField.prototype.setConversion = function(conversion) {
  this.arg = conversion.arg;
  this.input.value = conversion.arg.text;

  var prefixLen = 0;
  if (this.type.name === 'javascript') {
    var typed = conversion.arg.text;
    var lastDot = typed.lastIndexOf('.');
    if (lastDot !== -1) {
      prefixLen = lastDot;
    }
  }

  this.setMessage(conversion.message);

  conversion.getPredictions().then(function(predictions) {
    var items = [];
    predictions.forEach(function(item) {
      
      if (!item.hidden) {
        items.push({
          name: item.name.substring(prefixLen),
          complete: item.name,
          description: item.description || ''
        });
      }
    }, this);
    this.menu.show(items);
  }.bind(this), util.errorHandler);
};

JavascriptField.prototype.itemClicked = function(ev) {
  var parsed = this.type.parse(ev.arg, this.requisition.executionContext);
  promise.resolve(parsed).then(function(conversion) {
    this.onFieldChange({ conversion: conversion });
    this.setMessage(conversion.message);
  }.bind(this), util.errorHandler);
};

JavascriptField.prototype.onInputChange = function(ev) {
  this.item = ev.currentTarget.item;
  promise.resolve(this.getConversion()).then(function(conversion) {
    this.onFieldChange({ conversion: conversion });
    this.setMessage(conversion.message);
  }.bind(this), util.errorHandler);
};

JavascriptField.prototype.getConversion = function() {
  
  this.arg = new ScriptArgument(this.input.value, '{ ', ' }');
  return this.type.parse(this.arg, this.requisition.executionContext);
};

JavascriptField.DEFAULT_VALUE = '__JavascriptField.DEFAULT_VALUE';




exports.items = [ JavascriptField ];


});
















define('gcli/ui/fields/menu', ['require', 'exports', 'module' , 'util/util', 'util/l10n', 'util/domtemplate', 'gcli/argument', 'gcli/types', 'gcli/canon', 'text!gcli/ui/fields/menu.css', 'text!gcli/ui/fields/menu.html'], function(require, exports, module) {

'use strict';

var util = require('util/util');
var l10n = require('util/l10n');
var domtemplate = require('util/domtemplate');

var Argument = require('gcli/argument').Argument;
var Conversion = require('gcli/types').Conversion;
var canon = require('gcli/canon');

var menuCss = require('text!gcli/ui/fields/menu.css');
var menuHtml = require('text!gcli/ui/fields/menu.html');













function Menu(options) {
  options = options || {};
  this.document = options.document || document;
  this.type = options.type;

  
  if (!this.document) {
    throw new Error('No document');
  }

  this.element =  util.createElement(this.document, 'div');
  this.element.classList.add(options.menuClass || 'gcli-menu');
  if (options && options.field) {
    this.element.classList.add(options.menuFieldClass || 'gcli-menu-field');
  }

  
  if (menuCss != null) {
    util.importCss(menuCss, this.document, 'gcli-menu');
  }

  this.template = util.toDom(this.document, menuHtml);
  this.templateOptions = { blankNullUndefined: true, stack: 'menu.html' };

  
  this.items = null;

  this.onItemClick = util.createEvent('Menu.onItemClick');
}




Menu.prototype.l10n = l10n.propertyLookup;




Menu.prototype.destroy = function() {
  delete this.element;
  delete this.template;
  delete this.document;
};






Menu.prototype.onItemClickInternal = function(ev) {
  var name = ev.currentTarget.querySelector('.gcli-menu-name').textContent;
  var arg = new Argument(name);
  arg.suffix = ' ';

  this.onItemClick({ arg: arg });
};







Menu.prototype.show = function(items, match) {
  this.items = items.filter(function(item) {
    return item.hidden === undefined || item.hidden !== true;
  }.bind(this));

  if (match) {
    this.items = this.items.map(function(item) {
      return getHighlightingProxy(item, match, this.template.ownerDocument);
    }.bind(this));
  }

  if (this.items.length === 0) {
    this.element.style.display = 'none';
    return;
  }

  if (this.items.length >= Conversion.maxPredictions) {
    this.items.splice(-1);
    this.items.hasMore = true;
  }

  var options = this.template.cloneNode(true);
  domtemplate.template(options, this, this.templateOptions);

  util.clearElement(this.element);
  this.element.appendChild(options);

  this.element.style.display = 'block';
};




function getHighlightingProxy(item, match, document) {
  if (typeof Proxy === 'undefined') {
    return item;
  }
  return Proxy.create({
    get: function(rcvr, name) {
      var value = item[name];
      if (name !== 'name') {
        return value;
      }

      var startMatch = value.indexOf(match);
      if (startMatch === -1) {
        return value;
      }

      var before = value.substr(0, startMatch);
      var after = value.substr(startMatch + match.length);
      var parent = document.createElement('span');
      parent.appendChild(document.createTextNode(before));
      var highlight = document.createElement('span');
      highlight.classList.add('gcli-menu-typed');
      highlight.appendChild(document.createTextNode(match));
      parent.appendChild(highlight);
      parent.appendChild(document.createTextNode(after));
      return parent;
    }
  });
}




Menu.prototype.setChoiceIndex = function(choice) {
  var nodes = this.element.querySelectorAll('.gcli-menu-option');
  for (var i = 0; i < nodes.length; i++) {
    nodes[i].classList.remove('gcli-menu-highlight');
  }

  if (choice == null) {
    return;
  }

  if (nodes.length <= choice) {
    console.error('Cant highlight ' + choice + '. Only ' + nodes.length + ' options');
    return;
  }

  nodes.item(choice).classList.add('gcli-menu-highlight');
};






Menu.prototype.selectChoice = function() {
  var selected = this.element.querySelector('.gcli-menu-highlight .gcli-menu-name');
  if (!selected) {
    return false;
  }

  var name = selected.textContent;
  var arg = new Argument(name);
  arg.suffix = ' ';
  arg.prefix = ' ';

  this.onItemClick({ arg: arg });
  return true;
};




Menu.prototype.hide = function() {
  this.element.style.display = 'none';
};




Menu.prototype.setMaxHeight = function(height) {
  this.element.style.maxHeight = height + 'px';
};

exports.Menu = Menu;


});
define("text!gcli/ui/fields/menu.css", [], "");

define("text!gcli/ui/fields/menu.html", [], "\n" +
  "<div>\n" +
  "  <table class=\"gcli-menu-template\" aria-live=\"polite\">\n" +
  "    <tr class=\"gcli-menu-option\" foreach=\"item in ${items}\"\n" +
  "        onclick=\"${onItemClickInternal}\" title=\"${item.manual}\">\n" +
  "      <td class=\"gcli-menu-name\">${item.name}</td>\n" +
  "      <td class=\"gcli-menu-desc\">${item.description}</td>\n" +
  "    </tr>\n" +
  "  </table>\n" +
  "  <div class=\"gcli-menu-more\" if=\"${items.hasMore}\">${l10n.fieldMenuMore}</div>\n" +
  "</div>\n" +
  "");

















define('gcli/ui/fields/selection', ['require', 'exports', 'module' , 'util/promise', 'util/util', 'util/l10n', 'gcli/argument', 'gcli/ui/fields/menu', 'gcli/ui/fields'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var util = require('util/util');
var l10n = require('util/l10n');

var Argument = require('gcli/argument').Argument;

var Menu = require('gcli/ui/fields/menu').Menu;
var Field = require('gcli/ui/fields').Field;













function SelectionField(type, options) {
  Field.call(this, type, options);

  this.items = [];

  this.element = util.createElement(this.document, 'select');
  this.element.classList.add('gcli-field');
  this._addOption({
    name: l10n.lookupFormat('fieldSelectionSelect', [ options.name ])
  });

  promise.resolve(this.type.getLookup()).then(function(lookup) {
    lookup.forEach(this._addOption, this);
  }.bind(this), util.errorHandler);

  this.onInputChange = this.onInputChange.bind(this);
  this.element.addEventListener('change', this.onInputChange, false);

  this.onFieldChange = util.createEvent('SelectionField.onFieldChange');
}

SelectionField.prototype = Object.create(Field.prototype);

SelectionField.claim = function(type, context) {
  if (type.name === 'boolean') {
    return Field.BASIC;
  }
  return type.isSelection ? Field.DEFAULT : Field.NO_MATCH;
};

SelectionField.prototype.destroy = function() {
  Field.prototype.destroy.call(this);
  this.element.removeEventListener('change', this.onInputChange, false);
  delete this.element;
  delete this.document;
  delete this.onInputChange;
};

SelectionField.prototype.setConversion = function(conversion) {
  var index;
  this.items.forEach(function(item) {
    if (item.value && item.value === conversion.value) {
      index = item.index;
    }
  }, this);
  this.element.value = index;
  this.setMessage(conversion.message);
};

SelectionField.prototype.getConversion = function() {
  var item = this.items[this.element.value];
  return this.type.parse(new Argument(item.name, ' '));
};

SelectionField.prototype._addOption = function(item) {
  item.index = this.items.length;
  this.items.push(item);

  var option = util.createElement(this.document, 'option');
  option.textContent = item.name;
  option.value = item.index;
  this.element.appendChild(option);
};





function SelectionTooltipField(type, options) {
  Field.call(this, type, options);

  this.onInputChange = this.onInputChange.bind(this);
  this.arg = new Argument();

  this.menu = new Menu({ document: this.document, type: type });
  this.element = this.menu.element;

  this.onFieldChange = util.createEvent('SelectionTooltipField.onFieldChange');

  
  this.menu.onItemClick.add(this.itemClicked, this);
}

SelectionTooltipField.prototype = Object.create(Field.prototype);

SelectionTooltipField.claim = function(type, context) {
  return type.getType(context).isSelection ?
      Field.TOOLTIP_MATCH :
      Field.NO_MATCH;
};

SelectionTooltipField.prototype.destroy = function() {
  Field.prototype.destroy.call(this);
  this.menu.onItemClick.remove(this.itemClicked, this);
  this.menu.destroy();
  delete this.element;
  delete this.document;
  delete this.onInputChange;
};

SelectionTooltipField.prototype.setConversion = function(conversion) {
  this.arg = conversion.arg;
  this.setMessage(conversion.message);

  conversion.getPredictions().then(function(predictions) {
    var items = predictions.map(function(prediction) {
      
      
      
      return prediction.value && prediction.value.description ?
          prediction.value :
          prediction;
    }, this);
    this.menu.show(items, conversion.arg.text);
  }.bind(this), util.errorHandler);
};

SelectionTooltipField.prototype.itemClicked = function(ev) {
  var parsed = this.type.parse(ev.arg, this.requisition.executionContext);
  promise.resolve(parsed).then(function(conversion) {
    this.onFieldChange({ conversion: conversion });
    this.setMessage(conversion.message);
  }.bind(this), util.errorHandler);
};

SelectionTooltipField.prototype.onInputChange = function(ev) {
  this.item = ev.currentTarget.item;
  promise.resolve(this.getConversion()).then(function(conversion) {
    this.onFieldChange({ conversion: conversion });
    this.setMessage(conversion.message);
  }.bind(this), util.errorHandler);
};

SelectionTooltipField.prototype.getConversion = function() {
  
  this.arg = this.arg.beget({ text: this.input.value });
  return this.type.parse(this.arg, this.requisition.executionContext);
};




SelectionTooltipField.prototype.setChoiceIndex = function(choice) {
  this.menu.setChoiceIndex(choice);
};






SelectionTooltipField.prototype.selectChoice = function() {
  return this.menu.selectChoice();
};

Object.defineProperty(SelectionTooltipField.prototype, 'isImportant', {
  get: function() {
    return this.type.name !== 'command';
  },
  enumerable: true
});

SelectionTooltipField.DEFAULT_VALUE = '__SelectionTooltipField.DEFAULT_VALUE';




exports.items = [ SelectionField, SelectionTooltipField ];

});
















define('gcli/commands/connect', ['require', 'exports', 'module' , 'util/l10n', 'gcli/canon', 'util/connect/connector'], function(require, exports, module) {

'use strict';

var l10n = require('util/l10n');
var canon = require('gcli/canon');
var connector = require('util/connect/connector');




var connections = {};




var connection = {
  item: 'type',
  name: 'connection',
  parent: 'selection',
  lookup: function() {
    return Object.keys(connections).map(function(prefix) {
      return { name: prefix, value: connections[prefix] };
    });
  }
};




var connect = {
  item: 'command',
  name: 'connect',
  description: l10n.lookup('connectDesc'),
  manual: l10n.lookup('connectManual'),
  params: [
    {
      name: 'prefix',
      type: 'string',
      description: l10n.lookup('connectPrefixDesc')
    },
    {
      name: 'host',
      short: 'h',
      type: 'string',
      description: l10n.lookup('connectHostDesc'),
      defaultValue: 'localhost',
      option: true
    },
    {
      name: 'port',
      short: 'p',
      type: { name: 'number', max: 65536, min: 0 },
      description: l10n.lookup('connectPortDesc'),
      defaultValue: connector.defaultPort,
      option: true
    }
  ],
  returnType: 'string',

  exec: function(args, context) {
    if (connections[args.prefix] != null) {
      throw new Error(l10n.lookupFormat('connectDupReply', [ args.prefix ]));
    }

    var cxp = connector.connect(args.prefix, args.host, args.port);
    return cxp.then(function(connection) {
      connections[args.prefix] = connection;

      return connection.getCommandSpecs().then(function(commandSpecs) {
        var remoter = this.createRemoter(args.prefix, connection);
        canon.addProxyCommands(args.prefix, commandSpecs, remoter);

        
        return l10n.lookupFormat('connectReply',
                                 [ Object.keys(commandSpecs).length + 1 ]);
      }.bind(this));
    }.bind(this));
  },

  



  createRemoter: function(prefix, connection) {
    return function(cmdArgs, context) {
      var typed = context.typed;

      
      
      if (typed.indexOf(prefix) === 0) {
        typed = typed.substring(prefix.length).replace(/^ */, '');
      }

      return connection.execute(typed, cmdArgs).then(function(reply) {
        var typedData = context.typedData(reply.type, reply.data);
        if (!reply.error) {
          return typedData;
        }
        else {
          throw typedData;
        }
      });
    }.bind(this);
  }
};




var disconnect = {
  item: 'command',
  name: 'disconnect',
  description: l10n.lookup('disconnectDesc2'),
  manual: l10n.lookup('disconnectManual2'),
  params: [
    {
      name: 'prefix',
      type: 'connection',
      description: l10n.lookup('disconnectPrefixDesc'),
    },
    {
      name: 'force',
      type: 'boolean',
      description: l10n.lookup('disconnectForceDesc'),
      hidden: connector.disconnectSupportsForce,
      option: true
    }
  ],
  returnType: 'string',

  exec: function(args, context) {
    return args.prefix.disconnect(args.force).then(function() {
      var removed = canon.removeProxyCommands(args.prefix.prefix);
      delete connections[args.prefix.prefix];
      return l10n.lookupFormat('disconnectReply', [ removed.length ]);
    });
  }
};

exports.items = [ connection, connect, disconnect ];

});
















define('util/connect/connector', ['require', 'exports', 'module' , 'util/promise'], function(require, exports, module) {

'use strict';

var debuggerSocketConnect = Components.utils.import('resource://gre/modules/devtools/dbg-client.jsm', {}).debuggerSocketConnect;
var DebuggerClient = Components.utils.import('resource://gre/modules/devtools/dbg-client.jsm', {}).DebuggerClient;

var promise = require('util/promise');




Object.defineProperty(exports, 'defaultPort', {
  get: function() {
    var Services = Components.utils.import('resource://gre/modules/Services.jsm', {}).Services;
    try {
      return Services.prefs.getIntPref('devtools.debugger.chrome-debugging-port');
    }
    catch (ex) {
      console.error('Can\'t use default port from prefs. Using 9999');
      return 9999;
    }
  },
  enumerable: true
});





exports.connect = function(prefix, host, port) {
  var connection = new Connection(prefix, host, port);
  return connection.connect().then(function() {
    return connection;
  });
};




function Connection(prefix, host, port) {
  this.prefix = prefix;
  this.host = host;
  this.port = port;

  
  this.actor = undefined;
  this.transport = undefined;
  this.client = undefined;

  this.requests = {};
  this.nextRequestId = 0;
}







Connection.prototype.connect = function() {
  var deferred = promise.defer();

  this.transport = debuggerSocketConnect(this.host, this.port);
  this.client = new DebuggerClient(this.transport);

  this.client.connect(function() {
    this.client.listTabs(function(response) {
      this.actor = response.gcliActor;
      deferred.resolve();
    }.bind(this));
  }.bind(this));

  return deferred.promise;
};





Connection.prototype.getCommandSpecs = function() {
  var deferred = promise.defer();

  var request = { to: this.actor, type: 'getCommandSpecs' };

  this.client.request(request, function(response) {
    deferred.resolve(response.commandSpecs);
  });

  return deferred.promise;
};




Connection.prototype.execute = function(typed, cmdArgs) {
  var request = new Request(this.actor, typed, cmdArgs);
  this.requests[request.json.requestId] = request;

  this.client.request(request.json, function(response) {
    var request = this.requests[response.requestId];
    delete this.requests[response.requestId];

    request.complete(response.error, response.type, response.data);
  }.bind(this));

  return request.promise;
};

exports.disconnectSupportsForce = false;




Connection.prototype.disconnect = function(force) {
  var deferred = promise.defer();

  this.client.close(function() {
    deferred.resolve();
  });

  return deferred.promise;
};





function Request(actor, typed, args) {
  this.json = {
    to: actor,
    type: 'execute',
    typed: typed,
    args: args,
    requestId: 'id-' + Request._nextRequestId++,
  };

  this._deferred = promise.defer();
  this.promise = this._deferred.promise;
}

Request._nextRequestId = 0;







Request.prototype.complete = function(error, type, data) {
  this._deferred.resolve({
    error: error,
    type: type,
    data: data
  });
};


});
















define('gcli/commands/context', ['require', 'exports', 'module' , 'util/l10n'], function(require, exports, module) {

'use strict';

var l10n = require('util/l10n');




var context = {
  item: 'command',
  name: 'context',
  description: l10n.lookup('contextDesc'),
  manual: l10n.lookup('contextManual'),
  params: [
   {
     name: 'prefix',
     type: 'command',
     description: l10n.lookup('contextPrefixDesc'),
     defaultValue: null
   }
  ],
  returnType: 'string',
  noRemote: true,
  exec: function echo(args, context) {
    
    var requisition = context.__dlhjshfw;

    if (args.prefix == null) {
      requisition.prefix = null;
      return l10n.lookup('contextEmptyReply');
    }

    if (args.prefix.exec != null) {
      throw new Error(l10n.lookupFormat('contextNotParentError',
                                        [ args.prefix.name ]));
    }

    requisition.prefix = args.prefix.name;
    return l10n.lookupFormat('contextReply', [ args.prefix.name ]);
  }
};

exports.items = [ context ];

});
















define('gcli/commands/help', ['require', 'exports', 'module' , 'util/l10n', 'gcli/canon', 'text!gcli/commands/help_man.html', 'text!gcli/commands/help_list.html', 'text!gcli/commands/help.css'], function(require, exports, module) {

'use strict';

var l10n = require('util/l10n');
var canon = require('gcli/canon');

var helpManHtml = require('text!gcli/commands/help_man.html');
var helpListHtml = require('text!gcli/commands/help_list.html');
var helpCss = require('text!gcli/commands/help.css');




var helpCommand = {
  item: 'converter',
  from: 'commandData',
  to: 'view',
  exec: function(commandData, context) {
    return context.createView({
      html: helpManHtml,
      options: { allowEval: true, stack: 'help_man.html' },
      data: {
        l10n: l10n.propertyLookup,
        onclick: context.update,
        ondblclick: context.updateExec,
        describe: function(item) {
          return item.manual || item.description;
        },
        getTypeDescription: function(param) {
          var input = '';
          if (param.defaultValue === undefined) {
            input = l10n.lookup('helpManRequired');
          }
          else if (param.defaultValue === null) {
            input = l10n.lookup('helpManOptional');
          }
          else {
            var defaultValue = param.type.stringify(param.defaultValue);
            input = l10n.lookupFormat('helpManDefault', [ defaultValue ]);
          }
          return '(' + param.type.name + ', ' + input + ')';
        },
        getSynopsis: function(param) {
          var short = param.short ? '|-' + param.short : '';
          if (param.isPositionalAllowed) {
            return param.defaultValue !== undefined ?
                '[' + param.name + short + ']' :
                '<' + param.name + short + '>';
          }
          else {
            return param.type.name === 'boolean' ?
                '[--' + param.name + short + ']' :
                '[--' + param.name + short + ' ...]';
          }
        },
        command: commandData.command,
        subcommands: commandData.subcommands
      },
      css: helpCss,
      cssId: 'gcli-help'
    });
  }
};




var helpCommands = {
  item: 'converter',
  from: 'commandsData',
  to: 'view',
  exec: function(commandsData, context) {
    var heading;
    if (commandsData.commands.length === 0) {
      heading = l10n.lookupFormat('helpListNone', [ commandsData.prefix ]);
    }
    else if (commandsData.prefix == null) {
      heading = l10n.lookup('helpListAll');
    }
    else {
      heading = l10n.lookupFormat('helpListPrefix', [ commandsData.prefix ]);
    }

    return context.createView({
      html: helpListHtml,
      options: { allowEval: true, stack: 'help_list.html' },
      data: {
        l10n: l10n.propertyLookup,
        includeIntro: commandsData.prefix == null,
        heading: heading,
        onclick: context.update,
        ondblclick: context.updateExec,
        matchingCommands: commandsData.commands
      },
      css: helpCss,
      cssId: 'gcli-help'
    });
  }
};




var help = {
  item: 'command',
  name: 'help',
  description: l10n.lookup('helpDesc'),
  manual: l10n.lookup('helpManual'),
  params: [
    {
      name: 'search',
      type: 'string',
      description: l10n.lookup('helpSearchDesc'),
      manual: l10n.lookup('helpSearchManual3'),
      defaultValue: null
    }
  ],

  exec: function(args, context) {
    var command = canon.getCommand(args.search);
    if (command) {
      return context.typedData('commandData', {
        command: command,
        subcommands: getSubCommands(command)
      });
    }

    return context.typedData('commandsData', {
      prefix: args.search,
      commands: getMatchingCommands(args.search)
    });
  }
};




function getMatchingCommands(prefix) {
  var commands = canon.getCommands().filter(function(command) {
    if (command.hidden) {
      return false;
    }

    if (prefix && command.name.indexOf(prefix) !== 0) {
      
      return false;
    }
    if (!prefix && command.name.indexOf(' ') != -1) {
      
      return false;
    }
    return true;
  });
  commands.sort(function(c1, c2) {
    return c1.name.localeCompare(c2.name);
  });

  return commands;
}




function getSubCommands(command) {
  if (command.exec != null) {
    return [];
  }

  var subcommands = canon.getCommands().filter(function(subcommand) {
    return subcommand.name.indexOf(command.name) === 0 &&
           subcommand.name !== command.name &&
           !subcommand.hidden;
  });

  subcommands.sort(function(c1, c2) {
    return c1.name.localeCompare(c2.name);
  });

  return subcommands;
}

exports.items = [ help, helpCommand, helpCommands ];

});
define("text!gcli/commands/help_man.html", [], "\n" +
  "<div>\n" +
  "  <h3>${command.name}</h3>\n" +
  "\n" +
  "  <h4 class=\"gcli-help-header\">\n" +
  "    ${l10n.helpManSynopsis}:\n" +
  "    <span class=\"gcli-out-shortcut\" onclick=\"${onclick}\" data-command=\"${command.name}\">\n" +
  "      ${command.name}\n" +
  "      <span foreach=\"param in ${command.params}\">${getSynopsis(param)} </span>\n" +
  "    </span>\n" +
  "  </h4>\n" +
  "\n" +
  "  <h4 class=\"gcli-help-header\">${l10n.helpManDescription}:</h4>\n" +
  "\n" +
  "  <p class=\"gcli-help-description\">${describe(command)}</p>\n" +
  "\n" +
  "  <div if=\"${command.exec}\">\n" +
  "    <div foreach=\"groupName in ${command.paramGroups}\">\n" +
  "      <h4 class=\"gcli-help-header\">${groupName}:</h4>\n" +
  "      <ul class=\"gcli-help-parameter\">\n" +
  "        <li if=\"${command.params.length === 0}\">${l10n.helpManNone}</li>\n" +
  "        <li foreach=\"param in ${command.paramGroups[groupName]}\">\n" +
  "          <code>${getSynopsis(param)}</code> <em>${getTypeDescription(param)}</em>\n" +
  "          <br/>\n" +
  "          ${describe(param)}\n" +
  "        </li>\n" +
  "      </ul>\n" +
  "    </div>\n" +
  "  </div>\n" +
  "\n" +
  "  <div if=\"${!command.exec}\">\n" +
  "    <h4 class=\"gcli-help-header\">${l10n.subCommands}:</h4>\n" +
  "\n" +
  "    <ul class=\"gcli-help-${subcommands}\">\n" +
  "      <li if=\"${subcommands.length === 0}\">${l10n.subcommandsNone}</li>\n" +
  "      <li foreach=\"subcommand in ${subcommands}\">\n" +
  "        <strong>${subcommand.name}</strong>:\n" +
  "        ${subcommand.description}\n" +
  "        <span class=\"gcli-out-shortcut\" data-command=\"help ${subcommand.name}\"\n" +
  "            onclick=\"${onclick}\" ondblclick=\"${ondblclick}\">\n" +
  "          help ${subcommand.name}\n" +
  "        </span>\n" +
  "      </li>\n" +
  "    </ul>\n" +
  "  </div>\n" +
  "\n" +
  "</div>\n" +
  "");

define("text!gcli/commands/help_list.html", [], "\n" +
  "<div>\n" +
  "  <h3>${heading}</h3>\n" +
  "\n" +
  "  <table>\n" +
  "    <tr foreach=\"command in ${matchingCommands}\"\n" +
  "        onclick=\"${onclick}\" ondblclick=\"${ondblclick}\">\n" +
  "      <th class=\"gcli-help-name\">${command.name}</th>\n" +
  "      <td class=\"gcli-help-arrow\">-</td>\n" +
  "      <td>\n" +
  "        ${command.description}\n" +
  "        <span class=\"gcli-out-shortcut\" data-command=\"help ${command.name}\">help ${command.name}</span>\n" +
  "      </td>\n" +
  "    </tr>\n" +
  "  </table>\n" +
  "</div>\n" +
  "");

define("text!gcli/commands/help.css", [], "");

















define('gcli/commands/pref', ['require', 'exports', 'module' , 'util/l10n', 'gcli/settings', 'text!gcli/commands/pref_set_check.html'], function(require, exports, module) {

'use strict';

var l10n = require('util/l10n');
var settings = require('gcli/settings');




var allowSet = {
  item: 'setting',
  name: 'allowSet',
  type: 'boolean',
  description: l10n.lookup('allowSetDesc'),
  defaultValue: false
};




var pref = {
  item: 'command',
  name: 'pref',
  description: l10n.lookup('prefDesc'),
  manual: l10n.lookup('prefManual')
};




var prefShow = {
  item: 'command',
  name: 'pref show',
  description: l10n.lookup('prefShowDesc'),
  manual: l10n.lookup('prefShowManual'),
  params: [
    {
      name: 'setting',
      type: 'setting',
      description: l10n.lookup('prefShowSettingDesc'),
      manual: l10n.lookup('prefShowSettingManual')
    }
  ],
  exec: function(args, context) {
    return l10n.lookupFormat('prefShowSettingValue',
                             [ args.setting.name, args.setting.value ]);
  }
};




var prefSet = {
  item: 'command',
  name: 'pref set',
  description: l10n.lookup('prefSetDesc'),
  manual: l10n.lookup('prefSetManual'),
  params: [
    {
      name: 'setting',
      type: 'setting',
      description: l10n.lookup('prefSetSettingDesc'),
      manual: l10n.lookup('prefSetSettingManual')
    },
    {
      name: 'value',
      type: 'settingValue',
      description: l10n.lookup('prefSetValueDesc'),
      manual: l10n.lookup('prefSetValueManual')
    }
  ],
  exec: function(args, context) {
    var allowSet = settings.getSetting('allowSet');
    if (!allowSet.value &&
        args.setting.name !== allowSet.name) {
      return context.typedData('prefSetWarning', null);
    }

    args.setting.value = args.value;
  }
};

var prefSetWarning = {
  item: 'converter',
  from: 'prefSetWarning',
  to: 'view',
  exec: function(data, context) {
    var allowSet = settings.getSetting('settings');
    return context.createView({
      html: require('text!gcli/commands/pref_set_check.html'),
      options: { allowEval: true, stack: 'pref_set_check.html' },
      data: {
        l10n: l10n.propertyLookup,
        activate: function() {
          context.updateExec('pref set ' + allowSet.name + ' true');
        }
      }
    });
  }
};




var prefReset = {
  item: 'command',
  name: 'pref reset',
  description: l10n.lookup('prefResetDesc'),
  manual: l10n.lookup('prefResetManual'),
  params: [
    {
      name: 'setting',
      type: 'setting',
      description: l10n.lookup('prefResetSettingDesc'),
      manual: l10n.lookup('prefResetSettingManual')
    }
  ],
  exec: function(args, context) {
    args.setting.setDefault();
  }
};

exports.items = [
  pref, prefShow, prefSet, prefReset,
  allowSet, prefSetWarning
];


});
define("text!gcli/commands/pref_set_check.html", [], "<div>\n" +
  "  <p><strong>${l10n.prefSetCheckHeading}</strong></p>\n" +
  "  <p>${l10n.prefSetCheckBody}</p>\n" +
  "  <button onclick=\"${activate}\">${l10n.prefSetCheckGo}</button>\n" +
  "</div>\n" +
  "");

















define('gcli/ui/ffdisplay', ['require', 'exports', 'module' , 'gcli/ui/inputter', 'gcli/ui/completer', 'gcli/ui/tooltip', 'gcli/ui/focus', 'gcli/cli', 'gcli/types/javascript', 'gcli/types/node', 'gcli/types/resource', 'util/host', 'gcli/ui/intro', 'gcli/canon'], function(require, exports, module) {

'use strict';

var Inputter = require('gcli/ui/inputter').Inputter;
var Completer = require('gcli/ui/completer').Completer;
var Tooltip = require('gcli/ui/tooltip').Tooltip;
var FocusManager = require('gcli/ui/focus').FocusManager;

var Requisition = require('gcli/cli').Requisition;

var cli = require('gcli/cli');
var jstype = require('gcli/types/javascript');
var nodetype = require('gcli/types/node');
var resource = require('gcli/types/resource');
var host = require('util/host');
var intro = require('gcli/ui/intro');

var CommandOutputManager = require('gcli/canon').CommandOutputManager;





function setContentDocument(document) {
  if (document) {
    nodetype.setDocument(document);
    resource.setDocument(document);
  }
  else {
    resource.unsetDocument();
    nodetype.unsetDocument();
    jstype.unsetGlobalObject();
  }
}



















function FFDisplay(options) {
  if (options.eval) {
    cli.setEvalFunction(options.eval);
  }
  setContentDocument(options.contentDocument);
  host.chromeWindow = options.chromeWindow;

  this.commandOutputManager = options.commandOutputManager;
  if (this.commandOutputManager == null) {
    this.commandOutputManager = new CommandOutputManager();
  }

  this.onOutput = this.commandOutputManager.onOutput;
  this.requisition = new Requisition(options.environment,
                                     options.outputDocument,
                                     this.commandOutputManager);

  this.focusManager = new FocusManager(options, {
    document: options.chromeDocument,
    requisition: this.requisition,
  });
  this.onVisibilityChange = this.focusManager.onVisibilityChange;

  this.inputter = new Inputter(options, {
    requisition: this.requisition,
    focusManager: this.focusManager,
    element: options.inputElement
  });

  this.completer = new Completer(options, {
    requisition: this.requisition,
    inputter: this.inputter,
    backgroundElement: options.backgroundElement,
    element: options.completeElement
  });

  this.tooltip = new Tooltip(options, {
    requisition: this.requisition,
    focusManager: this.focusManager,
    inputter: this.inputter,
    element: options.hintElement
  });

  this.inputter.tooltip = this.tooltip;

  if (options.consoleWrap) {
    this.consoleWrap = options.consoleWrap;
    var win = options.consoleWrap.ownerDocument.defaultView;
    this.resizer = this.resizer.bind(this);

    win.addEventListener('resize', this.resizer, false);
    this.requisition.onTextChange.add(this.resizer, this);
  }

  this.options = options;
}






FFDisplay.prototype.maybeShowIntro = function() {
  intro.maybeShowIntro(this.commandOutputManager,
                       this.requisition.conversionContext);
};








FFDisplay.prototype.reattach = function(options) {
  setContentDocument(options.contentDocument);
  host.chromeWindow = options.chromeWindow;
  this.requisition.environment = options.environment;
};




FFDisplay.prototype.destroy = function() {
  if (this.consoleWrap) {
    var win = this.options.consoleWrap.ownerDocument.defaultView;

    this.requisition.onTextChange.remove(this.resizer, this);
    win.removeEventListener('resize', this.resizer, false);
  }

  this.tooltip.destroy();
  this.completer.destroy();
  this.inputter.destroy();
  this.focusManager.destroy();

  this.requisition.destroy();

  host.chromeWindow = undefined;
  setContentDocument(null);
  cli.unsetEvalFunction();

  delete this.options;

  
  
  
  
  
  
};




FFDisplay.prototype.resizer = function() {
  
  
  

  var parentRect = this.options.consoleWrap.getBoundingClientRect();
  
  var parentHeight = parentRect.bottom - parentRect.top - 64;

  
  
  if (parentHeight < 100) {
    this.options.hintElement.classList.add('gcliterm-hint-nospace');
  }
  else {
    this.options.hintElement.classList.remove('gcliterm-hint-nospace');
    this.options.hintElement.style.overflowY = null;
    this.options.hintElement.style.borderBottomColor = 'white';
  }

  
  
  var doc = this.options.hintElement.ownerDocument;

  var outputNode = this.options.hintElement.parentNode.parentNode.children[1];
  var outputs = outputNode.getElementsByClassName('gcliterm-msg-body');
  var listItems = outputNode.getElementsByClassName('hud-msg-node');

  
  
  
  
  
  var scrollbarWidth = 20;

  if (listItems.length > 0) {
    var parentWidth = outputNode.getBoundingClientRect().width - scrollbarWidth;
    var otherWidth;
    var body;

    for (var i = 0; i < listItems.length; ++i) {
      var listItem = listItems[i];
      
      otherWidth = 0;
      body = null;

      for (var j = 0; j < listItem.children.length; j++) {
        var child = listItem.children[j];

        if (child.classList.contains('gcliterm-msg-body')) {
          body = child.children[0];
        }
        else {
          otherWidth += child.getBoundingClientRect().width;
        }

        var styles = doc.defaultView.getComputedStyle(child, null);
        otherWidth += parseInt(styles.borderLeftWidth, 10) +
                      parseInt(styles.borderRightWidth, 10) +
                      parseInt(styles.paddingLeft, 10) +
                      parseInt(styles.paddingRight, 10) +
                      parseInt(styles.marginLeft, 10) +
                      parseInt(styles.marginRight, 10);
      }

      if (body) {
        body.style.width = (parentWidth - otherWidth) + 'px';
      }
    }
  }
};

exports.FFDisplay = FFDisplay;

});
















define('gcli/ui/inputter', ['require', 'exports', 'module' , 'util/promise', 'util/util', 'gcli/types', 'gcli/history', 'text!gcli/ui/inputter.css'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var util = require('util/util');
var KeyEvent = require('util/util').KeyEvent;

var Status = require('gcli/types').Status;
var History = require('gcli/history').History;

var inputterCss = require('text!gcli/ui/inputter.css');

var RESOLVED = promise.resolve(true);











function Inputter(options, components) {
  this.requisition = components.requisition;
  this.focusManager = components.focusManager;

  this.element = components.element;
  this.element.classList.add('gcli-in-input');
  this.element.spellcheck = false;

  this.document = this.element.ownerDocument;

  this.scratchpad = options.scratchpad;

  if (inputterCss != null) {
    this.style = util.importCss(inputterCss, this.document, 'gcli-inputter');
  }

  
  this.lastTabDownAt = 0;

  
  this._caretChange = null;

  
  this.onKeyDown = this.onKeyDown.bind(this);
  this.onKeyUp = this.onKeyUp.bind(this);
  this.element.addEventListener('keydown', this.onKeyDown, false);
  this.element.addEventListener('keyup', this.onKeyUp, false);

  
  this.history = new History();
  this._scrollingThroughHistory = false;

  
  this._choice = null;
  this.onChoiceChange = util.createEvent('Inputter.onChoiceChange');

  
  this.onMouseUp = this.onMouseUp.bind(this);
  this.element.addEventListener('mouseup', this.onMouseUp, false);

  if (this.focusManager) {
    this.focusManager.addMonitoredElement(this.element, 'input');
  }

  
  this._completed = RESOLVED;

  this.requisition.onTextChange.add(this.textChanged, this);

  this.assignment = this.requisition.getAssignmentAt(0);
  this.onAssignmentChange = util.createEvent('Inputter.onAssignmentChange');
  this.onInputChange = util.createEvent('Inputter.onInputChange');

  this.onResize = util.createEvent('Inputter.onResize');
  this.onWindowResize = this.onWindowResize.bind(this);
  this.document.defaultView.addEventListener('resize', this.onWindowResize, false);

  this._previousValue = undefined;
  this.requisition.update(this.element.value || '');
}




Inputter.prototype.destroy = function() {
  this.document.defaultView.removeEventListener('resize', this.onWindowResize, false);

  this.requisition.onTextChange.remove(this.textChanged, this);
  if (this.focusManager) {
    this.focusManager.removeMonitoredElement(this.element, 'input');
  }

  this.element.removeEventListener('mouseup', this.onMouseUp, false);
  this.element.removeEventListener('keydown', this.onKeyDown, false);
  this.element.removeEventListener('keyup', this.onKeyUp, false);

  this.history.destroy();

  if (this.style) {
    this.style.parentNode.removeChild(this.style);
    delete this.style;
  }

  delete this.onMouseUp;
  delete this.onKeyDown;
  delete this.onKeyUp;
  delete this.onWindowResize;
  delete this.tooltip;
  delete this.document;
  delete this.element;
};





Inputter.prototype.onWindowResize = function() {
  
  if (!this.element) {
    return;
  }

  
  var dimensions = this.getDimensions();
  if (dimensions) {
    this.onResize(dimensions);
  }
};





Inputter.prototype.getDimensions = function() {
  
  if (!this.element.getBoundingClientRect) {
    return undefined;
  }

  var fixedLoc = {};
  var currentElement = this.element.parentNode;
  while (currentElement && currentElement.nodeName !== '#document') {
    var style = this.document.defaultView.getComputedStyle(currentElement, '');
    if (style) {
      var position = style.getPropertyValue('position');
      if (position === 'absolute' || position === 'fixed') {
        var bounds = currentElement.getBoundingClientRect();
        fixedLoc.top = bounds.top;
        fixedLoc.left = bounds.left;
        break;
      }
    }
    currentElement = currentElement.parentNode;
  }

  var rect = this.element.getBoundingClientRect();
  return {
    top: rect.top - (fixedLoc.top || 0) + 1,
    height: rect.bottom - rect.top - 1,
    left: rect.left - (fixedLoc.left || 0) + 2,
    width: rect.right - rect.left
  };
};




Inputter.prototype.onMouseUp = function(ev) {
  this._checkAssignment();
};




Inputter.prototype.textChanged = function() {
  if (!this.document) {
    return; 
  }

  if (this._caretChange == null) {
    
    
    
    this._caretChange = Caret.TO_ARG_END;
  }

  var newStr = this.requisition.toString();
  var input = this.getInputState();

  input.typed = newStr;
  this._processCaretChange(input);

  if (this.element.value !== newStr) {
    this.element.value = newStr;
  }
  this.onInputChange({ inputState: input });
};





var Caret = {
  


  NO_CHANGE: 0,

  


  SELECT_ALL: 1,

  


  TO_END: 2,

  



  TO_ARG_END: 3
};






Inputter.prototype._processCaretChange = function(input) {
  var start, end;
  switch (this._caretChange) {
    case Caret.SELECT_ALL:
      start = 0;
      end = input.typed.length;
      break;

    case Caret.TO_END:
      start = input.typed.length;
      end = input.typed.length;
      break;

    case Caret.TO_ARG_END:
      
      
      
      start = input.cursor.start;
      do {
        start++;
      }
      while (start < input.typed.length && input.typed[start - 1] !== ' ');

      end = start;
      break;

    default:
      start = input.cursor.start;
      end = input.cursor.end;
      break;
  }

  start = (start > input.typed.length) ? input.typed.length : start;
  end = (end > input.typed.length) ? input.typed.length : end;

  var newInput = {
    typed: input.typed,
    cursor: { start: start, end: end }
  };

  if (this.element.selectionStart !== start) {
    this.element.selectionStart = start;
  }
  if (this.element.selectionEnd !== end) {
    this.element.selectionEnd = end;
  }

  this._checkAssignment(start);

  this._caretChange = null;
  return newInput;
};








Inputter.prototype._checkAssignment = function(start) {
  if (start == null) {
    start = this.element.selectionStart;
  }
  var newAssignment = this.requisition.getAssignmentAt(start);
  if (this.assignment !== newAssignment) {
    if (this.assignment.param.type.onLeave) {
      this.assignment.param.type.onLeave(this.assignment);
    }

    this.assignment = newAssignment;
    this.onAssignmentChange({ assignment: this.assignment });

    if (this.assignment.param.type.onEnter) {
      this.assignment.param.type.onEnter(this.assignment);
    }
  }
  else {
    if (this.assignment && this.assignment.param.type.onChange) {
      this.assignment.param.type.onChange(this.assignment);
    }
  }

  
  
  
  
  
  
  if (this.focusManager) {
    this.focusManager.setError(this.assignment.message);
  }
};










Inputter.prototype.setInput = function(str) {
  this._caretChange = Caret.TO_END;
  return this.requisition.update(str);
};





Inputter.prototype.setCursor = function(cursor) {
  this._caretChange = Caret.NO_CHANGE;
  this._processCaretChange({ typed: this.element.value, cursor: cursor });
};




Inputter.prototype.focus = function() {
  this.element.focus();
  this._checkAssignment();
};





Inputter.prototype.onKeyDown = function(ev) {
  if (ev.keyCode === KeyEvent.DOM_VK_UP || ev.keyCode === KeyEvent.DOM_VK_DOWN) {
    ev.preventDefault();
    return;
  }

  
  
  if (ev.keyCode === KeyEvent.DOM_VK_F1 ||
      ev.keyCode === KeyEvent.DOM_VK_ESCAPE ||
      ev.keyCode === KeyEvent.DOM_VK_UP ||
      ev.keyCode === KeyEvent.DOM_VK_DOWN) {
    return;
  }

  if (this.focusManager) {
    this.focusManager.onInputChange();
  }

  if (ev.keyCode === KeyEvent.DOM_VK_TAB) {
    this.lastTabDownAt = 0;
    if (!ev.shiftKey) {
      ev.preventDefault();
      
      
      this.lastTabDownAt = ev.timeStamp;
    }
    if (ev.metaKey || ev.altKey || ev.crtlKey) {
      if (this.document.commandDispatcher) {
        this.document.commandDispatcher.advanceFocus();
      }
      else {
        this.element.blur();
      }
    }
  }
};






Inputter.prototype.onKeyUp = function(ev) {
  this.handleKeyUp(ev).then(null, util.errorHandler);
};






Inputter.prototype.handleKeyUp = function(ev) {
  if (this.focusManager && ev.keyCode === KeyEvent.DOM_VK_F1) {
    this.focusManager.helpRequest();
    return RESOLVED;
  }

  if (this.focusManager && ev.keyCode === KeyEvent.DOM_VK_ESCAPE) {
    this.focusManager.removeHelp();
    return RESOLVED;
  }

  if (ev.keyCode === KeyEvent.DOM_VK_UP) {
    return this._handleUpArrow();
  }

  if (ev.keyCode === KeyEvent.DOM_VK_DOWN) {
    return this._handleDownArrow();
  }

  if (ev.keyCode === KeyEvent.DOM_VK_RETURN) {
    return this._handleReturn();
  }

  if (ev.keyCode === KeyEvent.DOM_VK_TAB && !ev.shiftKey) {
    return this._handleTab(ev);
  }

  
  if (this.scratchpad && this.scratchpad.shouldActivate(ev)) {
    if (this.scratchpad.activate(this.element.value)) {
      return this.requisition.update('');
    }
    return RESOLVED;
  }

  if (this._previousValue === this.element.value) {
    return RESOLVED;
  }

  this._scrollingThroughHistory = false;
  this._caretChange = Caret.NO_CHANGE;

  this._completed = this.requisition.update(this.element.value);
  this._previousValue = this.element.value;

  return this._completed.then(function(updated) {
    
    if (updated) {
      this._choice = null;
      this.onChoiceChange({ choice: this._choice });
    }
  }.bind(this));
};




Inputter.prototype._handleUpArrow = function() {
  if (this.tooltip && this.tooltip.isMenuShowing) {
    this.changeChoice(-1);
    return RESOLVED;
  }

  if (this.element.value === '' || this._scrollingThroughHistory) {
    this._scrollingThroughHistory = true;
    return this.requisition.update(this.history.backward());
  }

  
  
  if (this.assignment.getStatus() === Status.VALID) {
    this.requisition.increment(this.assignment);
    
    if (this.focusManager) {
      this.focusManager.onInputChange();
    }
  }
  else {
    this.changeChoice(-1);
  }

  return RESOLVED;
};




Inputter.prototype._handleDownArrow = function() {
  if (this.tooltip && this.tooltip.isMenuShowing) {
    this.changeChoice(+1);
    return RESOLVED;
  }

  if (this.element.value === '' || this._scrollingThroughHistory) {
    this._scrollingThroughHistory = true;
    return this.requisition.update(this.history.forward());
  }

  
  if (this.assignment.getStatus() === Status.VALID) {
    this.requisition.decrement(this.assignment,
                               this.requisition.executionContext);
    
    if (this.focusManager) {
      this.focusManager.onInputChange();
    }
  }
  else {
    this.changeChoice(+1);
  }

  return RESOLVED;
};




Inputter.prototype._handleReturn = function() {
  
  if (this.requisition.status === Status.VALID) {
    this._scrollingThroughHistory = false;
    this.history.add(this.element.value);
    this.requisition.exec();
  }
  else {
    
    
    if (!this.tooltip.selectChoice()) {
      this.focusManager.setError(true);
    }
  }

  this._choice = null;
  return RESOLVED;
};





Inputter.prototype._handleTab = function(ev) {
  
  
  var hasContents = (this.element.value.length > 0);

  
  
  
  
  
  
  if (hasContents && this.lastTabDownAt + 1000 > ev.timeStamp) {
    
    
    
    this._caretChange = Caret.TO_ARG_END;
    var inputState = this.getInputState();
    this._processCaretChange(inputState);

    if (this._choice == null) {
      this._choice = 0;
    }

    
    
    
    this._completed = this.requisition.complete(inputState.cursor,
                                                this._choice);
    this._previousValue = this.element.value;
  }
  this.lastTabDownAt = 0;
  this._scrollingThroughHistory = false;

  return this._completed.then(function(updated) {
    
    if (updated) {
      this._choice = null;
      this.onChoiceChange({ choice: this._choice });
    }
  }.bind(this));
};





Inputter.prototype.changeChoice = function(amount) {
  if (this._choice == null) {
    this._choice = 0;
  }
  
  
  
  this._choice += amount;
  this.onChoiceChange({ choice: this._choice });
};





Inputter.prototype.getCurrentAssignment = function() {
  var start = this.element.selectionStart;
  return this.requisition.getAssignmentAt(start);
};




Inputter.prototype.getInputState = function() {
  var input = {
    typed: this.element.value,
    cursor: {
      start: this.element.selectionStart,
      end: this.element.selectionEnd
    }
  };

  
  
  if (input.typed == null) {
    input = { typed: '', cursor: { start: 0, end: 0 } };
  }

  
  if (input.cursor.start == null) {
    input.cursor.start = 0;
  }

  return input;
};

exports.Inputter = Inputter;


});
















define('gcli/history', ['require', 'exports', 'module' ], function(require, exports, module) {

'use strict';






function History() {
  
  
  
  
  this._buffer = [''];

  
  
  this._current = 0;
}




History.prototype.destroy = function() {
  delete this._buffer;
};




History.prototype.add = function(command) {
  this._buffer.splice(1, 0, command);
  this._current = 0;
};




History.prototype.forward = function() {
  if (this._current > 0 ) {
    this._current--;
  }
  return this._buffer[this._current];
};




History.prototype.backward = function() {
  if (this._current < this._buffer.length - 1) {
    this._current++;
  }
  return this._buffer[this._current];
};

exports.History = History;

});
define("text!gcli/ui/inputter.css", [], "");

















define('gcli/ui/completer', ['require', 'exports', 'module' , 'util/promise', 'util/util', 'util/domtemplate', 'text!gcli/ui/completer.html'], function(require, exports, module) {

'use strict';

var promise = require('util/promise');
var util = require('util/util');
var domtemplate = require('util/domtemplate');

var completerHtml = require('text!gcli/ui/completer.html');












function Completer(options, components) {
  this.requisition = components.requisition;
  this.scratchpad = options.scratchpad;
  this.input = { typed: '', cursor: { start: 0, end: 0 } };
  this.choice = 0;

  this.element = components.element;
  this.element.classList.add('gcli-in-complete');
  this.element.setAttribute('tabindex', '-1');
  this.element.setAttribute('aria-live', 'polite');

  this.document = this.element.ownerDocument;

  this.inputter = components.inputter;

  this.inputter.onInputChange.add(this.update, this);
  this.inputter.onAssignmentChange.add(this.update, this);
  this.inputter.onChoiceChange.add(this.update, this);

  this.autoResize = components.autoResize;
  if (this.autoResize) {
    this.inputter.onResize.add(this.resized, this);

    var dimensions = this.inputter.getDimensions();
    if (dimensions) {
      this.resized(dimensions);
    }
  }

  this.template = util.toDom(this.document, completerHtml);
  
  util.removeWhitespace(this.template, true);

  this.update();
}




Completer.prototype.destroy = function() {
  this.inputter.onInputChange.remove(this.update, this);
  this.inputter.onAssignmentChange.remove(this.update, this);
  this.inputter.onChoiceChange.remove(this.update, this);

  if (this.autoResize) {
    this.inputter.onResize.remove(this.resized, this);
  }

  delete this.document;
  delete this.element;
  delete this.template;
  delete this.inputter;
};




Completer.prototype.resized = function(ev) {
  this.element.style.top = ev.top + 'px';
  this.element.style.height = ev.height + 'px';
  this.element.style.lineHeight = ev.height + 'px';
  this.element.style.left = ev.left + 'px';
  this.element.style.width = ev.width + 'px';
};




Completer.prototype.update = function(ev) {
  this.choice = (ev && ev.choice != null) ? ev.choice : 0;

  var data = this._getCompleterTemplateData();
  var template = this.template.cloneNode(true);
  domtemplate.template(template, data, { stack: 'completer.html' });

  util.clearElement(this.element);
  while (template.hasChildNodes()) {
    this.element.appendChild(template.firstChild);
  }
};




Completer.prototype._getCompleterTemplateData = function() {
  
  
  var promisedDirectTabText = promise.defer();
  var promisedArrowTabText = promise.defer();
  var promisedEmptyParameters = promise.defer();

  var input = this.inputter.getInputState();
  var current = this.requisition.getAssignmentAt(input.cursor.start);
  var predictionPromise;

  if (input.typed.trim().length !== 0) {
    predictionPromise = current.getPredictionAt(this.choice);
  }

  
  var onError = function(ex) {
    promisedDirectTabText.reject(ex);
    promisedArrowTabText.reject(ex);
    promisedEmptyParameters.reject(ex);
  };

  promise.resolve(predictionPromise).then(function(prediction) {
    
    
    var directTabText = '';
    var arrowTabText = '';
    var emptyParameters = [];

    if (input.typed.trim().length !== 0) {
      var cArg = current.arg;

      if (prediction) {
        var tabText = prediction.name;
        var existing = cArg.text;

        
        
        
        
        
        
        if (current.isInName()) {
          tabText = ' ' + tabText;
        }

        if (existing !== tabText) {
          
          
          
          var inputValue = existing.replace(/^\s*/, '');
          var isStrictCompletion = tabText.indexOf(inputValue) === 0;
          if (isStrictCompletion && input.cursor.start === input.typed.length) {
            
            var numLeadingSpaces = existing.match(/^(\s*)/)[0].length;

            directTabText = tabText.slice(existing.length - numLeadingSpaces);
          }
          else {
            
            
            arrowTabText = '\u21E5 ' + tabText;
          }
        }
      }
      else {
        
        
        
        if (cArg.type === 'NamedArgument' && cArg.valueArg == null) {
          emptyParameters.push('<' + current.param.type.name + '>\u00a0');
        }
      }
    }

    
    
    if (directTabText !== '') {
      directTabText += '\u00a0';
    }
    else if (!this.requisition.typedEndsWithSeparator()) {
      emptyParameters.unshift('\u00a0');
    }

    
    
    
    
    

    this.requisition.getAssignments().forEach(function(assignment) {
      
      if (!assignment.param.isPositionalAllowed) {
        return;
      }

      
      if (assignment.arg.toString().trim() !== '') {
        return;
      }

      if (directTabText !== '' && current === assignment) {
        return;
      }

      var text = (assignment.param.isDataRequired) ?
          '<' + assignment.param.name + '>\u00a0' :
          '[' + assignment.param.name + ']\u00a0';

      emptyParameters.push(text);
    }.bind(this));

    var command = this.requisition.commandAssignment.value;
    var addOptionsMarker = false;

    
    
    if (command && command.hasNamedParameters) {
      command.params.forEach(function(param) {
        var arg = this.requisition.getAssignment(param.name).arg;
        if (!param.isPositionalAllowed && !param.hidden
                && arg.type === 'BlankArgument') {
          addOptionsMarker = true;
        }
      }, this);
    }

    if (addOptionsMarker) {
      
      
      emptyParameters.push('[options]\u00a0');
    }

    promisedDirectTabText.resolve(directTabText);
    promisedArrowTabText.resolve(arrowTabText);
    promisedEmptyParameters.resolve(emptyParameters);
  }.bind(this), onError);

  return {
    statusMarkup: this._getStatusMarkup(input),
    unclosedJs: this._getUnclosedJs(),
    scratchLink: this._getScratchLink(),
    directTabText: promisedDirectTabText.promise,
    arrowTabText: promisedArrowTabText.promise,
    emptyParameters: promisedEmptyParameters.promise
  };
};






Completer.prototype._getStatusMarkup = function(input) {
  
  
  
  var statusMarkup = this.requisition.getInputStatusMarkup(input.cursor.start);

  statusMarkup.forEach(function(member) {
    member.string = member.string.replace(/ /g, '\u00a0'); 
    member.className = 'gcli-in-' + member.status.toString().toLowerCase();
  }, this);

  return statusMarkup;
};




Completer.prototype._getUnclosedJs = function() {
  
  var command = this.requisition.commandAssignment.value;
  return command && command.name === '{' &&
      this.requisition.getAssignment(0).arg.suffix.indexOf('}') === -1;
};




Completer.prototype._getScratchLink = function() {
  var command = this.requisition.commandAssignment.value;
  return this.scratchpad && command && command.name === '{' ?
      this.scratchpad.linkText :
      '';
};

exports.Completer = Completer;


});
define("text!gcli/ui/completer.html", [], "\n" +
  "<description\n" +
  "    xmlns=\"http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul\">\n" +
  "  <loop foreach=\"member in ${statusMarkup}\">\n" +
  "    <label class=\"${member.className}\" value=\"${member.string}\"></label>\n" +
  "  </loop>\n" +
  "  <label class=\"gcli-in-ontab\" value=\"${directTabText}\"/>\n" +
  "  <label class=\"gcli-in-todo\" foreach=\"param in ${emptyParameters}\" value=\"${param}\"/>\n" +
  "  <label class=\"gcli-in-ontab\" value=\"${arrowTabText}\"/>\n" +
  "  <label class=\"gcli-in-closebrace\" if=\"${unclosedJs}\" value=\"}\"/>\n" +
  "</description>\n" +
  "");

















define('gcli/ui/tooltip', ['require', 'exports', 'module' , 'util/util', 'util/domtemplate', 'gcli/cli', 'gcli/ui/fields', 'text!gcli/ui/tooltip.css', 'text!gcli/ui/tooltip.html'], function(require, exports, module) {

'use strict';

var util = require('util/util');
var domtemplate = require('util/domtemplate');

var CommandAssignment = require('gcli/cli').CommandAssignment;
var fields = require('gcli/ui/fields');

var tooltipCss = require('text!gcli/ui/tooltip.css');
var tooltipHtml = require('text!gcli/ui/tooltip.html');















function Tooltip(options, components) {
  this.inputter = components.inputter;
  this.requisition = components.requisition;
  this.focusManager = components.focusManager;

  this.element = components.element;
  this.element.classList.add(options.tooltipClass || 'gcli-tooltip');
  this.document = this.element.ownerDocument;

  this.panelElement = components.panelElement;
  if (this.panelElement) {
    this.panelElement.classList.add('gcli-panel-hide');
    this.focusManager.onVisibilityChange.add(this.visibilityChanged, this);
  }
  this.focusManager.addMonitoredElement(this.element, 'tooltip');

  
  this.fields = [];

  
  if (tooltipCss != null) {
    this.style = util.importCss(tooltipCss, this.document, 'gcli-tooltip');
  }

  this.template = util.toDom(this.document, tooltipHtml);
  this.templateOptions = { blankNullUndefined: true, stack: 'tooltip.html' };

  this.inputter.onChoiceChange.add(this.choiceChanged, this);
  this.inputter.onAssignmentChange.add(this.assignmentChanged, this);
  this.requisition.onTextChange.add(this.textChanged, this);

  
  this.assignment = undefined;
  this.assignmentChanged({ assignment: this.inputter.assignment });

  
  this.lastText = undefined;
}




Tooltip.prototype.destroy = function() {
  this.inputter.onAssignmentChange.remove(this.assignmentChanged, this);
  this.inputter.onChoiceChange.remove(this.choiceChanged, this);
  this.requisition.onTextChange.remove(this.textChanged, this);

  if (this.panelElement) {
    this.focusManager.onVisibilityChange.remove(this.visibilityChanged, this);
  }
  this.focusManager.removeMonitoredElement(this.element, 'tooltip');

  if (this.style) {
    this.style.parentNode.removeChild(this.style);
    delete this.style;
  }

  this.field.onFieldChange.remove(this.fieldChanged, this);
  this.field.destroy();

  delete this.lastText;
  delete this.assignment;

  delete this.errorEle;
  delete this.descriptionEle;
  delete this.highlightEle;

  delete this.document;
  delete this.element;
  delete this.panelElement;
  delete this.template;
};




Object.defineProperty(Tooltip.prototype, 'isMenuShowing', {
  get: function() {
    return this.focusManager.isTooltipVisible &&
           this.field != null &&
           this.field.menu != null;
  },
  enumerable: true
});




Tooltip.prototype.assignmentChanged = function(ev) {
  
  
  
  if (this.assignment === ev.assignment) {
    return;
  }

  this.assignment = ev.assignment;
  this.lastText = this.assignment.arg.text;

  if (this.field) {
    this.field.onFieldChange.remove(this.fieldChanged, this);
    this.field.destroy();
  }

  this.field = fields.getField(this.assignment.param.type, {
    document: this.document,
    name: this.assignment.param.name,
    requisition: this.requisition,
    required: this.assignment.param.isDataRequired,
    named: !this.assignment.param.isPositionalAllowed,
    tooltip: true
  });

  this.focusManager.setImportantFieldFlag(this.field.isImportant);

  this.field.onFieldChange.add(this.fieldChanged, this);
  this.field.setConversion(this.assignment.conversion);

  
  this.errorEle = undefined;
  this.descriptionEle = undefined;
  this.highlightEle = undefined;

  var contents = this.template.cloneNode(true);
  domtemplate.template(contents, this, this.templateOptions);
  util.clearElement(this.element);
  this.element.appendChild(contents);
  this.element.style.display = 'block';

  this.field.setMessageElement(this.errorEle);

  this._updatePosition();
};




Tooltip.prototype.choiceChanged = function(ev) {
  if (this.field && this.field.setChoiceIndex) {
    var conversion = this.assignment.conversion;
    conversion.constrainPredictionIndex(ev.choice).then(function(choice) {
      this.field.setChoiceIndex(choice);
    }.bind(this)).then(null, util.errorHandler);
  }
};






Tooltip.prototype.selectChoice = function(ev) {
  if (this.field && this.field.selectChoice) {
    return this.field.selectChoice();
  }
  return false;
};




Tooltip.prototype.fieldChanged = function(ev) {
  this.requisition.setAssignment(this.assignment, ev.conversion.arg,
                                 { matchPadding: true });

  var isError = ev.conversion.message != null && ev.conversion.message !== '';
  this.focusManager.setError(isError);

  
  
  
  this.document.defaultView.setTimeout(function() {
    this.inputter.focus();
  }.bind(this), 10);
};




Tooltip.prototype.textChanged = function() {
  
  
  
  if (this.assignment.arg.text === this.lastText) {
    return;
  }

  this.lastText = this.assignment.arg.text;

  this.field.setConversion(this.assignment.conversion);
  util.setTextContent(this.descriptionEle, this.description);

  this._updatePosition();
};




Tooltip.prototype._updatePosition = function() {
  var dimensions = this.getDimensionsOfAssignment();

  
  if (this.panelElement) {
    this.panelElement.style.left = (dimensions.start * 10) + 'px';
  }

  this.focusManager.updatePosition(dimensions);
};






Tooltip.prototype.getDimensionsOfAssignment = function() {
  var before = '';
  var assignments = this.requisition.getAssignments(true);
  for (var i = 0; i < assignments.length; i++) {
    if (assignments[i] === this.assignment) {
      break;
    }
    before += assignments[i].toString();
  }
  before += this.assignment.arg.prefix;

  var startChar = before.length;
  before += this.assignment.arg.text;
  var endChar = before.length;

  return { start: startChar, end: endChar };
};






Object.defineProperty(Tooltip.prototype, 'description', {
  get: function() {
    if (this.assignment instanceof CommandAssignment &&
            this.assignment.value == null) {
      return '';
    }

    return this.assignment.param.manual || this.assignment.param.description;
  },
  enumerable: true
});




Tooltip.prototype.visibilityChanged = function(ev) {
  if (!this.panelElement) {
    return;
  }

  if (ev.tooltipVisible) {
    this.panelElement.classList.remove('gcli-panel-hide');
  }
  else {
    this.panelElement.classList.add('gcli-panel-hide');
  }
};

exports.Tooltip = Tooltip;


});
define("text!gcli/ui/tooltip.css", [], "");

define("text!gcli/ui/tooltip.html", [], "\n" +
  "<div class=\"gcli-tt\" aria-live=\"polite\">\n" +
  "  <div class=\"gcli-tt-description\" save=\"${descriptionEle}\">${description}</div>\n" +
  "  ${field.element}\n" +
  "  <div class=\"gcli-tt-error\" save=\"${errorEle}\">${assignment.conversion.message}</div>\n" +
  "  <div class=\"gcli-tt-highlight\" save=\"${highlightEle}\"></div>\n" +
  "</div>\n" +
  "");



this.gcli = require('gcli/index');
