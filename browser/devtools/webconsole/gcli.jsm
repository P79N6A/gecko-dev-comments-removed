






















































var EXPORTED_SYMBOLS = [ "gcli" ];






var Node = Components.interfaces.nsIDOMNode;
var HTMLElement = Components.interfaces.nsIDOMHTMLElement;


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
    if (aObj.constructor && aObj.constructor.name) {
      return aObj.constructor.name;
    }
    
    
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
      if (aThing instanceof Node && aThing.tagName) {
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

    var str = aThing.toString().replace(/\n/g, "|");
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
      else if (aThing instanceof Node && aThing.tagName) {
        reply += "  " + debugElement(aThing) + "\n";
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

define('gcli/index', ['require', 'exports', 'module' , 'gcli/canon', 'gcli/types/basic', 'gcli/types/command', 'gcli/types/javascript', 'gcli/types/node', 'gcli/types/resource', 'gcli/types/setting', 'gcli/types/selection', 'gcli/settings', 'gcli/ui/intro', 'gcli/ui/focus', 'gcli/ui/fields/basic', 'gcli/ui/fields/javascript', 'gcli/ui/fields/selection', 'gcli/commands/help', 'gcli/ui/ffdisplay'], function(require, exports, module) {

  
  exports.addCommand = require('gcli/canon').addCommand;
  exports.removeCommand = require('gcli/canon').removeCommand;
  exports.lookup = mozl10n.lookup;
  exports.lookupFormat = mozl10n.lookupFormat;

  
  require('gcli/types/basic').startup();
  require('gcli/types/command').startup();
  require('gcli/types/javascript').startup();
  require('gcli/types/node').startup();
  require('gcli/types/resource').startup();
  require('gcli/types/setting').startup();
  require('gcli/types/selection').startup();

  require('gcli/settings').startup();
  require('gcli/ui/intro').startup();
  require('gcli/ui/focus').startup();
  require('gcli/ui/fields/basic').startup();
  require('gcli/ui/fields/javascript').startup();
  require('gcli/ui/fields/selection').startup();

  require('gcli/commands/help').startup();

  
  
  

  var FFDisplay = require('gcli/ui/ffdisplay').FFDisplay;

  



  exports._internal = {
    require: require,
    define: define,
    console: console,

    












    createDisplay: function(opts) {
      return new FFDisplay(opts);
    }
  };
});






define('gcli/canon', ['require', 'exports', 'module' , 'gcli/util', 'gcli/l10n', 'gcli/types', 'gcli/types/basic'], function(require, exports, module) {
var canon = exports;


var util = require('gcli/util');
var l10n = require('gcli/l10n');

var types = require('gcli/types');
var Status = require('gcli/types').Status;
var BooleanType = require('gcli/types/basic').BooleanType;








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

  if (this.returnType == null) {
    this.returnType = 'string';
  }

  
  
  
  
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
  this.paramSpec = paramSpec;
  this.name = this.paramSpec.name;
  this.type = this.paramSpec.type;
  this.groupName = groupName;
  this.defaultValue = this.paramSpec.defaultValue;

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
    if (this.defaultValue !== undefined) {
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

  
  
  if (this.defaultValue === undefined) {
    this.defaultValue = this.type.getBlank().value;
  }
}






Parameter.prototype.isKnownAs = function(name) {
  if (name === '--' + this.name) {
    return true;
  }
  return false;
};






Parameter.prototype.getBlank = function() {
  var conversion;

  if (this.type.getBlank) {
    return this.type.getBlank();
  }

  return this.type.parseString('');
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





Object.defineProperty(Parameter.prototype, 'isPositionalAllowed', {
  get: function() {
    return this.groupName == null;
  },
  enumerable: true
});

canon.Parameter = Parameter;





var commands = {};




var commandNames = [];




var commandSpecs = {};








canon.addCommand = function addCommand(commandSpec) {
  if (commands[commandSpec.name] != null) {
    
    delete commands[commandSpec.name];
    commandNames = commandNames.filter(function(test) {
      return test !== commandSpec.name;
    });
  }

  var command = new Command(commandSpec);
  commands[commandSpec.name] = command;
  commandNames.push(commandSpec.name);
  commandNames.sort();

  commandSpecs[commandSpec.name] = commandSpec;

  canon.onCanonChange();
  return command;
};





canon.removeCommand = function removeCommand(commandOrName) {
  var name = typeof commandOrName === 'string' ?
          commandOrName :
          commandOrName.name;

  
  delete commands[name];
  delete commandSpecs[name];
  commandNames = commandNames.filter(function(test) {
    return test !== name;
  });

  canon.onCanonChange();
};





canon.getCommand = function getCommand(name) {
  
  return commands[name] || undefined;
};




canon.getCommands = function getCommands() {
  
  return Object.keys(commands).map(function(name) {
    return commands[name];
  }, this);
};




canon.getCommandNames = function getCommandNames() {
  return commandNames.slice(0);
};





canon.getCommandSpecs = function getCommandSpecs() {
  return commandSpecs;
};




canon.onCanonChange = util.createEvent('canon.onCanonChange');











function CommandOutputManager() {
  this.onOutput = util.createEvent('CommandOutputManager.onOutput');
}

canon.CommandOutputManager = CommandOutputManager;





canon.commandOutputManager = new CommandOutputManager();


});






define('gcli/util', ['require', 'exports', 'module' ], function(require, exports, module) {








var eventDebug = false;




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
  var holdFire = false;
  var heldEvents = [];
  var eventCombiner = undefined;

  



  var event = function(ev) {
    if (holdFire) {
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
    handlers.push({ func: func, scope: scope });
  };

  





  event.remove = function(func, scope) {
    var found = false;
    handlers = handlers.filter(function(test) {
      var noMatch = (test.func !== func && test.scope !== scope);
      if (!noMatch) {
        found = true;
      }
      return noMatch;
    });
    if (!found) {
      console.warn('Failed to remove handler from ' + name);
    }
  };

  



  event.removeAll = function() {
    handlers = [];
  };

  



  event.holdFire = function() {
    if (eventDebug) {
      console.group('Holding fire: ' + name);
    }

    holdFire = true;
  };

  






  event.resumeFire = function() {
    if (eventDebug) {
      console.groupEnd('Resume fire: ' + name);
    }

    if (holdFire !== true) {
      throw new Error('Event not held: ' + name);
    }

    holdFire = false;
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
  var hash = 0;
  if (str.length == 0) {
    return hash;
  }
  for (var i = 0; i < str.length; i++) {
    var char = str.charCodeAt(i);
    hash = ((hash << 5) - hash) + char;
    hash = hash & hash; 
  }
  return hash;
}





exports.setContents = function(elem, contents) {
  if (typeof HTMLElement !== 'undefined' && contents instanceof HTMLElement) {
    exports.clearElement(elem);
    elem.appendChild(contents);
    return;
  }

  if (exports.isXmlDocument(elem.ownerDocument)) {
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
  else {
    elem.innerHTML = contents;
  }
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
















if ('KeyEvent' in this) {
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


exports.propertyLookup = Proxy.create({
  get: function(rcvr, name) {
    return exports.lookup(name);
  }
});


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
  return !this.arg.isBlank();
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
  return this.predictions || [];
};









Conversion.prototype.getPredictionAt = function(index) {
  if (index == null) {
    return undefined;
  }

  var predictions = this.getPredictions();
  if (predictions.length === 0) {
    return undefined;
  }

  index = index % predictions.length;
  if (index < 0) {
    index = predictions.length + index;
  }
  return predictions[index];
};









Conversion.prototype.constrainPredictionIndex = function(index) {
  if (index == null) {
    return undefined;
  }

  var predictions = this.getPredictions();
  if (predictions.length === 0) {
    return undefined;
  }

  index = index % predictions.length;
  if (index < 0) {
    index = predictions.length + index;
  }
  return index;
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







Type.prototype.getBlank = function() {
  return this.parse(new Argument());
};






Type.prototype.getType = function() {
  return this;
};

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
      type = new type({});
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
  this.text = text !== undefined ? text : '';
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

  if (options && options.normalize) {
    prefix = '{ ';
    suffix = ' }';
  }

  return new ScriptArgument(replText, prefix, suffix);
};






ScriptArgument.prototype.isBlank = function() {
  return false;
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






define('gcli/types/basic', ['require', 'exports', 'module' , 'gcli/l10n', 'gcli/types', 'gcli/types/spell', 'gcli/types/selection', 'gcli/argument'], function(require, exports, module) {


var l10n = require('gcli/l10n');
var types = require('gcli/types');
var Type = require('gcli/types').Type;
var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;
var ArrayConversion = require('gcli/types').ArrayConversion;
var Speller = require('gcli/types/spell').Speller;
var SelectionType = require('gcli/types/selection').SelectionType;

var Argument = require('gcli/argument').Argument;
var TrueNamedArgument = require('gcli/argument').TrueNamedArgument;
var FalseNamedArgument = require('gcli/argument').FalseNamedArgument;
var ArrayArgument = require('gcli/argument').ArrayArgument;





exports.startup = function() {
  types.registerType(StringType);
  types.registerType(NumberType);
  types.registerType(BooleanType);
  types.registerType(BlankType);
  types.registerType(DeferredType);
  types.registerType(ArrayType);
};

exports.shutdown = function() {
  types.unregisterType(StringType);
  types.unregisterType(NumberType);
  types.unregisterType(BooleanType);
  types.unregisterType(BlankType);
  types.unregisterType(DeferredType);
  types.unregisterType(ArrayType);
};





function StringType(typeSpec) {
  if (Object.keys(typeSpec).length > 0) {
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
    return new Conversion(undefined, arg, Status.INCOMPLETE, '');
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
  return undefined;
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
    return new Conversion(undefined, arg, Status.INCOMPLETE, '');
  }

  var value = parseInt(arg.text, 10);
  if (isNaN(value)) {
    return new Conversion(undefined, arg, Status.ERROR,
        l10n.lookupFormat('typesNumberNan', [ arg.text ]));
  }

  var max = this.getMax();
  if (max != null && value > max) {
    return new Conversion(undefined, arg, Status.ERROR,
        l10n.lookupFormat('typesNumberMax', [ value, max ]));
  }

  var min = this.getMin();
  if (min != null && value < min) {
    return new Conversion(undefined, arg, Status.ERROR,
        l10n.lookupFormat('typesNumberMin', [ value, min ]));
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
    var min = this.getMin();
    return min != null ? min : 0;
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
  if (min != null && value < min) {
    return min;
  }
  var max = this.getMax();
  if (max != null && value > max) {
    return max;
  }
  return value;
};

NumberType.prototype.name = 'number';

exports.NumberType = NumberType;





function BooleanType(typeSpec) {
  if (Object.keys(typeSpec).length > 0) {
    throw new Error('BooleanType can not be customized');
  }
}

BooleanType.prototype = Object.create(SelectionType.prototype);

BooleanType.prototype.lookup = [
  { name: 'false', value: false },
  { name: 'true', value: true }
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

BooleanType.prototype.getBlank = function() {
  return new Conversion(false, new Argument(), Status.VALID, '', this.lookup);
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

DeferredType.prototype.getType = function() {
  return this.defer();
};

Object.defineProperty(DeferredType.prototype, 'isImportant', {
  get: function() {
    return this.defer().isImportant;
  },
  enumerable: true
});

DeferredType.prototype.name = 'deferred';

exports.DeferredType = DeferredType;






function BlankType(typeSpec) {
  if (Object.keys(typeSpec).length > 0) {
    throw new Error('BlankType can not be customized');
  }
}

BlankType.prototype = Object.create(Type.prototype);

BlankType.prototype.stringify = function(value) {
  return '';
};

BlankType.prototype.parse = function(arg) {
  return new Conversion(undefined, arg);
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

ArrayType.prototype.getBlank = function(values) {
  return new ArrayConversion([], new ArrayArgument());
};

ArrayType.prototype.name = 'array';

exports.ArrayType = ArrayType;


});















define('gcli/types/spell', ['require', 'exports', 'module' ], function(require, exports, module) {













function Speller() {
  
  this._nWords = {};
}

Speller.letters = "abcdefghijklmnopqrstuvwxyz".split("");






Speller.prototype.train = function(words) {
  words.forEach(function(word) {
    word = word.toLowerCase();
    this._nWords[word] = this._nWords.hasOwnProperty(word) ?
            this._nWords[word] + 1 :
            1;
  }, this);
};




Speller.prototype.correct = function(word) {
  if (this._nWords.hasOwnProperty(word)) {
    return word;
  }

  var candidates = {};
  var list = this._edits(word);
  list.forEach(function(edit) {
    if (this._nWords.hasOwnProperty(edit)) {
      candidates[this._nWords[edit]] = edit;
    }
  }, this);

  if (this._countKeys(candidates) > 0) {
    return candidates[this._max(candidates)];
  }

  list.forEach(function(edit) {
    this._edits(edit).forEach(function(w) {
      if (this._nWords.hasOwnProperty(w)) {
        candidates[this._nWords[w]] = w;
      }
    }, this);
  }, this);

  return this._countKeys(candidates) > 0 ?
      candidates[this._max(candidates)] :
      null;
};




Speller.prototype._countKeys = function(object) {
  
  var count = 0;
  for (var attr in object) {
    if (object.hasOwnProperty(attr)) {
      count++;
    }
  }
  return count;
};






Speller.prototype._max = function(candidates) {
  var arr = [];
  for (var candidate in candidates) {
    if (candidates.hasOwnProperty(candidate)) {
      arr.push(candidate);
    }
  }
  return Math.max.apply(null, arr);
};





Speller.prototype._edits = function(word) {
  var results = [];

  
  for (var i = 0; i < word.length; i++) {
    results.push(word.slice(0, i) + word.slice(i + 1));
  }

  
  for (i = 0; i < word.length - 1; i++) {
    results.push(word.slice(0, i) + word.slice(i + 1, i + 2)
            + word.slice(i, i + 1) + word.slice(i + 2));
  }

  
  for (i = 0; i < word.length; i++) {
    Speller.letters.forEach(function(l) {
      results.push(word.slice(0, i) + l + word.slice(i + 1));
    }, this);
  }

  
  for (i = 0; i <= word.length; i++) {
    Speller.letters.forEach(function(l) {
      results.push(word.slice(0, i) + l + word.slice(i));
    }, this);
  }

  return results;
};

exports.Speller = Speller;


});






define('gcli/types/selection', ['require', 'exports', 'module' , 'gcli/l10n', 'gcli/types', 'gcli/types/spell', 'gcli/argument'], function(require, exports, module) {


var l10n = require('gcli/l10n');
var types = require('gcli/types');
var Type = require('gcli/types').Type;
var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;
var Speller = require('gcli/types/spell').Speller;
var Argument = require('gcli/argument').Argument;





exports.startup = function() {
  types.registerType(SelectionType);
};

exports.shutdown = function() {
  types.unregisterType(SelectionType);
};



















function SelectionType(typeSpec) {
  if (typeSpec) {
    Object.keys(typeSpec).forEach(function(key) {
      this[key] = typeSpec[key];
    }, this);
  }
}

SelectionType.prototype = Object.create(Type.prototype);

SelectionType.prototype.maxPredictions = 10;

SelectionType.prototype.stringify = function(value) {
  if (this.stringifyProperty != null) {
    return value[this.stringifyProperty];
  }
  var name = null;
  var lookup = this.getLookup();
  lookup.some(function(item) {
    if (item.value === value) {
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
  }, this);
};






SelectionType.prototype._findPredictions = function(arg) {
  var predictions = [];
  var lookup = this.getLookup();
  var i, option;

  
  
  if (arg.suffix.length > 0) {
    for (i = 0; i < lookup.length && predictions.length < this.maxPredictions; i++) {
      option = lookup[i];
      if (option.name === arg.text) {
        this._addToPredictions(predictions, option, arg);
      }
    }

    return predictions;
  }

  
  for (i = 0; i < lookup.length && predictions.length < this.maxPredictions; i++) {
    option = lookup[i];
    if (option.name.indexOf(arg.text) === 0) {
      this._addToPredictions(predictions, option, arg);
    }
  }

  
  if (predictions.length < (this.maxPredictions / 2)) {
    for (i = 0; i < lookup.length && predictions.length < this.maxPredictions; i++) {
      option = lookup[i];
      if (option.name.indexOf(arg.text) !== -1) {
        if (predictions.indexOf(option) === -1) {
          this._addToPredictions(predictions, option, arg);
        }
      }
    }
  }

  
  if (false && predictions.length === 0) {
    var speller = new Speller();
    var names = lookup.map(function(opt) {
      return opt.name;
    });
    speller.train(names);
    var corrected = speller.correct(arg.text);
    if (corrected) {
      lookup.forEach(function(opt) {
        if (opt.name === corrected) {
          predictions.push(opt);
        }
      }, this);
    }
  }

  return predictions;
};







SelectionType.prototype._addToPredictions = function(predictions, option, arg) {
  predictions.push(option);
};

SelectionType.prototype.parse = function(arg) {
  var predictions = this._findPredictions(arg);

  if (predictions.length === 0) {
    var msg = l10n.lookupFormat('typesSelectionNomatch', [ arg.text ]);
    return new Conversion(undefined, arg, Status.ERROR, msg, predictions);
  }

  
  
  if (this.noMatch) {
    this.noMatch();
  }

  var value = predictions[0].value;

  if (predictions[0].name === arg.text) {
    return new Conversion(value, arg, Status.VALID, '', predictions);
  }

  return new Conversion(undefined, arg, Status.INCOMPLETE, '', predictions);
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


});






define('gcli/types/command', ['require', 'exports', 'module' , 'gcli/canon', 'gcli/l10n', 'gcli/types', 'gcli/types/selection'], function(require, exports, module) {


var canon = require('gcli/canon');
var l10n = require('gcli/l10n');
var types = require('gcli/types');
var SelectionType = require('gcli/types/selection').SelectionType;
var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;





exports.startup = function() {
  types.registerType(CommandType);
};

exports.shutdown = function() {
  types.unregisterType(CommandType);
};









function CommandType() {
  this.stringifyProperty = 'name';
}

CommandType.prototype = Object.create(SelectionType.prototype);

CommandType.prototype.name = 'command';

CommandType.prototype.lookup = function() {
  var commands = canon.getCommands();
  commands.sort(function(c1, c2) {
    return c1.name.localeCompare(c2.name);
  });
  return commands.map(function(command) {
    return { name: command.name, value: command };
  }, this);
};




CommandType.prototype._addToPredictions = function(predictions, option, arg) {
  if (option.value.hidden) {
    return;
  }
  
  
  
  
  if (arg.text.length !== 0 || option.name.indexOf(' ') === -1) {
    predictions.push(option);
  }
};

CommandType.prototype.parse = function(arg) {
  
  
  var predictFunc = function() {
    return this._findPredictions(arg);
  }.bind(this);

  var predictions = this._findPredictions(arg);

  if (predictions.length === 0) {
    var msg = l10n.lookupFormat('typesSelectionNomatch', [ arg.text ]);
    return new Conversion(undefined, arg, Status.ERROR, msg, predictFunc);
  }

  var command = predictions[0].value;

  if (predictions.length === 1) {
    
    
    if (command.name === arg.text && typeof command.exec === 'function') {
      return new Conversion(command, arg, Status.VALID, '');
    }
    return new Conversion(undefined, arg, Status.INCOMPLETE, '', predictFunc);
  }

  
  if (predictions[0].name === arg.text) {
    return new Conversion(command, arg, Status.VALID, '', predictFunc);
  }

  return new Conversion(undefined, arg, Status.INCOMPLETE, '', predictFunc);
};


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
  if (Object.keys(typeSpec).length > 0) {
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

  
  if (typed === '') {
    return new Conversion(undefined, arg, Status.INCOMPLETE);
  }
  
  if (!isNaN(parseFloat(typed)) && isFinite(typed)) {
    return new Conversion(typed, arg);
  }
  
  if (typed.trim().match(/(null|undefined|NaN|Infinity|true|false)/)) {
    return new Conversion(typed, arg);
  }

  
  
  var beginning = this._findCompletionBeginning(typed);

  
  if (beginning.err) {
    return new Conversion(typed, arg, Status.ERROR, beginning.err);
  }

  
  
  if (beginning.state === ParseState.COMPLEX) {
    return new Conversion(typed, arg);
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
        
        
        
        return new Conversion(typed, arg, Status.VALID, '');
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
            value = value.substring(0, 37) + '…';
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





exports.getDocument = function() {
  return doc;
};





function NodeType(typeSpec) {
  if (Object.keys(typeSpec).length > 0) {
    throw new Error('NodeType can not be customized');
  }
}

NodeType.prototype = Object.create(Type.prototype);

NodeType.prototype.stringify = function(value) {
  return value.__gcliQuery || 'Error';
};

NodeType.prototype.parse = function(arg) {
  if (arg.text === '') {
    return new Conversion(undefined, arg, Status.INCOMPLETE);
  }

  var nodes;
  try {
    nodes = doc.querySelectorAll(arg.text);
  }
  catch (ex) {
    return new Conversion(undefined, arg, Status.ERROR,
            l10n.lookup('nodeParseSyntax'));
  }

  if (nodes.length === 0) {
    return new Conversion(undefined, arg, Status.INCOMPLETE,
        l10n.lookup('nodeParseNone'));
  }

  if (nodes.length === 1) {
    var node = nodes.item(0);
    node.__gcliQuery = arg.text;

    host.flashNodes(node, true);

    return new Conversion(node, arg, Status.VALID, '');
  }

  host.flashNodes(nodes, false);

  return new Conversion(undefined, arg, Status.ERROR,
          l10n.lookupFormat('nodeParseMultiple', [ nodes.length ]));
};

NodeType.prototype.name = 'node';


});






define('gcli/host', ['require', 'exports', 'module' ], function(require, exports, module) {

  XPCOMUtils.defineLazyGetter(this, "Highlighter", function() {
    var tmp = {};
    Components.utils.import("resource:///modules/highlighter.jsm", tmp);
    return tmp.Highlighter;
  });

  



  exports.chromeWindow = undefined;

  



  exports.flashNodes = function(nodes, match) {
    
    





  };


});






define('gcli/types/resource', ['require', 'exports', 'module' , 'gcli/types', 'gcli/types/selection'], function(require, exports, module) {


var types = require('gcli/types');
var SelectionType = require('gcli/types/selection').SelectionType;





exports.startup = function() {
  types.registerType(ResourceType);
};

exports.shutdown = function() {
  types.unregisterType(ResourceType);
  exports.clearResourceCache();
};

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




function ResourceType(typeSpec) {
  this.include = typeSpec.include;
  if (this.include !== Resource.TYPE_SCRIPT &&
      this.include !== Resource.TYPE_CSS &&
      this.include != null) {
    throw new Error('invalid include property: ' + this.include);
  }
}

ResourceType.prototype = Object.create(SelectionType.prototype);






ResourceType.prototype.getLookup = function() {
  var resources = [];
  if (this.include !== Resource.TYPE_SCRIPT) {
    Array.prototype.push.apply(resources, CssResource._getAllStyles());
  }
  if (this.include !== Resource.TYPE_CSS) {
    Array.prototype.push.apply(resources, ScriptResource._getAllScripts());
  }

  return resources.map(function(resource) {
    return { name: resource.name, value: resource };
  });
};

ResourceType.prototype.name = 'resource';








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
    ResourceCache._cached = {};
  }
};


});






define('gcli/types/setting', ['require', 'exports', 'module' , 'gcli/settings', 'gcli/types', 'gcli/types/selection', 'gcli/types/basic'], function(require, exports, module) {


var settings = require('gcli/settings');
var types = require('gcli/types');
var SelectionType = require('gcli/types/selection').SelectionType;
var DeferredType = require('gcli/types/basic').DeferredType;





exports.startup = function() {
  types.registerType(SettingType);
  types.registerType(SettingValueType);
};

exports.shutdown = function() {
  types.unregisterType(SettingType);
  types.unregisterType(SettingValueType);
};











var lastSetting = null;




function SettingType(typeSpec) {
  if (Object.keys(typeSpec).length > 0) {
    throw new Error('SettingType can not be customized');
  }
}

SettingType.prototype = Object.create(SelectionType.prototype);

SettingType.prototype.lookup = function() {
  return settings.getAll().map(function(setting) {
    return { name: setting.name, value: setting };
  });
};

SettingType.prototype.noMatch = function() {
  lastSetting = null;
};

SettingType.prototype.stringify = function(option) {
  lastSetting = option;
  return SelectionType.prototype.stringify.call(this, option);
};

SettingType.prototype.parse = function(arg) {
  var conversion = SelectionType.prototype.parse.call(this, arg);
  lastSetting = conversion.value;
  return conversion;
};

SettingType.prototype.name = 'setting';





function SettingValueType(typeSpec) {
  if (Object.keys(typeSpec).length > 0) {
    throw new Error('SettingType can not be customized');
  }
}

SettingValueType.prototype = Object.create(DeferredType.prototype);

SettingValueType.prototype.defer = function() {
  if (lastSetting != null) {
    return lastSetting.type;
  }
  else {
    return types.getType('blank');
  }
};

SettingValueType.prototype.name = 'settingValue';


});






define('gcli/settings', ['require', 'exports', 'module' , 'gcli/util', 'gcli/types'], function(require, exports, module) {

var imports = {};

Components.utils.import('resource://gre/modules/XPCOMUtils.jsm', imports);

imports.XPCOMUtils.defineLazyGetter(imports, 'prefBranch', function() {
  var prefService = Components.classes['@mozilla.org/preferences-service;1']
          .getService(Components.interfaces.nsIPrefService);
  return prefService.getBranch(null)
          .QueryInterface(Components.interfaces.nsIPrefBranch2);
});

imports.XPCOMUtils.defineLazyGetter(imports, 'supportsString', function() {
  return Components.classes["@mozilla.org/supports-string;1"]
          .createInstance(Components.interfaces.nsISupportsString);
});


var util = require('gcli/util');
var types = require('gcli/types');

var allSettings = [];





exports.startup = function() {
  imports.prefBranch.getChildList('').forEach(function(name) {
    allSettings.push(new Setting(name));
  }.bind(this));
  allSettings.sort(function(s1, s2) {
    return s1.name.localeCompare(s2.name);
  }.bind(this));
};

exports.shutdown = function() {
  allSettings = [];
};




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
        return types.getType('boolean');

      case imports.prefBranch.PREF_INT:
        return types.getType('number');

      case imports.prefBranch.PREF_STRING:
        return types.getType('string');

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
        
        var isL10n = /^chrome:\/\/.+\/locale\/.+\.properties/.test(value);
        if (!this.changed && isL10n) {
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





exports.getAll = function(filter) {
  if (filter == null) {
    return allSettings;
  }
  return allSettings.filter(function(setting) {
    return setting.name.indexOf(filter) !== -1;
  });
};




exports.addSetting = function(prefSpec) {
  var setting = new Setting(prefSpec);
  for (var i = 0; i < allSettings.length; i++) {
    if (allSettings[i].name === setting.name) {
      allSettings[i] = setting;
    }
  }
  return setting;
};




exports.removeSetting = function(nameOrSpec) {
};


});






define('gcli/ui/intro', ['require', 'exports', 'module' , 'gcli/settings', 'gcli/l10n', 'gcli/ui/view', 'gcli/cli', 'text!gcli/ui/intro.html'], function(require, exports, module) {

  var settings = require('gcli/settings');
  var l10n = require('gcli/l10n');
  var view = require('gcli/ui/view');
  var Output = require('gcli/cli').Output;

  


  var hideIntroSettingSpec = {
    name: 'hideIntro',
    type: 'boolean',
    description: l10n.lookup('hideIntroDesc'),
    defaultValue: false
  };
  var hideIntro;

  


  exports.startup = function() {
    hideIntro = settings.addSetting(hideIntroSettingSpec);
  };

  exports.shutdown = function() {
    settings.removeSetting(hideIntroSettingSpec);
    hideIntro = undefined;
  };

  


  exports.maybeShowIntro = function(commandOutputManager) {
    if (hideIntro.value) {
      return;
    }

    var output = new Output();
    commandOutputManager.onOutput({ output: output });

    var viewData = view.createView({
      html: require('text!gcli/ui/intro.html'),
      data: {
        showHideButton: true,
        onGotIt: function(ev) {
          hideIntro.value = true;
          output.onClose();
        }
      }
    });

    output.complete(viewData);
  };
});






define('gcli/ui/view', ['require', 'exports', 'module' , 'gcli/util', 'gcli/ui/domtemplate'], function(require, exports, module) {


var util = require('gcli/util');
var domtemplate = require('gcli/ui/domtemplate');























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






define('gcli/ui/domtemplate', ['require', 'exports', 'module' ], function(require, exports, module) {

  var obj = {};
  Components.utils.import('resource:///modules/devtools/Templater.jsm', obj);
  exports.template = obj.template;

});






define('gcli/cli', ['require', 'exports', 'module' , 'gcli/util', 'gcli/ui/view', 'gcli/canon', 'gcli/promise', 'gcli/types', 'gcli/types/basic', 'gcli/argument'], function(require, exports, module) {


var util = require('gcli/util');
var view = require('gcli/ui/view');

var canon = require('gcli/canon');
var Promise = require('gcli/promise').Promise;

var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;
var ArrayType = require('gcli/types/basic').ArrayType;
var StringType = require('gcli/types/basic').StringType;
var BooleanType = require('gcli/types/basic').BooleanType;

var Argument = require('gcli/argument').Argument;
var ArrayArgument = require('gcli/argument').ArrayArgument;
var NamedArgument = require('gcli/argument').NamedArgument;
var TrueNamedArgument = require('gcli/argument').TrueNamedArgument;
var MergedArgument = require('gcli/argument').MergedArgument;
var ScriptArgument = require('gcli/argument').ScriptArgument;

var evalCommand;




exports.startup = function() {
  evalCommand = canon.addCommand(evalCommandSpec);
};

exports.shutdown = function() {
  canon.removeCommand(evalCommandSpec.name);
  evalCommand = undefined;
};






















function Assignment(param, paramIndex) {
  this.param = param;
  this.paramIndex = paramIndex;
  this.onAssignmentChange = util.createEvent('Assignment.onAssignmentChange');

  this.setBlank();
}





Assignment.prototype.param = undefined;

Assignment.prototype.conversion = undefined;






Assignment.prototype.paramIndex = undefined;






Object.defineProperty(Assignment.prototype, 'arg', {
  get: function() {
    return this.conversion.arg;
  },
  enumerable: true
});






Object.defineProperty(Assignment.prototype, 'value', {
  get: function() {
    return this.conversion.value;
  },
  enumerable: true
});




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

  this.onAssignmentChange({
    assignment: this,
    conversion: this.conversion,
    oldConversion: oldConversion
  });
};




Assignment.prototype.setBlank = function() {
  this.setConversion(this.param.type.getBlank());
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
  if (this.param.isDataRequired && !this.conversion.isDataProvided()) {
    return Status.ERROR;
  }

  
  
  
  if (!this.param.isDataRequired && this.arg.isBlank()) {
    return Status.VALID;
  }

  return this.conversion.getStatus(arg);
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
  hidden: true,
  returnType: 'object',
  description: { key: 'cliEvalJavascript' },
  exec: function(args, context) {
    return customEval(args.javascript);
  },
  evalRegexp: /^\s*{\s*/
};





function CommandAssignment() {
  var commandParamMetadata = { name: '__command', type: 'command' };
  
  
  
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
  this.onAssignmentChange = util.createEvent('CommandAssignment.onAssignmentChange');

  this.setBlank();
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





function UnassignedAssignment() {
  this.param = new canon.Parameter({
    name: '__unassigned',
    type: 'string'
  });
  this.paramIndex = -1;
  this.onAssignmentChange = util.createEvent('UnassignedAssignment.onAssignmentChange');

  this.setBlank();
}

UnassignedAssignment.prototype = Object.create(Assignment.prototype);

UnassignedAssignment.prototype.getStatus = function(arg) {
  return Status.ERROR;
};

UnassignedAssignment.prototype.setUnassigned = function(args) {
  if (!args || args.length === 0) {
    this.setBlank();
  }
  else {
    var conversion = this.param.type.parse(new MergedArgument(args));
    this.setConversion(conversion);
  }
};





























function Requisition(environment, doc) {
  this.environment = environment;
  this.document = doc;
  if (this.document == null) {
    try {
      this.document = document;
    }
    catch (ex) {
      
    }
  }

  
  
  this.commandAssignment = new CommandAssignment();

  
  
  
  
  
  
  this._assignments = {};

  
  this.assignmentCount = 0;

  
  this._args = [];

  
  this._unassigned = new UnassignedAssignment();

  
  
  this._structuralChangeInProgress = false;

  this.commandAssignment.onAssignmentChange.add(this._commandAssignmentChanged, this);
  this.commandAssignment.onAssignmentChange.add(this._assignmentChanged, this);

  this.commandOutputManager = canon.commandOutputManager;

  this.onAssignmentChange = util.createEvent('Requisition.onAssignmentChange');
  this.onTextChange = util.createEvent('Requisition.onTextChange');
}





var MORE_THAN_THE_MOST_ARGS_POSSIBLE = 1000000;




Requisition.prototype.destroy = function() {
  this.commandAssignment.onAssignmentChange.remove(this._commandAssignmentChanged, this);
  this.commandAssignment.onAssignmentChange.remove(this._assignmentChanged, this);

  delete this.document;
  delete this.environment;
};





Requisition.prototype._assignmentChanged = function(ev) {
  
  if (ev.oldConversion != null &&
      ev.conversion.valueEquals(ev.oldConversion)) {
    return;
  }

  if (this._structuralChangeInProgress) {
    return;
  }

  this.onAssignmentChange(ev);

  
  
  if (ev.conversion.argEquals(ev.oldConversion)) {
    return;
  }

  this._structuralChangeInProgress = true;

  
  
  
  var i;
  if (ev.assignment.param.isPositionalAllowed) {
    for (i = 0; i < ev.assignment.paramIndex; i++) {
      var assignment = this.getAssignment(i);
      if (assignment.param.isPositionalAllowed) {
        if (assignment.ensureVisibleArgument()) {
          this._args.push(assignment.arg);
        }
      }
    }
  }

  
  var index = MORE_THAN_THE_MOST_ARGS_POSSIBLE;
  for (i = 0; i < this._args.length; i++) {
    if (this._args[i].assignment === ev.assignment) {
      if (i < index) {
        index = i;
      }
      this._args.splice(i, 1);
      i--;
    }
  }

  if (index === MORE_THAN_THE_MOST_ARGS_POSSIBLE) {
    this._args.push(ev.assignment.arg);
  }
  else {
    
    var newArgs = ev.conversion.arg.getArgs();
    for (i = 0; i < newArgs.length; i++) {
      this._args.splice(index + i, 0, newArgs[i]);
    }
  }
  this._structuralChangeInProgress = false;

  this.onTextChange();
};




Requisition.prototype._commandAssignmentChanged = function(ev) {
  
  
  
  if (ev.conversion.valueEquals(ev.oldConversion)) {
    return;
  }

  this._assignments = {};

  var command = this.commandAssignment.value;
  if (command) {
    for (var i = 0; i < command.params.length; i++) {
      var param = command.params[i];
      var assignment = new Assignment(param, i);
      assignment.onAssignmentChange.add(this._assignmentChanged, this);
      this._assignments[param.name] = assignment;
    }
  }
  this.assignmentCount = Object.keys(this._assignments).length;
};






Requisition.prototype.getAssignment = function(nameOrNumber) {
  var name = (typeof nameOrNumber === 'string') ?
    nameOrNumber :
    Object.keys(this._assignments)[nameOrNumber];
  return this._assignments[name] || undefined;
};




Requisition.prototype.getParameterNames = function() {
  return Object.keys(this._assignments);
};






Requisition.prototype.cloneAssignments = function() {
  return Object.keys(this._assignments).map(function(name) {
    return this._assignments[name];
  }, this);
};








Requisition.prototype.getStatus = function() {
  var status = Status.VALID;
  if (!this._unassigned.arg.isBlank()) {
    return Status.ERROR;
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




Requisition.prototype.setBlankArguments = function() {
  this.getAssignments().forEach(function(assignment) {
    assignment.setBlank();
  }, this);
};














Requisition.prototype.complete = function(cursor, predictionChoice) {
  var assignment = this.getAssignmentAt(cursor.start);

  var predictions = assignment.conversion.getPredictions();
  if (predictions.length > 0) {
    this.onTextChange.holdFire();

    var prediction = assignment.conversion.getPredictionAt(predictionChoice);

    
    var arg = assignment.arg.beget(prediction.name);
    var conversion = assignment.param.type.parse(arg);
    assignment.setConversion(conversion);

    
    
    
    if (this._args.indexOf(arg) === -1) {
      this._args.push(arg);
    }

    if (prediction.incomplete) {
      
      
      return;
    }

    
    
    
    
    
    
    
    
    

    var nextIndex = assignment.paramIndex + 1;
    var nextAssignment = this.getAssignment(nextIndex);
    if (nextAssignment) {
      
      var nextArg = nextAssignment.conversion.arg;
      if (nextArg.prefix.charAt(0) !== ' ') {
        nextArg.prefix = ' ' + nextArg.prefix;
        var nextConversion = nextAssignment.param.type.parse(nextArg);
        nextAssignment.setConversion(nextConversion);

        
        
        
        if (this._args.indexOf(nextArg) === -1) {
          this._args.push(nextArg);
        }
      }
    }
    else {
      
      
      var conversion = assignment.conversion;
      var arg = conversion.arg;
      if (arg.suffix.charAt(arg.suffix.length - 1) !== ' ') {
        arg.suffix = arg.suffix + ' ';

        
        
        
        
        
        
        assignment.setConversion(conversion);
      }
    }

    this.onTextChange();
    this.onTextChange.resumeFire();
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
      line.push(type.stringify(assignment.value));
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
      args.push({ arg: arg, char: arg.prefix[i], part: 'prefix' });
    }
    for (i = 0; i < arg.text.length; i++) {
      args.push({ arg: arg, char: arg.text[i], part: 'text' });
    }
    for (i = 0; i < arg.suffix.length; i++) {
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











Requisition.prototype.typedEndsWithWhitespace = function() {
  if (this._args) {
    return this._args.slice(-1)[0].suffix.slice(-1) === ' ';
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
        
        if (arg !== cTrace.arg || cTrace.part !== 'text') {
          
          if (!(arg.assignment instanceof CommandAssignment)) {
            status = Status.ERROR;
          }
        }
      }
    }

    markup.push({ status: status, string: argTrace.char });
  }

  
  var i = 0;
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
  var hidden = false;
  if (input && input.hidden) {
    hidden = true;
  }

  if (input) {
    if (typeof input === 'string') {
      this.update(input);
    }
    else if (typeof input.typed === 'string') {
      this.update(input.typed);
    }
    else if (input.command != null) {
      
      
      command = canon.getCommand(input.command);
      if (!command) {
        console.error('Command not found: ' + input.command);
      }
      args = input.args;
    }
  }

  if (!command) {
    command = this.commandAssignment.value;
    args = this.getArgsObject();
  }

  if (!command) {
    throw new Error('Unknown command');
  }

  
  var typed = this.toString();
  if (evalCommandSpec.evalRegexp.test(typed)) {
    typed = typed.replace(evalCommandSpec.evalRegexp, '');
    
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

  try {
    var context = exports.createExecutionContext(this);
    var reply = command.exec(args, context);

    if (reply != null && reply.isPromise) {
      reply.then(
          function(data) { output.complete(data); },
          function(error) { output.error = true; output.complete(error); });

      output.promise = reply;
      
      
    }
    else {
      output.complete(reply);
    }
  }
  catch (ex) {
    console.error(ex);
    output.error = true;
    output.complete(ex);
  }

  this.update('');
  return output;
};











Requisition.prototype.update = function(typed) {
  this._structuralChangeInProgress = true;

  this._args = this._tokenize(typed);
  var args = this._args.slice(0); 
  this._split(args);
  this._assign(args);

  this._structuralChangeInProgress = false;
  this.onTextChange();
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
  
  
  
  var conversion;
  if (args[0] instanceof ScriptArgument) {
    
    
    conversion = new Conversion(evalCommand, new ScriptArgument());
    this.commandAssignment.setConversion(conversion);
    return;
  }

  var argsUsed = 1;

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
  if (!this.commandAssignment.value) {
    this._unassigned.setUnassigned(args);
    return;
  }

  if (args.length === 0) {
    this.setBlankArguments();
    this._unassigned.setBlank();
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
      this._unassigned.setBlank();
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

    
    
    if (!assignment.param.isPositionalAllowed) {
      assignment.setBlank();
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

  this._unassigned.setUnassigned(args);
};

exports.Requisition = Requisition;




function Output(options) {
  options = options || {};
  this.command = options.command || '';
  this.args = options.args || {};
  this.typed = options.typed || '';
  this.canonical = options.canonical || '';
  this.hidden = options.hidden === true ? true : false;

  this.data = undefined;
  this.completed = false;
  this.error = false;
  this.start = new Date();

  this.onClose = util.createEvent('Output.onClose');
  this.onChange = util.createEvent('Output.onChange');
}





Output.prototype.complete = function(data) {
  this.data = data;

  this.end = new Date();
  this.duration = this.end.getTime() - this.start.getTime();
  this.completed = true;

  this.onChange({ output: this });
};






Output.prototype.toDom = function(element) {
  util.clearElement(element);
  var document = element.ownerDocument;

  var output = this.data;
  if (output == null) {
    return;
  }

  var node;
  if (typeof HTMLElement !== 'undefined' && output instanceof HTMLElement) {
    node = output;
  }
  else if (output.isView) {
    node = output.toDom(document);
  }
  else {
    if (this.command.returnType === 'terminal') {
      if (Array.isArray(output)) {
        node = util.createElement(document, 'div');
        output.forEach(function() {
          var child = util.createElement(document, 'textarea');
          child.classList.add('gcli-row-subterminal');
          child.readOnly = true;

          node.appendChild(child);
        });
      }
      else {
        node = util.createElement(document, 'textarea');
        node.classList.add('gcli-row-terminal');
        node.readOnly = true;
      }
    }
    else {
      node = util.createElement(document, 'p');
    }

    util.setContents(node, output.toString());
  }

  element.appendChild(node);
};





Output.prototype.toString = function(document) {
  var output = this.data;
  if (output == null) {
    return '';
  }

  if (typeof HTMLElement !== 'undefined' && output instanceof HTMLElement) {
    return output.textContent;
  }

  if (output.isView) {
    return output.toDom(document).textContent;
  }

  return output.toString();
};

exports.Output = Output;




exports.createExecutionContext = function(requisition) {
  return {
    exec: requisition.exec.bind(requisition),
    update: requisition.update.bind(requisition),
    document: requisition.document,
    environment: requisition.environment,
    createView: view.createView,
    createPromise: function() {
      return new Promise();
    }
  };
};


});






define('gcli/promise', ['require', 'exports', 'module' ], function(require, exports, module) {

  Components.utils.import("resource:///modules/devtools/Promise.jsm");
  exports.Promise = Promise;

});
define("text!gcli/ui/intro.html", [], "\n" +
  "<div>\n" +
  "  <p>\n" +
  "  GCLI is an experiment to create a highly usable <strong>graphical command\n" +
  "  line</strong> for developers. It's not a JavaScript\n" +
  "  <a href=\"https://en.wikipedia.org/wiki/Read�eval�print_loop\">REPL</a>, so\n" +
  "  it focuses on speed of input over JavaScript syntax and a rich display over\n" +
  "  monospace output.</p>\n" +
  "\n" +
  "  <p>Type <span class=\"gcli-out-shortcut\">help</span> for a list of commands,\n" +
  "  or press <code>F1/Escape</code> to show/hide command hints.</p>\n" +
  "\n" +
  "  <button onclick=\"${onGotIt}\" if=\"${showHideButton}\">Got it!</button>\n" +
  "</div>\n" +
  "");







define('gcli/ui/focus', ['require', 'exports', 'module' , 'gcli/util', 'gcli/settings', 'gcli/l10n', 'gcli/canon'], function(require, exports, module) {


var util = require('gcli/util');
var settings = require('gcli/settings');
var l10n = require('gcli/l10n');
var canon = require('gcli/canon');




var Eagerness = {
  NEVER: 1,
  SOMETIMES: 2,
  ALWAYS: 3
};
var eagerHelperSettingSpec = {
  name: 'eagerHelper',
  type: {
    name: 'selection',
    lookup: [
      { name: 'never', value: Eagerness.NEVER },
      { name: 'sometimes', value: Eagerness.SOMETIMES },
      { name: 'always', value: Eagerness.ALWAYS },
    ]
  },
  defaultValue: 1,
  description: l10n.lookup('eagerHelperDesc'),
  ignoreTypeDifference: true
};
var eagerHelper;




exports.startup = function() {
  eagerHelper = settings.addSetting(eagerHelperSettingSpec);
};

exports.shutdown = function() {
  settings.removeSetting(eagerHelperSettingSpec);
  eagerHelper = undefined;
};
















function FocusManager(options, components) {
  options = options || {};

  this._document = components.document || document;
  this._debug = options.debug || false;
  this._blurDelay = options.blurDelay || 150;
  this._window = this._document.defaultView;

  this._commandOutputManager = options.commandOutputManager ||
      canon.commandOutputManager;
  this._commandOutputManager.onOutput.add(this._outputted, this);

  this._blurDelayTimeout = null; 
  this._monitoredElements = [];  

  this._isError = false;
  this._hasFocus = false;
  this._helpRequested = false;
  this._recentOutput = false;

  this.onVisibilityChange = util.createEvent('FocusManager.onVisibilityChange');

  this._focused = this._focused.bind(this);
  this._document.addEventListener('focus', this._focused, true);

  eagerHelper.onChange.add(this._eagerHelperChanged, this);

  this.isTooltipVisible = undefined;
  this.isOutputVisible = undefined;
  this._checkShow();
}




FocusManager.prototype.destroy = function() {
  eagerHelper.onChange.remove(this._eagerHelperChanged, this);

  this._document.removeEventListener('focus', this._focused, true);
  this._commandOutputManager.onOutput.remove(this._outputted, this);

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
  delete this._commandOutputManager;
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
    outputVisible: this.isOutputVisible
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





FocusManager.prototype.onInputChange = function(ev) {
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
      console.debug('FocusManager.onVisibilityChange', ev);
    }
    this.onVisibilityChange(ev);
  }
};





FocusManager.prototype._shouldShowTooltip = function() {
  if (eagerHelper.value === Eagerness.NEVER) {
    return { visible: false, reason: 'eagerHelper !== NEVER' };
  }

  if (eagerHelper.value === Eagerness.ALWAYS) {
    return { visible: true, reason: 'eagerHelper !== ALWAYS' };
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
  if (this._recentOutput) {
    return { visible: true, reason: 'recentOutput' };
  }

  return { visible: false, reason: 'default' };
};

exports.FocusManager = FocusManager;


});






define('gcli/ui/fields/basic', ['require', 'exports', 'module' , 'gcli/util', 'gcli/l10n', 'gcli/argument', 'gcli/types', 'gcli/types/basic', 'gcli/ui/fields'], function(require, exports, module) {


var util = require('gcli/util');
var l10n = require('gcli/l10n');

var Argument = require('gcli/argument').Argument;
var TrueNamedArgument = require('gcli/argument').TrueNamedArgument;
var FalseNamedArgument = require('gcli/argument').FalseNamedArgument;
var ArrayArgument = require('gcli/argument').ArrayArgument;

var Conversion = require('gcli/types').Conversion;
var ArrayConversion = require('gcli/types').ArrayConversion;

var StringType = require('gcli/types/basic').StringType;
var NumberType = require('gcli/types/basic').NumberType;
var BooleanType = require('gcli/types/basic').BooleanType;
var DeferredType = require('gcli/types/basic').DeferredType;
var ArrayType = require('gcli/types/basic').ArrayType;

var Field = require('gcli/ui/fields').Field;
var fields = require('gcli/ui/fields');





exports.startup = function() {
  fields.addField(StringField);
  fields.addField(NumberField);
  fields.addField(BooleanField);
  fields.addField(DeferredField);
  fields.addField(ArrayField);
};

exports.shutdown = function() {
  fields.removeField(StringField);
  fields.removeField(NumberField);
  fields.removeField(BooleanField);
  fields.removeField(DeferredField);
  fields.removeField(ArrayField);
};





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
  
  this.arg = this.arg.beget(this.element.value, { prefixSpace: true });
  return this.type.parse(this.arg);
};

StringField.claim = function(type) {
  return type instanceof StringType ? Field.MATCH : Field.BASIC;
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
  var arg;
  if (this.named) {
    arg = this.element.checked ?
            new TrueNamedArgument(this.name) :
            new FalseNamedArgument();
  }
  else {
    arg = new Argument(' ' + this.element.checked);
  }
  return this.type.parse(arg);
};






function DeferredField(type, options) {
  Field.call(this, type, options);
  this.options = options;
  this.requisition.onAssignmentChange.add(this.update, this);

  this.element = util.createElement(this.document, 'div');
  this.update();

  this.onFieldChange = util.createEvent('DeferredField.onFieldChange');
}

DeferredField.prototype = Object.create(Field.prototype);

DeferredField.prototype.update = function() {
  var subtype = this.type.defer();
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

DeferredField.claim = function(type) {
  return type instanceof DeferredType ? Field.MATCH : Field.NO_MATCH;
};

DeferredField.prototype.destroy = function() {
  Field.prototype.destroy.call(this);
  this.requisition.onAssignmentChange.remove(this.update, this);
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

Object.defineProperty(DeferredField.prototype, 'isImportant', {
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
  this.addButton.innerHTML = l10n.lookup('fieldArrayAdd');
  this.element.appendChild(this.addButton);

  
  this.container = util.createElement(this.document, 'div');
  this.container.classList.add('gcli-array-members');
  this.element.appendChild(this.container);

  this.onInputChange = this.onInputChange.bind(this);

  this.onFieldChange = util.createEvent('ArrayField.onFieldChange');
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
  
  util.clearElement(this.container);
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
  
  var element = util.createElement(this.document, 'div');
  element.classList.add('gcli-array-member');
  this.container.appendChild(element);

  
  var field = fields.getField(this.type.subtype, this.options);
  field.onFieldChange.add(function() {
    var conversion = this.getConversion();
    this.onFieldChange({ conversion: conversion });
    this.setMessage(conversion.message);
  }, this);

  if (subConversion) {
    field.setConversion(subConversion);
  }
  element.appendChild(field.element);

  
  var delButton = util.createElement(this.document, 'button');
  delButton.classList.add('gcli-array-member-del');
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


});






define('gcli/ui/fields', ['require', 'exports', 'module' , 'gcli/util', 'gcli/types/basic'], function(require, exports, module) {


var util = require('gcli/util');
var KeyEvent = require('gcli/util').KeyEvent;

var BlankType = require('gcli/types/basic').BlankType;

















function Field(type, options) {
  this.type = type;
  this.document = options.document;
  this.requisition = options.requisition;
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
    util.setContents(this.messageElement, message);
  }
};





Field.prototype.onInputChange = function(ev) {
  var conversion = this.getConversion();
  this.onFieldChange({ conversion: conversion });
  this.setMessage(conversion.message);

  if (ev.keyCode === KeyEvent.DOM_VK_RETURN) {
    this.requisition.exec();
  }
};





Field.prototype.isImportant = false;






Field.claim = function() {
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
};














exports.getField = function(type, options) {
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

BlankField.claim = function(type) {
  return type instanceof BlankType ? Field.MATCH : Field.NO_MATCH;
};

BlankField.prototype.setConversion = function(conversion) {
  this.setMessage(conversion.message);
};

BlankField.prototype.getConversion = function() {
  return this.type.parse(new Argument());
};

exports.addField(BlankField);


});






define('gcli/ui/fields/javascript', ['require', 'exports', 'module' , 'gcli/util', 'gcli/argument', 'gcli/types/javascript', 'gcli/ui/fields/menu', 'gcli/ui/fields'], function(require, exports, module) {


var util = require('gcli/util');

var Argument = require('gcli/argument').Argument;
var JavascriptType = require('gcli/types/javascript').JavascriptType;

var Menu = require('gcli/ui/fields/menu').Menu;
var Field = require('gcli/ui/fields').Field;
var fields = require('gcli/ui/fields');





exports.startup = function() {
  fields.addField(JavascriptField);
};

exports.shutdown = function() {
  fields.removeField(JavascriptField);
};





function JavascriptField(type, options) {
  Field.call(this, type, options);

  this.onInputChange = this.onInputChange.bind(this);
  this.arg = new Argument('', '{ ', ' }');

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

  this.setConversion(this.type.parse(new Argument('')));

  this.onFieldChange = util.createEvent('JavascriptField.onFieldChange');

  
  this.menu.onItemClick.add(this.itemClicked, this);
}

JavascriptField.prototype = Object.create(Field.prototype);

JavascriptField.claim = function(type) {
  return type instanceof JavascriptType ? Field.TOOLTIP_MATCH : Field.NO_MATCH;
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

JavascriptField.prototype.itemClicked = function(ev) {
  this.onFieldChange(ev);
  this.setMessage(ev.conversion.message);
};

JavascriptField.prototype.onInputChange = function(ev) {
  this.item = ev.currentTarget.item;
  var conversion = this.getConversion();
  this.onFieldChange({ conversion: conversion });
  this.setMessage(conversion.message);
};

JavascriptField.prototype.getConversion = function() {
  
  this.arg = this.arg.beget(this.input.value, { normalize: true });
  return this.type.parse(this.arg);
};

JavascriptField.DEFAULT_VALUE = '__JavascriptField.DEFAULT_VALUE';


});






define('gcli/ui/fields/menu', ['require', 'exports', 'module' , 'gcli/util', 'gcli/argument', 'gcli/canon', 'gcli/ui/domtemplate', 'text!gcli/ui/fields/menu.css', 'text!gcli/ui/fields/menu.html'], function(require, exports, module) {


var util = require('gcli/util');

var Argument = require('gcli/argument').Argument;
var canon = require('gcli/canon');

var domtemplate = require('gcli/ui/domtemplate');

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




Menu.prototype.destroy = function() {
  delete this.element;
  delete this.items;
  delete this.template;
};






Menu.prototype.onItemClickInternal = function(ev) {
  var name = ev.currentTarget.querySelector('.gcli-menu-name').innerHTML;
  var arg = new Argument(name);
  arg.suffix = ' ';

  var conversion = this.type.parse(arg);
  this.onItemClick({ conversion: conversion });
};







Menu.prototype.show = function(items, match) {
  this.items = items.filter(function(item) {
    return item.hidden === undefined || item.hidden !== true;
  }.bind(this));

  if (match) {
    this.items = this.items.map(function(item) {
      return gethighlightingProxy(item, match, this.template.ownerDocument);
    }.bind(this));
  }

  if (this.items.length === 0) {
    this.element.style.display = 'none';
    return;
  }

  var options = this.template.cloneNode(true);
  domtemplate.template(options, this, this.templateOptions);

  util.clearElement(this.element);
  this.element.appendChild(options);

  this.element.style.display = 'block';
};




function gethighlightingProxy(item, match, document) {
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
  if (selected) {
    var name = selected.innerHTML;
    var arg = new Argument(name);
    arg.suffix = ' ';

    var conversion = this.type.parse(arg);
    this.onItemClick({ conversion: conversion });
  }
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
  "<table class=\"gcli-menu-template\" aria-live=\"polite\">\n" +
  "  <tr class=\"gcli-menu-option\" foreach=\"item in ${items}\"\n" +
  "      onclick=\"${onItemClickInternal}\" title=\"${item.manual}\">\n" +
  "    <td class=\"gcli-menu-name\">${item.name}</td>\n" +
  "    <td class=\"gcli-menu-desc\">${item.description}</td>\n" +
  "  </tr>\n" +
  "</table>\n" +
  "");







define('gcli/ui/fields/selection', ['require', 'exports', 'module' , 'gcli/util', 'gcli/l10n', 'gcli/argument', 'gcli/types', 'gcli/types/basic', 'gcli/types/selection', 'gcli/ui/fields/menu', 'gcli/ui/fields'], function(require, exports, module) {


var util = require('gcli/util');
var l10n = require('gcli/l10n');

var Argument = require('gcli/argument').Argument;
var Status = require('gcli/types').Status;
var Conversion = require('gcli/types').Conversion;
var BooleanType = require('gcli/types/basic').BooleanType;
var SelectionType = require('gcli/types/selection').SelectionType;

var Menu = require('gcli/ui/fields/menu').Menu;
var Field = require('gcli/ui/fields').Field;
var fields = require('gcli/ui/fields');





exports.startup = function() {
  fields.addField(SelectionField);
  fields.addField(SelectionTooltipField);
};

exports.shutdown = function() {
  fields.removeField(SelectionField);
  fields.removeField(SelectionTooltipField);
};













function SelectionField(type, options) {
  Field.call(this, type, options);

  this.items = [];

  this.element = util.createElement(this.document, 'select');
  this.element.classList.add('gcli-field');
  this._addOption({
    name: l10n.lookupFormat('fieldSelectionSelect', [ options.name ])
  });
  var lookup = this.type.getLookup();
  lookup.forEach(this._addOption, this);

  this.onInputChange = this.onInputChange.bind(this);
  this.element.addEventListener('change', this.onInputChange, false);

  this.onFieldChange = util.createEvent('SelectionField.onFieldChange');
}

SelectionField.prototype = Object.create(Field.prototype);

SelectionField.claim = function(type) {
  if (type instanceof BooleanType) {
    return Field.BASIC;
  }
  return type instanceof SelectionType ? Field.DEFAULT : Field.NO_MATCH;
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
  option.innerHTML = item.name;
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

SelectionTooltipField.claim = function(type) {
  return type.getType() instanceof SelectionType ? Field.TOOLTIP_MATCH : Field.NO_MATCH;
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
  var items = conversion.getPredictions().map(function(prediction) {
    
    
    
    return prediction.value.description ? prediction.value : prediction;
  }, this);
  this.menu.show(items, conversion.arg.text);
  this.setMessage(conversion.message);
};

SelectionTooltipField.prototype.itemClicked = function(ev) {
  this.onFieldChange(ev);
  this.setMessage(ev.conversion.message);
};

SelectionTooltipField.prototype.onInputChange = function(ev) {
  this.item = ev.currentTarget.item;
  var conversion = this.getConversion();
  this.onFieldChange({ conversion: conversion });
  this.setMessage(conversion.message);
};

SelectionTooltipField.prototype.getConversion = function() {
  
  this.arg = this.arg.beget('typed', { normalize: true });
  return this.type.parse(this.arg);
};




SelectionTooltipField.prototype.setChoiceIndex = function(choice) {
  this.menu.setChoiceIndex(choice);
};





SelectionTooltipField.prototype.selectChoice = function() {
  this.menu.selectChoice();
};

Object.defineProperty(SelectionTooltipField.prototype, 'isImportant', {
  get: function() {
    return this.type.name !== 'command';
  },
  enumerable: true
});

SelectionTooltipField.DEFAULT_VALUE = '__SelectionTooltipField.DEFAULT_VALUE';


});






define('gcli/commands/help', ['require', 'exports', 'module' , 'gcli/canon', 'gcli/l10n', 'gcli/ui/view', 'text!gcli/commands/help_man.html', 'text!gcli/commands/help_list.html', 'text!gcli/commands/help.css'], function(require, exports, module) {
var help = exports;


var canon = require('gcli/canon');
var l10n = require('gcli/l10n');
var view = require('gcli/ui/view');



exports.helpManHtml = require('text!gcli/commands/help_man.html');
exports.helpListHtml = require('text!gcli/commands/help_list.html');
exports.helpCss = require('text!gcli/commands/help.css');




var helpCommandSpec = {
  name: 'help',
  description: l10n.lookup('helpDesc'),
  manual: l10n.lookup('helpManual'),
  params: [
    {
      name: 'search',
      type: 'string',
      description: l10n.lookup('helpSearchDesc'),
      manual: l10n.lookup('helpSearchManual2'),
      defaultValue: null
    }
  ],
  returnType: 'html',

  exec: function(args, context) {
    var match = canon.getCommand(args.search || undefined);
    if (match) {
      return view.createView({
        html: exports.helpManHtml,
        options: { allowEval: true, stack: 'help_man.html' },
        data: getManTemplateData(match, context),
        css: exports.helpCss,
        cssId: 'gcli-help'
      });
    }

    return view.createView({
      html: exports.helpListHtml,
      options: { allowEval: true, stack: 'help_list.html' },
      data: getListTemplateData(args, context),
      css: exports.helpCss,
      cssId: 'gcli-help'
    });
  }
};




help.startup = function() {
  canon.addCommand(helpCommandSpec);
};

help.shutdown = function() {
  canon.removeCommand(helpCommandSpec);
};





function updateCommand(element, context) {
  var typed = element.querySelector('.gcli-help-command').textContent;
  context.update(typed);
}





function executeCommand(element, context) {
  context.exec({
    visible: true,
    typed: element.querySelector('.gcli-help-command').textContent
  });
}




function getListTemplateData(args, context) {
  return {
    l10n: l10n.propertyLookup,
    includeIntro: args.search == null,

    onclick: function(ev) {
      updateCommand(ev.currentTarget, context);
    },

    ondblclick: function(ev) {
      executeCommand(ev.currentTarget, context);
    },

    getHeading: function() {
      return args.search == null ?
              'Available Commands:' :
              'Commands starting with \'' + args.search + '\':';
    },

    getMatchingCommands: function() {
      var matching = canon.getCommands().filter(function(command) {
        if (command.hidden) {
          return false;
        }

        if (args.search && command.name.indexOf(args.search) !== 0) {
          
          return false;
        }
        if (!args.search && command.name.indexOf(' ') != -1) {
          
          return false;
        }
        return true;
      });
      matching.sort();
      return matching;
    }
  };
}




function getManTemplateData(command, context) {
  var manTemplateData = {
    l10n: l10n.propertyLookup,
    command: command,

    onclick: function(ev) {
      updateCommand(ev.currentTarget, context);
    },

    ondblclick: function(ev) {
      executeCommand(ev.currentTarget, context);
    },

    getTypeDescription: function(param) {
      var input = '';
      if (param.defaultValue === undefined) {
        input = 'required';
      }
      else if (param.defaultValue === null) {
        input = 'optional';
      }
      else {
        input = param.defaultValue;
      }
      return '(' + param.type.name + ', ' + input + ')';
    }
  };

  Object.defineProperty(manTemplateData, 'subcommands', {
    get: function() {
      var matching = canon.getCommands().filter(function(subcommand) {
        return subcommand.name.indexOf(command.name) === 0 &&
                subcommand.name !== command.name;
      });
      matching.sort();
      return matching;
    },
    enumerable: true
  });

  return manTemplateData;
}

});
define("text!gcli/commands/help_man.html", [], "\n" +
  "<div>\n" +
  "  <h3>${command.name}</h3>\n" +
  "\n" +
  "  <h4 class=\"gcli-help-header\">\n" +
  "    ${l10n.helpManSynopsis}:\n" +
  "    <span class=\"gcli-help-synopsis\" onclick=\"${onclick}\">\n" +
  "      <span class=\"gcli-help-command\">${command.name}</span>\n" +
  "      <span foreach=\"param in ${command.params}\">\n" +
  "        ${param.defaultValue !== undefined ? '[' + param.name + ']' : param.name}\n" +
  "      </span>\n" +
  "    </span>\n" +
  "  </h4>\n" +
  "\n" +
  "  <h4 class=\"gcli-help-header\">${l10n.helpManDescription}:</h4>\n" +
  "\n" +
  "  <p class=\"gcli-help-description\">\n" +
  "    ${command.manual || command.description}\n" +
  "  </p>\n" +
  "\n" +
  "  <div if=\"${command.exec}\">\n" +
  "    <h4 class=\"gcli-help-header\">${l10n.helpManParameters}:</h4>\n" +
  "\n" +
  "    <ul class=\"gcli-help-parameter\">\n" +
  "      <li if=\"${command.params.length === 0}\">${l10n.helpManNone}</li>\n" +
  "      <li foreach=\"param in ${command.params}\">\n" +
  "        <tt>${param.name}</tt> ${getTypeDescription(param)}\n" +
  "        <br/>\n" +
  "        ${param.manual || param.description}\n" +
  "      </li>\n" +
  "    </ul>\n" +
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
  "        <span class=\"gcli-help-synopsis\" onclick=\"${onclick}\" ondblclick=\"${ondblclick}\">\n" +
  "          <span class=\"gcli-help-command\">help ${subcommand.name}</span>\n" +
  "        </span>\n" +
  "      </li>\n" +
  "    </ul>\n" +
  "  </div>\n" +
  "\n" +
  "</div>\n" +
  "");

define("text!gcli/commands/help_list.html", [], "\n" +
  "<div>\n" +
  "  <h3>${getHeading()}</h3>\n" +
  "\n" +
  "  <table>\n" +
  "    <tr foreach=\"command in ${getMatchingCommands()}\"\n" +
  "        onclick=\"${onclick}\" ondblclick=\"${ondblclick}\">\n" +
  "      <th class=\"gcli-help-name\">${command.name}</th>\n" +
  "      <td class=\"gcli-help-arrow\">&#x2192;</td>\n" +
  "      <td>\n" +
  "        ${command.description}\n" +
  "        <span class=\"gcli-out-shortcut gcli-help-command\">help ${command.name}</span>\n" +
  "      </td>\n" +
  "    </tr>\n" +
  "  </table>\n" +
  "</div>\n" +
  "");

define("text!gcli/commands/help.css", [], "");







define('gcli/ui/ffdisplay', ['require', 'exports', 'module' , 'gcli/ui/inputter', 'gcli/ui/completer', 'gcli/ui/tooltip', 'gcli/ui/focus', 'gcli/cli', 'gcli/types/javascript', 'gcli/types/node', 'gcli/types/resource', 'gcli/host', 'gcli/ui/intro', 'gcli/canon'], function(require, exports, module) {

var Inputter = require('gcli/ui/inputter').Inputter;
var Completer = require('gcli/ui/completer').Completer;
var Tooltip = require('gcli/ui/tooltip').Tooltip;
var FocusManager = require('gcli/ui/focus').FocusManager;

var Requisition = require('gcli/cli').Requisition;

var cli = require('gcli/cli');
var jstype = require('gcli/types/javascript');
var nodetype = require('gcli/types/node');
var resource = require('gcli/types/resource');
var host = require('gcli/host');
var intro = require('gcli/ui/intro');

var commandOutputManager = require('gcli/canon').commandOutputManager;





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

  this.onOutput = commandOutputManager.onOutput;
  this.requisition = new Requisition(options.environment, options.outputDocument);

  
  this.focusManager = new FocusManager(options, {
    
    document: options.chromeDocument
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
  intro.maybeShowIntro(commandOutputManager);
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






define('gcli/ui/inputter', ['require', 'exports', 'module' , 'gcli/util', 'gcli/types', 'gcli/history', 'text!gcli/ui/inputter.css'], function(require, exports, module) {


var util = require('gcli/util');
var KeyEvent = require('gcli/util').KeyEvent;

var Status = require('gcli/types').Status;
var History = require('gcli/history').History;

var inputterCss = require('text!gcli/ui/inputter.css');












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

  this.requisition.onTextChange.add(this.textChanged, this);

  this.assignment = this.requisition.getAssignmentAt(0);
  this.onAssignmentChange = util.createEvent('Inputter.onAssignmentChange');
  this.onInputChange = util.createEvent('Inputter.onInputChange');

  this.onResize = util.createEvent('Inputter.onResize');
  this.onWindowResize = this.onWindowResize.bind(this);
  this.document.defaultView.addEventListener('resize', this.onWindowResize, false);

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

  var rect = this.element.getBoundingClientRect();
  return {
    top: rect.top + 1,
    height: rect.bottom - rect.top - 1,
    left: rect.left + 2,
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

  this.element.value = newStr;
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

  this.element.selectionStart = start;
  this.element.selectionEnd = end;

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
    this.assignment = newAssignment;
    this.onAssignmentChange({ assignment: this.assignment });
  }

  
  
  
  
  
  
  if (this.focusManager) {
    var message = this.assignment.conversion.message;
    this.focusManager.setError(message != null && message !== '');
  }
};










Inputter.prototype.setInput = function(str) {
  this.requisition.update(str);
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
    this.focusManager.onInputChange(ev);
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
  if (this.focusManager && ev.keyCode === KeyEvent.DOM_VK_F1) {
    this.focusManager.helpRequest();
    return;
  }

  if (this.focusManager && ev.keyCode === KeyEvent.DOM_VK_ESCAPE) {
    this.focusManager.removeHelp();
    return;
  }

  if (ev.keyCode === KeyEvent.DOM_VK_UP) {
    if (this.tooltip && this.tooltip.isMenuShowing) {
      this.changeChoice(-1);
    }
    else if (this.element.value === '' || this._scrollingThroughHistory) {
      this._scrollingThroughHistory = true;
      this.requisition.update(this.history.backward());
    }
    else {
      
      
      if (this.assignment.getStatus() === Status.VALID) {
        this.assignment.increment();
        
        if (this.focusManager) {
          this.focusManager.onInputChange(ev);
        }
      }
      else {
        this.changeChoice(-1);
      }
    }
    return;
  }

  if (ev.keyCode === KeyEvent.DOM_VK_DOWN) {
    if (this.tooltip && this.tooltip.isMenuShowing) {
      this.changeChoice(+1);
    }
    else if (this.element.value === '' || this._scrollingThroughHistory) {
      this._scrollingThroughHistory = true;
      this.requisition.update(this.history.forward());
    }
    else {
      
      if (this.assignment.getStatus() === Status.VALID) {
        this.assignment.decrement();
        
        if (this.focusManager) {
          this.focusManager.onInputChange(ev);
        }
      }
      else {
        this.changeChoice(+1);
      }
    }
    return;
  }

  
  if (ev.keyCode === KeyEvent.DOM_VK_RETURN) {
    var worst = this.requisition.getStatus();
    
    if (worst === Status.VALID) {
      this._scrollingThroughHistory = false;
      this.history.add(this.element.value);
      this.requisition.exec();
    }
    else {
      
      
      this.tooltip.selectChoice();

      
      
    }

    this._choice = null;
    return;
  }

  if (ev.keyCode === KeyEvent.DOM_VK_TAB && !ev.shiftKey) {
    
    
    
    
    
    
    if (this.lastTabDownAt + 1000 > ev.timeStamp) {
      
      
      
      this._caretChange = Caret.TO_ARG_END;
      var inputState = this.getInputState();
      this._processCaretChange(inputState);
      if (this._choice == null) {
        this._choice = 0;
      }
      this.requisition.complete(inputState.cursor, this._choice);
    }
    this.lastTabDownAt = 0;
    this._scrollingThroughHistory = false;

    this._choice = null;
    this.onChoiceChange({ choice: this._choice });
    return;
  }

  
  if (this.scratchpad && this.scratchpad.shouldActivate(ev)) {
    if (this.scratchpad.activate(this.element.value)) {
      this.requisition.update('');
    }
    return;
  }

  this._scrollingThroughHistory = false;
  this._caretChange = Caret.NO_CHANGE;

  this.requisition.update(this.element.value);

  this._choice = null;
  this.onChoiceChange({ choice: this._choice });
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
    console.log('fixing input.typed=""', input);
  }

  
  if (input.cursor.start == null) {
    input.cursor.start = 0;
  }

  return input;
};

exports.Inputter = Inputter;


});






define('gcli/history', ['require', 'exports', 'module' ], function(require, exports, module) {






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







define('gcli/ui/completer', ['require', 'exports', 'module' , 'gcli/util', 'gcli/ui/domtemplate', 'text!gcli/ui/completer.html'], function(require, exports, module) {


var util = require('gcli/util');
var domtemplate = require('gcli/ui/domtemplate');

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

  if (components.autoResize) {
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
  this.inputter.onResize.remove(this.resized, this);

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






function isStrictCompletion(inputValue, completion) {
  
  
  inputValue = inputValue.replace(/^\s*/, '');
  
  
  return completion.indexOf(inputValue) === 0;
}




Completer.prototype.update = function(ev) {
  if (ev && ev.choice != null) {
    this.choice = ev.choice;
  }
  this._preTemplateUpdate();

  var template = this.template.cloneNode(true);
  domtemplate.template(template, this, { stack: 'completer.html' });

  util.clearElement(this.element);
  while (template.hasChildNodes()) {
    this.element.appendChild(template.firstChild);
  }
};






Completer.prototype._preTemplateUpdate = function() {
  this.input = this.inputter.getInputState();

  this.directTabText = '';
  this.arrowTabText = '';

  
  
  
  if (this.input.typed.trim().length === 0) {
    return;
  }

  var current = this.inputter.assignment;
  var prediction = current.conversion.getPredictionAt(this.choice);
  if (!prediction) {
    return;
  }

  var tabText = prediction.name;
  var existing = current.arg.text;

  if (existing === tabText) {
    return;
  }

  if (isStrictCompletion(existing, tabText) &&
          this.input.cursor.start === this.input.typed.length) {
    
    var numLeadingSpaces = existing.match(/^(\s*)/)[0].length;

    this.directTabText = tabText.slice(existing.length - numLeadingSpaces);
  }
  else {
    
    
    this.arrowTabText = ' \u00a0\u21E5 ' + tabText;
  }
};






Object.defineProperty(Completer.prototype, 'statusMarkup', {
  get: function() {
    var markup = this.requisition.getInputStatusMarkup(this.input.cursor.start);
    markup.forEach(function(member) {
      member.string = member.string.replace(/ /g, '\u00a0'); 
      member.className = 'gcli-in-' + member.status.toString().toLowerCase();
    }, this);
    return markup;
  },
  enumerable: true
});




Object.defineProperty(Completer.prototype, 'scratchLink', {
  get: function() {
    if (!this.scratchpad) {
      return null;
    }
    var command = this.requisition.commandAssignment.value;
    return command && command.name === '{' ? this.scratchpad.linkText : null;
  },
  enumerable: true
});





Object.defineProperty(Completer.prototype, 'unclosedJs', {
  get: function() {
    var command = this.requisition.commandAssignment.value;
    var jsCommand = command && command.name === '{';
    var unclosedJs = jsCommand &&
        this.requisition.getAssignment(0).arg.suffix.indexOf('}') === -1;
    return unclosedJs;
  },
  enumerable: true
});




Object.defineProperty(Completer.prototype, 'emptyParameters', {
  get: function() {
    var typedEndSpace = this.requisition.typedEndsWithWhitespace();
    
    var directTabText = this.directTabText;
    
    
    
    var firstBlankParam = true;
    var params = [];
    this.requisition.getAssignments().forEach(function(assignment) {
      if (!assignment.param.isPositionalAllowed) {
        return;
      }

      if (!assignment.arg.isBlank()) {
        if (directTabText !== '') {
          firstBlankParam = false;
        }
        return;
      }

      if (directTabText !== '' && firstBlankParam) {
        firstBlankParam = false;
        return;
      }

      var text = (assignment.param.isDataRequired) ?
          '<' + assignment.param.name + '>' :
          '[' + assignment.param.name + ']';

      
      
      if (!typedEndSpace || !firstBlankParam) {
        text = '\u00a0' + text; 
      }

      firstBlankParam = false;
      params.push(text);
    }.bind(this));
    return params;
  },
  enumerable: true
});

exports.Completer = Completer;


});
define("text!gcli/ui/completer.html", [], "\n" +
  "<description>\n" +
  "  <loop foreach=\"member in ${statusMarkup}\">\n" +
  "    <label class=\"${member.className}\">${member.string}</label>\n" +
  "  </loop>\n" +
  "  <label class=\"gcli-in-ontab\">${directTabText}</label>\n" +
  "  <label class=\"gcli-in-todo\" foreach=\"param in ${emptyParameters}\">${param}</label>\n" +
  "  <label class=\"gcli-in-ontab\">${arrowTabText}</label>\n" +
  "  <label class=\"gcli-in-closebrace\" if=\"${unclosedJs}\">}</label>\n" +
  "</description>\n" +
  "");







define('gcli/ui/tooltip', ['require', 'exports', 'module' , 'gcli/util', 'gcli/cli', 'gcli/ui/fields', 'gcli/ui/domtemplate', 'text!gcli/ui/tooltip.css', 'text!gcli/ui/tooltip.html'], function(require, exports, module) {


var util = require('gcli/util');
var CommandAssignment = require('gcli/cli').CommandAssignment;

var fields = require('gcli/ui/fields');
var domtemplate = require('gcli/ui/domtemplate');

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

  
  this.assignment = undefined;
  this.assignmentChanged({ assignment: this.inputter.assignment });
}




Tooltip.prototype.destroy = function() {
  this.inputter.onAssignmentChange.remove(this.assignmentChanged, this);
  this.inputter.onChoiceChange.remove(this.choiceChanged, this);

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

  delete this.errorEle;
  delete this.descriptionEle;
  delete this.highlightEle;

  delete this.field;
  delete this.focusManager;
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

  if (this.assignment) {
    this.assignment.onAssignmentChange.remove(this.assignmentContentsChanged, this);
  }
  this.assignment = ev.assignment;

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
  this.assignment.onAssignmentChange.add(this.assignmentContentsChanged, this);

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
    var choice = this.assignment.conversion.constrainPredictionIndex(ev.choice);
    this.field.setChoiceIndex(choice);
  }
};





Tooltip.prototype.selectChoice = function(ev) {
  if (this.field && this.field.selectChoice) {
    this.field.selectChoice();
  }
};




Tooltip.prototype.fieldChanged = function(ev) {
  this.assignment.setConversion(ev.conversion);

  var isError = ev.conversion.message != null && ev.conversion.message !== '';
  this.focusManager.setError(isError);

  
  
  
  this.document.defaultView.setTimeout(function() {
    this.inputter.focus();
  }.bind(this), 10);
};




Tooltip.prototype.assignmentContentsChanged = function(ev) {
  
  
  
  if (ev.conversion.arg.text === ev.oldConversion.arg.text) {
    return;
  }

  this.field.setConversion(ev.conversion);
  util.setContents(this.descriptionEle, this.description);

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

    var output = this.assignment.param.manual;
    if (output) {
      var wrapper = this.document.createElement('span');
      util.setContents(wrapper, output);
      if (!this.assignment.param.isDataRequired) {
        var optional = this.document.createElement('span');
        optional.appendChild(this.document.createTextNode(' (Optional)'));
        wrapper.appendChild(optional);
      }
      return wrapper;
    }

    return this.assignment.param.description;
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






var gcli = require("gcli/index");
