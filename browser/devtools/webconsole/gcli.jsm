






















































var EXPORTED_SYMBOLS = [ "gcli" ];






var Node = Components.interfaces.nsIDOMNode;


Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");




var setTimeout;
var clearTimeout;

(function() {
  


  var nextID = 1;

  


  var timers = {};

  


  function TimerCallback(callback) {
    this._callback = callback;
    var interfaces = [ Components.interfaces.nsITimerCallback ];
    this.QueryInterface = XPCOMUtils.generateQI(interfaces);
  }

  TimerCallback.prototype.notify = function(timer) {
    try {
      for (var timerID in timers) {
        if (timers[timerID] === timer) {
          delete timers[timerID];
          break;
        }
      }
      this._callback.apply(null, []);
    }
    catch (ex) {
      console.error(ex);
    }
  };

  









  setTimeout = function setTimeout(callback, delay) {
    var timer = Components.classes["@mozilla.org/timer;1"]
                          .createInstance(Components.interfaces.nsITimer);

    var timerID = nextID++;
    timers[timerID] = timer;

    timer.initWithCallback(new TimerCallback(callback), delay, timer.TYPE_ONE_SHOT);
    return timerID;
  };

  





  clearTimeout = function clearTimeout(timerID) {
    var timer = timers[timerID];
    if (timer) {
      timer.cancel();
      delete timers[timerID];
    }
  };
})();







var console = {};
(function() {
  


















  function fmt(aStr, aMaxLen, aMinLen, aOptions) {
    if (aMinLen == null) {
      aMinLen = aMaxLen;
    }
    if (aStr == null) {
      aStr = "";
    }
    if (aStr.length > aMaxLen) {
      if (aOptions && aOptions.truncate == "start") {
        return "_" + aStr.substring(aStr.length - aMaxLen + 1);
      }
      else {
        return aStr.substring(0, aMaxLen - 1) + "_";
      }
    }
    if (aStr.length < aMinLen) {
      return Array(aMinLen - aStr.length + 1).join(" ") + aStr;
    }
    return aStr;
  }

  








  function getCtorName(aObj) {
    return Object.prototype.toString.call(aObj).slice(8, -1);
  }

  








  function stringify(aThing) {
    if (aThing === undefined) {
      return "undefined";
    }

    if (aThing === null) {
      return "null";
    }

    if (typeof aThing == "object") {
      var type = getCtorName(aThing);
      if (type == "XULElement") {
        return debugElement(aThing);
      }
      type = (type == "Object" ? "" : type + " ");
      var json;
      try {
        json = JSON.stringify(aThing);
      }
      catch (ex) {
        
        json = "{" + Object.keys(aThing).join(":..,") + ":.., " + "}";
      }
      return type + fmt(json, 50, 0);
    }

    if (typeof aThing == "function") {
      return fmt(aThing.toString().replace(/\s+/g, " "), 80, 0);
    }

    var str = aThing.toString();
    return fmt(str, 80, 0);
  }

  







  function debugElement(aElement) {
    return "<" + aElement.tagName +
        (aElement.id ? "#" + aElement.id : "") +
        (aElement.className ?
            "." + aElement.className.split(" ").join(" .") :
            "") +
        ">";
  }

  







  function log(aThing) {
    if (aThing === null) {
      return "null\n";
    }

    if (aThing === undefined) {
      return "undefined\n";
    }

    if (typeof aThing == "object") {
      var reply = "";
      var type = getCtorName(aThing);
      if (type == "Error") {
        reply += "  " + aThing.message + "\n";
        reply += logProperty("stack", aThing.stack);
      }
      else if (type == "XULElement") {
        reply += "  " + debugElement(aThing) + " (XUL)\n";
      }
      else {
        var keys = Object.getOwnPropertyNames(aThing);
        if (keys.length > 0) {
          reply += type + "\n";
          keys.forEach(function(aProp) {
            reply += logProperty(aProp, aThing[aProp]);
          }, this);
        }
        else {
          reply += type + "\n";
          var root = aThing;
          var logged = [];
          while (root != null) {
            var properties = Object.keys(root);
            properties.sort();
            properties.forEach(function(property) {
              if (!(property in logged)) {
                logged[property] = property;
                reply += logProperty(property, aThing[property]);
              }
            });

            root = Object.getPrototypeOf(root);
            if (root != null) {
              reply += '  - prototype ' + getCtorName(root) + '\n';
            }
          }
        }
      }

      return reply;
    }

    return "  " + aThing.toString() + "\n";
  }

  










  function logProperty(aProp, aValue) {
    var reply = "";
    if (aProp == "stack" && typeof value == "string") {
      var trace = parseStack(aValue);
      reply += formatTrace(trace);
    }
    else {
      reply += "    - " + aProp + " = " + stringify(aValue) + "\n";
    }
    return reply;
  }

  








  function parseStack(aStack) {
    var trace = [];
    aStack.split("\n").forEach(function(line) {
      if (!line) {
        return;
      }
      var at = line.lastIndexOf("@");
      var posn = line.substring(at + 1);
      trace.push({
        file: posn.split(":")[0],
        line: posn.split(":")[1],
        call: line.substring(0, at)
      });
    }, this);
    return trace;
  }

  









  function getStack(aFrame) {
    if (!aFrame) {
      aFrame = Components.stack.caller;
    }
    var trace = [];
    while (aFrame) {
      trace.push({
        file: aFrame.filename,
        line: aFrame.lineNumber,
        call: aFrame.name
      });
      aFrame = aFrame.caller;
    }
    return trace;
  }

  







  function formatTrace(aTrace) {
    var reply = "";
    aTrace.forEach(function(frame) {
      reply += fmt(frame.file, 20, 20, { truncate: "start" }) + " " +
               fmt(frame.line, 5, 5) + " " +
               fmt(frame.call, 75, 75) + "\n";
    });
    return reply;
  }

  










  function createDumper(aLevel) {
    return function() {
      var args = Array.prototype.slice.call(arguments, 0);
      var data = args.map(function(arg) {
        return stringify(arg);
      });
      dump(aLevel + ": " + data.join(", ") + "\n");
    };
  }

  










  function createMultiLineDumper(aLevel) {
    return function() {
      dump(aLevel + "\n");
      var args = Array.prototype.slice.call(arguments, 0);
      args.forEach(function(arg) {
        dump(log(arg));
      });
    };
  }

  


  console.debug = createMultiLineDumper("debug");
  console.log = createDumper("log");
  console.info = createDumper("info");
  console.warn = createDumper("warn");
  console.error = createMultiLineDumper("error");
  console.trace = function Console_trace() {
    var trace = getStack(Components.stack.caller);
    dump(formatTrace(trace) + "\n");
  },
  console.clear = function Console_clear() {};

  console.dir = createMultiLineDumper("dir");
  console.dirxml = createMultiLineDumper("dirxml");
  console.group = createDumper("group");
  console.groupEnd = createDumper("groupEnd");

})();













function define(moduleName, deps, payload) {
  if (typeof moduleName != "string") {
    console.error(this.depth + " Error: Module name is not a string.");
    console.trace();
    return;
  }

  if (arguments.length == 2) {
    payload = deps;
  }

  if (define.debugDependencies) {
    console.log("define: " + moduleName + " -> " + payload.toString()
        .slice(0, 40).replace(/\n/, '\\n').replace(/\r/, '\\r') + "...");
  }

  if (moduleName in define.modules) {
    console.error(this.depth + " Error: Redefining module: " + moduleName);
  }
  define.modules[moduleName] = payload;
}




define.modules = {};




define.debugDependencies = false;





(function() {
  







  function Domain() {
    this.modules = {};

    if (define.debugDependencies) {
      this.depth = "";
    }
  }

  












  Domain.prototype.require = function(deps, callback) {
    if (Array.isArray(deps)) {
      var params = deps.map(function(dep) {
        return this.lookup(dep);
      }, this);
      if (callback) {
        callback.apply(null, params);
      }
      return undefined;
    }
    else {
      return this.lookup(deps);
    }
  };

  





  Domain.prototype.lookup = function(moduleName) {
    if (moduleName in this.modules) {
      var module = this.modules[moduleName];
      if (define.debugDependencies) {
        console.log(this.depth + " Using module: " + moduleName);
      }
      return module;
    }

    if (!(moduleName in define.modules)) {
      console.error(this.depth + " Missing module: " + moduleName);
      return null;
    }

    var module = define.modules[moduleName];

    if (define.debugDependencies) {
      console.log(this.depth + " Compiling module: " + moduleName);
    }

    if (typeof module == "function") {
      if (define.debugDependencies) {
        this.depth += ".";
      }

      var exports = {};
      try {
        module(this.require.bind(this), exports, { id: moduleName, uri: "" });
      }
      catch (ex) {
        console.error("Error using module: " + moduleName, ex);
        throw ex;
      }
      module = exports;

      if (define.debugDependencies) {
        this.depth = this.depth.slice(0, -1);
      }
    }

    
    this.modules[moduleName] = module;

    return module;
  };

  




  define.Domain = Domain;
  define.globalDomain = new Domain();
})();





var require = define.globalDomain.require.bind(define.globalDomain);






var mozl10n = {};

(function(aMozl10n) {
  var temp = {};
  Components.utils.import("resource://gre/modules/Services.jsm", temp);
  var stringBundle = temp.Services.strings.createBundle(
          "chrome://browser/locale/devtools/gclicommands.properties");

  




  aMozl10n.lookup = function(name) {
    try {
      return stringBundle.GetStringFromName(name);
    }
    catch (ex) {
      throw new Error("Failure in lookup('" + name + "')");
    }
  };

  





  aMozl10n.lookupFormat = function(name, swaps) {
    try {
      return stringBundle.formatStringFromName(name, swaps, swaps.length);
    }
    catch (ex) {
      throw new Error("Failure in lookupFormat('" + name + "')");
    }
  };

})(mozl10n);

define('gcli/index', ['require', 'exports', 'module' , 'gcli/canon', 'gcli/types/basic', 'gcli/types/javascript', 'gcli/types/node', 'gcli/cli', 'gcli/ui/display'], function(require, exports, module) {

  
  exports.addCommand = require('gcli/canon').addCommand;
  exports.removeCommand = require('gcli/canon').removeCommand;
  exports.lookup = mozl10n.lookup;
  exports.lookupFormat = mozl10n.lookupFormat;

  
  require('gcli/types/basic').startup();
  require('gcli/types/javascript').startup();
  require('gcli/types/node').startup();
  require('gcli/cli').startup();

  var Requisition = require('gcli/cli').Requisition;
  var Display = require('gcli/ui/display').Display;

  var cli = require('gcli/cli');
  var jstype = require('gcli/types/javascript');
  var nodetype = require('gcli/types/node');

  



  exports._internal = {
    require: require,
    define: define,
    console: console,

    













    createView: function(opts) {
      jstype.setGlobalObject(opts.jsEnvironment.globalObject);
      nodetype.setDocument(opts.contentDocument);
      cli.setEvalFunction(opts.jsEnvironment.evalFunction);

      if (opts.requisition == null) {
        opts.requisition = new Requisition(opts.environment, opts.chromeDocument);
      }

      opts.display = new Display(opts);
    },

    


    removeView: function(opts) {
      opts.display.destroy();
      delete opts.display;

      opts.requisition.destroy();
      delete opts.requisition;

      cli.unsetEvalFunction();
      nodetype.unsetDocument();
      jstype.unsetGlobalObject();
    },

    commandOutputManager: require('gcli/canon').commandOutputManager
  };
});






define('gcli/canon', ['require', 'exports', 'module' , 'gcli/util', 'gcli/l10n', 'gcli/types', 'gcli/types/basic'], function(require, exports, module) {
var canon = exports;


var util = require('gcli/util');
var l10n = require('gcli/l10n');

var types = require('gcli/types');
var Status = require('gcli/types').Status;
var BooleanType = require('gcli/types/basic').BooleanType;





var commands = {};




var commandNames = [];








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

  this.description = 'description' in this ? this.description : undefined;
  this.description = lookup(this.description, 'canonDescNone');
  this.manual = 'manual' in this ? this.manual : undefined;
  this.manual = lookup(this.manual);

  
  
  var paramSpecs = this.params;
  this.params = [];

  
  
  
  
  
  var usingGroups = false;

  
  
  
  
  paramSpecs.forEach(function(spec) {
    if (!spec.group) {
      if (usingGroups) {
        console.error('Parameters can\'t come after param groups.' +
            ' Ignoring ' + this.name + '/' + spec.name);
      }
      else {
        var param = new Parameter(spec, this, null);
        this.params.push(param);
      }
    }
    else {
      spec.params.forEach(function(ispec) {
        var param = new Parameter(ispec, this, spec.group);
        this.params.push(param);
      }, this);

      usingGroups = true;
    }
  }, this);
}

canon.Command = Command;






function Parameter(paramSpec, command, groupName) {
  this.command = command || { name: 'unnamed' };

  Object.keys(paramSpec).forEach(function(key) {
    this[key] = paramSpec[key];
  }, this);

  this.description = 'description' in this ? this.description : undefined;
  this.description = lookup(this.description, 'canonDescNone');
  this.manual = 'manual' in this ? this.manual : undefined;
  this.manual = lookup(this.manual);
  this.groupName = groupName;

  if (!this.name) {
    throw new Error('In ' + this.command.name +
      ': all params must have a name');
  }

  var typeSpec = this.type;
  this.type = types.getType(typeSpec);
  if (this.type == null) {
    console.error('Known types: ' + types.getTypeNames().join(', '));
    throw new Error('In ' + this.command.name + '/' + this.name +
      ': can\'t find type for: ' + JSON.stringify(typeSpec));
  }

  
  
  if (this.type instanceof BooleanType) {
    if ('defaultValue' in this) {
      console.error('In ' + this.command.name + '/' + this.name +
          ': boolean parameters can not have a defaultValue.' +
          ' Ignoring');
    }
    this.defaultValue = false;
  }

  
  
  
  
  if (this.defaultValue != null) {
    try {
      var defaultText = this.type.stringify(this.defaultValue);
      var defaultConversion = this.type.parseString(defaultText);
      if (defaultConversion.getStatus() !== Status.VALID) {
        console.error('In ' + this.command.name + '/' + this.name +
            ': Error round tripping defaultValue. status = ' +
            defaultConversion.getStatus());
      }
    }
    catch (ex) {
      console.error('In ' + this.command.name + '/' + this.name +
        ': ' + ex);
    }
  }
}






Parameter.prototype.isKnownAs = function(name) {
  if (name === '--' + this.name) {
    return true;
  }
  return false;
};





Parameter.prototype.isDataRequired = function() {
  return this.defaultValue === undefined;
};





Parameter.prototype.isPositionalAllowed = function() {
  return this.groupName == null;
};

canon.Parameter = Parameter;









canon.addCommand = function addCommand(commandSpec) {
  var command = new Command(commandSpec);
  commands[commandSpec.name] = command;
  commandNames.push(commandSpec.name);
  commandNames.sort();

  canon.canonChange();
  return command;
};





canon.removeCommand = function removeCommand(commandOrName) {
  var name = typeof commandOrName === 'string' ?
          commandOrName :
          commandOrName.name;
  delete commands[name];
  commandNames = commandNames.filter(function(test) {
    return test !== name;
  });

  canon.canonChange();
};





canon.getCommand = function getCommand(name) {
  return commands[name];
};




canon.getCommands = function getCommands() {
  
  return Object.keys(commands).map(function(name) {
    return commands[name];
  }, this);
};




canon.getCommandNames = function getCommandNames() {
  return commandNames.slice(0);
};




canon.canonChange = util.createEvent('canon.canonChange');











function CommandOutputManager() {
  this._event = util.createEvent('CommandOutputManager');
}






CommandOutputManager.prototype.sendCommandOutput = function(output) {
  this._event({ output: output });
};





CommandOutputManager.prototype.addListener = function(fn, ctx) {
  this._event.add(fn, ctx);
};




CommandOutputManager.prototype.removeListener = function(fn, ctx) {
  this._event.remove(fn, ctx);
};

canon.CommandOutputManager = CommandOutputManager;





canon.commandOutputManager = new CommandOutputManager();


});






define('gcli/util', ['require', 'exports', 'module' ], function(require, exports, module) {




























exports.createEvent = function(name) {
  var handlers = [];

  



  var event = function(ev) {
    
    
    for (var i = 0; i < handlers.length; i++) {
      var handler = handlers[i];
      handler.func.call(handler.scope, ev);
    }
  };

  




  event.add = function(func, scope) {
    handlers.push({ func: func, scope: scope });
  };

  





  event.remove = function(func, scope) {
    handlers = handlers.filter(function(test) {
      return test.func !== func && test.scope !== scope;
    });
  };

  



  event.removeAll = function() {
    handlers = [];
  };

  return event;
};




var dom = {};

dom.NS_XHTML = 'http://www.w3.org/1999/xhtml';












dom.createElement = function(doc, tag) {
  if (dom.isXmlDocument(doc)) {
    return doc.createElementNS(dom.NS_XHTML, tag);
  }
  else {
    return doc.createElement(tag);
  }
};





dom.clearElement = function(elem) {
  while (elem.hasChildNodes()) {
    elem.removeChild(elem.firstChild);
  }
};







dom.importCss = function(cssText, doc) {
  doc = doc || document;

  var style = dom.createElement(doc, 'style');
  style.appendChild(doc.createTextNode(cssText));

  var head = doc.getElementsByTagName('head')[0] || doc.documentElement;
  head.appendChild(style);

  return style;
};





dom.setInnerHtml = function(elem, html) {
  if (dom.isXmlDocument(elem.ownerDocument)) {
    dom.clearElement(elem);
    html = '<div xmlns="' + dom.NS_XHTML + '">' + html + '</div>';
    var range = elem.ownerDocument.createRange();
    var child = range.createContextualFragment(html).childNodes[0];
    while (child.hasChildNodes()) {
      elem.appendChild(child.firstChild);
    }
  }
  else {
    elem.innerHTML = html;
  }
};









dom.isXmlDocument = function(doc) {
  doc = doc || document;
  
  if (doc.contentType && doc.contentType != 'text/html') {
    return true;
  }
  
  if (doc.xmlVersion != null) {
    return true;
  }
  return false;
};

exports.dom = dom;







var event = {};













if ('KeyEvent' in this) {
  event.KeyEvent = this.KeyEvent;
}
else {
  event.KeyEvent = {
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

exports.event = event;


});






define('gcli/l10n', ['require', 'exports', 'module' ], function(require, exports, module) {

Components.utils.import('resource://gre/modules/XPCOMUtils.jsm');
Components.utils.import('resource://gre/modules/Services.jsm');

XPCOMUtils.defineLazyGetter(this, 'stringBundle', function () {
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
    return stringBundle.GetStringFromName(key);
  }
  catch (ex) {
    console.error('Failed to lookup ', key, ex);
    return key;
  }
};


exports.lookupFormat = function(key, swaps) {
  try {
    return stringBundle.formatStringFromName(key, swaps, swaps.length);
  }
  catch (ex) {
    console.error('Failed to format ', key, ex);
    return key;
  }
};


});






define('gcli/types', ['require', 'exports', 'module' , 'gcli/argument'], function(require, exports, module) {
var types = exports;


var Argument = require('gcli/argument').Argument;







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
  }
};
types.Status = Status;

































function Conversion(value, arg, status, message, predictions) {
  
  this.value = value;

  
  this.arg = arg;
  if (arg == null) {
    throw new Error('Missing arg');
  }

  this._status = status || Status.VALID;
  this.message = message;
  this.predictions = predictions;
}

types.Conversion = Conversion;







Conversion.prototype.assign = function(assignment) {
  this.arg.assign(assignment);
};




Conversion.prototype.isDataProvided = function() {
  var argProvided = this.arg.text !== '';
  return this.value !== undefined || argProvided;
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
  return this.value === that.value;
};






Conversion.prototype.argEquals = function(that) {
  return this.arg.equals(that.arg);
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
  return this.predictions || [];
};








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

ArrayConversion.prototype.assign = function(assignment) {
  this.conversions.forEach(function(conversion) {
    conversion.assign(assignment);
  }, this);
  this.assignment = assignment;
};

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

types.ArrayConversion = ArrayConversion;








function Type() {
}






Type.prototype.stringify = function(value) {
  throw new Error('Not implemented');
};








Type.prototype.parse = function(arg) {
  throw new Error('Not implemented');
};






Type.prototype.parseString = function(str) {
  return this.parse(new Argument(str));
},







Type.prototype.name = undefined;





Type.prototype.increment = function(value) {
  return undefined;
};





Type.prototype.decrement = function(value) {
  return undefined;
};






Type.prototype.getDefault = undefined;

types.Type = Type;





var registeredTypes = {};

types.getTypeNames = function() {
  return Object.keys(registeredTypes);
};










types.registerType = function(type) {
  if (typeof type === 'object') {
    if (type instanceof Type) {
      if (!type.name) {
        throw new Error('All registered types must have a name');
      }
      registeredTypes[type.name] = type;
    }
    else {
      throw new Error('Can\'t registerType using: ' + type);
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

types.registerTypes = function registerTypes(newTypes) {
  Object.keys(newTypes).forEach(function(name) {
    var type = newTypes[name];
    type.name = name;
    newTypes.registerType(type);
  });
};




types.deregisterType = function(type) {
  delete registeredTypes[type.name];
};




types.getType = function(typeSpec) {
  var type;
  if (typeof typeSpec === 'string') {
    type = registeredTypes[typeSpec];
    if (typeof type === 'function') {
      type = new type();
    }
    return type;
  }

  if (typeof typeSpec === 'object') {
    if (!typeSpec.name) {
      throw new Error('Missing \'name\' member to typeSpec');
    }

    type = registeredTypes[typeSpec.name];
    if (typeof type === 'function') {
      type = new type(typeSpec);
    }
    return type;
  }

  throw new Error('Can\'t extract type from ' + typeSpec);
};


});






define('gcli/argument', ['require', 'exports', 'module' ], function(require, exports, module) {
var argument = exports;














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





Argument.prototype.merge = function(following) {
  
  
  return new Argument(
    this.text + this.suffix + following.prefix + following.text,
    this.prefix, following.suffix);
};






Argument.prototype.beget = function(replText, options) {
  var prefix = this.prefix;
  var suffix = this.suffix;

  var quote = (replText.indexOf(' ') >= 0 || replText.length == 0) ?
      '\'' : '';

  if (options) {
    prefix = (options.prefixSpace ? ' ' : '') + quote;
    suffix = quote;
  }

  return new Argument(replText, prefix, suffix);
};




Argument.prototype.isBlank = function() {
  return this.text === '' &&
      this.prefix.trim() === '' &&
      this.suffix.trim() === '';
};




Argument.prototype.assign = function(assignment) {
  this.assignment = assignment;
};







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

argument.Argument = Argument;







function ScriptArgument(text, prefix, suffix) {
  this.text = text;
  this.prefix = prefix !== undefined ? prefix : '';
  this.suffix = suffix !== undefined ? suffix : '';

  while (this.text.charAt(0) === ' ') {
    this.prefix = this.prefix + ' ';
    this.text = this.text.substring(1);
  }

  while (this.text.charAt(this.text.length - 1) === ' ') {
    this.suffix = ' ' + this.suffix;
    this.text = this.text.slice(0, -1);
  }
}

ScriptArgument.prototype = Object.create(Argument.prototype);






ScriptArgument.prototype.beget = function(replText, options) {
  var prefix = this.prefix;
  var suffix = this.suffix;

  var quote = (replText.indexOf(' ') >= 0 || replText.length == 0) ?
      '\'' : '';

  if (options && options.normalize) {
    prefix = '{ ';
    suffix = ' }';
  }

  return new ScriptArgument(replText, prefix, suffix);
};

argument.ScriptArgument = ScriptArgument;







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





MergedArgument.prototype.assign = function(assignment) {
  this.args.forEach(function(arg) {
    arg.assign(assignment);
  }, this);

  this.assignment = assignment;
};

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

argument.MergedArgument = MergedArgument;






function TrueNamedArgument(name, arg) {
  this.arg = arg;
  this.text = arg ? arg.text : '--' + name;
  this.prefix = arg ? arg.prefix : ' ';
  this.suffix = arg ? arg.suffix : '';
}

TrueNamedArgument.prototype = Object.create(Argument.prototype);

TrueNamedArgument.prototype.assign = function(assignment) {
  if (this.arg) {
    this.arg.assign(assignment);
  }
  this.assignment = assignment;
};

TrueNamedArgument.prototype.getArgs = function() {
  
  
  
  
  
  return this.arg ? [ this, this.arg ] : [ this ];
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

argument.TrueNamedArgument = TrueNamedArgument;






function FalseNamedArgument() {
  this.text = '';
  this.prefix = '';
  this.suffix = '';
}

FalseNamedArgument.prototype = Object.create(Argument.prototype);

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

argument.FalseNamedArgument = FalseNamedArgument;

















function NamedArgument(nameArg, valueArg) {
  this.nameArg = nameArg;
  this.valueArg = valueArg;

  this.text = valueArg.text;
  this.prefix = nameArg.toString() + valueArg.prefix;
  this.suffix = valueArg.suffix;
}

NamedArgument.prototype = Object.create(Argument.prototype);

NamedArgument.prototype.assign = function(assignment) {
  this.nameArg.assign(assignment);
  this.valueArg.assign(assignment);
  this.assignment = assignment;
};

NamedArgument.prototype.getArgs = function() {
  return [ this.nameArg, this.valueArg ];
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

argument.NamedArgument = NamedArgument;






function ArrayArgument() {
  this.args = [];
}

ArrayArgument.prototype = Object.create(Argument.prototype);

ArrayArgument.prototype.addArgument = function(arg) {
  this.args.push(arg);
};

ArrayArgument.prototype.addArguments = function(args) {
  Array.prototype.push.apply(this.args, args);
};

ArrayArgument.prototype.getArguments = function() {
  return this.args;
};

ArrayArgument.prototype.assign = function(assignment) {
  this.args.forEach(function(arg) {
    arg.assign(assignment);
  }, this);

  this.assignment = assignment;
};

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

  if (!(that instanceof ArrayArgument)) {
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

argument.ArrayArgument = ArrayArgument;


});






define('gcli/types/basic', ['require', 'exports', 'module' , 'gcli/l10n', 'gcli/types', 'gcli/argument'], function(require, exports, module) {


var l10n = require('gcli/l10n');
var types = require('gcli/types');
var Type = require('gcli/types').Type;
var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;
var ArrayConversion = require('gcli/types').ArrayConversion;

var Argument = require('gcli/argument').Argument;
var TrueNamedArgument = require('gcli/argument').TrueNamedArgument;
var FalseNamedArgument = require('gcli/argument').FalseNamedArgument;
var ArrayArgument = require('gcli/argument').ArrayArgument;





exports.startup = function() {
  types.registerType(StringType);
  types.registerType(NumberType);
  types.registerType(BooleanType);
  types.registerType(BlankType);
  types.registerType(SelectionType);
  types.registerType(DeferredType);
  types.registerType(ArrayType);
};

exports.shutdown = function() {
  types.unregisterType(StringType);
  types.unregisterType(NumberType);
  types.unregisterType(BooleanType);
  types.unregisterType(BlankType);
  types.unregisterType(SelectionType);
  types.unregisterType(DeferredType);
  types.unregisterType(ArrayType);
};





function StringType(typeSpec) {
  if (typeSpec != null) {
    throw new Error('StringType can not be customized');
  }
}

StringType.prototype = Object.create(Type.prototype);

StringType.prototype.stringify = function(value) {
  if (value == null) {
    return '';
  }
  return value.toString();
};

StringType.prototype.parse = function(arg) {
  if (arg.text == null || arg.text === '') {
    return new Conversion(null, arg, Status.INCOMPLETE, '');
  }
  return new Conversion(arg.text, arg);
};

StringType.prototype.name = 'string';

exports.StringType = StringType;





function NumberType(typeSpec) {
  if (typeSpec) {
    this._min = typeSpec.min;
    this._max = typeSpec.max;
    this._step = typeSpec.step || 1;
  }
  else {
    this._step = 1;
  }
}

NumberType.prototype = Object.create(Type.prototype);

NumberType.prototype.stringify = function(value) {
  if (value == null) {
    return '';
  }
  return '' + value;
};

NumberType.prototype.getMin = function() {
  if (this._min) {
    if (typeof this._min === 'function') {
      return this._min();
    }
    if (typeof this._min === 'number') {
      return this._min;
    }
  }
  return 0;
};

NumberType.prototype.getMax = function() {
  if (this._max) {
    if (typeof this._max === 'function') {
      return this._max();
    }
    if (typeof this._max === 'number') {
      return this._max;
    }
  }
  return undefined;
};

NumberType.prototype.parse = function(arg) {
  if (arg.text.replace(/\s/g, '').length === 0) {
    return new Conversion(null, arg, Status.INCOMPLETE, '');
  }

  var value = parseInt(arg.text, 10);
  if (isNaN(value)) {
    return new Conversion(null, arg, Status.ERROR,
        l10n.lookupFormat('typesNumberNan', [ arg.text ]));
  }

  if (this.getMax() != null && value > this.getMax()) {
    return new Conversion(null, arg, Status.ERROR,
        l10n.lookupFormat('typesNumberMax', [ value, this.getMax() ]));
  }

  if (value < this.getMin()) {
    return new Conversion(null, arg, Status.ERROR,
        l10n.lookupFormat('typesNumberMin', [ value, this.getMin() ]));
  }

  return new Conversion(value, arg);
};

NumberType.prototype.decrement = function(value) {
  if (typeof value !== 'number' || isNaN(value)) {
    return this.getMax() || 1;
  }
  var newValue = value - this._step;
  
  newValue = Math.ceil(newValue / this._step) * this._step;
  return this._boundsCheck(newValue);
};

NumberType.prototype.increment = function(value) {
  if (typeof value !== 'number' || isNaN(value)) {
    return this.getMin();
  }
  var newValue = value + this._step;
  
  newValue = Math.floor(newValue / this._step) * this._step;
  if (this.getMax() == null) {
    return newValue;
  }
  return this._boundsCheck(newValue);
};






NumberType.prototype._boundsCheck = function(value) {
  var min = this.getMin();
  if (value < min) {
    return min;
  }
  var max = this.getMax();
  if (value > max) {
    return max;
  }
  return value;
};

NumberType.prototype.name = 'number';

exports.NumberType = NumberType;




function SelectionType(typeSpec) {
  if (typeSpec) {
    Object.keys(typeSpec).forEach(function(key) {
      this[key] = typeSpec[key];
    }, this);
  }
}

SelectionType.prototype = Object.create(Type.prototype);

SelectionType.prototype.stringify = function(value) {
  var name = null;
  var lookup = this.getLookup();
  lookup.some(function(item) {
    var test = (item.value == null) ? item : item.value;
    if (test === value) {
      name = item.name;
      return true;
    }
    return false;
  }, this);
  return name;
};






SelectionType.prototype.getLookup = function() {
  if (this.lookup) {
    if (typeof this.lookup === 'function') {
      return this.lookup();
    }
    return this.lookup;
  }

  if (Array.isArray(this.data)) {
    this.lookup = this._dataToLookup(this.data);
    return this.lookup;
  }

  if (typeof(this.data) === 'function') {
    return this._dataToLookup(this.data());
  }

  throw new Error('SelectionType has no data');
};






SelectionType.prototype._dataToLookup = function(data) {
  return data.map(function(option) {
    return { name: option, value: option };
  });
};








SelectionType.prototype._findPredictions = function(arg) {
  var predictions = [];
  this.getLookup().forEach(function(item) {
    if (item.name.indexOf(arg.text) === 0) {
      predictions.push(item);
    }
  }, this);
  return predictions;
};

SelectionType.prototype.parse = function(arg) {
  var predictions = this._findPredictions(arg);

  if (predictions.length === 1 && predictions[0].name === arg.text) {
    var value = predictions[0].value ? predictions[0].value : predictions[0];
    return new Conversion(value, arg);
  }

  
  
  if (this.noMatch) {
    this.noMatch();
  }

  if (predictions.length > 0) {
    
    
    
    
    
    
    
    var predictFunc = function() {
      return this._findPredictions(arg);
    }.bind(this);
    return new Conversion(null, arg, Status.INCOMPLETE, '', predictFunc);
  }

  return new Conversion(null, arg, Status.ERROR,
      l10n.lookupFormat('typesSelectionNomatch', [ arg.text ]));
};








SelectionType.prototype.decrement = function(value) {
  var lookup = this.getLookup();
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




SelectionType.prototype.increment = function(value) {
  var lookup = this.getLookup();
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

SelectionType.prototype.name = 'selection';

exports.SelectionType = SelectionType;





function BooleanType(typeSpec) {
  if (typeSpec != null) {
    throw new Error('BooleanType can not be customized');
  }
}

BooleanType.prototype = Object.create(SelectionType.prototype);

BooleanType.prototype.lookup = [
  { name: 'true', value: true },
  { name: 'false', value: false }
];

BooleanType.prototype.parse = function(arg) {
  if (arg instanceof TrueNamedArgument) {
    return new Conversion(true, arg);
  }
  if (arg instanceof FalseNamedArgument) {
    return new Conversion(false, arg);
  }
  return SelectionType.prototype.parse.call(this, arg);
};

BooleanType.prototype.stringify = function(value) {
  return '' + value;
};

BooleanType.prototype.getDefault = function() {
  return new Conversion(false, new Argument(''));
};

BooleanType.prototype.name = 'boolean';

exports.BooleanType = BooleanType;





function DeferredType(typeSpec) {
  if (typeof typeSpec.defer !== 'function') {
    throw new Error('Instances of DeferredType need typeSpec.defer to be a function that returns a type');
  }
  Object.keys(typeSpec).forEach(function(key) {
    this[key] = typeSpec[key];
  }, this);
}

DeferredType.prototype = Object.create(Type.prototype);

DeferredType.prototype.stringify = function(value) {
  return this.defer().stringify(value);
};

DeferredType.prototype.parse = function(arg) {
  return this.defer().parse(arg);
};

DeferredType.prototype.decrement = function(value) {
  var deferred = this.defer();
  return (deferred.decrement ? deferred.decrement(value) : undefined);
};

DeferredType.prototype.increment = function(value) {
  var deferred = this.defer();
  return (deferred.increment ? deferred.increment(value) : undefined);
};

DeferredType.prototype.increment = function(value) {
  var deferred = this.defer();
  return (deferred.increment ? deferred.increment(value) : undefined);
};

DeferredType.prototype.name = 'deferred';

exports.DeferredType = DeferredType;






function BlankType(typeSpec) {
  if (typeSpec != null) {
    throw new Error('BlankType can not be customized');
  }
}

BlankType.prototype = Object.create(Type.prototype);

BlankType.prototype.stringify = function(value) {
  return '';
};

BlankType.prototype.parse = function(arg) {
  return new Conversion(null, arg);
};

BlankType.prototype.name = 'blank';

exports.BlankType = BlankType;





function ArrayType(typeSpec) {
  if (!typeSpec.subtype) {
    console.error('Array.typeSpec is missing subtype. Assuming string.' +
        JSON.stringify(typeSpec));
    typeSpec.subtype = 'string';
  }

  Object.keys(typeSpec).forEach(function(key) {
    this[key] = typeSpec[key];
  }, this);
  this.subtype = types.getType(this.subtype);
}

ArrayType.prototype = Object.create(Type.prototype);

ArrayType.prototype.stringify = function(values) {
  
  return values.join(' ');
};

ArrayType.prototype.parse = function(arg) {
  if (arg instanceof ArrayArgument) {
    var conversions = arg.getArguments().map(function(subArg) {
      var conversion = this.subtype.parse(subArg);
      
      
      
      subArg.conversion = conversion;
      return conversion;
    }, this);
    return new ArrayConversion(conversions, arg);
  }
  else {
    console.error('non ArrayArgument to ArrayType.parse', arg);
    throw new Error('non ArrayArgument to ArrayType.parse');
  }
};

ArrayType.prototype.getDefault = function() {
  return new ArrayConversion([], new ArrayArgument());
};

ArrayType.prototype.name = 'array';

exports.ArrayType = ArrayType;


});






define('gcli/types/javascript', ['require', 'exports', 'module' , 'gcli/l10n', 'gcli/types'], function(require, exports, module) {


var l10n = require('gcli/l10n');
var types = require('gcli/types');

var Conversion = types.Conversion;
var Type = types.Type;
var Status = types.Status;





exports.startup = function() {
  types.registerType(JavascriptType);
};

exports.shutdown = function() {
  types.unregisterType(JavascriptType);
};





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
  if (typeSpec != null) {
    throw new Error('JavascriptType can not be customized');
  }
}

JavascriptType.prototype = Object.create(Type.prototype);

JavascriptType.prototype.stringify = function(value) {
  if (value == null) {
    return '';
  }
  return value;
};





JavascriptType.MAX_COMPLETION_MATCHES = 10;

JavascriptType.prototype.parse = function(arg) {
  var typed = arg.text;
  var scope = globalObject;

  
  
  var beginning = this._findCompletionBeginning(typed);

  
  if (beginning.err) {
    return new Conversion(typed, arg, Status.ERROR, beginning.err);
  }

  
  
  if (beginning.state !== ParseState.NORMAL) {
    return new Conversion(typed, arg, Status.INCOMPLETE, '');
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
        return new Conversion(typed, arg, Status.ERROR,
                l10n.lookup('jstypeParseScope'));
      }

      if (prop === '') {
        return new Conversion(typed, arg, Status.INCOMPLETE, '');
      }

      
      
      if (this._isSafeProperty(scope, prop)) {
        return new Conversion(typed, arg);
      }

      try {
        scope = scope[prop];
      }
      catch (ex) {
        
        
        
        return new Conversion(typed, arg, Status.INCOMPLETE, '');
      }
    }
  }
  else {
    matchProp = properties[0].trimLeft();
  }

  
  
  if (prop && !prop.match(/^[0-9A-Za-z]*$/)) {
    return new Conversion(typed, arg);
  }

  
  if (scope == null) {
    return new Conversion(typed, arg, Status.ERROR,
        l10n.lookupFormat('jstypeParseMissing', [ prop ]));
  }

  
  
  if (!matchProp.match(/^[0-9A-Za-z]*$/)) {
    return new Conversion(typed, arg);
  }

  
  if (this._isIteratorOrGenerator(scope)) {
    return null;
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
    return new Conversion(typed, arg, Status.INCOMPLETE, '');
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
    predictions = undefined;
  }

  return new Conversion(typed, arg, status, message, predictions);
};





function isVendorPrefixed(name) {
  return name.indexOf('moz') === 0 ||
         name.indexOf('webkit') === 0 ||
         name.indexOf('ms') === 0;
}




var ParseState = {
  NORMAL: 0,
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













JavascriptType.prototype._findCompletionBeginning = function(text) {
  var bodyStack = [];

  var state = ParseState.NORMAL;
  var start = 0;
  var c;
  for (var i = 0; i < text.length; i++) {
    c = text[i];

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

exports.JavascriptType = JavascriptType;


});






define('gcli/types/node', ['require', 'exports', 'module' , 'gcli/host', 'gcli/l10n', 'gcli/types'], function(require, exports, module) {


var host = require('gcli/host');
var l10n = require('gcli/l10n');
var types = require('gcli/types');
var Type = require('gcli/types').Type;
var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;





exports.startup = function() {
  types.registerType(NodeType);
};

exports.shutdown = function() {
  types.unregisterType(NodeType);
};





var doc;
if (typeof document !== 'undefined') {
  doc = document;
}




exports.setDocument = function(document) {
  doc = document;
};




exports.unsetDocument = function() {
  doc = undefined;
};





function NodeType(typeSpec) {
  if (typeSpec != null) {
    throw new Error('NodeType can not be customized');
  }
}

NodeType.prototype = Object.create(Type.prototype);

NodeType.prototype.stringify = function(value) {
  return value.__gcliQuery || 'Error';
};

NodeType.prototype.parse = function(arg) {
  if (arg.text === '') {
    return new Conversion(null, arg, Status.INCOMPLETE,
            l10n.lookup('nodeParseNone'));
  }

  var nodes;
  try {
    nodes = doc.querySelectorAll(arg.text);
  }
  catch (ex) {
    return new Conversion(null, arg, Status.ERROR,
            l10n.lookup('nodeParseSyntax'));
  }

  if (nodes.length === 0) {
    return new Conversion(null, arg, Status.INCOMPLETE,
        l10n.lookup('nodeParseNone'));
  }

  if (nodes.length === 1) {
    var node = nodes.item(0);
    node.__gcliQuery = arg.text;

    host.flashNode(node, 'green');

    return new Conversion(node, arg, Status.VALID, '');
  }

  Array.prototype.forEach.call(nodes, function(n) {
    host.flashNode(n, 'red');
  });

  return new Conversion(null, arg, Status.ERROR,
          l10n.lookupFormat('nodeParseMultiple', [ nodes.length ]));
};

NodeType.prototype.name = 'node';


});






define('gcli/host', ['require', 'exports', 'module' ], function(require, exports, module) {






exports.flashNode = function(node, color) {
  if (!node.__gcliHighlighting) {
    node.__gcliHighlighting = true;
    var original = node.style.background;
    node.style.background = color;
    setTimeout(function() {
      node.style.background = original;
      delete node.__gcliHighlighting;
    }, 1000);
  }
};


});






define('gcli/cli', ['require', 'exports', 'module' , 'gcli/util', 'gcli/canon', 'gcli/promise', 'gcli/types', 'gcli/types/basic', 'gcli/argument'], function(require, exports, module) {


var util = require('gcli/util');

var canon = require('gcli/canon');
var Promise = require('gcli/promise').Promise;

var types = require('gcli/types');
var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;
var Type = require('gcli/types').Type;
var ArrayType = require('gcli/types/basic').ArrayType;
var StringType = require('gcli/types/basic').StringType;
var BooleanType = require('gcli/types/basic').BooleanType;
var SelectionType = require('gcli/types/basic').SelectionType;

var Argument = require('gcli/argument').Argument;
var ArrayArgument = require('gcli/argument').ArrayArgument;
var NamedArgument = require('gcli/argument').NamedArgument;
var TrueNamedArgument = require('gcli/argument').TrueNamedArgument;
var MergedArgument = require('gcli/argument').MergedArgument;
var ScriptArgument = require('gcli/argument').ScriptArgument;

var evalCommand;




exports.startup = function() {
  types.registerType(CommandType);
  evalCommand = canon.addCommand(evalCommandSpec);
};

exports.shutdown = function() {
  types.unregisterType(CommandType);
  canon.removeCommand(evalCommandSpec.name);
  evalCommand = undefined;
};





















function Assignment(param, paramIndex) {
  this.param = param;
  this.paramIndex = paramIndex;
  this.assignmentChange = util.createEvent('Assignment.assignmentChange');

  this.setDefault();
}





Assignment.prototype.param = undefined;

Assignment.prototype.conversion = undefined;






Assignment.prototype.paramIndex = undefined;




Assignment.prototype.getArg = function() {
  return this.conversion.arg;
};




Assignment.prototype.getValue = function() {
  return this.conversion.value;
};




Assignment.prototype.getMessage = function() {
  return this.conversion.message ? this.conversion.message : '';
};






Assignment.prototype.getPredictions = function() {
  return this.conversion.getPredictions();
};










Assignment.prototype.setConversion = function(conversion) {
  var oldConversion = this.conversion;

  this.conversion = conversion;
  this.conversion.assign(this);

  if (this.conversion.equals(oldConversion)) {
    return;
  }

  this.assignmentChange({
    assignment: this,
    conversion: this.conversion,
    oldConversion: oldConversion
  });
};





Assignment.prototype.setDefault = function() {
  var conversion;
  if (this.param.getDefault) {
    conversion = this.param.getDefault();
  }
  else if (this.param.type.getDefault) {
    conversion = this.param.type.getDefault();
  }
  else {
    conversion = this.param.type.parse(new Argument());
  }

  this.setConversion(conversion);
};





Assignment.prototype.ensureVisibleArgument = function() {
  
  
  
  
  
  if (!this.conversion.arg.isBlank()) {
    return false;
  }

  var arg = this.conversion.arg.beget('', {
    prefixSpace: this.param instanceof CommandAssignment
  });
  this.conversion = this.param.type.parse(arg);
  this.conversion.assign(this);

  return true;
};








Assignment.prototype.getStatus = function(arg) {
  if (this.param.isDataRequired() && !this.conversion.isDataProvided()) {
    return Status.ERROR;
  }

  
  
  
  if (!this.param.isDataRequired() && this.getArg().isBlank()) {
    return Status.VALID;
  }

  return this.conversion.getStatus(arg);
};




Assignment.prototype.complete = function() {
  var predictions = this.conversion.getPredictions();
  if (predictions.length > 0) {
    var arg = this.conversion.arg.beget(predictions[0].name);
    if (!predictions[0].incomplete) {
      arg.suffix = ' ';
    }
    var conversion = this.param.type.parse(arg);
    this.setConversion(conversion);
  }
};




Assignment.prototype.decrement = function() {
  var replacement = this.param.type.decrement(this.conversion.value);
  if (replacement != null) {
    var str = this.param.type.stringify(replacement);
    var arg = this.conversion.arg.beget(str);
    var conversion = new Conversion(replacement, arg);
    this.setConversion(conversion);
  }
};




Assignment.prototype.increment = function() {
  var replacement = this.param.type.increment(this.conversion.value);
  if (replacement != null) {
    var str = this.param.type.stringify(replacement);
    var arg = this.conversion.arg.beget(str);
    var conversion = new Conversion(replacement, arg);
    this.setConversion(conversion);
  }
};




Assignment.prototype.toString = function() {
  return this.conversion.toString();
};

exports.Assignment = Assignment;









function CommandType() {
}

CommandType.prototype = Object.create(Type.prototype);

CommandType.prototype.name = 'command';

CommandType.prototype.decrement = SelectionType.prototype.decrement;
CommandType.prototype.increment = SelectionType.prototype.increment;
CommandType.prototype._findValue = SelectionType.prototype._findValue;

CommandType.prototype.stringify = function(command) {
  return command.name;
};





CommandType.prototype._findPredictions = function(arg) {
  var predictions = [];
  canon.getCommands().forEach(function(command) {
    if (command.name.indexOf(arg.text) === 0) {
      
      
      
      
      if (arg.text.length !== 0 || command.name.indexOf(' ') === -1) {
        predictions.push(command);
      }
    }
  }, this);
  return predictions;
};

CommandType.prototype.parse = function(arg) {
  
  
  var predictFunc = function() {
    return this._findPredictions(arg);
  }.bind(this);

  var predictions = this._findPredictions(arg);

  if (predictions.length === 0) {
    return new Conversion(null, arg, Status.ERROR,
        'Can\'t use \'' + arg.text + '\'.', predictFunc);
  }

  var command = predictions[0];

  if (predictions.length === 1) {
    
    
    if (command.name === arg.text && typeof command.exec === 'function') {
      return new Conversion(command, arg, Status.VALID, '');
    }
    return new Conversion(null, arg, Status.INCOMPLETE, '', predictFunc);
  }

  
  if (command.name === arg.text) {
    return new Conversion(command, arg, Status.VALID, '', predictFunc);
  }

  return new Conversion(null, arg, Status.INCOMPLETE, '', predictFunc);
};





var customEval = eval;





exports.setEvalFunction = function(newCustomEval) {
  customEval = newCustomEval;
};











exports.unsetEvalFunction = function() {
  customEval = undefined;
};




var evalCommandSpec = {
  name: '{',
  params: [
    {
      name: 'javascript',
      type: 'javascript',
      description: ''
    }
  ],
  returnType: 'html',
  description: { key: 'cliEvalJavascript' },
  exec: function(args, context) {
    
    var resultPrefix = '<em>{ ' + args.javascript + ' }</em> &#x2192; ';
    try {
      var result = customEval(args.javascript);

      if (result === null) {
        return resultPrefix + 'null.';
      }

      if (result === undefined) {
        return resultPrefix + 'undefined.';
      }

      if (typeof result === 'function') {
        
        return resultPrefix +
            (result + '').replace(/\n/g, '<br>').replace(/ /g, '&#160;');
      }

      return resultPrefix + result;
    }
    catch (ex) {
      return resultPrefix + 'Exception: ' + ex.message;
    }
  }
};





function CommandAssignment() {
  this.param = new canon.Parameter({
    name: '__command',
    type: 'command',
    description: 'The command to execute'
  });
  this.paramIndex = -1;
  this.assignmentChange = util.createEvent('CommandAssignment.assignmentChange');

  this.setDefault();
}

CommandAssignment.prototype = Object.create(Assignment.prototype);

CommandAssignment.prototype.getStatus = function(arg) {
  return Status.combine(
    Assignment.prototype.getStatus.call(this, arg),
    this.conversion.value && !this.conversion.value.exec ?
      Status.INCOMPLETE : Status.VALID
  );
};





function UnassignedAssignment() {
  this.param = new canon.Parameter({
    name: '__unassigned',
    type: 'string'
  });
  this.paramIndex = -1;
  this.assignmentChange = util.createEvent('UnassignedAssignment.assignmentChange');

  this.setDefault();
}

UnassignedAssignment.prototype = Object.create(Assignment.prototype);

UnassignedAssignment.prototype.getStatus = function(arg) {
  return Status.ERROR;
};

UnassignedAssignment.prototype.setUnassigned = function(args) {
  if (!args || args.length === 0) {
    this.setDefault();
  }
  else {
    var conversion = this.param.type.parse(new MergedArgument(args));
    this.setConversion(conversion);
  }
};

































function Requisition(environment, doc) {
  this.environment = environment;
  this.document = doc || document;

  
  
  this.commandAssignment = new CommandAssignment();

  
  
  
  
  
  
  this._assignments = {};

  
  this.assignmentCount = 0;

  
  this._args = [];

  
  this._unassigned = new UnassignedAssignment();

  
  
  this._structuralChangeInProgress = false;

  this.commandAssignment.assignmentChange.add(this._onCommandAssignmentChange, this);
  this.commandAssignment.assignmentChange.add(this._onAssignmentChange, this);

  this.commandOutputManager = canon.commandOutputManager;

  this.assignmentChange = util.createEvent('Requisition.assignmentChange');
  this.commandChange = util.createEvent('Requisition.commandChange');
  this.inputChange = util.createEvent('Requisition.inputChange');
}





var MORE_THAN_THE_MOST_ARGS_POSSIBLE = 1000000;




Requisition.prototype.destroy = function() {
  this.commandAssignment.assignmentChange.remove(this._onCommandAssignmentChange, this);
  this.commandAssignment.assignmentChange.remove(this._onAssignmentChange, this);

  delete this.document;
  delete this.environment;
};





Requisition.prototype._onAssignmentChange = function(ev) {
  
  if (ev.oldConversion != null &&
      ev.conversion.valueEquals(ev.oldConversion)) {
    return;
  }

  if (this._structuralChangeInProgress) {
    return;
  }

  this.assignmentChange(ev);

  
  
  if (ev.conversion.argEquals(ev.oldConversion)) {
    return;
  }

  this._structuralChangeInProgress = true;

  
  
  
  if (ev.assignment.param.isPositionalAllowed()) {
    for (var i = 0; i < ev.assignment.paramIndex; i++) {
      var assignment = this.getAssignment(i);
      if (assignment.param.isPositionalAllowed()) {
        if (assignment.ensureVisibleArgument()) {
          this._args.push(assignment.getArg());
        }
      }
    }
  }

  
  var index = MORE_THAN_THE_MOST_ARGS_POSSIBLE;
  for (var i = 0; i < this._args.length; i++) {
    if (this._args[i].assignment === ev.assignment) {
      if (i < index) {
        index = i;
      }
      this._args.splice(i, 1);
      i--;
    }
  }

  if (index === MORE_THAN_THE_MOST_ARGS_POSSIBLE) {
    this._args.push(ev.assignment.getArg());
  }
  else {
    
    var newArgs = ev.conversion.arg.getArgs();
    for (var i = 0; i < newArgs.length; i++) {
      this._args.splice(index + i, 0, newArgs[i]);
    }
  }
  this._structuralChangeInProgress = false;

  this.inputChange();
};




Requisition.prototype._onCommandAssignmentChange = function(ev) {
  this._assignments = {};

  var command = this.commandAssignment.getValue();
  if (command) {
    for (var i = 0; i < command.params.length; i++) {
      var param = command.params[i];
      var assignment = new Assignment(param, i);
      assignment.assignmentChange.add(this._onAssignmentChange, this);
      this._assignments[param.name] = assignment;
    }
  }
  this.assignmentCount = Object.keys(this._assignments).length;

  this.commandChange({
    requisition: this,
    oldValue: ev.oldValue,
    newValue: command
  });
};






Requisition.prototype.getAssignment = function(nameOrNumber) {
  var name = (typeof nameOrNumber === 'string') ?
    nameOrNumber :
    Object.keys(this._assignments)[nameOrNumber];
  return this._assignments[name] || undefined;
},




Requisition.prototype.getParameterNames = function() {
  return Object.keys(this._assignments);
},






Requisition.prototype.cloneAssignments = function() {
  return Object.keys(this._assignments).map(function(name) {
    return this._assignments[name];
  }, this);
};








Requisition.prototype.getStatus = function() {
  var status = Status.VALID;
  this.getAssignments(true).forEach(function(assignment) {
    var assignStatus = assignment.getStatus();
    if (assignment.getStatus() > status) {
      status = assignStatus;
    }
  }, this);
  if (status === Status.INCOMPLETE) {
    status = Status.ERROR;
  }
  return status;
};





Requisition.prototype.getArgsObject = function() {
  var args = {};
  this.getAssignments().forEach(function(assignment) {
    args[assignment.param.name] = assignment.getValue();
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




Requisition.prototype.setDefaultArguments = function() {
  this.getAssignments().forEach(function(assignment) {
    assignment.setDefault();
  }, this);
};




Requisition.prototype.toCanonicalString = function() {
  var line = [];

  var cmd = this.commandAssignment.getValue() ?
      this.commandAssignment.getValue().name :
      this.commandAssignment.getArg().text;
  line.push(cmd);

  Object.keys(this._assignments).forEach(function(name) {
    var assignment = this._assignments[name];
    var type = assignment.param.type;
    
    
    
    if (assignment.getValue() !== assignment.param.defaultValue) {
      line.push(' ');
      line.push(type.stringify(assignment.getValue()));
    }
  }, this);

  
  var command = this.commandAssignment.getValue();
  if (cmd === '{') {
    if (this.getAssignment(0).getArg().suffix.indexOf('}') === -1) {
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
  this._args.forEach(function(arg) {
    for (var i = 0; i < arg.prefix.length; i++) {
      args.push({ arg: arg, char: arg.prefix[i], part: 'prefix' });
    }
    for (var i = 0; i < arg.text.length; i++) {
      args.push({ arg: arg, char: arg.text[i], part: 'text' });
    }
    for (var i = 0; i < arg.suffix.length; i++) {
      args.push({ arg: arg, char: arg.suffix[i], part: 'suffix' });
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







Requisition.prototype.getInputStatusMarkup = function(cursor) {
  var argTraces = this.createInputArgTrace();
  
  
  cursor = cursor === 0 ? 0 : cursor - 1;
  var cTrace = argTraces[cursor];

  var statuses = [];
  for (var i = 0; i < argTraces.length; i++) {
    var argTrace = argTraces[i];
    var arg = argTrace.arg;
    var status = Status.VALID;
    if (argTrace.part === 'text') {
      status = arg.assignment.getStatus(arg);
      
      if (status === Status.INCOMPLETE) {
        
        if (arg !== cTrace.arg || cTrace.part !== 'text') {
          
          if (!(arg.assignment instanceof CommandAssignment)) {
            status = Status.ERROR;
          }
        }
      }
    }

    statuses.push(status);
  }

  return statuses;
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

    
    if (this._args.length > i + 1) {
      
      assignment = this._args[i + 1].assignment;
    }
    else if (assignment &&
        assignment.paramIndex + 1 < this.assignmentCount) {
      
      assignment = this.getAssignment(assignment.paramIndex + 1);
    }

    for (j = 0; j < arg.suffix.length; j++) {
      assignForPos.push(assignment);
    }
  }

  
  

  var reply = assignForPos[cursor - 1];

  if (!reply) {
    throw new Error('Missing assignment.' +
      ' cursor=' + cursor + ' text.length=' + this.toString().length);
  }

  return reply;
};










Requisition.prototype.exec = function(input) {
  var command;
  var args;
  var visible = true;

  if (input) {
    if (input.args != null) {
      
      
      command = canon.getCommand(input.typed);
      if (!command) {
        console.error('Command not found: ' + command);
      }
      args = input.args;

      
      
      visible = 'visible' in input ? input.visible : false;
    }
    else {
      this.update(input);
    }
  }

  if (!command) {
    command = this.commandAssignment.getValue();
    args = this.getArgsObject();
  }

  if (!command) {
    return false;
  }

  var outputObject = {
    command: command,
    args: args,
    typed: this.toCanonicalString(),
    completed: false,
    start: new Date()
  };

  this.commandOutputManager.sendCommandOutput(outputObject);

  var onComplete = (function(output, error) {
    if (visible) {
      outputObject.end = new Date();
      outputObject.duration = outputObject.end.getTime() - outputObject.start.getTime();
      outputObject.error = error;
      outputObject.output = output;
      outputObject.completed = true;
      this.commandOutputManager.sendCommandOutput(outputObject);
    }
  }).bind(this);

  try {
    var context = new ExecutionContext(this.environment, this.document);
    var reply = command.exec(args, context);

    if (reply != null && reply.isPromise) {
      reply.then(
        function(data) { onComplete(data, false); },
        function(error) { onComplete(error, true); });

      
      
    }
    else {
      onComplete(reply, false);
    }
  }
  catch (ex) {
    onComplete(ex, true);
  }

  this.clear();
  return true;
};














Requisition.prototype.update = function(input) {
  if (input.cursor == null) {
    input.cursor = { start: input.length, end: input.length };
  }

  this._structuralChangeInProgress = true;

  this._args = this._tokenize(input.typed);

  var args = this._args.slice(0); 
  this._split(args);
  this._assign(args);

  this._structuralChangeInProgress = false;

  this.inputChange();
};




Requisition.prototype.clear = function() {
  this.update({ typed: '', cursor: { start: 0, end: 0 } });
};




var In = {
  





  WHITESPACE: 1,

  




  SIMPLE: 2,

  



  SINGLE_Q: 3,

  



  DOUBLE_Q: 4,

  












  SCRIPT: 5
};








Requisition.prototype._tokenize = function(typed) {
  
  if (typed == null || typed.length === 0) {
    return [ new Argument('', '', '') ];
  }

  if (isSimple(typed)) {
    return [ new Argument(typed, '', '') ];
  }

  var mode = In.WHITESPACE;

  
  
  
  
  
  
  typed = typed
      .replace(/\\\\/g, '\\')
      .replace(/\\b/g, '\b')
      .replace(/\\f/g, '\f')
      .replace(/\\n/g, '\n')
      .replace(/\\r/g, '\r')
      .replace(/\\t/g, '\t')
      .replace(/\\v/g, '\v')
      .replace(/\\n/g, '\n')
      .replace(/\\r/g, '\r')
      .replace(/\\ /g, '\uF000')
      .replace(/\\'/g, '\uF001')
      .replace(/\\"/g, '\uF002')
      .replace(/\\{/g, '\uF003')
      .replace(/\\}/g, '\uF004');

  function unescape2(escaped) {
    return escaped
        .replace(/\uF000/g, ' ')
        .replace(/\uF001/g, '\'')
        .replace(/\uF002/g, '"')
        .replace(/\uF003/g, '{')
        .replace(/\uF004/g, '}');
  }

  var i = 0;          
  var start = 0;      
  var prefix = '';    
  var args = [];      
  var blockDepth = 0; 

  
  
  

  while (true) {
    var c = typed[i];
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
          var str = unescape2(typed.substring(start, i));
          args.push(new Argument(str, prefix, ''));
          mode = In.WHITESPACE;
          start = i;
          prefix = '';
        }
        break;

      case In.SINGLE_Q:
        if (c === '\'') {
          var str = unescape2(typed.substring(start, i));
          args.push(new Argument(str, prefix, c));
          mode = In.WHITESPACE;
          start = i + 1;
          prefix = '';
        }
        break;

      case In.DOUBLE_Q:
        if (c === '"') {
          var str = unescape2(typed.substring(start, i));
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
            var str = unescape2(typed.substring(start, i));
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
        var str = unescape2(typed.substring(start, i + 1));
        args.push(new ScriptArgument(str, prefix, ''));
      }
      else {
        var str = unescape2(typed.substring(start, i + 1));
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
  
  
  
  if (args[0] instanceof ScriptArgument) {
    
    
    var conversion = new Conversion(evalCommand, new Argument());
    this.commandAssignment.setConversion(conversion);
    return;
  }

  var argsUsed = 1;
  var conversion;

  while (argsUsed <= args.length) {
    var arg = (argsUsed === 1) ?
      args[0] :
      new MergedArgument(args, 0, argsUsed);
    conversion = this.commandAssignment.param.type.parse(arg);

    
    
    
    if (!conversion.value || conversion.value.exec) {
      break;
    }

    
    
    
    

    argsUsed++;
  }

  this.commandAssignment.setConversion(conversion);

  for (var i = 0; i < argsUsed; i++) {
    args.shift();
  }

  
};




Requisition.prototype._assign = function(args) {
  if (!this.commandAssignment.getValue()) {
    this._unassigned.setUnassigned(args);
    return;
  }

  if (args.length === 0) {
    this.setDefaultArguments();
    this._unassigned.setDefault();
    return;
  }

  
  
  if (this.assignmentCount === 0) {
    this._unassigned.setUnassigned(args);
    return;
  }

  
  
  if (this.assignmentCount === 1) {
    var assignment = this.getAssignment(0);
    if (assignment.param.type instanceof StringType) {
      var arg = (args.length === 1) ?
        args[0] :
        new MergedArgument(args);
      var conversion = assignment.param.type.parse(arg);
      assignment.setConversion(conversion);
      this._unassigned.setDefault();
      return;
    }
  }

  
  
  var names = this.getParameterNames();

  
  var arrayArgs = {};

  
  this.getAssignments(false).forEach(function(assignment) {
    
    
    var i = 0;
    while (i < args.length) {
      if (assignment.param.isKnownAs(args[i].text)) {
        var arg = args.splice(i, 1)[0];
        names = names.filter(function(test) {
          return test !== assignment.param.name;
        });

        
        if (assignment.param.type instanceof BooleanType) {
          arg = new TrueNamedArgument(null, arg);
        }
        else {
          var valueArg = null;
          if (i + 1 >= args.length) {
            valueArg = args.splice(i, 1)[0];
          }
          arg = new NamedArgument(arg, valueArg);
        }

        if (assignment.param.type instanceof ArrayType) {
          var arrayArg = arrayArgs[assignment.param.name];
          if (!arrayArg) {
            arrayArg = new ArrayArgument();
            arrayArgs[assignment.param.name] = arrayArg;
          }
          arrayArg.addArgument(arg);
        }
        else {
          var conversion = assignment.param.type.parse(arg);
          assignment.setConversion(conversion);
        }
      }
      else {
        
        i++;
      }
    }
  }, this);

  
  names.forEach(function(name) {
    var assignment = this.getAssignment(name);

    
    
    if (!assignment.param.isPositionalAllowed()) {
      assignment.setDefault();
      return;
    }

    
    
    if (assignment.param.type instanceof ArrayType) {
      var arrayArg = arrayArgs[assignment.param.name];
      if (!arrayArg) {
        arrayArg = new ArrayArgument();
        arrayArgs[assignment.param.name] = arrayArg;
      }
      arrayArg.addArguments(args);
      args = [];
    }
    else {
      var arg = (args.length > 0) ?
          args.splice(0, 1)[0] :
          new Argument();

      var conversion = assignment.param.type.parse(arg);
      assignment.setConversion(conversion);
    }
  }, this);

  
  Object.keys(arrayArgs).forEach(function(name) {
    var assignment = this.getAssignment(name);
    var conversion = assignment.param.type.parse(arrayArgs[name]);
    assignment.setConversion(conversion);
  }, this);

  if (args.length > 0) {
    this._unassigned.setUnassigned(args);
  }
  else {
    this._unassigned.setDefault();
  }
};

exports.Requisition = Requisition;





function ExecutionContext(environment, document) {
  this.environment = environment;
  this.document = document;
}

ExecutionContext.prototype.createPromise = function() {
  return new Promise();
};


});






define('gcli/promise', ['require', 'exports', 'module' ], function(require, exports, module) {

  Components.utils.import("resource:///modules/devtools/Promise.jsm");
  exports.Promise = Promise;

});






define('gcli/ui/display', ['require', 'exports', 'module' , 'gcli/ui/inputter', 'gcli/ui/arg_fetch', 'gcli/ui/menu', 'gcli/ui/focus'], function(require, exports, module) {

var Inputter = require('gcli/ui/inputter').Inputter;
var ArgFetcher = require('gcli/ui/arg_fetch').ArgFetcher;
var CommandMenu = require('gcli/ui/menu').CommandMenu;
var FocusManager = require('gcli/ui/focus').FocusManager;





function Display(options) {
  this.hintElement = options.hintElement;
  this.gcliTerm = options.gcliTerm;
  this.consoleWrap = options.consoleWrap;
  this.requisition = options.requisition;

  
  this.focusManager = new FocusManager({ document: options.chromeDocument });
  this.focusManager.onFocus.add(this.gcliTerm.show, this.gcliTerm);
  this.focusManager.onBlur.add(this.gcliTerm.hide, this.gcliTerm);
  this.focusManager.addMonitoredElement(this.gcliTerm.hintNode, 'gcliTerm');

  this.inputter = new Inputter({
    document: options.contentDocument,
    requisition: options.requisition,
    inputElement: options.inputElement,
    completeElement: options.completeElement,
    completionPrompt: '',
    backgroundElement: options.backgroundElement,
    focusManager: this.focusManager
  });

  this.menu = new CommandMenu({
    document: options.contentDocument,
    requisition: options.requisition,
    menuClass: 'gcliterm-menu'
  });
  this.hintElement.appendChild(this.menu.element);

  this.argFetcher = new ArgFetcher({
    document: options.contentDocument,
    requisition: options.requisition,
    argFetcherClass: 'gcliterm-argfetcher'
  });
  this.hintElement.appendChild(this.argFetcher.element);

  this.chromeWindow = options.chromeDocument.defaultView;
  this.resizer = this.resizer.bind(this);
  this.chromeWindow.addEventListener('resize', this.resizer, false);
  this.requisition.commandChange.add(this.resizer, this);
}




Display.prototype.destroy = function() {
  this.chromeWindow.removeEventListener('resize', this.resizer, false);
  delete this.resizer;
  delete this.chromeWindow;
  delete this.consoleWrap;

  this.hintElement.removeChild(this.menu.element);
  this.menu.destroy();
  this.hintElement.removeChild(this.argFetcher.element);
  this.argFetcher.destroy();

  this.inputter.destroy();

  this.focusManager.removeMonitoredElement(this.gcliTerm.hintNode, 'gcliTerm');
  this.focusManager.onFocus.remove(this.gcliTerm.show, this.gcliTerm);
  this.focusManager.onBlur.remove(this.gcliTerm.hide, this.gcliTerm);
  this.focusManager.destroy();

  delete this.gcliTerm;
  delete this.hintElement;
};




Display.prototype.resizer = function() {
  var parentRect = this.consoleWrap.getBoundingClientRect();
  var parentHeight = parentRect.bottom - parentRect.top - 64;

  if (parentHeight < 100) {
    this.hintElement.classList.add('gcliterm-hint-nospace');
  }
  else {
    this.hintElement.classList.remove('gcliterm-hint-nospace');

    var isMenuVisible = this.menu.element.style.display !== 'none';
    if (isMenuVisible) {
      this.menu.setMaxHeight(parentHeight);

      
      
      
      
      
      var idealMenuHeight = (19 * this.menu.items.length) + 22;

      if (idealMenuHeight > parentHeight) {
        this.hintElement.style.overflowY = 'scroll';
        this.hintElement.style.borderBottomColor = 'threedshadow';
      }
      else {
        this.hintElement.style.overflowY = null;
        this.hintElement.style.borderBottomColor = 'white';
      }
    }
    else {
      this.argFetcher.setMaxHeight(parentHeight);

      this.hintElement.style.overflowY = null;
      this.hintElement.style.borderBottomColor = 'white';
    }
  }
};

exports.Display = Display;

});






define('gcli/ui/inputter', ['require', 'exports', 'module' , 'gcli/util', 'gcli/types', 'gcli/history', 'text!gcli/ui/inputter.css'], function(require, exports, module) {
var cliView = exports;


var KeyEvent = require('gcli/util').event.KeyEvent;
var dom = require('gcli/util').dom;

var Status = require('gcli/types').Status;
var History = require('gcli/history').History;

var inputterCss = require('text!gcli/ui/inputter.css');





function Inputter(options) {
  this.requisition = options.requisition;

  
  this.element = options.inputElement || 'gcli-input';
  if (typeof this.element === 'string') {
    this.document = options.document || document;
    var name = this.element;
    this.element = this.document.getElementById(name);
    if (!this.element) {
      throw new Error('No element with id=' + name + '.');
    }
  }
  else {
    
    this.document = this.element.ownerDocument;
  }

  if (inputterCss != null) {
    this.style = dom.importCss(inputterCss, this.document);
  }

  this.element.spellcheck = false;

  
  this.lastTabDownAt = 0;

  
  this._caretChange = null;

  
  this.onKeyDown = this.onKeyDown.bind(this);
  this.onKeyUp = this.onKeyUp.bind(this);
  this.element.addEventListener('keydown', this.onKeyDown, false);
  this.element.addEventListener('keyup', this.onKeyUp, false);

  this.completer = options.completer || new Completer(options);
  this.completer.decorate(this);

  
  this.history = options.history || new History(options);
  this._scrollingThroughHistory = false;

  
  this.onMouseUp = function(ev) {
    this.completer.update(this.getInputState());
  }.bind(this);
  this.element.addEventListener('mouseup', this.onMouseUp, false);

  this.focusManager = options.focusManager;
  if (this.focusManager) {
    this.focusManager.addMonitoredElement(this.element, 'input');
  }

  this.requisition.inputChange.add(this.onInputChange, this);

  this.update();
}




Inputter.prototype.destroy = function() {
  this.requisition.inputChange.remove(this.onInputChange, this);
  if (this.focusManager) {
    this.focusManager.removeMonitoredElement(this.element, 'input');
  }

  this.element.removeEventListener('keydown', this.onKeyDown, false);
  this.element.removeEventListener('keyup', this.onKeyUp, false);
  delete this.onKeyDown;
  delete this.onKeyUp;

  this.history.destroy();
  this.completer.destroy();

  if (this.style) {
    this.style.parentNode.removeChild(this.style);
    delete this.style;
  }

  delete this.document;
  delete this.element;
};




Inputter.prototype.appendAfter = function(element) {
  this.element.parentNode.insertBefore(element, this.element.nextSibling);
};




Inputter.prototype.onInputChange = function() {
  if (this._caretChange == null) {
    
    
    
    this._caretChange = Caret.TO_ARG_END;
  }
  this._setInputInternal(this.requisition.toString());
};














Inputter.prototype._setInputInternal = function(str, update) {
  if (!this.document) {
    return; 
  }

  if (this.element.value && this.element.value === str) {
    this._processCaretChange(this.getInputState(), false);
    return;
  }

  
  
  this.document.defaultView.setTimeout(function() {
    if (!this.document) {
      return; 
    }

    
    
    
    var input = this.getInputState();
    input.typed = str;
    this._processCaretChange(input);
    this.element.value = str;

    if (update) {
      this.update();
    }
  }.bind(this), 0);
};





var Caret = {
  


  NO_CHANGE: 0,

  


  SELECT_ALL: 1,

  


  TO_END: 2,

  



  TO_ARG_END: 3
};








Inputter.prototype._processCaretChange = function(input, forceUpdate) {
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

    case null:
    case Caret.NO_CHANGE:
      start = input.cursor.start;
      end = input.cursor.end;
      break;
  }

  start = (start > input.typed.length) ? input.typed.length : start;
  end = (end > input.typed.length) ? input.typed.length : end;

  var newInput = { typed: input.typed, cursor: { start: start, end: end }};
  if (start !== input.cursor.start || end !== input.cursor.end || forceUpdate) {
    this.completer.update(newInput);
  }

  this.element.selectionStart = newInput.cursor.start;
  this.element.selectionEnd = newInput.cursor.end;

  this._caretChange = null;
  return newInput;
};










Inputter.prototype.setInput = function(str) {
  this.element.value = str;
  this.update();
};




Inputter.prototype.focus = function() {
  this.element.focus();
};





Inputter.prototype.onKeyDown = function(ev) {
  if (ev.keyCode === KeyEvent.DOM_VK_UP || ev.keyCode === KeyEvent.DOM_VK_DOWN) {
    ev.preventDefault();
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
  
  if (ev.keyCode === KeyEvent.DOM_VK_RETURN) {
    var worst = this.requisition.getStatus();
    
    if (worst === Status.VALID) {
      this._scrollingThroughHistory = false;
      this.history.add(this.element.value);
      this.requisition.exec();
    }
    
    
    return;
  }

  if (ev.keyCode === KeyEvent.DOM_VK_TAB && !ev.shiftKey) {
    
    
    
    
    
    
    if (this.lastTabDownAt + 1000 > ev.timeStamp) {
      
      
      
      this._caretChange = Caret.TO_ARG_END;
      this._processCaretChange(this.getInputState(), true);
      this.getCurrentAssignment().complete();
    }
    this.lastTabDownAt = 0;
    this._scrollingThroughHistory = false;
    return;
  }

  if (ev.keyCode === KeyEvent.DOM_VK_UP) {
    if (this.element.value === '' || this._scrollingThroughHistory) {
      this._scrollingThroughHistory = true;
      this._setInputInternal(this.history.backward(), true);
    }
    else {
      this.getCurrentAssignment().increment();
    }
    return;
  }

  if (ev.keyCode === KeyEvent.DOM_VK_DOWN) {
    if (this.element.value === '' || this._scrollingThroughHistory) {
      this._scrollingThroughHistory = true;
      this._setInputInternal(this.history.forward(), true);
    }
    else {
      this.getCurrentAssignment().decrement();
    }
    return;
  }

  this._scrollingThroughHistory = false;
  this._caretChange = Caret.NO_CHANGE;
  this.update();
};





Inputter.prototype.getCurrentAssignment = function() {
  var start = this.element.selectionStart;
  return this.requisition.getAssignmentAt(start);
};




Inputter.prototype.update = function() {
  var input = this.getInputState();
  this.requisition.update(input);
  this.completer.update(input);
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
    console.log('fixing input.typed=""', input);
  }

  return input;
};

cliView.Inputter = Inputter;














function Completer(options) {
  this.document = options.document || document;
  this.requisition = options.requisition;
  this.elementCreated = false;

  this.element = options.completeElement || 'gcli-row-complete';
  if (typeof this.element === 'string') {
    var name = this.element;
    this.element = this.document.getElementById(name);

    if (!this.element) {
      this.elementCreated = true;
      this.element = dom.createElement(this.document, 'div');
      this.element.className = 'gcli-in-complete gcliVALID';
      this.element.setAttribute('tabindex', '-1');
      this.element.setAttribute('aria-live', 'polite');
    }
  }

  this.completionPrompt = typeof options.completionPrompt === 'string'
      ? options.completionPrompt
      : '&#x00bb;';

  if (options.inputBackgroundElement) {
    this.backgroundElement = options.inputBackgroundElement;
  }
  else {
    this.backgroundElement = this.element;
  }
}




Completer.prototype.destroy = function() {
  delete this.document;
  delete this.element;
  delete this.backgroundElement;

  if (this.elementCreated) {
    this.document.defaultView.removeEventListener('resize', this.resizer, false);
  }

  delete this.inputter;
};






Completer.copyStyles = [ 'fontSize', 'fontFamily', 'fontWeight', 'fontStyle' ];





Completer.prototype.decorate = function(inputter) {
  this.inputter = inputter;
  var input = inputter.element;

  
  
  if (this.elementCreated) {
    this.inputter.appendAfter(this.element);

    var styles = this.document.defaultView.getComputedStyle(input, null);
    Completer.copyStyles.forEach(function(style) {
      this.element.style[style] = styles[style];
    }, this);

    
    
    this.element.style.color = input.style.backgroundColor;

    
    
    
    
    this.element.style.backgroundColor = (this.backgroundElement != this.element) ?
        'transparent' :
        input.style.backgroundColor;
    input.style.backgroundColor = 'transparent';

    
    input.style.paddingLeft = '20px';

    this.resizer = this.resizer.bind(this);
    this.document.defaultView.addEventListener('resize', this.resizer, false);
    this.resizer();
  }
};




Completer.prototype.resizer = function() {
  var rect = this.inputter.element.getBoundingClientRect();
  
  var height = rect.bottom - rect.top - 4;

  this.element.style.top = rect.top + 'px';
  this.element.style.height = height + 'px';
  this.element.style.lineHeight = height + 'px';
  this.element.style.left = rect.left + 'px';
  this.element.style.width = (rect.right - rect.left) + 'px';
};






function isStrictCompletion(inputValue, completion) {
  
  
  inputValue = inputValue.replace(/^\s*/, '');
  
  
  return completion.indexOf(inputValue) === 0;
}




Completer.prototype.update = function(input) {
  var current = this.requisition.getAssignmentAt(input.cursor.start);
  var predictions = current.getPredictions();

  var completion = '<span class="gcli-prompt">' + this.completionPrompt + '</span> ';
  if (input.typed.length > 0) {
    var scores = this.requisition.getInputStatusMarkup(input.cursor.start);
    completion += this.markupStatusScore(scores, input);
  }

  if (input.typed.length > 0 && predictions.length > 0) {
    var tab = predictions[0].name;
    var existing = current.getArg().text;
    if (isStrictCompletion(existing, tab) && input.cursor.start === input.typed.length) {
      
      var numLeadingSpaces = existing.match(/^(\s*)/)[0].length;
      var suffix = tab.slice(existing.length - numLeadingSpaces);
      completion += '<span class="gcli-in-ontab">' + suffix + '</span>';
    } else {
      
      completion += ' &#xa0;<span class="gcli-in-ontab">&#x21E5; ' +
          tab + '</span>';
    }
  }

  
  
  var command = this.requisition.commandAssignment.getValue();
  if (command && command.name === '{') {
    if (this.requisition.getAssignment(0).getArg().suffix.indexOf('}') === -1) {
      completion += '<span class="gcli-in-closebrace">}</span>';
    }
  }

  dom.setInnerHtml(this.element, completion);
};




Completer.prototype.markupStatusScore = function(scores, input) {
  var completion = '';
  if (scores.length === 0) {
    return completion;
  }

  var i = 0;
  var lastStatus = -1;
  while (true) {
    if (lastStatus !== scores[i]) {
      var state = scores[i];
      if (!state) {
        console.error('No state at i=' + i + '. scores.len=' + scores.length);
        state = Status.VALID;
      }
      completion += '<span class="gcli-in-' + state.toString().toLowerCase() + '">';
      lastStatus = scores[i];
    }
    var char = input.typed[i];
    if (char === ' ') {
      char = '&#xa0;';
    }
    completion += char;
    i++;
    if (i === input.typed.length) {
      completion += '</span>';
      break;
    }
    if (lastStatus !== scores[i]) {
      completion += '</span>';
    }
  }

  return completion;
};

cliView.Completer = Completer;


});






define('gcli/history', ['require', 'exports', 'module' ], function(require, exports, module) {






function History() {
  
  
  
  
  this._buffer = [''];

  
  
  this._current = 0;
}




History.prototype.destroy = function() {

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

});define("text!gcli/ui/inputter.css", [], void 0);






define('gcli/ui/arg_fetch', ['require', 'exports', 'module' , 'gcli/util', 'gcli/types', 'gcli/ui/field', 'gcli/ui/domtemplate', 'text!gcli/ui/arg_fetch.css', 'text!gcli/ui/arg_fetch.html'], function(require, exports, module) {
var argFetch = exports;


var dom = require('gcli/util').dom;
var Status = require('gcli/types').Status;

var getField = require('gcli/ui/field').getField;
var Templater = require('gcli/ui/domtemplate').Templater;

var editorCss = require('text!gcli/ui/arg_fetch.css');
var argFetchHtml = require('text!gcli/ui/arg_fetch.html');











function ArgFetcher(options) {
  this.document = options.document || document;
  this.requisition = options.requisition;

  
  if (!this.document) {
    throw new Error('No document');
  }

  this.element =  dom.createElement(this.document, 'div');
  this.element.className = options.argFetcherClass || 'gcli-argfetch';
  
  this.fields = [];

  this.tmpl = new Templater();
  
  this.okElement = null;

  
  if (editorCss != null) {
    this.style = dom.importCss(editorCss, this.document);
  }

  var templates = dom.createElement(this.document, 'div');
  dom.setInnerHtml(templates, argFetchHtml);
  this.reqTempl = templates.querySelector('.gcli-af-template');

  this.requisition.commandChange.add(this.onCommandChange, this);
  this.requisition.inputChange.add(this.onInputChange, this);

  this.onCommandChange();
}




ArgFetcher.prototype.destroy = function() {
  this.requisition.inputChange.remove(this.onInputChange, this);
  this.requisition.commandChange.remove(this.onCommandChange, this);

  if (this.style) {
    this.style.parentNode.removeChild(this.style);
    delete this.style;
  }

  this.fields.forEach(function(field) { field.destroy(); });

  delete this.document;
  delete this.element;
  delete this.okElement;
  delete this.reqTempl;
};




ArgFetcher.prototype.onCommandChange = function(ev) {
  var command = this.requisition.commandAssignment.getValue();
  if (!command || !command.exec) {
    this.element.style.display = 'none';
  }
  else {
    if (ev && ev.oldValue === ev.newValue) {
      
      return;
    }

    this.fields.forEach(function(field) { field.destroy(); });
    this.fields = [];

    var reqEle = this.reqTempl.cloneNode(true);
    this.tmpl.processNode(reqEle, this);
    dom.clearElement(this.element);
    this.element.appendChild(reqEle);

    var status = this.requisition.getStatus();
    this.okElement.disabled = (status === Status.VALID);

    this.element.style.display = 'block';
  }
};




ArgFetcher.prototype.onInputChange = function(ev) {
  var command = this.requisition.commandAssignment.getValue();
  if (command && command.exec) {
    var status = this.requisition.getStatus();
    this.okElement.disabled = (status !== Status.VALID);
  }
};





ArgFetcher.prototype.getInputFor = function(assignment) {
  try {
    var newField = getField(assignment.param.type, {
      document: this.document,
      type: assignment.param.type,
      name: assignment.param.name,
      requisition: this.requisition,
      required: assignment.param.isDataRequired(),
      named: !assignment.param.isPositionalAllowed()
    });

    
    newField.fieldChanged.add(function(ev) {
      assignment.setConversion(ev.conversion);
    }, this);
    assignment.assignmentChange.add(function(ev) {
      newField.setConversion(ev.conversion);
    }.bind(this));

    this.fields.push(newField);
    newField.setConversion(this.assignment.conversion);

    
    
    assignment.field = newField;

    return newField.element;
  }
  catch (ex) {
    
    
    console.error(ex);
    return '';
  }
};




ArgFetcher.prototype.linkMessageElement = function(assignment, element) {
  
  var field = assignment.field;
  delete assignment.field;
  if (field == null) {
    console.error('Missing field for ' + assignment.param.name);
    return 'Missing field';
  }
  field.setMessageElement(element);
  return '';
};




ArgFetcher.prototype.onFormOk = function(ev) {
  this.requisition.exec();
};




ArgFetcher.prototype.onFormCancel = function(ev) {
  this.requisition.clear();
};




ArgFetcher.prototype.setMaxHeight = function(height, isTooBig) {
  this.fields.forEach(function(field) {
    if (field.menu) {
      
      
      
      
      
      field.menu.setMaxHeight(height - 105);
    }
  });
};

argFetch.ArgFetcher = ArgFetcher;


});






define('gcli/ui/field', ['require', 'exports', 'module' , 'gcli/util', 'gcli/l10n', 'gcli/argument', 'gcli/types', 'gcli/types/basic', 'gcli/types/javascript', 'gcli/ui/menu'], function(require, exports, module) {


var dom = require('gcli/util').dom;
var createEvent = require('gcli/util').createEvent;
var l10n = require('gcli/l10n');

var Argument = require('gcli/argument').Argument;
var TrueNamedArgument = require('gcli/argument').TrueNamedArgument;
var FalseNamedArgument = require('gcli/argument').FalseNamedArgument;
var ArrayArgument = require('gcli/argument').ArrayArgument;

var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;
var ArrayConversion = require('gcli/types').ArrayConversion;

var StringType = require('gcli/types/basic').StringType;
var NumberType = require('gcli/types/basic').NumberType;
var BooleanType = require('gcli/types/basic').BooleanType;
var BlankType = require('gcli/types/basic').BlankType;
var SelectionType = require('gcli/types/basic').SelectionType;
var DeferredType = require('gcli/types/basic').DeferredType;
var ArrayType = require('gcli/types/basic').ArrayType;
var JavascriptType = require('gcli/types/javascript').JavascriptType;

var Menu = require('gcli/ui/menu').Menu;
















function Field(document, type, named, name, requ) {
}






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
    if (message == null) {
      message = '';
    }
    dom.setInnerHtml(this.messageElement, message);
  }
};





Field.prototype.onInputChange = function() {
  var conversion = this.getConversion();
  this.fieldChanged({ conversion: conversion });
  this.setMessage(conversion.message);
};






Field.claim = function() {
  throw new Error('Field should not be used directly');
};
Field.MATCH = 5;
Field.DEFAULT_MATCH = 4;
Field.IF_NOTHING_BETTER = 1;
Field.NO_MATCH = 0;





var fieldCtors = [];
function addField(fieldCtor) {
  if (typeof fieldCtor !== 'function') {
    console.error('addField erroring on ', fieldCtor);
    throw new Error('addField requires a Field constructor');
  }
  fieldCtors.push(fieldCtor);
}

function removeField(field) {
  if (typeof field !== 'string') {
    fields = fields.filter(function(test) {
      return test !== field;
    });
    delete fields[field];
  }
  else if (field instanceof Field) {
    removeField(field.name);
  }
  else {
    console.error('removeField erroring on ', field);
    throw new Error('removeField requires an instance of Field');
  }
}

function getField(type, options) {
  var ctor;
  var highestClaim = -1;
  fieldCtors.forEach(function(fieldCtor) {
    var claim = fieldCtor.claim(type);
    if (claim > highestClaim) {
      highestClaim = claim;
      ctor = fieldCtor;
    }
  });

  if (!ctor) {
    console.error('Unknown field type ', type, ' in ', fieldCtors);
    throw new Error('Can\'t find field for ' + type);
  }

  return new ctor(type, options);
}

exports.Field = Field;
exports.addField = addField;
exports.removeField = removeField;
exports.getField = getField;





function StringField(type, options) {
  this.document = options.document;
  this.type = type;
  this.arg = new Argument();

  this.element = dom.createElement(this.document, 'input');
  this.element.type = 'text';
  this.element.className = 'gcli-field';

  this.onInputChange = this.onInputChange.bind(this);
  this.element.addEventListener('keyup', this.onInputChange, false);

  this.fieldChanged = createEvent('StringField.fieldChanged');
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
  
  this.arg = this.arg.beget(this.element.value, { prefixSpace: true });
  return this.type.parse(this.arg);
};

StringField.claim = function(type) {
  return type instanceof StringType ? Field.MATCH : Field.IF_NOTHING_BETTER;
};

exports.StringField = StringField;
addField(StringField);





function NumberField(type, options) {
  this.document = options.document;
  this.type = type;
  this.arg = new Argument();

  this.element = dom.createElement(this.document, 'input');
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

  this.fieldChanged = createEvent('NumberField.fieldChanged');
}

NumberField.prototype = Object.create(Field.prototype);

NumberField.claim = function(type) {
  return type instanceof NumberType ? Field.MATCH : Field.NO_MATCH;
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
  this.arg = this.arg.beget(this.element.value, { prefixSpace: true });
  return this.type.parse(this.arg);
};

exports.NumberField = NumberField;
addField(NumberField);





function BooleanField(type, options) {
  this.document = options.document;
  this.type = type;
  this.name = options.name;
  this.named = options.named;

  this.element = dom.createElement(this.document, 'input');
  this.element.type = 'checkbox';
  this.element.id = 'gcliForm' + this.name;

  this.onInputChange = this.onInputChange.bind(this);
  this.element.addEventListener('change', this.onInputChange, false);

  this.fieldChanged = createEvent('BooleanField.fieldChanged');
}

BooleanField.prototype = Object.create(Field.prototype);

BooleanField.claim = function(type) {
  return type instanceof BooleanType ? Field.MATCH : Field.NO_MATCH;
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
  var value = this.element.checked;
  var arg = this.named ?
    value ? new TrueNamedArgument(this.name) : new FalseNamedArgument() :
    new Argument(' ' + value);
  return new Conversion(value, arg);
};

exports.BooleanField = BooleanField;
addField(BooleanField);













function SelectionField(type, options) {
  this.document = options.document;
  this.type = type;
  this.items = [];

  this.element = dom.createElement(this.document, 'select');
  this.element.className = 'gcli-field';
  this._addOption({
    name: l10n.lookupFormat('fieldSelectionSelect', [ options.name ])
  });
  var lookup = this.type.getLookup();
  lookup.forEach(this._addOption, this);

  this.onInputChange = this.onInputChange.bind(this);
  this.element.addEventListener('change', this.onInputChange, false);

  this.fieldChanged = createEvent('SelectionField.fieldChanged');
}

SelectionField.prototype = Object.create(Field.prototype);

SelectionField.claim = function(type) {
  return type instanceof SelectionType ? Field.DEFAULT_MATCH : Field.NO_MATCH;
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
  var arg = new Argument(item.name, ' ');
  var value = item.value ? item.value : item;
  return new Conversion(value, arg);
};

SelectionField.prototype._addOption = function(item) {
  item.index = this.items.length;
  this.items.push(item);

  var option = dom.createElement(this.document, 'option');
  option.innerHTML = item.name;
  option.value = item.index;
  this.element.appendChild(option);
};

exports.SelectionField = SelectionField;
addField(SelectionField);





function JavascriptField(type, options) {
  this.document = options.document;
  this.type = type;
  this.requ = options.requisition;

  this.onInputChange = this.onInputChange.bind(this);
  this.arg = new Argument('', '{ ', ' }');

  this.element = dom.createElement(this.document, 'div');

  this.input = dom.createElement(this.document, 'input');
  this.input.type = 'text';
  this.input.addEventListener('keyup', this.onInputChange, false);
  this.input.style.marginBottom = '0';
  this.input.className = 'gcli-field';
  this.element.appendChild(this.input);

  this.menu = new Menu({ document: this.document, field: true });
  this.element.appendChild(this.menu.element);

  this.setConversion(this.type.parse(new Argument('')));

  this.fieldChanged = createEvent('JavascriptField.fieldChanged');

  
  this.menu.onItemClick = this.onItemClick.bind(this);
}

JavascriptField.prototype = Object.create(Field.prototype);

JavascriptField.claim = function(type) {
  return type instanceof JavascriptType ? Field.MATCH : Field.NO_MATCH;
};

JavascriptField.prototype.destroy = function() {
  Field.prototype.destroy.call(this);
  this.input.removeEventListener('keyup', this.onInputChange, false);
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
  if (this.type instanceof JavascriptType) {
    var typed = conversion.arg.text;
    var lastDot = typed.lastIndexOf('.');
    if (lastDot !== -1) {
      prefixLen = lastDot;
    }
  }

  var items = [];
  var predictions = conversion.getPredictions();
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
  this.setMessage(conversion.message);
};

JavascriptField.prototype.onItemClick = function(ev) {
  this.item = ev.currentTarget.item;
  this.arg = this.arg.beget(this.item.complete, { normalize: true });
  var conversion = this.type.parse(this.arg);
  this.fieldChanged({ conversion: conversion });
  this.setMessage(conversion.message);
};

JavascriptField.prototype.onInputChange = function(ev) {
  this.item = ev.currentTarget.item;
  var conversion = this.getConversion();
  this.fieldChanged({ conversion: conversion });
  this.setMessage(conversion.message);
};

JavascriptField.prototype.getConversion = function() {
  
  this.arg = this.arg.beget(this.input.value, { normalize: true });
  return this.type.parse(this.arg);
};

JavascriptField.DEFAULT_VALUE = '__JavascriptField.DEFAULT_VALUE';

exports.JavascriptField = JavascriptField;
addField(JavascriptField);






function DeferredField(type, options) {
  this.document = options.document;
  this.type = type;
  this.options = options;
  this.requisition = options.requisition;
  this.requisition.assignmentChange.add(this.update, this);

  this.element = dom.createElement(this.document, 'div');
  this.update();

  this.fieldChanged = createEvent('DeferredField.fieldChanged');
}

DeferredField.prototype = Object.create(Field.prototype);

DeferredField.prototype.update = function() {
  var subtype = this.type.defer();
  if (subtype === this.subtype) {
    return;
  }

  if (this.field) {
    this.field.destroy();
  }

  this.subtype = subtype;
  this.field = getField(subtype, this.options);
  this.field.fieldChanged.add(this.fieldChanged, this);

  dom.clearElement(this.element);
  this.element.appendChild(this.field.element);
};

DeferredField.claim = function(type) {
  return type instanceof DeferredType ? Field.MATCH : Field.NO_MATCH;
};

DeferredField.prototype.destroy = function() {
  Field.prototype.destroy.call(this);
  this.requisition.assignmentChange.remove(this.update, this);
  delete this.element;
  delete this.document;
  delete this.onInputChange;
};

DeferredField.prototype.setConversion = function(conversion) {
  this.field.setConversion(conversion);
};

DeferredField.prototype.getConversion = function() {
  return this.field.getConversion();
};

exports.DeferredField = DeferredField;
addField(DeferredField);






function BlankField(type, options) {
  this.document = options.document;
  this.type = type;
  this.element = dom.createElement(this.document, 'div');

  this.fieldChanged = createEvent('BlankField.fieldChanged');
}

BlankField.prototype = Object.create(Field.prototype);

BlankField.claim = function(type) {
  return type instanceof BlankType ? Field.MATCH : Field.NO_MATCH;
};

BlankField.prototype.setConversion = function() { };

BlankField.prototype.getConversion = function() {
  return new Conversion(null);
};

exports.BlankField = BlankField;
addField(BlankField);






function ArrayField(type, options) {
  this.document = options.document;
  this.type = type;
  this.options = options;
  this.requ = options.requisition;

  this._onAdd = this._onAdd.bind(this);
  this.members = [];

  
  this.element = dom.createElement(this.document, 'div');
  this.element.className = 'gcliArrayParent';

  
  this.addButton = dom.createElement(this.document, 'button');
  this.addButton.className = 'gcliArrayMbrAdd';
  this.addButton.addEventListener('click', this._onAdd, false);
  this.addButton.innerHTML = l10n.lookup('fieldArrayAdd');
  this.element.appendChild(this.addButton);

  
  this.container = dom.createElement(this.document, 'div');
  this.container.className = 'gcliArrayMbrs';
  this.element.appendChild(this.container);

  this.onInputChange = this.onInputChange.bind(this);

  this.fieldChanged = createEvent('ArrayField.fieldChanged');
}

ArrayField.prototype = Object.create(Field.prototype);

ArrayField.claim = function(type) {
  return type instanceof ArrayType ? Field.MATCH : Field.NO_MATCH;
};

ArrayField.prototype.destroy = function() {
  Field.prototype.destroy.call(this);
  this.addButton.removeEventListener('click', this._onAdd, false);
};

ArrayField.prototype.setConversion = function(conversion) {
  
  dom.clearElement(this.container);
  this.members = [];

  conversion.conversions.forEach(function(subConversion) {
    this._onAdd(null, subConversion);
  }, this);
};

ArrayField.prototype.getConversion = function() {
  var conversions = [];
  var arrayArg = new ArrayArgument();
  for (var i = 0; i < this.members.length; i++) {
    var conversion = this.members[i].field.getConversion();
    conversions.push(conversion);
    arrayArg.addArgument(conversion.arg);
  }
  return new ArrayConversion(conversions, arrayArg);
};

ArrayField.prototype._onAdd = function(ev, subConversion) {
  
  var element = dom.createElement(this.document, 'div');
  element.className = 'gcliArrayMbr';
  this.container.appendChild(element);

  
  var field = getField(this.type.subtype, this.options);
  field.fieldChanged.add(function() {
    var conversion = this.getConversion();
    this.fieldChanged({ conversion: conversion });
    this.setMessage(conversion.message);
  }, this);

  if (subConversion) {
    field.setConversion(subConversion);
  }
  element.appendChild(field.element);

  
  var delButton = dom.createElement(this.document, 'button');
  delButton.className = 'gcliArrayMbrDel';
  delButton.addEventListener('click', this._onDel, false);
  delButton.innerHTML = l10n.lookup('fieldArrayDel');
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

exports.ArrayField = ArrayField;
addField(ArrayField);


});






define('gcli/ui/menu', ['require', 'exports', 'module' , 'gcli/util', 'gcli/types', 'gcli/argument', 'gcli/canon', 'gcli/ui/domtemplate', 'text!gcli/ui/menu.css', 'text!gcli/ui/menu.html'], function(require, exports, module) {


var dom = require('gcli/util').dom;

var Conversion = require('gcli/types').Conversion;
var Argument = require('gcli/argument').Argument;
var canon = require('gcli/canon');

var Templater = require('gcli/ui/domtemplate').Templater;

var menuCss = require('text!gcli/ui/menu.css');
var menuHtml = require('text!gcli/ui/menu.html');












function Menu(options) {
  options = options || {};
  this.document = options.document || document;

  
  if (!this.document) {
    throw new Error('No document');
  }

  this.element =  dom.createElement(this.document, 'div');
  this.element.classList.add(options.menuClass || 'gcli-menu');
  if (options && options.field) {
    this.element.classList.add(options.menuFieldClass || 'gcli-menu-field');
  }

  
  if (menuCss != null) {
    this.style = dom.importCss(menuCss, this.document);
  }

  var templates = dom.createElement(this.document, 'div');
  dom.setInnerHtml(templates, menuHtml);
  this.optTempl = templates.querySelector('.gcli-menu-template');

  
  this.items = null;
}




Menu.prototype.destroy = function() {
  if (this.style) {
    this.style.parentNode.removeChild(this.style);
    delete this.style;
  }

  delete this.element;
  delete this.items;
  delete this.optTempl;
};







Menu.prototype.onItemClick = function(ev) {
};







Menu.prototype.show = function(items, error) {
  this.error = error;
  this.items = items;

  if (this.error == null && this.items.length === 0) {
    this.element.style.display = 'none';
    return;
  }

  var options = this.optTempl.cloneNode(true);
  new Templater().processNode(options, this);

  dom.clearElement(this.element);
  this.element.appendChild(options);

  this.element.style.display = 'block';
};




Menu.prototype.hide = function() {
  this.element.style.display = 'none';
};




Menu.prototype.setMaxHeight = function(height) {
  this.element.style.maxHeight = height + 'px';
};

exports.Menu = Menu;









function CommandMenu(options) {
  Menu.call(this, options);
  this.requisition = options.requisition;

  this.requisition.commandChange.add(this.onCommandChange, this);
  canon.canonChange.add(this.onCommandChange, this);

  this.onCommandChange();
}

CommandMenu.prototype = Object.create(Menu.prototype);




CommandMenu.prototype.destroy = function() {
  this.requisition.commandChange.remove(this.onCommandChange, this);
  canon.canonChange.remove(this.onCommandChange, this);

  Menu.prototype.destroy.call(this);
};




CommandMenu.prototype.onItemClick = function(ev) {
  var type = this.requisition.commandAssignment.param.type;

  
  
  
  
  
  var text = type.stringify(ev.currentTarget.currentItem);
  var arg = new Argument(text);
  arg.suffix = ' ';

  var conversion = type.parse(arg);
  this.requisition.commandAssignment.setConversion(conversion);
};




CommandMenu.prototype.onCommandChange = function(ev) {
  var command = this.requisition.commandAssignment.getValue();
  if (!command || !command.exec) {
    var error = this.requisition.commandAssignment.getMessage();
    var predictions = this.requisition.commandAssignment.getPredictions();

    if (predictions.length === 0) {
      var commandType = this.requisition.commandAssignment.param.type;
      var conversion = commandType.parse(new Argument());
      predictions = conversion.getPredictions();
    }

    predictions.sort(function(command1, command2) {
      return command1.name.localeCompare(command2.name);
    });
    var items = [];
    predictions.forEach(function(item) {
      if (item.description && !item.hidden) {
        items.push(item);
      }
    }, this);

    this.show(items, error);
  }
  else {
    if (ev && ev.oldValue === ev.newValue) {
      return; 
    }

    this.hide();
  }
};

exports.CommandMenu = CommandMenu;


});






define('gcli/ui/domtemplate', ['require', 'exports', 'module' ], function(require, exports, module) {

  Components.utils.import("resource:///modules/devtools/Templater.jsm");
  exports.Templater = Templater;

});
define("text!gcli/ui/menu.css", [], void 0);
define("text!gcli/ui/menu.html", [], "\n" +
  "<table class=\"gcli-menu-template\" aria-live=\"polite\">\n" +
  "  <tr class=\"gcli-menu-option\" foreach=\"item in ${items}\"\n" +
  "      onclick=\"${onItemClick}\"\n" +
  "      title=\"${__element.currentItem = item; (item.manual || '')}\">\n" +
  "    <td class=\"gcli-menu-name\">${item.name}</td>\n" +
  "    <td class=\"gcli-menu-desc\">${item.description}</td>\n" +
  "  </tr>\n" +
  "  <tr if=\"${error}\">\n" +
  "    <td class=\"gcli-menu-error\" colspan=\"2\">${error}</td>\n" +
  "  </tr>\n" +
  "</table>\n" +
  "");

define("text!gcli/ui/arg_fetch.css", [], void 0);
define("text!gcli/ui/arg_fetch.html", [], "\n" +
  "<!--\n" +
  "Template for an Assignment.\n" +
  "Evaluated each time the commandAssignment changes\n" +
  "-->\n" +
  "<div class=\"gcli-af-template\" aria-live=\"polite\">\n" +
  "  <div>\n" +
  "    <div class=\"gcli-af-cmddesc\">\n" +
  "      ${requisition.commandAssignment.getValue().description}\n" +
  "    </div>\n" +
  "    <table class=\"gcli-af-params\">\n" +
  "      <tbody foreach=\"assignment in ${requisition.getAssignments()}\">\n" +
  "        <!-- Parameter -->\n" +
  "        <tr>\n" +
  "          <td class=\"gcli-af-paramname\">\n" +
  "            <label for=\"gcliForm${assignment.param.name}\">\n" +
  "              ${assignment.param.description ? assignment.param.description + ':' : ''}\n" +
  "            </label>\n" +
  "          </td>\n" +
  "          <td>${getInputFor(assignment)}</td>\n" +
  "          <td>\n" +
  "            <span class=\"gcli-af-required\" if=\"${assignment.param.isDataRequired()}\">*</span>\n" +
  "          </td>\n" +
  "        </tr>\n" +
  "        <tr>\n" +
  "          <td class=\"gcli-af-error\" colspan=\"2\">\n" +
  "            ${linkMessageElement(assignment, __element)}\n" +
  "          </td>\n" +
  "        </tr>\n" +
  "      </tbody>\n" +
  "      <tfoot>\n" +
  "        <tr>\n" +
  "          <td colspan=\"3\" class=\"gcli-af-submit\">\n" +
  "            <input type=\"submit\" value=\"Cancel\" onclick=\"${onFormCancel}\"/>\n" +
  "            <input type=\"submit\" value=\"OK\" onclick=\"${onFormOk}\" save=\"${okElement}\"/>\n" +
  "          </td>\n" +
  "        </tr>\n" +
  "      </tfoot>\n" +
  "    </table>\n" +
  "  </div>\n" +
  "</div>\n" +
  "");







define('gcli/ui/focus', ['require', 'exports', 'module' , 'gcli/util'], function(require, exports, module) {


var util = require('gcli/util');
















function FocusManager(options) {
  options = options || {};

  this._debug = options.debug || false;
  this._blurDelayTimeout = null; 
  this._monitoredElements = [];  

  this.hasFocus = false;
  this.blurDelay = options.blurDelay || 250;
  this.document = options.document || document;

  this.onFocus = util.createEvent('FocusManager.onFocus');
  this.onBlur = util.createEvent('FocusManager.onBlur');

  
  
  this._onDocumentFocus = function() {
    this.reportBlur('document');
  }.bind(this);
  this.document.addEventListener('focus', this._onDocumentFocus, true);
}




FocusManager.prototype.destroy = function() {
  this.document.removeEventListener('focus', this._onDocumentFocus, true);
  delete this.document;

  for (var i = 0; i < this._monitoredElements.length; i++) {
    var monitor = this._monitoredElements[i];
    console.error('Hanging monitored element: ', monitor.element);

    monitor.element.removeEventListener('focus', monitor.onFocus, true);
    monitor.element.removeEventListener('blur', monitor.onBlur, true);
  }

  if (this._blurDelayTimeout) {
    clearTimeout(this._blurDelayTimeout);
    this._blurDelayTimeout = null;
  }
};








FocusManager.prototype.addMonitoredElement = function(element, where) {
  if (this._debug) {
    console.log('FocusManager.addMonitoredElement(' + (where || 'unknown') + ')');
  }

  var monitor = {
    element: element,
    where: where,
    onFocus: function() { this.reportFocus(where); }.bind(this),
    onBlur: function() { this.reportBlur(where); }.bind(this)
  };

  element.addEventListener('focus', monitor.onFocus, true);
  element.addEventListener('blur', monitor.onBlur, true);
  this._monitoredElements.push(monitor);
};





FocusManager.prototype.removeMonitoredElement = function(element) {
  var monitor;
  var matchIndex;

  for (var i = 0; i < this._monitoredElements.length; i++) {
    if (this._monitoredElements[i].element === element) {
      monitor = this._monitoredElements[i];
      matchIndex = i;
    }
  }

  if (!monitor) {
    if (this._debug) {
      console.error('Missing monitor for element. ', element);
    }
    return;
  }

  this._monitoredElements.splice(matchIndex, 1);
  element.removeEventListener('focus', monitor.onFocus, true);
  element.removeEventListener('blur', monitor.onBlur, true);
};






FocusManager.prototype.reportFocus = function(where) {
  if (this._debug) {
    console.log('FocusManager.reportFocus(' + (where || 'unknown') + ')');
  }

  if (this._blurDelayTimeout) {
    if (this._debug) {
      console.log('FocusManager.cancelBlur');
    }
    clearTimeout(this._blurDelayTimeout);
    this._blurDelayTimeout = null;
  }

  if (!this.hasFocus) {
    this.hasFocus = true;
    this.onFocus();
  }
};







FocusManager.prototype.reportBlur = function(where) {
  if (this._debug) {
    console.log('FocusManager.reportBlur(' + where + ')');
  }

  if (this.hasFocus) {
    if (this._blurDelayTimeout) {
      if (this._debug) {
        console.log('FocusManager.blurPending');
      }
      return;
    }

    this._blurDelayTimeout = setTimeout(function() {
      if (this._debug) {
        console.log('FocusManager.blur');
      }
      this.hasFocus = false;
      this.onBlur();
      this._blurDelayTimeout = null;
    }.bind(this), this.blurDelay);
  }
};

exports.FocusManager = FocusManager;


});





var gcli = require("gcli/index");
